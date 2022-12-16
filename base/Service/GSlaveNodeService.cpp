﻿#include "GSlaveNodeService.h"
#include "GConfigService.h"
#include "GServiceMgr.h"
#include "GApplication.h"
#include "GMasterNodeService.h"
#include "Utils/GStringUtils.h"

std::string GSlaveNodeService::MGS_KEY_SLAVE_NODE_DISCONNECT = "__MGS_KEY_SLAVE_NODE_DISCONNECT__";

GSlaveNodeService::GSlaveNodeService()
	: m_isOnline(false)
	, m_stopReconnect(false)
	, m_netType(GNetType::UNKNOWN)
	, m_token(0)
	, m_groupID(0)
	, m_nodePort(0)
{}

GSlaveNodeService::~GSlaveNodeService()
{}

uint32_t GSlaveNodeService::onInit()
{
	G_CHECK_SERVICE(GConfigService);

	m_isOnline = false;
	m_stopReconnect = false;

	auto& ini = m_serviceMgr->getService<GConfigService>()->iniReader();
	auto appName = GApplication::getInstance()->getAppName();

	// 不需要此服务
	if (ini.GetBoolean(appName, "SlaveNodeEnable", false) == false)
		return SCODE_START_FAIL_NO_ERR;

	auto ip		= ini.Get(appName, "SlaveNodeIP", "");
	auto port	= ini.GetInteger(appName, "SlaveNodePort", 0);
	auto group  = ini.GetInteger(appName, "SlaveNodeGroup", 0);
	auto isKcp  = ini.GetBoolean(appName, "SlaveNodeIsKcp", false);
	auto description = ini.Get(appName, "SlaveNodeDesc", "");
	auto serviceId = ini.GetInteger(appName, "SlaveNodeServiceId", 0);

	if (description.empty())
	{
		description = "{}";
	}
	rapidjson::StringStream stream(description.c_str());
	m_descriptionJson.ParseStream<0>(stream);

	if (m_descriptionJson.HasParseError())
	{
		LOG(ERROR) << "[GSlaveNodeService] json parse error : " << m_descriptionJson.GetParseError();
		return SCODE_START_FAIL_EXIT_APP;
	}

	if(ip.empty() || port <= 0)
		return SCODE_START_FAIL_EXIT_APP;

	m_nodeIP = ip;
	m_nodePort = port;
	m_groupID = group;
	m_description = description;
	m_netType = isKcp ? GNetType::KCP : GNetType::TCP;
	m_serviceId = serviceId;

	if (isKcp)
		m_client = std::make_unique<net_uv::KCPClient>();
	else
		m_client = std::make_unique<net_uv::TCPClient>();

	m_client->setConnectCallback(std::bind(&GSlaveNodeService::onConnectCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_client->setDisconnectCallback(std::bind(&GSlaveNodeService::onDisconnectCall, this, std::placeholders::_1, std::placeholders::_2));
	m_client->setRecvCallback(std::bind(&GSlaveNodeService::onRecvCall, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	m_client->setClientCloseCallback(std::bind(&GSlaveNodeService::onCloseCall, this, std::placeholders::_1));
	m_client->setRemoveSessionCallback(std::bind(&GSlaveNodeService::onRemoveSessionCall, this, std::placeholders::_1, std::placeholders::_2));

	m_msgMgr = std::make_unique<net_uv::NetMsgMgr>();
	m_msgMgr->setUserData(this);
	m_msgMgr->setSendCallback([](net_uv::NetMsgMgr* mgr, uint32_t sessionID, char* data, uint32_t len)
	{
		auto ins = reinterpret_cast<GSlaveNodeService*>(mgr->getUserData());
		ins->m_client->send(sessionID, data, len);
	});
	m_msgMgr->setCloseSctCallback([](net_uv::NetMsgMgr* mgr, uint32_t sessionID)
	{
		auto ins = reinterpret_cast<GSlaveNodeService*>(mgr->getUserData());
		ins->m_client->disconnect(sessionID);
	});
	m_msgMgr->setOnMsgCallback([](net_uv::NetMsgMgr* mgr, uint32_t sessionID, uint32_t msgID, char* data, uint32_t len)
	{
		auto ins = reinterpret_cast<GSlaveNodeService*>(mgr->getUserData());
		ins->onRecvMsg(sessionID, msgID, data, len);
	});
	m_msgMgr->setHeartBeatTime(2);
	m_msgMgr->setHeartBeatLoseMaxCount(3);

	m_noticeCenter = std::make_unique<GNoticeCenter>();


	auto scheduler = GApplication::getInstance()->getScheduler();
	scheduler->schedule([=](float)
		{
			if (m_isOnline || m_stopReconnect)
				return;
			m_client->connect(m_nodeIP.c_str(), m_nodePort, 0);
		}, this, 5.0f, false, "reconnect");

	return SCODE_START_SUCCESS;
}

void GSlaveNodeService::onStartService()
{
	m_client->connect(m_nodeIP.c_str(), m_nodePort, 0);
}

void GSlaveNodeService::onStopService()
{
	m_stopReconnect = true;
	m_client->closeClient();
}

void GSlaveNodeService::onUpdate(float)
{
	m_client->updateFrame();
	m_msgMgr->updateFrame();

	switch (this->m_status)
	{
	case GServiceStatus::RUNNING:
	{}break;
	case GServiceStatus::STOP_ING:
	{
		if (m_client->isCloseFinish())
		{
			this->stopServiceFinish();
		}
	}break;
	default:
		break;
	}
}

void GSlaveNodeService::onDestroy()
{
	auto scheduler = GApplication::getInstance()->getScheduler();
	if (scheduler)
	{
		scheduler->unScheduleTarget(this);
	}
}

void GSlaveNodeService::sendMsg(uint32_t msgID, char* data, uint32_t len)
{
	m_msgMgr->sendMsg(0, msgID, data, len);
}

void GSlaveNodeService::onConnectCallback(net_uv::Client* client, net_uv::Session* session, int32_t status)
{
	// 0:failed 1:succeeded 2:timed out
	if (status == 1)
	{
		LOG(INFO) << "[GSlaveNodeService] login..., GroupID: " << m_groupID;
		m_msgMgr->onConnect(session->getSessionID());

		auto bufLen = sizeof(NSMsg::RegServerReq) + m_description.size();
		char* buf = new char[bufLen];
		
		auto req = (NSMsg::RegServerReq*)buf;
		req->groupID = m_groupID;
		req->token = m_token;
		req->serviceId = m_serviceId;
		req->infoLen = m_description.size();
		if (m_description.size() > 0)
			memcpy(buf + sizeof(NSMsg::RegServerReq), m_description.c_str(), m_description.size());

		m_msgMgr->sendMsg(session->getSessionID(), NSMsg::MSG_REG_SERVER_NODE_REQ, (char*)req, bufLen);

		delete[] buf;
	}
	else
	{
		m_isOnline = false;

		if(status == 0)
			LOG(ERROR) << "[GSlaveNodeService] connect fail, GroupID: " << m_groupID;
		else
			LOG(ERROR) << "[GSlaveNodeService] connect timeout, GroupID: " << m_groupID;
	}
}

void GSlaveNodeService::onDisconnectCall(net_uv::Client* client, net_uv::Session* session)
{
	m_isOnline = false;
	m_msgMgr->onDisconnect(session->getSessionID());
	m_noticeCenter->emitEvent(MGS_KEY_SLAVE_NODE_DISCONNECT);

	if (m_stopReconnect == false)
		m_client->connect(m_nodeIP.c_str(), m_nodePort, 0);
}

void GSlaveNodeService::onRecvCall(net_uv::Client* client, net_uv::Session* session, char* data, uint32_t len)
{
	m_msgMgr->onBuff(session->getSessionID(), data, len);
}

void GSlaveNodeService::onCloseCall(net_uv::Client* client)
{}

void GSlaveNodeService::onRemoveSessionCall(net_uv::Client* client, net_uv::Session* session)
{}

void GSlaveNodeService::onRecvMsg(uint32_t sessionID, uint32_t msgID, char* data, uint32_t len)
{
	switch (msgID)
	{
	case NSMsg::MSG_REG_SERVER_NODE_ACK:
	{
		auto msg = reinterpret_cast<NSMsg::RegServerAck*>(data);
		// 注册成功
		if (msg->code == 0)
		{
			m_token = msg->token;
			m_isOnline = true;
			LOG(INFO) << "[GSlaveNodeService] login successful, GroupID: " << m_groupID;
		}
		else
		{
			m_isOnline = false;
			m_client->disconnect(sessionID);

			LOG(ERROR) << "[GSlaveNodeService] register failed!, GroupID: " << m_groupID << ", code:" << msg->code;
		}
	}break;
	default:
		m_noticeCenter->emitEvent(StringUtils::msgKey(msgID), sessionID, data, len);
		break;
	}
}

