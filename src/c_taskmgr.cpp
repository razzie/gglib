#include "threadglobal.hpp"
#include "c_taskmgr.hpp"

using namespace gg;

static thread_global<gg::thread*> s_threads;


class wait_task : public task
{
    uint32_t m_wait;
    uint32_t m_elapsed;

public:
    wait_task(uint32_t wait_time)
     : task("wait"), m_wait(wait_time), m_elapsed(0) {}
    ~wait_task() {}

    bool run(uint32_t elapsed)
    {
        m_elapsed += elapsed;
        return (m_elapsed > m_wait);
    }
};

class function_task : public task
{
    std::function<bool(uint32_t)> m_func;

public:
    function_task(const std::function<bool(uint32_t)>& f)
     : task("function"), m_func(f) {}
    ~function_task() {}

    bool run(uint32_t elapsed)
    {
        return m_func(elapsed);
    }
};

/*class kill_thread_task : public task
{
public:
    kill_thread_task() {}
    ~kill_thread_task() {}

    bool run(uint32_t)
    {
        delete static_cast<c_thread*>( task_manager::get_current_thread() );
        return true;
    }
};*/


c_thread::c_thread(std::string name)
 : m_name(name)
 , m_thread(
    [](void* o) { static_cast<c_thread*>(o)->mainloop(); },
    static_cast<void*>(this) )
{
}

c_thread::~c_thread()
{
    this->exit_and_join();

    m_task_pool_mutex.lock();
    m_tasks.insert(m_tasks.end(), m_task_pool.begin(), m_task_pool.end());
    m_task_pool.clear();
    m_task_pool_mutex.unlock();

    auto it = m_tasks.begin(), end = m_tasks.end();
    for (; it != end; ++it)
    {
        delete it->m_timer;
        it->m_task->drop();
    }
}

std::string c_thread::get_name() const
{
    return m_name;
}

void c_thread::add_task(task* t)
{
    m_task_pool_mutex.lock();
    m_task_pool.push_back({t, new c_timer()});
    m_task_pool_mutex.unlock();

    m_cond.notify_all();
}

void c_thread::add_delayed_task(task* t, uint32_t delay_ms)
{
    task* parent = new wait_task(delay_ms);
    parent->add_child(t);
    this->add_task(parent);
}

void c_thread::suspend()
{
    m_suspended = true;
}

void c_thread::resume()
{
    m_suspended = false;
    m_cond.notify_all();
}

void c_thread::exit_and_join()
{
    this->finish();
    if (m_thread.joinable()) m_thread.join();
}

void c_thread::wait_for_cond()
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);
    m_cond.wait(m_cond_mutex);
}

void c_thread::finish()
{
    m_finished = true;
    m_cond.notify_all();
}

void c_thread::mainloop()
{
    thread_global<thread*>::scope thread_scope(&s_threads, this);

    for(;;)
    {
        // there are no tasks to run or thread was suspended
        if ((m_tasks.size() == 0 && m_task_pool.size() == 0)
            || m_suspended)
        {
            this->wait_for_cond();
        }

        // someone called exit_and_join()
        if (m_finished) return;

        // removing task from active list at the end of cycle, as the it is either finished or moved to the pool
        for (auto it = m_tasks.begin(); it != m_tasks.end(); it = m_tasks.erase(it))
        {
            // if either someone called exit_and_join() or suspend() we go to outer loop
            if (m_finished || m_suspended) break;

            // if an exception is thrown, we better remove the task asap
            bool result = true;

            try
            {
                result = it->m_task->run( it->m_timer->get_elapsed() );
            }
            catch (std::exception& e)
            {
                std::cerr << "exception caught while running task '" << it->m_task->get_name() << "': " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "unknown exception caught while running task '" << it->m_task->get_name() << "'" << std::endl;
            }

            if (result) // run() returned 'true', so let's remove it
            {
                auto subs = it->m_task->get_children();
                auto sub_it = subs.begin(), sub_end = subs.end();

                // adding child tasks to the pool
                for (; sub_it != sub_end; ++sub_it)
                {
                    m_task_pool.push_back({*sub_it, new c_timer()});
                }

                // deleting task (and its timer) as it is finished now
                delete it->m_timer;
                it->m_task->drop();
            }
            else // task is not finished, so let's put it to pool
            {
                // moving task to pool again
                m_task_pool_mutex.lock();
                m_task_pool.push_back(*it);
                m_task_pool_mutex.unlock();
            }
        }

        // switching active and pool task lists
        m_task_pool_mutex.lock();
        std::swap(m_tasks, m_task_pool);
        m_task_pool_mutex.unlock();
    }
}


c_task_manager::c_task_manager(application* app)
 : m_app(app)
{
}

c_task_manager::~c_task_manager()
{
    auto it = m_threads.begin();

    while (it != m_threads.end())
    {
        if (it->second != nullptr)
        {
            delete it->second;
            it->second = nullptr;
        }

        it = m_threads.erase(it);
    }
}

application* c_task_manager::get_app() const
{
    return m_app;
}

gg::thread* c_task_manager::create_thread(std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto ret = m_threads.insert( std::make_pair(name, new c_thread(name)) );

    if (!ret.second)
        throw std::runtime_error("failed to create thread");

    return ret.first->second;
}

gg::thread* c_task_manager::get_thread(std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_threads.find(name);
    if (it == m_threads.end())
        return nullptr;
    else
        return it->second;
}

void c_task_manager::async_invoke(std::function<void()> func) const
{
    struct async_invoke_data
    {
        std::function<void()> m_func;
        tthread::thread* m_thread;
    };

    tthread::thread* async_invoke_thread = new tthread::thread(
        [](void* ptr)
        {
            async_invoke_data* f = static_cast<async_invoke_data*>(ptr);
            tthread::thread* t = f->m_thread;

            try
            {
                f->m_func();
            }
            catch (std::exception& e)
            {
                std::cout << "async_invoke caught exception: " << e.what() << std::endl;
            }

            delete f;
            delete t;
        },
        static_cast<void*>( new async_invoke_data {func} ));

    async_invoke_thread->detach();
}

task* c_task_manager::create_task(std::function<void()> func) const
{
    return new function_task(
        [func](uint32_t) -> bool { func(); return true; });
}

task* c_task_manager::create_wait_task(uint32_t wait_time) const
{
    return new wait_task(wait_time);
}

task* c_task_manager::create_persistent_task(std::function<bool(uint32_t)> func) const
{
    return new function_task(func);
}

thread* task_manager::get_current_thread()
{
    optional<thread*> t = s_threads.get();
    return (t.is_valid() ? t.get() : nullptr);
}
