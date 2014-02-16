#include <algorithm>
#include <functional>
#include <stdexcept>
#include "c_eventmgr.hpp"
#include "c_netmgr.hpp"
#include "gg/application.hpp"

using namespace gg;


class event_task : public task
{
    c_event_manager* m_evtmgr;
    c_event m_evt;

public:
    event_task(c_event_manager* evtmgr,
               remote_application* orig,
               event_type t,
               event::attribute_list al)
     : m_evtmgr(evtmgr)
     , m_evt(orig, t, std::forward<event::attribute_list>(al))
    {
    }

    event_task(c_event_manager* evtmgr, c_event evt)
     : m_evtmgr(evtmgr)
     , m_evt(evt)
    {
    }

    ~event_task() {}

    bool run(uint32_t)
    {
        m_evtmgr->trigger_event(&m_evt);
        return true;
    }
};

class func_event_listener : public event_listener
{
    event_callback m_cb;

public:
    func_event_listener(event_callback cb) : m_cb(cb) {}
    ~func_event_listener() {}
    bool on_event(const event& evt) { return m_cb(evt); }
};


bool serialize_event_type(const var& v, buffer* buf)
{
    if (buf == nullptr || v.get_type() != typeid(event_type))
        return false;

    const event_type& e = v.get<event_type>();
    size_t hash_code = e.get_hash();

    buf->push(reinterpret_cast<const uint8_t*>(&hash_code), sizeof(size_t));
    if (!serialize_string(e.get_name(), buf)) return false;

    return true;
}

optional<var> deserialize_event_type(buffer* buf)
{
    if (buf == nullptr || buf->available() == 0) return {};

    size_t hash_code;
    if (buf->pop(reinterpret_cast<uint8_t*>(&hash_code), sizeof(size_t)) != sizeof(size_t)) return {};

    optional<var> opt_name = deserialize_string(buf);
    if (!opt_name.is_valid() || opt_name->get_type() != typeid(std::string)) return {};
    std::string name = opt_name->get<std::string>();

    if (name.empty()) return event_type(hash_code);
    else return event_type(name);
}

bool serialize_event(const var& v, buffer* buf, const serializer* s)
{
    if (buf == nullptr || s == nullptr || v.get_type() != typeid(c_event))
        return false;

    const c_event& e = v.get<c_event>();
    return e.serialize(buf, s);
}

optional<var> deserialize_event(buffer* buf, const serializer* s)
{
    if (buf == nullptr || buf->available() == 0 || s == nullptr) return {};

    try
    {
        var v;
        v.construct<c_event>(nullptr, buf, s);
        return std::move(v);
    }
    catch (std::exception& e)
    {
        return {};
    }
}


c_event::c_event(remote_application* orig, event_type t, event::attribute_list&& al)
 : m_orig(orig)
 , m_type(t)
 , m_attributes(al)
{
    if (m_orig != nullptr) m_orig->grab();
}

c_event::c_event(const c_event& e)
 : m_orig(e.m_orig)
 , m_type(e.m_type)
 , m_attributes(e.m_attributes)
{
}

c_event::c_event(c_event&& e)
 : m_orig(e.m_orig)
 , m_type(std::move(e.m_type))
 , m_attributes(std::move(e.m_attributes))
{
}

c_event::~c_event()
{
    if (m_orig != nullptr) m_orig->drop();
}

c_event::c_event(remote_application* orig, buffer* buf, const serializer* s) // deserialize
 : m_orig(orig)
 , m_type(static_cast<size_t>(0))
{
    if (buf == nullptr || buf->available() == 0 || s == nullptr)
        throw std::runtime_error("unable to deserialize event");

    grab_guard bufgrab(buf);

    /*size_t hash_code;
    if (buf->pop(reinterpret_cast<uint8_t*>(&hash_code), sizeof(size_t)) != sizeof(size_t))
        throw std::runtime_error("unable to deserialize event");

    m_type = event_type(hash_code);*/

    optional<var> opt_event_type = deserialize_event_type(buf);
    if (!opt_event_type.is_valid() || opt_event_type->get_type() != typeid(event_type))
        throw std::runtime_error("unable to deserialize event");

    m_type = opt_event_type->get<event_type>();

    optional<uint8_t> attrcnt = buf->pop();
    if (!attrcnt.is_valid())
        throw std::runtime_error("unable to deserialize event");

    uint8_t attr_count = attrcnt.get();

    for (uint8_t i = 0; i < attr_count; ++i)
    {
        optional<var> attr = deserialize_string(buf);
        optional<var> val = s->deserialize(buf);

        if (!attr.is_valid() || !val.is_valid()
            || attr->get_type() != typeid(std::string))
            throw std::runtime_error("unable to deserialize event");

        add(std::move(attr->get<std::string>()), std::move(*val));
    }
}

bool c_event::serialize(buffer* buf, const serializer* s) const
{
    if (buf == nullptr || s == nullptr) return false;

    grab_guard bufgrab(buf);

    //size_t hash_code = m_type.get_hash();
    //buf->push(reinterpret_cast<uint8_t*>(&hash_code), sizeof(size_t));
    if (!serialize_event_type(m_type, buf)) return false;

    uint8_t attr_count = m_attributes.size();
    buf->push(attr_count);

    auto it = m_attributes.begin(), end = m_attributes.end();
    for (; it != end; ++it)
    {
        if (!serialize_string(it->first, buf)) return false;
        if (!s->serialize(it->second, buf)) return false;
    }

    return true;
}

void c_event::set_originator(remote_application* orig)
{
    if (m_orig != nullptr) m_orig->drop();
    if (orig != nullptr) orig->grab();
    m_orig = orig;
}

remote_application* c_event::get_originator() const
{
    return m_orig;
}

event_type c_event::get_type() const
{
    return m_type;
}

void c_event::add(std::string addr, var val)
{
    m_attributes[addr] = val;
}

const var& c_event::operator[] (std::string attr) const
{
    return m_attributes.at(attr);
}

const var& c_event::get_attribute(std::string attr) const
{
    return m_attributes.at(attr);
}

const event::attribute_list& c_event::get_attributes() const
{
    return m_attributes;
}

std::ostream& gg::operator<< (std::ostream& o, const event::attribute& a)
{
    return o << a.first << ":" << a.second.to_stream();
}

std::ostream& gg::operator<< (std::ostream& o, const event::attribute_list& al)
{
    auto it = al.begin();

    o << "[" << it->first << ":" << it->second.to_stream();
    std::for_each(++it, al.end(), [&](const event::attribute& a)
    {
        o << "," << a.first << ":" << a.second.to_stream();
    });
    o << "]";

    return o;
}

std::ostream& gg::operator<< (std::ostream& o, const event& e)
{
    const event_type& t = e.get_type();
    const std::string& tname = t.get_name();

    if (!tname.empty()) o << tname;
    else o << t.get_hash();

    return o << e.get_attributes();
}


event_type::event_type(std::string name)
 : m_name(name)
 , m_hash(std::hash<std::string>()(name))
{
}

event_type::event_type(const char* str)
 : m_name(str)
 , m_hash(std::hash<std::string>()(m_name))
{
}

event_type::event_type(size_t hash_code)
 : m_hash(hash_code)
{
}

event_type::event_type(const event_type& t)
 : m_name(t.m_name)
 , m_hash(t.m_hash)
{
}

event_type::event_type(event_type&& t)
 : m_name(std::move(t.m_name))
 , m_hash(t.m_hash)
{
}

event_type::~event_type()
{
}

event_type& event_type::operator= (const event_type& t)
{
    m_name = t.m_name;
    m_hash = t.m_hash;
    return *this;
}

event_type& event_type::operator= (event_type&& t)
{
    m_name = std::move(t.m_name);
    m_hash = t.m_hash;
    return *this;
}

std::string event_type::get_name() const
{
    return m_name;
}

size_t event_type::get_hash() const
{
    return m_hash;
}

event_type::operator std::string() const
{
    return m_name;
}

event_type::operator size_t() const
{
    return m_hash;
}

bool event_type::comparator::operator() (const event_type& a, const event_type& b) const
{
    return (a.get_hash() < b.get_hash());
}


c_event_manager::c_event_manager(application* app)
 : m_app(app)
 , m_thread("event manager")
 , m_remote_access(true)
{
    m_app->get_serializer()->add_rule<event_type>(serialize_event_type, deserialize_event_type);
    m_app->get_serializer()->add_rule_ex<c_event>(serialize_event, deserialize_event);
}

c_event_manager::~c_event_manager()
{
}

application* c_event_manager::get_app() const
{
    return m_app;
}

void c_event_manager::enable_remote_access()
{
    m_remote_access = true;
}

void c_event_manager::disable_remote_access()
{
    m_remote_access = false;
}

bool c_event_manager::is_remote_access_enabled() const
{
    return m_remote_access;
}

event_listener* c_event_manager::add_listener(event_type t, event_callback cb)
{
    event_listener* l = new func_event_listener(cb);
    this->add_listener(t, l);
    return l;
}

void c_event_manager::add_listener(event_type t, event_listener* l)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    l->grab();
    m_listeners[t].push_back(l);
}

void c_event_manager::remove_listener(event_type t, event_listener* l)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto l_list = m_listeners[t];
    auto l_it = l_list.begin(), l_end = l_list.end();

    for (; l_it != l_end; ++l_it)
    {
        if (*l_it == l)
        {
            l_list.erase(l_it);
            (*l_it)->drop();
            return;
        }
    }
}

void c_event_manager::push_event(event_type t, event::attribute_list al)
{
    this->push_event(t, std::forward<event::attribute_list>(al), nullptr);
}

void c_event_manager::push_event(event_type t, event::attribute_list al, remote_application* orig)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_thread.add_task( new event_task(this, orig, t, std::forward<event::attribute_list>(al)) );
}

void c_event_manager::push_event(c_event evt)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_thread.add_task( new event_task(this, evt) );
}

bool c_event_manager::trigger_event(event_type t, event::attribute_list al)
{
    c_event evt(nullptr, t, std::forward<event::attribute_list>(al));
    return this->trigger_event(&evt);
}

bool c_event_manager::trigger_event(event_type t, event::attribute_list al, remote_application* orig)
{
    c_event evt(orig, t, std::forward<event::attribute_list>(al));
    return this->trigger_event(&evt);
}

bool c_event_manager::trigger_event(const event* evt)
{
    if (evt == nullptr) return false;

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto l_list = m_listeners[evt->get_type()];
    auto l_it = l_list.begin(), l_end = l_list.end();

    for (; l_it != l_end; ++l_it)
    {
        auto filters = (*l_it)->get_filters();
        auto f = filters.begin(), f_end = filters.end();

        for (; f != f_end; ++f)
        {
            // this listener should be skipped if one of its filters return true
            if ((*f)(*evt)) goto skip_listener;
        }

        // we don't continue if the event is consumed
        if ((*l_it)->on_event(*evt)) return true;

        skip_listener: continue;
    }

    return false;
}
