#ifndef C_TASKMGR_HPP_INCLUDED
#define C_TASKMGR_HPP_INCLUDED

#include "tinythread.h"
#include "gg/taskmgr.hpp"
#include "c_timer.hpp"

namespace gg
{
    class c_thread : public gg::thread
    {
        struct task_helper
        {
            task* m_task;
            c_timer* m_timer;
        };

        std::string m_name;
        tthread::thread m_thread;
        tthread::condition_variable m_cond;
        mutable tthread::mutex m_cond_mutex;
        mutable tthread::mutex m_task_pool_mutex;
        std::list<task_helper> m_tasks;
        std::list<task_helper> m_task_pool;
        volatile bool m_finished = false;
        volatile bool m_suspended = false;

        void wait_for_cond();
        void finish();
        void mainloop();

    public:
        c_thread(std::string name);
        c_thread(const c_thread&) = delete;
        c_thread(c_thread&&) = delete;
        ~c_thread();
        std::string get_name() const;
        void add_task(task*);
        void add_delayed_task(task*, uint32_t delay_ms);
        void suspend();
        void resume();
        void exit_and_join();
    };

    class c_task_manager : public gg::task_manager
    {
        std::map<std::string, c_thread*> m_threads;

    public:
        c_task_manager();
        ~c_task_manager();
        thread* create_thread(std::string name);
        thread* get_thread(std::string name);

        task* create_wait_task(uint32_t wait_time) const;
        task* create_periodic_task(std::function<bool(uint32_t)> func) const;
        task* create_periodic_task(std::function<bool()> func) const;
        task* create_task(std::function<void()> func) const;
    };
};

#endif // C_TASKMGR_HPP_INCLUDED
