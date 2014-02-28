#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include "c_logger.hpp"
#include "gg/console.hpp"

using namespace gg;


nullstream::nullstream()
 : std::ostream(this)
{
}

nullstream::~nullstream()
{
}

int nullstream::overflow(int c)
{
    return c;
}

int nullstream::sync()
{
    return 0;
}


c_logger* c_logger::get_instance()
{
    static c_logger s_instance;
    return &s_instance;
}

c_logger::c_logger()
 : std::ostream(this)
 , m_cout(&std::cout)
 , m_cout_rdbuf(nullptr)
 , m_stream(nullptr)
 , m_file(nullptr)
 , m_log_to_file(false)
 , m_timestamp(true)
{
}

c_logger::~c_logger()
{
    sync();
    if (m_file != nullptr) delete m_file;
}

void c_logger::register_cout(std::ostream& o)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    if (m_cout_rdbuf != nullptr)
        throw std::runtime_error("can't change cout while hook is enabled");

    m_cout = &o;
}

void c_logger::enable_cout_hook()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    if (m_cout_rdbuf != nullptr) return; // already enabled

    // inject own hook
    m_cout_rdbuf = m_cout->rdbuf(new c_logger::wrapper(this));
}

void c_logger::disable_cout_hook()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    if (m_cout_rdbuf == nullptr) return; // already disabled

    //flush all thread buffers
    for (auto& it : m_sync_log) flush_thread(it.first);

    // inject original hook
    m_cout->rdbuf(m_cout_rdbuf);
}

void c_logger::enable_timestamp()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_timestamp = true;
}

void c_logger::disable_timestamp()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_timestamp = false;
}

void c_logger::log_to_stream(std::ostream& o)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_stream = &o;
    if (m_file != nullptr) { delete m_file; m_file = nullptr; }
    if (m_console != nullptr) { m_console->drop(); m_console = nullptr; }
}

void c_logger::log_to_file(std::string filename)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_stream = nullptr;
    if (m_file != nullptr) { delete m_file; m_file = nullptr; }
    if (m_console != nullptr) { m_console->drop(); m_console = nullptr; }
    m_file = new std::fstream(filename, std::ios_base::out);
}

void c_logger::log_to_console(console* c)
{
    if (c == nullptr) return;

    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_stream = nullptr;
    if (m_file != nullptr) { delete m_file; m_file = nullptr; }
    if (m_console != nullptr) { m_console->drop(); m_console = nullptr; }
    m_console = c;
    m_console->grab();
}

std::ostream& c_logger::get_nullstream() const
{
    return m_nullstream;
}

void c_logger::push_hook(std::ostream& o)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_hooks.begin(&o);
}

void c_logger::pop_hook()
{
    sync();
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_hooks.end();
}

void c_logger::flush_thread(tthread::thread::id t)
{
    std::string& msg = m_sync_log[t];
    if (msg.empty()) return;

    optional<std::ostream*> o = m_hooks.get();
    if (o && o.get() != m_cout)
    {
        o.get()->write(msg.c_str(), msg.size());
    }
    else
    {
        if (m_timestamp) add_timestamp(msg);

        if (m_stream != nullptr)
        {
            m_stream->write(msg.c_str(), msg.size());
        }
        else if (m_file != nullptr)
        {
            m_file->write(msg.c_str(), msg.size());
        }
        else if (m_console != nullptr)
        {
            console::output* out = m_console->create_output();
            *out << msg;
            out->drop();
        }
        else
        {
            m_cout_rdbuf->sputn(msg.c_str(), msg.size());
        }
    }

    msg.erase();
}

int c_logger::overflow (int c)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    char _c = c;
    m_sync_log[tthread::this_thread::get_id()].append(&_c, 1);
    return c;
}

int c_logger::sync()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    flush_thread(tthread::this_thread::get_id());
    return 0;
}

void c_logger::add_timestamp(std::string& str) const
{
    uint32_t elapsed = m_timer.peek_elapsed();
    std::stringstream ss;

    ss << std::setfill('0') << "["
     << std::setw(2) << elapsed / 60000 << ":"
     << std::setw(2) << elapsed / 1000 << ":"
     << std::setw(3) << elapsed % 1000 << "] ";

    str.insert(0, ss.str());
}


c_logger::wrapper::wrapper(c_logger* l)
 : m_logger(l)
{
}

c_logger::wrapper::~wrapper()
{
}

int c_logger::wrapper::overflow(int c)
{
    return m_logger->overflow(c);
}

int c_logger::wrapper::sync()
{
    return m_logger->sync();
}


logger::scoped_hook::scoped_hook(std::ostream& o)
{
    c_logger::get_instance()->push_hook(o);
}

logger::scoped_hook::~scoped_hook()
{
    c_logger::get_instance()->pop_hook();
}
