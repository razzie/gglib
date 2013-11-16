#include <stdio.h>
#include "managed_cout.hpp"

using namespace gg;

managed_cout::managed_buf::managed_buf()
{
};

managed_cout::managed_buf::~managed_buf()
{
    sync();
};

int managed_cout::managed_buf::overflow(int c)
{
    managed_cout* mc = managed_cout::get_instance();

    tthread::lock_guard<tthread::mutex> guard(mc->m_mutex);

    tthread::thread::id tid = tthread::this_thread::get_id();
    std::stack<callback>& cbstack = mc->m_hooks[tid];

    if (cbstack.empty())
    {
        fputc(c, stdout);
    }
    else
    {
        managed_cout::callback& cb = cbstack.top();

        if (cb.type == managed_cout::callback::STREAM)
        {
            std::ostream* o = cb.data.stream;
            if (o == &std::cout) fputc(c, stdout);
            else o->put(c);
        }
        else
        {
            (*cb.data.console) << (char)c;
        }
    }

    return c;
}


managed_cout::managed_cout()
 : m_own_rdbuf(new managed_cout::managed_buf())
{
}

managed_cout::~managed_cout()
{
}

void managed_cout::push_hook(std::ostream& o)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    tthread::thread::id tid = tthread::this_thread::get_id();
    managed_cout::callback cb;
    cb.type = managed_cout::callback::STREAM;
    cb.data.stream = &o;

    m_hooks[tid].push( cb );
}

void managed_cout::push_hook(console::output& o)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    tthread::thread::id tid = tthread::this_thread::get_id();
    managed_cout::callback cb;
    cb.type = managed_cout::callback::CONSOLE;
    cb.data.console = &o;

    m_hooks[tid].push( cb );
}

void managed_cout::pop_hook()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    tthread::thread::id tid = tthread::this_thread::get_id();
    m_hooks[tid].pop();
}

managed_cout* managed_cout::get_instance()
{
    static managed_cout* s_instance = nullptr;

    if (s_instance == nullptr)
        s_instance = new managed_cout();

    return s_instance;
}

void managed_cout::enable()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    if (m_cout_rdbuf != nullptr)
        throw std::runtime_error("managed_cout is already enabled");

    m_cout_rdbuf = std::cout.rdbuf(m_own_rdbuf);
}

void managed_cout::disable()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    if (m_cout_rdbuf == nullptr)
        throw std::runtime_error("managed_cout is already disabled");

    std::cout.rdbuf(m_cout_rdbuf);
}

bool managed_cout::is_enabled() const
{
    return (m_cout_rdbuf != nullptr);
}
