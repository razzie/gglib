#include "c_console.hpp"
#include "c_application.hpp"
#include "c_eventmgr.hpp"
#include "c_taskmgr.hpp"
#include "c_logger.hpp"
#include "c_serializer.hpp"
#include "c_scripteng.hpp"
#include "c_netmgr.hpp"
#include "c_idgen.hpp"

using namespace gg;


application* application::create_instance(std::string name, uint32_t ver_major, uint32_t ver_minor)
{
    return new c_application(name, ver_major, ver_minor);
}

c_application::c_application(std::string name, uint32_t ver_major, uint32_t ver_minor)
 : m_name(name)
 , m_ver_major(ver_major)
 , m_ver_minor(ver_minor)
{
    setlocale(LC_ALL, "");

    c_logger::get_instance()->enable_cout_hook();

    m_serializer = new c_serializer(this);
    m_eventmgr = new c_event_manager(this);
    m_taskmgr = new c_task_manager(this);
    m_scripteng = new c_script_engine(this);
    m_netmgr = new c_network_manager(this);
    m_idgen = new c_id_generator(this);
}

c_application::~c_application()
{
    m_exit_code = 0;
    m_cond.notify_all();

    delete m_idgen;
    delete m_netmgr;
    delete m_scripteng;
    delete m_taskmgr;
    delete m_eventmgr;
    delete m_serializer;
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

logger* c_application::get_logger()
{
    return c_logger::get_instance();
}

serializer* c_application::get_serializer()
{
    return m_serializer;
}

script_engine* c_application::get_script_engine()
{
    return m_scripteng;
}

network_manager* c_application::get_network_manager()
{
    return m_netmgr;
}

id_generator* c_application::get_id_generator()
{
    return m_idgen;
}

console* c_application::create_console()
{
    return new c_console(this, m_name,
                         auto_drop<console::controller>( m_scripteng->get_console_controller() ),
                         "Press TAB to list available commands");
}

console* c_application::create_console(std::string name, std::string welcome_text)
{
    return new c_console(this, name, nullptr, welcome_text);
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
