﻿#pragma once

#include "GIService.h"
#include "GNoticeCenter.h"
#include "GTypes.h"

#include "rapidjson/stream.h"
#include "rapidjson/document.h"
#include "rapidjson/reader.h"

/// 服务器同步-从节点服务
class GSlaveNodeService final : public GIService
{
public:

	static std::string MGS_KEY_SLAVE_NODE_DISCONNECT;

public:
	G_DEFINE_SERVICE(GSlaveNodeService);

	GSlaveNodeService();

	virtual ~GSlaveNodeService();

	virtual uint32_t onInit() override;

	virtual void onStartService() override;

	virtual void onStopService() override;

	virtual void onUpdate(float) override;

	virtual void onDestroy() override;

	G_FORCEINLINE bool isOnline();

	G_FORCEINLINE GNoticeCenter* noticeCenter();

	G_FORCEINLINE const rapidjson::Document& descriptionJson();

	void sendMsg(uint32_t msgID, char* data, uint32_t len);

	G_SYNTHESIZE_READONLY(GNetType, m_netType, NetType);

	G_SYNTHESIZE_READONLY(std::string, m_nodeIP, NodeIP);

	G_SYNTHESIZE_READONLY(uint32_t, m_nodePort, NodePort);

	G_SYNTHESIZE_READONLY(int32_t, m_groupID, GroupID);

	G_SYNTHESIZE_READONLY(std::string, m_description, Description);

	G_SYNTHESIZE_READONLY(int32_t, m_serviceId, ServiceId)
	
protected:

	void onConnectCallback(net_uv::Client* client, net_uv::Session* session, int32_t status);

	void onDisconnectCall(net_uv::Client* client, net_uv::Session* session);

	void onRecvCall(net_uv::Client* client, net_uv::Session* session, char* data, uint32_t len);

	void onCloseCall(net_uv::Client* client);

	void onRemoveSessionCall(net_uv::Client* client, net_uv::Session* session);

	void onRecvMsg(uint32_t sessionID, uint32_t msgID, char* data, uint32_t len);
	
private:
	std::unique_ptr<net_uv::Client>	   m_client;
	std::unique_ptr<net_uv::NetMsgMgr> m_msgMgr;
	std::unique_ptr<GNoticeCenter>     m_noticeCenter;

	rapidjson::Document m_descriptionJson;
	bool		m_isOnline;
	bool		m_stopReconnect;
	uint32_t		m_token;
};

bool GSlaveNodeService::isOnline()
{
	return m_isOnline;
}

GNoticeCenter* GSlaveNodeService::noticeCenter()
{
	return m_noticeCenter.get();
}
const rapidjson::Document& GSlaveNodeService::descriptionJson()
{
	return m_descriptionJson;
}
