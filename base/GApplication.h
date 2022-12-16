#pragma once

#include "net_uv.h"
#include "GScheduler.h"
#include "Service/GServiceMgr.h"
#include "Coroutine/GCoroManager.h"

class GApplication
{
public:

	GApplication(int argc, char** argv);

	virtual ~GApplication();

	static GApplication* getInstance();

	int32_t run(uint32_t interval = 1000U / 60U);
	
	void end();

public:

	void updateFPS();

	void runWithNextFrame(const std::function<void()>& callback);

public:

	G_FORCEINLINE float getRunTime() const;

	G_FORCEINLINE uint32_t getRunTime32() const;

	G_FORCEINLINE GServiceMgr* getServiceMgr() const;

	G_FORCEINLINE const char* getAppName() const;

private:

	void init();

	void mainLoop();

	G_SYNTHESIZE_READONLY(float, m_deltaTime, DeltaTime);
	G_SYNTHESIZE_READONLY(uint64_t, m_deltaTimeMilli, DeltaTimeMillisecond);

	G_SYNTHESIZE_READONLY(uint64_t, m_startTime, StartTime);
	G_SYNTHESIZE_READONLY(uint64_t, m_lastTime, LastTime);

	G_SYNTHESIZE_READONLY(int32_t, m_fps, FPS);

	G_SYNTHESIZE_READONLY(GScheduler*, m_scheduler, Scheduler);
	G_SYNTHESIZE_READONLY(uv_loop_t*, m_loop, Loop);

	G_SYNTHESIZE_READONLY_REF(GCoroManager, m_coroManager, CoroManager);

private:
	std::unique_ptr<GServiceMgr> m_serviceMgr;

	bool m_isStart;
	int32_t m_fpst;

	uv_timer_t m_updateTimer;
	uv_timer_t m_fpsTimer;

	std::string m_appName;

	std::vector<std::function<void()>> m_runWithNextFrameCalls;

	static GApplication* uniqueInstance;
};


uint32_t GApplication::getRunTime32() const
{
	return (uint32_t)(uv_now(m_loop) - m_startTime);
}

float GApplication::getRunTime() const
{
	return (float)((uv_now(m_loop) - m_startTime) / 1000.0f);
}

GServiceMgr* GApplication::getServiceMgr() const
{
	return m_serviceMgr.get();
}

const char* GApplication::getAppName() const
{
	return m_appName.c_str();
}
