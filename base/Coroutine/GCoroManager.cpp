#include "GCoroManager.h"
#define MINICORO_IMPL
#include "minicoro.h"
#include "../GApplication.h"

struct CoroEnv
{
    CoroEnv(const std::function<void()>& task, mco_coro* ctx, int32_t id)
    {
        this->task = task;
        this->ctx = ctx;
        this->id = id;
    }
    std::function<void()> task;
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
        }
    }
    m_coros.clear();
}

int32_t GCoroManager::spawn(const std::function<void()>& entry)
{
    // stack size 128kb
    mco_desc desc = mco_desc_init(coro_entry, 128 * 1024);
    mco_coro* co;
    mco_result res = mco_create(&co, &desc);
    assert(res == MCO_SUCCESS);

    CoroEnv* env = new CoroEnv(entry, co, m_seed++);
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
                coenv.reset();
            }
            else
            {
                try
                {
                    res = mco_resume(coenv->ctx);
                }
                catch (const std::exception& e)
                {
                    res = MCO_GENERIC_ERROR;
                    LOG(ERROR) << "mco_resume std exception:" << e.what();
                }
                catch (...)
                {
                    res = MCO_GENERIC_ERROR;
                    LOG(ERROR) << "mco_resume unknow exception";
                }

                if (mco_status(coenv->ctx) != MCO_SUSPENDED || res != MCO_SUCCESS)
                {
                    coenv->id = -1;
                    mco_destroy(coenv->ctx);
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

void coro_yield()
{
    mco_result res = mco_yield(mco_running());
    if (res != MCO_SUCCESS)
    {
        LOG(ERROR) << "mco_yield error:" << res;
    }
}

void coro_skill(int32_t co_id)
{
    GApplication::getInstance()->getCoroManager().kill(co_id);
}

void coro_sleep(float second)
{
    if (second < 0.0f)
        second = 0.0f;

    auto start = GApplication::getInstance()->getRunTime();
    do
    {
        coro_yield();
    } while (GApplication::getInstance()->getRunTime() - start < second);
}

void coro_skill_self()
{
    GApplication::getInstance()->getCoroManager().kill_ptr(mco_running());
}

