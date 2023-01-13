#include "GCoroManager.h"
#define MINICORO_IMPL
#include "minicoro.h"
#include "../GApplication.h"

struct CoroEnv
{
    CoroEnv(const std::function<void()>& task, const std::function<void()>& finalization, mco_coro* ctx, int32_t id)
    {
        this->task = task;
        this->finalization = finalization;
        this->ctx = ctx;
        this->id = id;
    }
    std::function<void()> task;
    std::function<void()> finalization;
    mco_coro* ctx;
    int32_t id;
};

void coro_entry(mco_coro* co)
{
    auto env = reinterpret_cast<CoroEnv*>(co->user_data);
    env->task();
}



GCoroManager::GCoroManager()
{
    m_seed = 0;
    m_coros.reserve(100);
}

GCoroManager::~GCoroManager()
{
    for (auto& coenv : m_coros)
    {
        if (coenv)
        {
            mco_destroy(coenv->ctx);
            if (coenv->finalization) coenv->finalization();
        }
    }
    m_coros.clear();
}

int32_t GCoroManager::spawn(const std::function<void()>& entry, const std::function<void()>& finalization)
{
    // stack size 128kb
    mco_desc desc = mco_desc_init(coro_entry, 128 * 1024);
    mco_coro* co;
    mco_result res = mco_create(&co, &desc);
    assert(res == MCO_SUCCESS);

    CoroEnv* env = new CoroEnv(entry, finalization, co, m_seed++);
    co->user_data = env;

    for (auto& coenv : m_coros)
    {
        if (!coenv)
        {
            coenv.reset(env);
            return env->id;
        }
    }
    m_coros.emplace_back(env);

    return env->id;
}

void GCoroManager::update()
{
    if (m_coros.empty())
        return;

    mco_result res;
    for (auto& coenv : m_coros)
    {
        if (coenv)
        {
            if (coenv->id < 0)
            {
                mco_destroy(coenv->ctx);
                if (coenv->finalization) coenv->finalization();
                coenv.reset();
            }
            else
            {
                // https://github.com/edubart/minicoro/issues/2
                // minicoro cannot catch the exception thrown by the coroutine task
#if 0
                try
                {
                    res = mco_resume(coenv->ctx);
                }
                catch (const std::exception& e)
                {
                    res = MCO_GENERIC_ERROR;
                    LogFatal() << "mco_resume std exception:" << e.what();
                }
                catch (...)
                {
                    res = MCO_GENERIC_ERROR;
                    LogFatal() << "mco_resume unknow exception";
                }
#else
                res = mco_resume(coenv->ctx);
#endif

                if (mco_status(coenv->ctx) != MCO_SUSPENDED || res != MCO_SUCCESS)
                {
                    coenv->id = -1;
                    mco_destroy(coenv->ctx);
                    if (coenv->finalization) coenv->finalization();
                    coenv.reset();
                }
            }
        }
    }
}

void GCoroManager::kill(int32_t co_id)
{
    for (auto& coenv : m_coros)
    {
        if (coenv && coenv->id == co_id)
        {
            coenv->id = -1;
            break;
        }
    }
}

void GCoroManager::kill_ptr(void* co)
{
    for (auto& coenv : m_coros)
    {
        if (coenv && coenv->ctx == co)
        {
            coenv->id = -1;
            break;
        }
    }
}

void coro_await(float timeout, const std::function<bool()>& condition)
{
    if (timeout < 0.0f)
        timeout = 0.0f;

    auto application = GApplication::getInstance();
    auto start = application->getRunTime();
    do
    {
        coro_yield();
    } while (application->getRunTime() - start < timeout && condition());
}

void coro_sleep(float second)
{
    if (second < 0.0f)
        second = 0.0f;

    auto application = GApplication::getInstance();
    auto start = application->getRunTime();
    do
    {
        coro_yield();
    } while (application->getRunTime() - start < second);
}


void coro_skill(int32_t co_id)
{
    GApplication::getInstance()->getCoroManager().kill(co_id);
}

int32_t coro_spawn(const std::function<void()>& entry, const std::function<void()>& finalization)
{
    return GApplication::getInstance()->getCoroManager().spawn(entry, finalization);
}

void coro_skill_self()
{
    GApplication::getInstance()->getCoroManager().kill_ptr(mco_running());
}

void coro_yield()
{
    mco_result res = mco_yield(mco_running());
    if (res != MCO_SUCCESS)
    {
        LogError() << "mco_yield error:" << res;
    }
}
