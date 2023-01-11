#pragma once

#include "GIService.h"

#if ORMPP_ENABLE_MYSQL | ORMPP_ENABLE_SQLITE3 | ORMPP_ENABLE_PG
#else
#define ORMPP_ENABLE_MYSQL 1
#endif

#ifdef ORMPP_ENABLE_MYSQL
#include "ormpp/mysql.hpp"
#endif

#ifdef ORMPP_ENABLE_SQLITE3
#include "ormpp/sqlite.hpp"
#endif

#ifdef ORMPP_ENABLE_PG
#include "ormpp/postgresql.hpp"
#endif

#include "ormpp/connection_pool.hpp"
#include "ormpp/dbng.hpp"

#ifdef ORMPP_ENABLE_MYSQL
typedef ormpp::dbng<ormpp::mysql> dbng_type;
#endif

#ifdef ORMPP_ENABLE_SQLITE3
#include "ormpp/sqlite.hpp"
typedef ormpp::dbng<ormpp::sqlite> dbng_type;
#endif

#ifdef ORMPP_ENABLE_PG
#include "ormpp/postgresql.hpp"
typedef ormpp::dbng<ormpp::postgresql> dbng_type;
#endif

struct DbngHandle
{
	DbngHandle() = delete;
	DbngHandle(const DbngHandle&) = delete;
	DbngHandle(std::shared_ptr<dbng_type> ptr)
	{
		m_ptr = ptr;
	}

	~DbngHandle()
	{
		ormpp::connection_pool<dbng_type>::instance().return_back(m_ptr);
		m_ptr = nullptr;
	}

	explicit operator bool()const noexcept { return m_ptr != nullptr; }

	dbng_type* operator->() const { return m_ptr.get(); }

private:
	std::shared_ptr<dbng_type> m_ptr;
};

/// 数据库服务
class GDBService final : public GIService
{
public:
	G_DEFINE_SERVICE(GDBService);
	
	virtual uint32_t onInit() override;

	virtual void onStopService() override;

	DbngHandle get_dbng();
};

static bool check_dbng_has_error(const DbngHandle& handle)
{
	if (handle)
	{
		if (handle->has_error())
		{
			LogError() << "dbng error: " << handle->get_last_error();
			return true;
		}
		return false;
	}
	LogError() << "dbng error: null_ptr";
	return true;
}

