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

    static bool serialize(const var& v, buffer* buf, const serializer* s)
    {
        if (v.get_type() != typeid(authentication) || buf == nullptr || s == nullptr)
            return false;

        const authentication& auth = v.get<authentication>();

        uint32_t magic = auth.get_magic_code();
        buf->push(reinterpret_cast<uint8_t*>(&magic), sizeof(uint32_t));

        serialize_string(auth.get_name(), buf);

        return s->serialize(auth.get_auth_data(), buf);
    }

    static optional<var> deserialize(buffer* buf, const serializer* s)
    {
        if (buf == nullptr || buf->available() < 6 || s == nullptr) return {};

        uint32_t magic;
        buf->pop(reinterpret_cast<uint8_t*>(&magic), sizeof(uint32_t));

        optional<var> str = deserialize_string(buf);
        if (!str.is_valid()) return {};

        optional<var> data = s->deserialize(buf);
        if (!data.is_valid()) return {};

        return std::move(authentication(magic, std::move(str->get<std::string>()), std::move(*data)));
    }
};

template<bool is_request>
class request_or_response
{
    id m_id;
    var m_data;

public:
    request_or_response(id _id, var _data)
     : m_id(_id), m_data(_data) {}

    request_or_response(const request_or_response& req)
     : m_id(req.m_id), m_data(req.m_data) {}

    request_or_response(request_or_response&& req)
     : m_id(req.m_id), m_data(std::move(req.m_data)) {}

    ~request_or_response() {}

    id get_id() const { return m_id; }
    var& get_data() { return m_data; }
    const var& get_data() const { return m_data; }

    static bool serialize(const var& v, buffer* buf, const serializer* s)
    {
        if (v.get_type() != typeid(request_or_response) || buf == nullptr || s == nullptr)
            return false;

        const request_or_response& req = v.get<request_or_response>();

        uint32_t _id = req.get_id();
        buf->push(reinterpret_cast<uint8_t*>(&_id), sizeof(uint32_t));

        return s->serialize(req.get_data(), buf);
    }

    static optional<var> deserialize(buffer* buf, const serializer* s)
    {
        if (buf == nullptr || buf->available() < 4 || s == nullptr) return {};

        uint32_t _id;
        buf->pop(reinterpret_cast<uint8_t*>(&_id), sizeof(uint32_t));

        optional<var> data = s->deserialize(buf);
        if (!data.is_valid()) return {};

        return std::move(request_or_response(_id, *data));
    }
};

typedef request_or_response<true> request;
typedef request_or_response<false> response;

class exec_request
{
    std::string m_fn;
    varlist m_vl;

public:
    exec_request(std::string fn, varlist vl) : m_fn(fn), m_vl(vl) {};
    exec_request(const exec_request& req) : m_fn(req.m_fn), m_vl(req.m_vl) {};
    exec_request(exec_request&& req) : m_fn(std::move(req.m_fn)), m_vl(std::move(req.m_vl)) {};
    ~exec_request() {}

    const std::string& get_function() const { return m_fn; }
    const varlist& get_varlist() const { return m_vl; }

    static bool serialize(const var& v, buffer* buf, const serializer* s)
    {
        if (v.get_type() != typeid(exec_request) || buf == nullptr || s == nullptr)
            return false;

        const exec_request& req = v.get<exec_request>();
        var v_fn;
        var v_vl;
        v_fn.const_reference(req.get_function());
        v_vl.const_reference(req.get_varlist());

        serialize_string(v_fn, buf);
        return serialize_varlist(v_vl, buf, s);
    }

    static optional<var> deserialize(buffer* buf, const serializer* s)
    {
        if (buf == nullptr || s == nullptr) return {};

        optional<var> v_fn = deserialize_string(buf);
        if (!v_fn.is_valid()) return {};

        optional<var> v_vl = deserialize_varlist(buf, s);
        if (!v_vl.is_valid()) return {};

        return std::move( exec_request(std::move(v_fn->get<std::string>()), std::move(v_vl->get<varlist>())) );
    }
};

class parse_and_exec_request
{
    std::string m_expr;

public:
    parse_and_exec_request(std::string expr) : m_expr(expr) {}
    parse_and_exec_request(const parse_and_exec_request& req) : m_expr(req.m_expr) {}
    parse_and_exec_request(parse_and_exec_request&& req) : m_expr(std::move(req.m_expr)) {}
    ~parse_and_exec_request() {}

    const std::string& get_expression() const { return m_expr; }

    static bool serialize(const var& v, buffer* buf, const serializer* s)
    {
        if (v.get_type() != typeid(parse_and_exec_request) || buf == nullptr || s == nullptr)
            return false;

        const parse_and_exec_request& req = v.get<parse_and_exec_request>();
        var v_fn;
        v_fn.const_reference(req.get_expression());

        serialize_string(v_fn, buf);
        return true;
    }

    static optional<var> deserialize(buffer* buf, const serializer* s)
    {
        if (buf == nullptr || s == nullptr) return {};

        optional<var> v_fn = deserialize_string(buf);
        if (!v_fn.is_valid()) return {};

        return std::move( parse_and_exec_request(std::move(v_fn->get<std::string>())) );
    }
};

class exec_response
{
    var m_retval;
    std::string m_outp;

public:
    exec_response(var&& retval, std::string&& outp) : m_retval(retval), m_outp(outp) {}
    exec_response(var&& retval, const std::stringstream& outp) : m_retval(retval), m_outp(outp.str()) {}
    exec_response(const exec_response& resp) : m_retval(resp.m_retval), m_outp(resp.m_outp) {}
    exec_response(exec_response&& resp) : m_retval(std::move(resp.m_retval)), m_outp(std::move(resp.m_outp)) {}
    ~exec_response() {}

    var& get_return_value() { return m_retval; }
    const var& get_return_value() const { return m_retval; }
    std::string& get_output() { return m_outp; }
    const std::string& get_output() const { return m_outp; }

    static bool serialize(const var& v, buffer* buf, const serializer* s)
    {
        if (v.get_type() != typeid(exec_response) || buf == nullptr || s == nullptr)
            return false;

        const exec_response& resp = v.get<exec_response>();

        if (!s->serialize(resp.get_return_value(), buf)) return false;

        var v_outp;
        v_outp.const_reference(resp.get_output());
        serialize_string(v_outp, buf);

        return true;
    }

    static optional<var> deserialize(buffer* buf, const serializer* s)
    {
        if (buf == nullptr || s == nullptr) return {};

        optional<var> v_retval = s->deserialize(buf);
        if (!v_retval.is_valid()) return {};

        optional<var> v_outp = deserialize_string(buf);
        if (!v_outp.is_valid()) return {};

        return std::move( exec_response(std::move(*v_retval), std::move(v_outp->get<std::string>())) );
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

class func_request_handler : public remote_application::request_handler
{
    std::function<bool(var&)> m_req_handler;

public:
    func_request_handler(decltype(m_req_handler) h) : m_req_handler(h) {}
    func_request_handler(const func_request_handler&) = delete;
    ~func_request_handler() {}
    bool handle_request(var& data) { return m_req_handler(data); }
};


c_remote_application::c_remote_application(c_application* app, std::string address, uint16_t port, var auth_data)
 : m_app(app)
 , m_conn(new c_connection(address, port, true))
 , m_auth_ok(false)
 , m_auth_data(auth_data)
 , m_err(&std::cout)
 , m_packet_err(0)
{
    m_conn->set_packet_handler(this);
}

c_remote_application::c_remote_application(c_application* app, connection* conn)
 : m_app(app)
 , m_conn(conn)
 , m_auth_ok(false)
 , m_err(&std::cout)
 , m_packet_err(0)
{
    m_conn->grab();
    m_conn->set_packet_handler(this);
    m_conn->set_connection_handler(this);

    async_invoke([&]
    {
        if (wait_for_authentication(1000))
            m_app->add_client(this);
        else
            this->remote_application::drop();
    });
}

c_remote_application::~c_remote_application()
{
    disconnect();
    m_conn->drop();
}

application* c_remote_application::get_app() const
{
    return m_app;
}

void c_remote_application::handle_packet(connection* conn)
{
    if (conn != m_conn) return; // shouldn't happen

    //tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    optional<var> data = m_app->get_serializer()->deserialize(conn->get_input_buffer());
    if (!data.is_valid())
    {
        if (++m_packet_err > 3)
        {
            *m_err << "Deserialization attempt failed 3 times.. dropping connection" << std::endl;
            m_conn->close();
        }
        return;
    }

    m_packet_err = 0;

    if (data->get_type() == typeid(authentication))
    {
        if (m_auth_ok) // we are already authenticated
        {
            *m_err << "Remote end requested re-authentication (" <<
                m_conn->get_address() << ":" << m_conn->get_port() << ")" << std::endl;
            m_conn->close();
            return;
        }

        authentication& auth = data->get<authentication>();

        // magic code mismatch probably means different gglib protocol versions
        if (auth.get_magic_code() != gglib_magic_code)
        {
            *m_err << "Remote end uses incorrect magic code (" <<
                m_conn->get_address() << ":" << m_conn->get_port() << ")" << std::endl;
            m_conn->close();
            return;
        }

        // if we are on server side
        if (m_conn->get_listener() != nullptr)
        {
            // trying to authenticate
            authentication_handler *h = m_app->get_auth_handler(m_conn->get_listener());
            if (h != nullptr && !h->authenticate(this, auth.get_auth_data())) // failed
            {
                *m_err << "Failed authentication (" <<
                    m_conn->get_address() << ":" << m_conn->get_port() << ")" << std::endl;
                m_conn->close();
                return;
            }

            m_name = std::move(auth.get_name());
            m_auth_data = std::move(auth.get_auth_data());
            m_auth_ok = true;

            // the remote end authenticated itself successfully, now it's our turn
            send_var(authentication(gglib_magic_code, m_app->get_name(), {}));
        }
        else // client side
        {
            m_name = std::move(auth.get_name());
            m_auth_data = std::move(auth.get_auth_data());
            m_auth_ok = true;
        }

        return;
    }
    else if (!m_auth_ok)
    {
        *m_err << "Received data from unauthorized remote end (" <<
            m_conn->get_address() << ":" << m_conn->get_port() << ")" << std::endl;

        m_conn->close();
        return;
    }
    else if (data->get_type() == typeid(c_event))
    {
        c_event_manager* evtmgr = static_cast<c_event_manager*>(m_app->get_event_manager());
        if (!evtmgr->is_remote_access_enabled())
        {
            /* *m_err << "Remote end tried to push an event, but remote access is disabled ("
                m_conn->get_address() << ":" << m_conn->get_port() << ")" << std::endl;*/
            return;
        }

        c_event evt = data->get<c_event>();
        evt.set_originator(this);
        evtmgr->push_event(std::move(evt));

        return;
    }
    else if (data->get_type() == typeid(request))
    {
        request& req = data->get<request>();

        // if the request is handles..
        if (handle_request(req.get_data()))
        {
            // ..then we send a response
            send_var( response(req.get_id(), std::move(req.get_data())) );
        }

        return;
    }
    else if (data->get_type() == typeid(response))
    {
        response& resp = data->get<response>();

        tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

        auto it = m_responses.find(resp.get_id());
        if (it != m_responses.end())
        {
            it->second = std::move(resp.get_data());
        }

        return;
    }
    else
    {
        *m_err << "Unknown incoming data.. hack attempt?" << std::endl;
        m_conn->close();

        return;
    }
}

void c_remote_application::handle_connection_open(connection*)
{
    return;
}

void c_remote_application::handle_connection_close(connection* conn)
{
    if (conn != m_conn) return; // shouldn't happen

    // if this is the server side connection
    if (m_conn->get_listener() != nullptr)
    {
        m_app->remove_client(this);
        this->remote_application::drop();
    }
}

bool c_remote_application::send_var(const var& data) const
{
    if (!m_conn->is_opened()) return false;
    return m_app->get_serializer()->serialize(data, m_conn->get_output_buffer());
}

bool c_remote_application::handle_request(var& data) const
{
    if (data.get_type() == typeid(exec_request))
    {
        c_script_engine* se = static_cast<c_script_engine*>(m_app->get_script_engine());
        if (!se->is_remote_access_enabled())
        {
            /* *m_err << "Remote end tried to execute a function, but remote access is disabled ("
                m_conn->get_address() << ":" << m_conn->get_port() << ")" << std::endl;*/
            return false;
        }

        exec_request& req = data.get<exec_request>();
        std::stringstream ss;

        // doing the actual exec
        optional<var> rv = se->exec(req.get_function(), req.get_varlist(), ss);
        if (!rv.is_valid()) return false;

        // responding in case of success
        data = std::move( exec_response(std::move(*rv), ss) );
        return true;
    }
    else if (data.get_type() == typeid(parse_and_exec_request))
    {
        c_script_engine* se = static_cast<c_script_engine*>(m_app->get_script_engine());
        if (!se->is_remote_access_enabled())
        {
            /* *m_err << "Remote end tried to execute a function, but remote access is disabled ("
                m_conn->get_address() << ":" << m_conn->get_port() << ")" << std::endl;*/
            return false;
        }

        parse_and_exec_request& req = data.get<parse_and_exec_request>();
        std::stringstream ss;

        // doing the actual exec
        optional<var> rv = se->parse_and_exec(req.get_expression(), ss);
        if (!rv.is_valid()) return false;

        // responding in case of success
        data = std::move( exec_response(std::move(*rv), ss) );
        return true;
    }
    else
    {
        auto it = m_req_handlers.find(data.get_type());
        if (it != m_req_handlers.end())
        {
            return it->second->handle_request(data);
        }
    }

    return false;
}

bool c_remote_application::wait_for_authentication(uint32_t timeout) const
{
    uint32_t elapsed = 0;

    for(;;)
    {
        if (m_auth_ok) return true;
        if (elapsed > timeout) return false;

        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(50));
        elapsed += 50;
    }
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

bool c_remote_application::connect()
{
    if (m_conn->is_opened()) return false; // already connected
    if (!m_conn->is_opened() && m_conn->get_listener() != nullptr) return false; // can't connect to client from server side

    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    if (!m_conn->open()) return false; // failed to open connection

    if (!send_var(authentication(gglib_magic_code, m_app->get_name(), m_auth_data)))
    {
        m_conn->close();
        return false;
    }

    if (!wait_for_authentication(1000)) // remote end didn't respond to our auth request
    {
        m_conn->close();
        return false;
    }

    return true;
}

void c_remote_application::disconnect()
{
    if (!m_conn->is_opened()) return;
    m_conn->close();
}

bool c_remote_application::is_connected() const
{
    return (m_conn->is_opened() && m_auth_ok);
}

void c_remote_application::add_request_handler(typeinfo ti, request_handler* h)
{
    if (h == nullptr) return;

    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    auto it = m_req_handlers.find(ti);
    if (it == m_req_handlers.end()) // adding new one
    {
        h->grab();
        m_req_handlers[ti] = h;
    }
    else // replacing existing one
    {
        h->grab();
        it->second->drop();
        it->second = h;
    }
}

void c_remote_application::add_request_handler(typeinfo ti, std::function<bool(var&)> f)
{
    request_handler* h = new func_request_handler(f);
    this->add_request_handler(ti, h);
    h->drop();
}

void c_remote_application::remove_request_handler(typeinfo ti)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    auto it = m_req_handlers.find(ti);
    if (it != m_req_handlers.end())
    {
        it->second->drop();
        m_req_handlers.erase(it);
    }
}

optional<var> c_remote_application::send_request(var data, uint32_t timeout) const
{
    uint32_t elapsed = 0;
    id _id = m_app->get_id_manager()->get_random_id();

    // sending request
    if (!send_var(request(_id, data))) return {};

    // letting handle_packet know that we wait for a response of this id
    m_mutex.lock();
    auto it = m_responses.insert(std::make_pair(_id, var {})).first;
    m_mutex.unlock();

    for (;;)
    {
        {
            tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
            if (!it->second.is_empty()) return std::move(it->second);
        }

        if (elapsed > timeout)
        {
            m_mutex.lock();
            m_responses.erase(it);
            m_mutex.unlock();
            return {};
        }

        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(50));
        elapsed += 50;
    }
}

void c_remote_application::send_async_request(var data, uint32_t timeout, std::function<void(optional<var>)> callback) const
{
    async_invoke([&]
    {
        optional<var> rv = this->send_request(data, timeout);
        callback(std::move(rv));
    });
}

void c_remote_application::push_event(event_type t, event::attribute_list al) const
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

void c_remote_application::set_error_stream(std::ostream& err)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    m_err = &err;
    m_conn->set_error_stream(err);
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

    m_serializer->add_rule_ex(typeid(authentication), &authentication::serialize, &authentication::deserialize);
    m_serializer->add_rule_ex(typeid(request), &request::serialize, &request::deserialize);
    m_serializer->add_rule_ex(typeid(response), &response::serialize, &response::deserialize);
    m_serializer->add_rule_ex(typeid(exec_request), &exec_request::serialize, &exec_request::deserialize);
    m_serializer->add_rule_ex(typeid(parse_and_exec_request), &parse_and_exec_request::serialize, &parse_and_exec_request::deserialize);
    m_serializer->add_rule_ex(typeid(exec_response), &exec_response::serialize, &exec_response::deserialize);
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

    l->set_connection_handler(this);
    m_ports[l] = auth_handler;
    return true;
}

bool c_application::open_port(uint16_t port, std::function<bool(remote_application*, const var&)> auth_handler)
{
    return this->open_port(port, static_cast<authentication_handler*>( auto_drop(new func_authentication_handler(auth_handler)) ));
    /*authentication_handler* h = new func_authentication_handler(auth_handler);
    bool rc = open_port(port, h);
    h->drop();
    return rc;*/
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
    new c_remote_application(this, conn);
    return;
}

void c_application::handle_connection_close(connection* conn)
{
    return;
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
                         auto_drop( m_scripteng->create_console_controller() ),
                         "Press TAB to list available commands");
}

console* c_application::create_console(std::string name, std::string welcome_text)
{
    return new c_console(this, name, nullptr, welcome_text);
}
