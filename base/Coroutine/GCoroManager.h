#pragma once

#include "minicoro.h"
#include <vector>


struct CoroEnv;
class GCoroManager final
{
public:

    GCoroManager();

    ~GCoroManager();

public:

    int32_t spawn(const std::function<void()>& entry);

    void update();

    void kill(int32_t co_id);

    void kill_ptr(void* co);

private:

    std::vector<std::unique_ptr<CoroEnv>> m_coros;
    int32_t m_seed;
};

void coro_yield();
void coro_skill(int32_t co_id);
void coro_skill_self();
