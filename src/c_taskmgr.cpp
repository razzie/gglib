#include <iostream>
#include "threadglobal.hpp"
#include "c_taskmgr.hpp"
#include "c_logger.hpp"

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
    function_task(std::function<bool(uint32_t)> func)
     : task("persistent function task"), m_func(func) {}

    function_task(std::function<void()> func)
     : task("function task"), m_func([func](uint32_t) -> bool { func(); return true; }) {}

    ~function_task() {}

    bool run(uint32_t elapsed)
    {
        return m_func(elapsed);
    }
};


void gg::async_invoke(std::function<void()> func)
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
                *c_logger::get_instance() << "async_invoke caught exception: " << e.what() << std::endl;
            }

            delete f;
            delete t;
        },
        static_cast<void*>( new async_invoke_data {func} ));

    async_invoke_thread->detach();
}


mutex::guard&& mutex::get_guard()
{
    return std::move(guard(this));
}

mutex::guard::guard(mutex* m)
 : m_mutex(m)
{
    m_mutex->lock();
}

mutex::guard::guard(guard&& g)
 : m_mutex(g.m_mutex)
{
    g.m_mutex = nullptr;
}

mutex::guard::~guard()
{
    if (m_mutex != nullptr) m_mutex->unlock();
}


c_condition::c_condition()
{
}

c_condition::~c_condition()
{
    trigger();
}

void c_condition::interrupt(void* _data)
{
    timeout_data* data = static_cast<timeout_data*>(_data);
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(data->timeout));
    data->cond->notify_all();
}

void c_condition::wait()
{
    cond_data cdata {};

    m_mutex.lock();
    m_conds.insert(&cdata);
    m_mutex.unlock();

    cdata.cond_mutex.lock();
    cdata.cond.wait(cdata.cond_mutex);
    cdata.cond_mutex.unlock();

    m_mutex.lock();
    m_conds.erase(&cdata);
    m_mutex.unlock();
}

void c_condition::wait(uint32_t timeout_ms)
{
    cond_data cdata {};
    timeout_data tdata {&cdata.cond, timeout_ms};

    m_mutex.lock();
    m_conds.insert(&cdata);
    m_mutex.unlock();

    tthread::thread interrupt_thread(interrupt, static_cast<void*>(&tdata));

    cdata.cond_mutex.lock();
    cdata.cond.wait(cdata.cond_mutex);
    cdata.cond_mutex.unlock();

    m_mutex.lock();
    m_conds.erase(&cdata);
    m_mutex.unlock();
}

void c_condition::trigger()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    for (auto it : m_conds) it->cond.notify_all();
}


task::task()
 : m_name("unknown")
{
}

task::task(std::string name)
 : m_name(name)
{
}

void task::rename(std::string name)
{
    m_name = name;
}

void task::add_child(task* t)
{
    m_children.push_back(t);
}

enumerator<task*> task::get_children()
{
    return std::move(make_ref_enumerator<task*>(m_children));
}

std::string task::get_name() const
{
    return std::string("unknown");
}


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

    for (auto& it : m_tasks)
    {
        delete it.m_timer;
        it.m_task->drop();
    }
}

std::string c_thread::get_name() const
{
    return m_name;
}

void c_thread::add_task(task* t)
{
    t->grab();

    m_task_pool_mutex.lock();
    m_task_pool.push_back({t, new c_timer()});
    m_task_pool_mutex.unlock();

    m_cond.notify_all();
}

void c_thread::add_task(std::function<void()> func)
{
    task* t = new function_task(func);
    this->add_task(t);
    t->drop();
}

void c_thread::add_delayed_task(task* t, uint32_t delay_ms)
{
    t->grab();

    task* parent = new wait_task(delay_ms);
    parent->add_child(t);

    this->add_task(parent);
    parent->drop();
}

void c_thread::add_delayed_task(std::function<void()> func, uint32_t delay_ms)
{
    task* t = new function_task(func);
    task* parent = new wait_task(delay_ms);
    parent->add_child(t);
    t->drop();

    this->add_task(parent);
    parent->drop();
}

void c_thread::add_persistent_task(std::function<bool(uint32_t)> func)
{
    task* t = new function_task(func);
    this->add_task(t);
    t->drop();
}

void c_thread::add_delayed_persistent_task(std::function<bool(uint32_t)> func, uint32_t delay_ms)
{
    task* t = new function_task(func);
    task* parent = new wait_task(delay_ms);
    parent->add_child(t);
    t->drop();

    this->add_task(parent);
    parent->drop();
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
                *c_logger::get_instance() << "exception caught while running task '" << it->m_task->get_name() << "': " << e.what() << std::endl;
            }
            catch (...)
            {
                *c_logger::get_instance() << "unknown exception caught while running task '" << it->m_task->get_name() << "'" << std::endl;
            }

            if (result) // run() returned 'true', so let's remove it
            {
                // adding child tasks to the pool
                auto subs = it->m_task->get_children();
                for (; subs.has_next(); subs.next())
                {
                    task* t = *subs.get();
                    m_task_pool.push_back( {t, new c_timer()} );
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

thread* task_manager::get_current_thread()
{
    optional<thread*> t = s_threads.get();
    return (t ? *t : nullptr);
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
    async_invoke(std::forward<decltype(func)>(func));
}

task* c_task_manager::create_task(std::function<void()> func) const
{
    return new function_task(func);
}

task* c_task_manager::create_wait_task(uint32_t wait_time) const
{
    return new wait_task(wait_time);
}

task* c_task_manager::create_persistent_task(std::function<bool(uint32_t)> func) const
{
    return new function_task(func);
}

mutex* c_task_manager::create_mutex() const
{
    return new c_mutex<tthread::mutex>();
}

mutex* c_task_manager::create_recursive_mutex() const
{
    return new c_mutex<tthread::recursive_mutex>();
}

condition* c_task_manager::create_condition() const
{
    return new c_condition();
}
