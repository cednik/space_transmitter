#pragma once

#include <functional>
#include <list>

#include <time.hpp>

class DelayedTask {
public:
    typedef std::function<void()> Callback;
    typedef timeout::time_type Time;

    static void create(Time t, Callback fcn) {
        s_tasks.emplace_front(t, fcn);
    }
    static void process() {
        for (auto p_task = s_tasks.begin(); p_task != s_tasks.end();) {
            if (p_task->m_timeout) {
                if (p_task->m_task)
                    p_task->m_task();
                auto next = std::next(p_task);
                s_tasks.erase(p_task);
                p_task = next;
            } else {
                ++p_task;
            }
        }
    }
    DelayedTask(Time t, Callback fcn)
        : m_task(fcn), m_timeout(t)
    {}
private:
    Callback m_task;
    timeout m_timeout;

    static std::list<DelayedTask> s_tasks;
};

std::list<DelayedTask> DelayedTask::s_tasks;
