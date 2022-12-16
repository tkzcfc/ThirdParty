#include "GMasterNodeService.h"
#include "GApplication.h"
#include "GConfigService.h"
#include "Utils/GStringUtils.h"
#include "Utils/GMisc.h"

namespace NSMsg {
	std::string MGS_KEY_FIRST_REG_SLAVE_NODE = "##MGS_KEY_FIRST_REG_SLAVE_NODE";
	std::string MGS_KEY_SLAVE_NODE_ONLINE = "##MGS_KEY_SLAVE_NODE_ONLINE";
	std::string MGS_KEY_SLAVE_NODE_LEV = "##MGS_KEY_SLAVE_NODE_LEV";
}

GMasterNodeService::GMasterNodeService()
	: m_netType(GNetType::UNKNOWN)
	, m_groupID(-1)
{}

GMasterNodeService::~GMasterNodeService()
{}

uint32_t GMasterNodeService::onInit()
{
	G_CHECK_SERVICE(GConfigService);

	auto cfgService = m_serviceMgr->getService<GConfigService>();
	auto& ini = cfgService->iniReader();
	auto appName = GApplication::getInstance()->getAppName();

	//! 不需要此服务
	if (ini.GetBoolean(appName, "MasterNodeEnable", false) == false)
	{
		return SCODE_START_FAIL_NO_ERR;
	}

	auto ip = ini.Get(appName, "MasterNodeIP", "");
	auto port = ini.GetInteger(appName, "MasterNodePort", 0);
	auto group = ini.GetInteger(appName, "MasterNodeGroup", 0);
	auto isKcp = ini.GetBoolean(appName, "MasterNodeIsKcp", false);

	if (ip.empty() || port <= 0)
		return SCODE_START_FAIL_EXIT_APP;

	m_groupID = group;
	m_netType = isKcp ? GNetType::KCP : GNetType::TCP;

	if(isKcp)
		m_svr = std::make_unique<net_uv::KCPServer>();
	else
		m_svr = std::make_unique<net_uv::TCPServer>();

	m_svr->setNewConnectCallback(std::bind(&GMasterNodeService::onNewConnectCallback, this, std::placeholders::_1, std::placeholders::_2));
	m_svr->setRecvCallback(std::bind(&GMasterNodeService::onRecvCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	m_svr->setDisconnectCallback(std::bind(&GMasterNodeService::onDisconnectCallback, this, std::placeholders::_1, std::placeholders::_2));
	m_svr->setCloseCallback([](net_uv::Server*) {});

	if (false == m_svr->startServer(ip.c_str(), port, false, 0xffff))
		return SCODE_START_FAIL_EXIT_APP;

	m_msgMgr = std::make_unique<net_uv::NetMsgMgr>();
	m_msgMgr->setUserData(this);
	m_msgMgr->setSendCallback([](net_uv::NetMsgMgr* mgr, uint32_t sessionID, char* data, uint32_t len)
	{
		auto ins = reinterpret_cast<GMasterNodeService*>(mgr->getUserData());
		ins->m_svr->send(sessionID, data, len);
	});
	m_msgMgr->setCloseSctCallback([](net_uv::NetMsgMgr* mgr, uint32_t sessionID) 
	{
		auto ins = reinterpret_cast<GMasterNodeService*>(mgr->getUserData());
		ins->m_svr->disconnect(sessionID);
	});
	m_msgMgr->setOnMsgCallback([](net_uv::NetMsgMgr* mgr, uint32_t sessionID, uint32_t msgID, char* data, uint32_t len)
	{
		auto ins = reinterpret_cast<GMasterNodeService*>(mgr->getUserData());
		ins->onRecvMsg(sessionID, msgID, data, len);
	});
	m_msgMgr->setHeartBeatTime(3);
	m_msgMgr->setHeartBeatLoseMaxCount(3);

	m_noticeCenter = std::make_unique<GNoticeCenter>();

	return SCODE_START_SUCCESS;
}

void GMasterNodeService::onStartService()
{}

void GMasterNodeService::onStopService()
{
	m_svr->stopServer();
}

void GMasterNodeService::onUpdate(float)
{
	m_svr->updateFrame();
	m_msgMgr->updateFrame();
	switch (this->m_status)
	{
	case GServiceStatus::RUNNING:
	{}break;
	case GServiceStatus::STOP_ING:
	{
		if (m_svr->isCloseFinish())
		{
			stopServiceFinish();
		}
	}break;
	default:
		break;
	}
}

void GMasterNodeService::onDestroy()
{}

void GMasterNodeService::sendMsg(uint32_t slaveNodeID, uint32_t msgID, char* data, uint32_t len)
{
	for (auto& it : m_arrSlaveNodInfos)
	{
		if (it.slaveId == slaveNodeID)
		{
			m_msgMgr->sendMsg(it.sessionId, msgID, data, len);
			break;
		}
	}
}

void GMasterNodeService::onNewConnectCallback(net_uv::Server* svr, net_uv::Session* session)
{
	LOG(INFO) << "new connect" << session->getSessionID();
	m_msgMgr->onConnect(session->getSessionID());
}

void GMasterNodeService::onRecvCallback(net_uv::Server* svr, net_uv::Session* session, char* data, uint32_t len)
{
	m_msgMgr->onBuff(session->getSessionID(), data, len);
}

void GMasterNodeService::onDisconnectCallback(net_uv::Server* svr, net_uv::Session* session)
{
	LOG(INFO) << "disconnect:" << session->getSessionID();
	m_msgMgr->onDisconnect(session->getSessionID());

	for (auto it = m_arrSlaveNodInfos.begin(); it != m_arrSlaveNodInfos.end();)
	{
		if (it->sessionId == session->getSessionID())
		{
			auto slaveId = it->slaveId;
			LOG(INFO) << "SlaveNode[" << slaveId << "] off-line, GroupID: " << m_groupID << "SessionID:" << session->getSessionID();
			it = m_arrSlaveNodInfos.erase(it);
			m_noticeCenter->emitEvent(NSMsg::MGS_KEY_SLAVE_NODE_LEV, slaveId);
			break;
		}
		else
		{
			++it;
		}
	}
}

void GMasterNodeService::onRecvMsg(uint32_t sessionID, uint32_t msgID, char* data, uint32_t len)
{
	switch (msgID)
	{
	case NSMsg::MSG_REG_SERVER_NODE_REQ:
	{
		auto msg = reinterpret_cast<NSMsg::RegServerReq*>(data);
		auto token = msg->token;
		auto slaveId = msg->serviceId;

		// 组id不同
		if (msg->groupID != m_groupID)
		{
			NSMsg::RegServerAck ack;
			ack.code = -1;
			ack.token = 0;
			m_msgMgr->sendMsg(sessionID, NSMsg::MSG_REG_SERVER_NODE_ACK, (char*)&ack, sizeof(ack));
			GApplication::getInstance()->runWithNextFrame([=]() {
				m_svr->disconnect(sessionID);
				});
			LOG(INFO) << "SlaveNode[" << slaveId << "] Login failed, GroupID: " << msg->groupID;
			return;
		}

		// 已注册
		for (auto& it : m_arrSlaveNodInfos)
		{
			if (it.slaveId == slaveId)
			{
				NSMsg::RegServerAck ack;
				ack.code = -2;
				ack.token = 0;
				m_msgMgr->sendMsg(sessionID, NSMsg::MSG_REG_SERVER_NODE_ACK, (char*)&ack, sizeof(ack));
				LOG(ERROR) << "SlaveNode[" << slaveId << "] repeat Login";
				return;
			}
		}

		bool isFirst = false;
		auto itSlaveId = m_tokenSlaveIdCacheMap.find(token);
		if (itSlaveId != m_tokenSlaveIdCacheMap.end())
		{
			// 服务id和之前记录的token对不上
			if (slaveId != itSlaveId->second)
			{
				NSMsg::RegServerAck ack;
				ack.code = -3;
				ack.token = 0;
				m_msgMgr->sendMsg(sessionID, NSMsg::MSG_REG_SERVER_NODE_ACK, (char*)&ack, sizeof(ack));
				LOG(ERROR) << "SlaveNode[" << slaveId << "] bad token";
				return;
			}
		}
		else
		{
			// 子节点过多
			if (m_tokenSlaveIdCacheMap.size() > 1000000)
			{
				NSMsg::RegServerAck ack;
				ack.code = -4;
				ack.token = 0;
				m_msgMgr->sendMsg(sessionID, NSMsg::MSG_REG_SERVER_NODE_ACK, (char*)&ack, sizeof(ack));
				GApplication::getInstance()->runWithNextFrame([=]() {
					m_svr->disconnect(sessionID);
					});
				LOG(ERROR) << "SlaveNode[" << sessionID << "] Login failed, Insufficient server resources";
				return;
			}

			// 保证token唯一
			do
			{
				token = GMisc::randomUint32_t(100, INT_MAX - 100);
			} while (m_tokenSlaveIdCacheMap.find(token) != m_tokenSlaveIdCacheMap.end());

			// 清除之前的token映射关系
			for (auto it = m_tokenSlaveIdCacheMap.begin(); it != m_tokenSlaveIdCacheMap.end(); )
			{
				if (it->second == slaveId)
				{
					it = m_tokenSlaveIdCacheMap.erase(it);
				}
				else
				{
					++it;
				}
			}
			m_tokenSlaveIdCacheMap[token] = slaveId;

			// 子节点异常，数量过多
			if (m_tokenSlaveIdCacheMap.size() > 1000)
			{
				LOG(ERROR) << "token size: " << m_tokenSlaveIdCacheMap.size();
				LOG(ERROR) << "Received a large number of attack messages: NSMsg::MSG_REG_SERVER_NODE_REQ";
			}
			isFirst = true;
		}

		NSMsg::RegServerAck ack;
		ack.code = 0;
		ack.token = token;

		bool find = false;
		for (auto& it : m_arrSlaveNodInfos)
		{
			if (it.sessionId == sessionID)
			{
				find = true;
				G_ASSERT(false);
				LOG(ERROR) << "SlaveNode[" << sessionID << "] repeat Login";
			}
		}

		if (!find)
		{
			m_arrSlaveNodInfos.push_back(SlaveNodeInfo());

			auto& node = m_arrSlaveNodInfos.back();
			node.sessionId = sessionID;
			node.slaveId = slaveId;
			node.info.assign(data + sizeof(NSMsg::RegServerReq), msg->infoLen);

			rapidjson::StringStream stream(node.info.c_str());
			node.infoJson.ParseStream<0>(stream);
			if (node.infoJson.HasParseError())
			{
				LOG(ERROR) << "[GMasterNodeService] json parse error : " << node.infoJson.GetParseError();
				LOG(ERROR) << "error json: " << node.info;
				ack.code = -5;
				ack.token = 0;
				if (msg->token <= 0)
				{
					m_tokenSlaveIdCacheMap.erase(m_tokenSlaveIdCacheMap.find(token));
				}
			}

			// 注册成功
			if (ack.code == 0)
			{
				if (isFirst)
				{
					m_noticeCenter->emitEvent(NSMsg::MGS_KEY_FIRST_REG_SLAVE_NODE, slaveId);
					LOG(INFO) << "SlaveNode[" << sessionID << "] login successful, GroupID: " << msg->groupID;
				}
				else
				{
					LOG(INFO) << "SlaveNode[" << sessionID << "] reconnection successful, GroupID: " << msg->groupID;
				}
				m_noticeCenter->emitEvent(NSMsg::MGS_KEY_SLAVE_NODE_ONLINE, slaveId);
			}
			else
			{
				LOG(INFO) << "SlaveNode[" << sessionID << "] Login failed, code: " << ack.code;
			}
		}

		m_msgMgr->sendMsg(sessionID, NSMsg::MSG_REG_SERVER_NODE_ACK, (char*)&ack, sizeof(ack));

	}break;
	default:
		for (auto& it : m_arrSlaveNodInfos)
		{
			if (it.sessionId == sessionID)
			{
				m_noticeCenter->emitEvent(StringUtils::msgKey(msgID), it.slaveId, data, len);
				return;
			}
		}
		m_svr->disconnect(sessionID);
		LOG(ERROR) << "Invalid message ID received, msgID: " << msgID << ", sessionID:" << sessionID;
		break;
	}
}

SlaveNodeInfo* GMasterNodeService::getSlaveNodeInfo(uint32_t slaveNodeID)
{
	for (auto & it : m_arrSlaveNodInfos)
	{
		if (it.slaveId == slaveNodeID)
			return &it;
	}
	return NULL;
}
