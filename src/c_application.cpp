#include <list>
#include <stdexcept>
#include "c_console.hpp"
#include "c_application.hpp"
#include "c_eventmgr.hpp"
#include "c_taskmgr.hpp"
#include "c_logger.hpp"
#include "c_serializer.hpp"
#include "c_scripteng.hpp"
#include "c_netmgr.hpp"
#include "c_idman.hpp"
#include "enumutil.hpp"

using namespace gg;

const uint32_t gglib_magic_code = 0xdeadbeef;


class authentication
{
    uint32_t m_magic_code;
    std::string m_name;
    var m_auth_data;

public:
    authentication(uint32_t magic, std::string name, var auth_data)
     : m_magic_code(magic), m_name(name), m_auth_data(auth_data) {}

    authentication(const authentication& auth)
     : m_magic_code(auth.m_magic_code), m_name(auth.m_name), m_auth_data(auth.m_auth_data) {}

    authentication(authentication&& auth)
     : m_magic_code(auth.m_magic_code), m_name(std::move(auth.m_name)), m_auth_data(std::move(auth.m_auth_data)) {}

    ~authentication() {}

    uint32_t get_magic_code() const { return m_magic_code; }

    std::string& get_name() { return m_name; }
    const std::string& get_name() const { return m_name; }

    var& get_auth_data() { return m_auth_data; }
    const var& get_auth_data() const { return m_auth_data; }

    static bool serialize(const var& v, buffer* buf, serializer* s)
    {
        if (v.get_type() != typeid(authentication) || buf == nullptr || s == nullptr)
            return false;

        const authentication& auth = v.get<authentication>();

        uint32_t magic = auth.get_magic_code();
        buf->push(reinterpret_cast<uint8_t*>(&magic), sizeof(uint32_t));

        serialize_string(auth.get_name(), buf);

        return s->serialize(auth.get_auth_data(), buf);
    }

    static optional<var> deserialize(buffer* buf, serializer* s)
    {
        if (buf == nullptr || buf->available() < 6 || s == nullptr) return {};

        uint32_t magic;
        buf->pop(reinterpret_cast<uint8_t*>(&magic), sizeof(uint32_t));

        optional<var> str = deserialize_string(buf);
        if (!str.is_valid() || str->get_type() != typeid(std::string)) return {};

        optional<var> data = s->deserialize(buf);
        if (!data.is_valid()) return {};

        return authentication(magic, std::move(*str), std::move(*data));
    }
};

class func_authentication_handler : public authentication_handler
{
    std::function<bool(remote_application*, const var&)> m_auth_handler;

public:
    func_authentication_handler(decltype(m_auth_handler) h) : m_auth_handler(h) {}
    func_authentication_handler(const func_authentication_handler&) = delete;
    ~func_authentication_handler() {}
    bool authenticate(remote_application* app, const var& auth_data) { return m_auth_handler(app, auth_data); }
};


c_remote_application::c_remote_application(c_application* app, std::string address, uint16_t port, var auth_data)
 : m_app(app)
 , m_conn(new c_connection(address, port, true))
 , m_auth_ok(false)
 , m_auth_data(auth_data)
{
    m_conn->set_packet_handler(this);
    connect();
}

c_remote_application::c_remote_application(c_application* app, connection* conn, var auth_data)
 : m_app(app)
 , m_conn(conn)
 , m_auth_ok(true)
 , m_auth_data(auth_data)
{
    m_conn->grab();
    m_conn->set_packet_handler(this);
}

c_remote_application::~c_remote_application()
{
    disconnect();
    m_conn->drop();
}

bool c_remote_application::send_data(const var& data)
{
    if (!is_connected()) return false;
    return m_app->get_serializer()->serialize(data, m_conn->get_output_buffer());
}

bool c_remote_application::handle_data(var& data)
{
    if (data.get_type() == typeid(authentication))
    {
        if (m_auth_ok) return true; // we are already authenticated

        authentication auth = data.get<authentication>();

        // magic code mismatch probably means different gglib protocol versions
        if (auth.get_magic_code() != gglib_magic_code) return true;

        // trying to authenticate
        authentication_handler *h = m_app->get_auth_handler(m_conn->get_listener());
        if (h != nullptr && !h->authenticate(this, auth.get_auth_data())) return true; // failed

        m_name = std::move(auth.get_name());
        m_auth_data = std::move(auth.get_auth_data());
        m_auth_ok = true;
        return true;
    }
    else if (data.get_type() == typeid(c_event))
    {
        c_event evt = data.get<c_event>();
        return true;
    }

    return false;
}

var c_remote_application::wait_for_data(typeinfo ti)
{
    for(;;)
    {
        m_cond.wait(m_cond_mutex);

        tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
        var& v = m_last_data[ti];

        if (!v.is_empty()) return std::move(v);
    }
}

void c_remote_application::handle_packet(connection* conn)
{
    if (conn != m_conn) return; // shouldn't happen

    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    optional<var> data = m_app->get_serializer()->deserialize(conn->get_input_buffer());
    if (!data.is_valid()) return;

    if (!handle_data(*data))
    {
        m_last_data[data->get_type()] = std::move(*data);
        m_cond.notify_all();
    }
}

bool c_remote_application::connect()
{
    if (is_connected()) return false; // already connected

    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    if (m_conn->open()) return false; // failed to open connection

    if (!send_data(authentication(gglib_magic_code, m_app->get_name(), m_auth_data)))
    {
        m_conn->close();
        return false;
    }

    /*var data = wait_for_data(typeid(authentication)); // waiting for the other end's authentication
    authentication auth = data.get<authentication>();

    // magic code mismatch probably means different gglib protocol versions
    if (auth.get_magic_code() != gglib_magic_code) return;

    // trying to authenticate
    authentication_handler *h = m_app->get_auth_handler(m_conn->get_listener());
    if (h != nullptr && !h->authenticate(this, auth.get_auth_data())) return; // failed

    m_name = std::move(auth.get_name());
    m_auth_data = std::move(auth.get_auth_data());
    m_auth_ok = true;
    return true;*/
}

void c_remote_application::disconnect()
{
    if (!is_connected()) return;

}

bool c_remote_application::is_connected() const
{
    return (m_conn->is_opened());
}

std::string c_remote_application::get_name() const
{
    return m_name;
}

std::string c_remote_application::get_address() const
{
    return m_conn->get_address();
}

uint16_t c_remote_application::get_port() const
{
    return m_conn->get_port();
}

const var& c_remote_application::get_auth_data() const
{
    return m_auth_data;
}

void c_remote_application::push_event(event_type t, event::attribute_list al)
{
    if (!is_connected()) throw std::runtime_error("not connected to remote application");

    serializer* srl = m_app->get_serializer();

    c_event e(nullptr, t, std::forward<event::attribute_list>(al));
    var v;
    v.reference(e);

    if (!srl->serialize(v, m_conn->get_output_buffer()))
        throw std::runtime_error("event serialization error");
}

optional<var> c_remote_application::exec(std::string fn, varlist vl, std::ostream& output) const
{
    if (!is_connected()) throw std::runtime_error("not connected to remote application");
    return {};
}

optional<var> c_remote_application::parse_and_exec(std::string expr, std::ostream& output) const
{
    if (!is_connected()) throw std::runtime_error("not connected to remote application");
    return {};
}


application* application::create(std::string name)
{
    return new c_application(name);
}

c_application::c_application(std::string name)
 : m_name(name)
{
    setlocale(LC_ALL, "");

    c_logger::get_instance()->enable_cout_hook();

    m_serializer = new c_serializer(this);
    m_eventmgr = new c_event_manager(this);
    m_taskmgr = new c_task_manager(this);
    m_scripteng = new c_script_engine(this);
    m_netmgr = new c_network_manager(this);
    m_idman = new c_id_manager(this);
}

c_application::~c_application()
{
    m_exit_code = 0;
    m_cond.notify_all();

    delete m_idman;
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

bool c_application::open_port(uint16_t port, authentication_handler* auth_handler)
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);

    listener* l = new c_listener(port, true);
    if (!l->open())
    {
        l->drop();
        return false;
    }

    m_ports[l] = auth_handler;
    return true;
}

bool c_application::open_port(uint16_t port, std::function<bool(remote_application*, const var&)> auth_handler)
{
    //return this->open_port(port, grab_ptr<authentication_handler>( new func_authentication_handler(auth_handler) ));
    authentication_handler* h = new func_authentication_handler(auth_handler);
    bool rc = open_port(port, h);
    h->drop();
    return rc;
}

void c_application::close_port(uint16_t port)
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);

    auto it = m_ports.begin(), end = m_ports.end();
    for (; it != end; ++it)
    {
        if (it->first->get_port() == port)
        {
            it->first->drop();
            m_ports.erase(it);
            return;
        }
    }
}

void c_application::add_client(remote_application* rem_app)
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);
    m_clients.insert(rem_app);
}

void c_application::remove_client(remote_application* rem_app)
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);
    m_clients.erase(rem_app);
}

authentication_handler* c_application::get_auth_handler(listener* l)
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);
    auto it = m_ports.find(l);
    if (it != m_ports.end()) return it->second;
    else return nullptr;
}

void c_application::close_ports()
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);

}

remote_application* c_application::connect(std::string address, uint16_t port, var auth_data)
{
    return new c_remote_application(this, address, port, auth_data);
}

enumerator<remote_application*> c_application::get_remote_applications()
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);

    std::list<grab_ptr<remote_application, true>> tmplist;
    for (auto& it : m_clients) tmplist.push_back(it);

    conversion_container<decltype(tmplist), remote_application*> convlist(
        std::move(tmplist), [](grab_ptr<remote_application, true>& it)->remote_application*& { return it; });

    return std::move(convlist);
}

void c_application::handle_connection_open(connection* conn)
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);

}

void c_application::handle_connection_close(connection* conn)
{
    tthread::lock_guard<tthread::mutex> guard(m_cond_mutex);

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

id_manager* c_application::get_id_manager()
{
    return m_idman;
}

console* c_application::create_console()
{
    return new c_console(this, m_name,
                         grab_ptr<console::controller>( m_scripteng->create_console_controller() ),
                         "Press TAB to list available commands");
}

console* c_application::create_console(std::string name, std::string welcome_text)
{
    return new c_console(this, name, nullptr, welcome_text);
}
