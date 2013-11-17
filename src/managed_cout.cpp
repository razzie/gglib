#include <stdio.h>
#include "tinythread.h"
#include "managed_cout.hpp"
#include "threadglobal.hpp"

using namespace gg;

class managed_buf : public std::streambuf
{
public:
    managed_buf();
    virtual ~managed_buf();
protected:
    virtual int overflow (int c);
};

static managed_buf gs_managed_buf;

static tthread::mutex gs_mutex;
static std::streambuf* gs_cout_rdbuf = nullptr;
static managed_buf* gs_own_rdbuf = &gs_managed_buf;
static recursive_thread_global<std::ostream*> gs_hooks;


managed_buf::managed_buf()
{
};

managed_buf::~managed_buf()
{
    sync();
};

int managed_buf::overflow(int c)
{
    tthread::lock_guard<tthread::mutex> guard(gs_mutex);

    optional<std::ostream*> o = gs_hooks.get();
    if (o.is_valid())
    {
        if (o.get() == &std::cout) fputc(c, stdout);
        else o.get()->put(c);
    }

    return c;
}


void managed_cout::push_hook(std::ostream& o)
{
    tthread::lock_guard<tthread::mutex> guard(gs_mutex);
    gs_hooks.begin(&o);
}

void managed_cout::pop_hook()
{
    tthread::lock_guard<tthread::mutex> guard(gs_mutex);
    gs_hooks.end();
}

void managed_cout::enable()
{
    tthread::lock_guard<tthread::mutex> guard(gs_mutex);

    if (gs_cout_rdbuf != nullptr)
        throw std::runtime_error("managed_cout is already enabled");

    gs_cout_rdbuf = std::cout.rdbuf(gs_own_rdbuf);
}

void managed_cout::disable()
{
    tthread::lock_guard<tthread::mutex> guard(gs_mutex);

    if (gs_cout_rdbuf == nullptr)
        throw std::runtime_error("managed_cout is already disabled");

    std::cout.rdbuf(gs_cout_rdbuf);
}

bool managed_cout::is_enabled()
{
    return (gs_cout_rdbuf != nullptr);
}
