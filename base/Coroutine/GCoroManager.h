#pragma once

#include "minicoro.h"
#include <vector>
#include <memory>
#include <functional>

struct CoroEnv;
class GCoroManager final
{
public:

    GCoroManager();

    ~GCoroManager();

public:

    int32_t spawn(const std::function<void()>& entry, const std::function<void()>& finalization = nullptr);

    void update();

    void kill(int32_t co_id);

    void kill_ptr(void* co);

private:

    std::vector<std::unique_ptr<CoroEnv>> m_coros;
    int32_t m_seed;
};


int32_t coro_spawn(const std::function<void()>& entry, const std::function<void()>& finalization = nullptr);
void coro_skill(int32_t co_id);
void coro_skill_self();
void coro_await(float timeout, const std::function<bool()>& condition);
void coro_sleep(float second);
void coro_yield();
