#include "GDBService.h"
#include "GConfigService.h"
#include "GServiceMgr.h"
#include "GApplication.h"
#include "GMasterNodeService.h"
#include "Utils/GStringUtils.h"
#include "ormpp/ormpp_cfg.hpp"

using namespace ormpp;

uint32_t GDBService::onInit()
{
	G_CHECK_SERVICE(GConfigService);

	auto cfgService = m_serviceMgr->getService<GConfigService>();
	auto& ini = cfgService->iniReader();
	auto appName = GApplication::getInstance()->getAppName();

	// 不需要此服务
	if (ini.GetBoolean(appName, "DBServiceEnable", false) == false)
		return SCODE_START_FAIL_NO_ERR;

	auto dbName = ini.Get(appName, "DBService_Cfg", "");
	if (dbName.empty())
	{
		LOG(ERROR) << "DBService_Cfg is empty";
		return SCODE_START_FAIL_EXIT_APP;
	}

	ormpp_cfg cfg{};
	bool ret = config_manager::from_file(cfg, dbName);
	if (!ret) {
		LOG(ERROR) << "ormpp_cfg init failed: " << dbName;
		return SCODE_START_FAIL_EXIT_APP;
	}

	try {
		ormpp::connection_pool<dbng_type>::instance().init(
			cfg.db_conn_num,
			cfg.db_ip.data(), 
			cfg.user_name.data(),
			cfg.pwd.data(), 
			cfg.db_name.data(), 
			cfg.timeout, 
			cfg.db_port);
	}
	catch (const std::exception& e) {
		LOG(ERROR) << "ormpp::connection_pool failed: " << e.what();
		return SCODE_START_FAIL_EXIT_APP;
	}

	return SCODE_START_SUCCESS;
}

void GDBService::onStopService()
{
	this->stopServiceFinish();
}

DbngHandle GDBService::get_dbng()
{
	auto ptr = ormpp::connection_pool<dbng_type>::instance().get();
	if (ptr == nullptr)
	{
		LOG(ERROR) << "No idle connection";
	}
	return ptr;
}
