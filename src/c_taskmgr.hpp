#ifndef C_TASKMGR_HPP_INCLUDED
#define C_TASKMGR_HPP_INCLUDED

#include <map>
#include <set>
#include "tinythread.h"
#include "gg/taskmgr.hpp"
#include "c_timer.hpp"

namespace gg
{
    void async_invoke(std::function<void()> func);

    template<class M>
    class c_mutex : public mutex
    {
        M m_mutex;

    public:
        c_mutex() {}
        c_mutex(const c_mutex&) = delete;
        c_mutex(c_mutex&&) = delete;
        ~c_mutex() {}
        void lock() { m_mutex.lock(); }
        void unlock() { m_mutex.unlock(); }
    };

    class c_condition : public condition
    {
        struct cond_data
        {
            tthread::condition_variable cond;
            tthread::mutex cond_mutex;
        };

        struct timeout_data
        {
            tthread::condition_variable* cond;
            uint32_t timeout;
        };

        static void interrupt(void*); // timeout_data*

        tthread::mutex m_mutex;
        std::set<cond_data*> m_conds;

    public:
        c_condition();
        c_condition(const c_condition&) = delete;
        c_condition(c_condition&&) = delete;
        ~c_condition();
        void wait();
        void wait(uint32_t timeout_ms);
        void trigger();
    };

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
        void add_task(std::function<void()> func);
        void add_delayed_task(task*, uint32_t delay_ms);
        void add_delayed_task(std::function<void()> func, uint32_t delay_ms);
        void add_persistent_task(std::function<bool(uint32_t)> func);
        void add_delayed_persistent_task(std::function<bool(uint32_t)> func, uint32_t delay_ms);
        void suspend();
        void resume();
        void exit_and_join();
    };

    class c_task_manager : public gg::task_manager
    {
        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::map<std::string, c_thread*> m_threads;

    public:
        c_task_manager(application* app);
        ~c_task_manager();
        application* get_app() const;
        thread* create_thread(std::string name);
        thread* get_thread(std::string name);
        void async_invoke(std::function<void()> func) const;
        task* create_task(std::function<void()> func) const;
        task* create_wait_task(uint32_t wait_ms) const;
        task* create_persistent_task(std::function<bool(uint32_t)> func) const;
        mutex* create_mutex() const;
        mutex* create_recursive_mutex() const;
        condition* create_condition() const;
    };
};

#endif // C_TASKMGR_HPP_INCLUDED
