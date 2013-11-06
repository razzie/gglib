#include "c_console.hpp"
#include "c_application.hpp"
#include "c_eventmgr.hpp"
#include "c_taskmgr.hpp"
#include "c_scripteng.hpp"
#include "c_timer.hpp"

using namespace gg;

application* create_instance(std::string name, uint32_t ver_major, uint32_t ver_minor)
{
    return new c_application(name, ver_major, ver_minor);
}

c_application::c_application(std::string name, uint32_t ver_major, uint32_t ver_minor)
 : m_name(name), m_ver_major(ver_major), m_ver_minor(ver_minor)
{
    m_eventmgr = new c_event_manager();
    m_taskmgr = new c_task_manager();
    m_scripteng = new c_script_engine();
}

c_application::~c_application()
{
    m_exit_code = 0;
    m_cond.notify_all();

    delete m_eventmgr;
    delete m_taskmgr;
    delete m_scripteng;
}

std::string c_application::get_name() const
{
    return m_name;
}

uint32_t c_application::get_major_version() const
{
    return m_ver_major;
}

uint32_t c_application::get_minor_version() const
{
    return m_ver_minor;
}

event_manager* c_application::get_event_manager()
{
    return m_eventmgr;
}

task_manager* c_application::get_task_manager()
{
    return m_taskmgr;
}

script_engine* c_application::get_script_engine()
{
    return m_scripteng;
}

console* c_application::create_console()
{
    return new c_console(m_name, m_scripteng->get_console_controller());
}

console* c_application::create_console(std::string name)
{
    return new c_console(name, nullptr);
}

timer* c_application::create_timer()
{
    return new c_timer();
}

int c_application::start()
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);
    m_cond.wait(m_cond_mutex);

    return m_exit_code;
}

void c_application::exit(int exit_code)
{
    m_exit_code = exit_code;
    m_cond.notify_all();
}
