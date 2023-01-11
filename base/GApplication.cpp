#include "GApplication.h"
#include "Utils/cmd.h"
#include "Platform/GFileSystem.h"
#include "Utils/GStringUtils.h"
#include "Utils/CrashReport.h"
#include "Utils/GEnableHighPrecisionTimer.h"

INITIALIZE_EASYLOGGINGPP;

GApplication* GApplication::uniqueInstance = NULL;

GApplication* GApplication::getInstance()
{
	return uniqueInstance;
}

GApplication::GApplication(int argc, char** argv)
{
	uniqueInstance = this;
	m_scheduler = NULL;
	m_startTime = 0;
	m_lastTime = 0;
	m_deltaTimeMilli = 0;
	m_deltaTime = 0.0f;
	m_loop = NULL;
	m_fps = 0;
	m_fpst = 0;
	m_appName = "";
	m_isStart = false;

	m_runWithNextFrameCalls.reserve(100);

	cmd::init_cmd_params(argc, argv);
	m_appName = cmd::try_get("name");

	if (m_appName.empty())
	{
		std::string executable = GFileSystem::getExeName();
		std::string ext = GFileSystem::getFileExtension(executable);
		m_appName = executable.substr(0, executable.size() - ext.size());

		if (m_appName.size() > 2 && m_appName.substr(m_appName.size() - 2) == "_d")
		{
			m_appName = m_appName.substr(0, m_appName.size() - 2);
		}
	}

	GFileSystem::setCwd(cmd::try_get("cwd", GFileSystem::getExeDirectory()));

	///////////////////////////////////// crash report /////////////////////////////////////
	SetCrashReport(m_appName.c_str());

	///////////////////////////////////// init log /////////////////////////////////////
	logConfiguration();

	///////////////////////////////////// init /////////////////////////////////////
	init();
}

GApplication::~GApplication()
{
}

void GApplication::logConfiguration()
{
	el::Configurations conf;

	if (!conf.parseFromFile(cmd::try_get("log_conf", "easylog.conf")))
	{
		conf.setToDefault();

		conf.setGlobally(el::ConfigurationType::Enabled, "true");
		conf.setGlobally(el::ConfigurationType::ToFile, "true");
		conf.setGlobally(el::ConfigurationType::MillisecondsWidth, "3");
		// 10MB
		conf.setGlobally(el::ConfigurationType::MaxLogFileSize, "10485760");

		conf.setGlobally(el::ConfigurationType::Format, std::string("%datetime{%Y-%M-%d %H:%m:%s,%g} %levshort/%logger [%file:%line] %msg"));
		//conf.setGlobally(el::ConfigurationType::Format, std::string("%datetime{%Y-%M-%d %H:%m:%s,%g} %levshort/%logger [%file:%line] [%func] %msg"));
		//conf.set(el::Level::Debug,	 el::ConfigurationType::Format, std::string("%datetime{%Y-%M-%d %H:%m:%s,%g} %level [%logger] [%func] %msg"));
		//conf.set(el::Level::Error,	 el::ConfigurationType::Format, std::string("%datetime{%Y-%M-%d %H:%m:%s,%g} %level [%logger] [%file]:[%line] [%func] %msg"));
		//conf.set(el::Level::Fatal,	 el::ConfigurationType::Format, std::string("%datetime{%Y-%M-%d %H:%m:%s,%g} %level [%logger] [%file]:[%line] %msg"));
		//conf.set(el::Level::Trace,	 el::ConfigurationType::Format, std::string("%datetime{%Y-%M-%d %H:%m:%s,%g} %level [%logger] [%file]:[%line] %msg"));
	}

#if G_TARGET_PLATFORM == G_PLATFORM_WIN32
	auto logDir = StringUtils::format("log\\%s\\", m_appName.c_str());
#else
	auto logDir = StringUtils::format("log/%s/", m_appName.c_str());
#endif
	conf.setGlobally(el::ConfigurationType::Filename, logDir + "log_%datetime{%Y%M%d}.log");

	// 选择划分级别的日志
	el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);
	// 启用颜色输出
	el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
	// 设置级别门阀值，修改参数可以控制日志输出
	el::Loggers::setLoggingLevel(el::Level::Debug);
	// 设置为默认配置
	el::Loggers::setDefaultConfigurations(conf, true);

	el::Loggers::getLogger(ELPP_DEFAULT_LOGGER);
}

void GApplication::init()
{
	LogInfo() << "-----------application init-----------";
	m_scheduler = GScheduler::getInstance();
	m_serviceMgr = std::make_unique<GServiceMgr>();

#if G_TARGET_PLATFORM == G_PLATFORM_WIN32
	auto menu = ::GetSystemMenu(GetConsoleWindow(), FALSE);
	::RemoveMenu(menu, SC_CLOSE, 0);
	::SetConsoleCtrlHandler([](DWORD cEvent)->BOOL {
		switch (cEvent)
		{
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		{
			auto ret = ::MessageBox(NULL, TEXT("Exit Process ?"), TEXT("Warning"), MB_YESNO);
			if (ret == IDYES)
			{
				GApplication::getInstance()->end();
			}
			return TRUE;
		}
		default:
			break;
		}
		return TRUE;
	}, TRUE);
#endif
}

int32_t GApplication::run(uint32_t interval)
{
	LogInfo() << "-----------application run-----------";
	m_isStart = true;
	m_lastTime = 0;
	m_loop = uv_loop_new();

	std::unique_ptr<GEnableHighPrecisionTimer> enableHighPrecisionTimer;
	if (interval < 1000 / 60)
	{
		enableHighPrecisionTimer = std::make_unique<GEnableHighPrecisionTimer>(1);
	}

	// timer
	uv_timer_init(m_loop, &m_updateTimer);
	m_updateTimer.data = this;
	uv_timer_start(&m_updateTimer, [](uv_timer_t* handle) {
		auto* self = static_cast<GApplication*>(handle->data);
		self->mainLoop();
	}, 0, interval);

	// fps timer
	uv_timer_init(m_loop, &m_fpsTimer);
	m_fpsTimer.data = this;
	uv_timer_start(&m_fpsTimer, [](uv_timer_t* handle) {
		auto* self = static_cast<GApplication*>(handle->data);
		self->updateFPS();
	}, 0, 1000);

	// loop
	uv_run(m_loop, UV_RUN_DEFAULT);

	m_serviceMgr->destroy();
	m_serviceMgr = NULL;
	m_scheduler->unScheduleAll();

	uv_loop_delete(m_loop);

	LogInfo() << "-----------application exit-----------";

	UnSetCrashReport();

	return 0;
}

void GApplication::end()
{
	if (!m_isStart)
	{
		return;
	}
	m_isStart = false;

	if (m_serviceMgr)
	{
		m_serviceMgr->stopAllService([=]() 
		{
			LogInfo() << "start stop loop.";
			uv_timer_stop(&m_updateTimer);
			uv_close((uv_handle_t*)&m_updateTimer, NULL);
			uv_timer_stop(&m_fpsTimer);
			uv_close((uv_handle_t*)&m_fpsTimer, NULL);			
			uv_stop(m_loop);
		});
	}
}

void GApplication::mainLoop()
{
	m_fpst++;
	if (m_lastTime <= 0)
	{
		m_lastTime = uv_now(m_loop);
		m_startTime = m_lastTime;
		m_serviceMgr->start();
		return;
	}
	m_deltaTimeMilli = uv_now(m_loop) - m_lastTime;
	m_deltaTime = m_deltaTimeMilli / 1000.0f;
	m_lastTime = uv_now(m_loop);

	if (!m_runWithNextFrameCalls.empty())
	{
		auto calls = m_runWithNextFrameCalls;
		m_runWithNextFrameCalls.clear();
		for (auto& call : calls)
		{
			call();
		}
	}

	m_scheduler->update(m_deltaTime);
	m_serviceMgr->update(m_deltaTime);
	m_coroManager.update();
}


void GApplication::updateFPS()
{
	m_fps = m_fpst;
	m_fpst = 0;
#if G_TARGET_PLATFORM == G_PLATFORM_WIN32
	char szTitle[64] = { 0 };
	sprintf(szTitle, "%s(fps:%d)", m_appName.c_str(), m_fps);
	::SetConsoleTitleA(szTitle);
#endif
}

void GApplication::runWithNextFrame(const std::function<void()>& callback)
{
	m_runWithNextFrameCalls.emplace_back(callback);
}


/// <summary>
/// WorkContext
/// </summary>
typedef std::tuple<
	std::function<void()>, 
	std::function<void()>,
	uv_work_t*> WorkContext;

static void work_cb(uv_work_t* req) 
{
	auto data = reinterpret_cast<WorkContext*>(req->data);
	std::get<0>(*data)();
}

static void after_work_cb(uv_work_t* req, int status) 
{
	auto data = reinterpret_cast<WorkContext*>(req->data);

	if (auto& callback = std::get<1>(*data))
	{
		callback();
	}

	delete req->data;
	delete req;
}

void GApplication::runAsyncWork(const std::function<void()>& work, const std::function<void()>& callback)
{
	auto work_req = new uv_work_t();
	work_req->data = new WorkContext(work, callback, work_req);

	auto r = uv_queue_work(m_loop, work_req, work_cb, after_work_cb);
	G_ASSERT(r == 0);
}
