#include <functional>
#include "c_eventmgr.hpp"
#include "gg/application.hpp"

using namespace gg;


class event_task : public task
{
    c_event_manager* m_evtmgr;
    event* m_evt;

public:
    event_task(c_event_manager* evtmgr,
               event* evt)
    {
        m_evtmgr = evtmgr;
        m_evt = evt;
    }

    ~event_task() {}

    bool run(uint32_t)
    {
        m_evtmgr->trigger_event(m_evt);
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


bool serialize_string(const var& v, buffer* buf);
optional<var> deserialize_string(buffer* buf);

bool c_event::serialize(const var& v, buffer* buf, const serializer* s)
{
    if (buf == nullptr || v.is_empty() || v.get_type() != typeid(c_event))
        return false;

    grab_guard bufgrab(buf);

    const c_event& e = v.get<c_event>();
    auto it = e.m_attributes.begin(), end = e.m_attributes.end();
    uint8_t attr_count = e.m_attributes.size();
    size_t hash_code = e.get_hash();

    buf->push(reinterpret_cast<uint8_t*>(&hash_code), sizeof(size_t));
    buf->push(attr_count);

    for (; it != end; ++it)
    {
        serialize_string(it->first, buf);
        if (!s->serialize(it->second, buf)) return false;
    }

    return true;
}

optional<var> c_event::deserialize(buffer* buf, const serializer* s)
{
    if (buf == nullptr || buf->available() == 0)
        return {};

    grab_guard bufgrab(buf);

    size_t hash_code;
    if (buf->pop(reinterpret_cast<uint8_t*>(&hash_code), sizeof(size_t)) != sizeof(size_t)) return {};
    c_event e(hash_code);

    optional<uint8_t> attrcnt = buf->pop();
    if (!attrcnt.is_valid()) return {};
    uint8_t attr_count = attrcnt.get();

    for (uint8_t i = 0; i < attr_count; ++i)
    {
        optional<var> attr = deserialize_string(buf);
        optional<var> val = s->deserialize(buf);

        if (!attr.is_valid() || !val.is_valid()
            || attr.get().get_type() != typeid(std::string))
            return {};

        e.add(std::move(attr.get().get<std::string>()), std::move(val.get()));
    }

    return {};
}


event* event::create(std::string name, std::initializer_list<attribute> il)
{
    return new c_event(name, il);
}

event* event::create(size_t hash_code, std::initializer_list<attribute> il)
{
    return new c_event(hash_code, il);
}

c_event::c_event(std::string name, std::initializer_list<attribute> il)
 : m_hash(std::hash<std::string>()(name))
 , m_attributes(il)
{
}

c_event::c_event(size_t hash_code, std::initializer_list<attribute> il)
 : m_hash(hash_code)
 , m_attributes(il)
{
}

c_event::~c_event()
{
}

size_t c_event::get_hash() const
{
    return m_hash;
}

void c_event::add(std::string key, var value)
{
    m_attributes.insert(std::make_pair(key,value));
}

void c_event::add(std::initializer_list<attribute> il)
{
    m_attributes.insert(il);
}

var& c_event::operator[] (std::string attr)
{
    return m_attributes.at(attr);
}

const var& c_event::operator[] (std::string attr) const
{
    return m_attributes.at(attr);
}


c_event_type::c_event_type(std::string name, c_event_manager* mgr)
 : m_name(name)
 , m_hash(std::hash<std::string>()(name))
 , m_parent_mgr(mgr)
{
}

c_event_type::c_event_type(c_event_type&& e)
 : m_name(std::move(e.m_name))
 , m_hash(e.m_hash)
 , m_listeners(std::move(e.m_listeners))
 , m_parent_mgr(e.m_parent_mgr)
{
}

c_event_type::~c_event_type()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto it=m_listeners.begin();
         it!=m_listeners.end();
         it=m_listeners.erase(it))
    {
        (*it)->drop();
    }
}

std::string c_event_type::get_name() const
{
    return m_name;
}

size_t c_event_type::get_hash() const
{
    return m_hash;
}

void c_event_type::add_listener(event_listener* l)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    l->grab();
    m_listeners.push_back(l);
}

void c_event_type::add_listener(event_callback cb)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    event_listener* l = m_parent_mgr->create_event_listener(cb);
    this->add_listener(l);
}

void c_event_type::remove_listener(event_listener* l)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_listeners.remove_if([l](event_listener* elem) -> bool
        {
            if (elem == l)
            {
                elem->drop();
                return true;
            }
            else
                return false;
        });
}


c_event_manager::c_event_manager(application* app)
 : m_app(app)
 , m_thread("event manager")
{
    m_app->get_serializer()->add_rule_ex<c_event>(c_event::serialize, c_event::deserialize);
}

c_event_manager::~c_event_manager()
{
}

application* c_event_manager::get_app() const
{
    return m_app;
}

c_event_type* c_event_manager::create_event_type(std::string name)
{
    size_t hash_code = std::hash<std::string>()(name);

    if (m_evt_types.count(hash_code) > 0)
        return get_event_type(hash_code);

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto ret = m_evt_types.insert( std::make_pair(hash_code, c_event_type(name, this)) );

    if (!ret.second)
        throw std::runtime_error("failed to create event type");

    return &ret.first->second;
}

c_event_type* c_event_manager::get_event_type(std::string name)
{
    size_t hash_code = std::hash<std::string>()(name);

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_evt_types.find(hash_code);
    if (it == m_evt_types.end())
        return nullptr;
    else
        return &it->second;
}

c_event_type* c_event_manager::get_event_type(size_t hash_code)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_evt_types.find(hash_code);
    if (it == m_evt_types.end())
        return nullptr;
    else
        return &it->second;
}

event_listener* c_event_manager::create_event_listener(event_callback cb) const
{
    return new func_event_listener(cb);
}

void c_event_manager::push_event(event* evt)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    if (nullptr == this->get_event_type(evt->get_hash()))
        throw std::runtime_error("unknown event type");

    m_thread.add_task( new event_task(this,evt) );
}

bool c_event_manager::trigger_event(event* evt)
{
    c_event_type* evt_type = this->get_event_type(evt->get_hash());

    if (nullptr == evt_type)
        throw std::runtime_error("unknown event type");

    tthread::lock_guard<tthread::mutex> guard(evt_type->m_mutex);

    auto_drop<event> evt_releaser(evt); // it will call evt->drop() when this function returns

    for (auto l=evt_type->m_listeners.begin(); l!=evt_type->m_listeners.end(); l++)
    {
        auto filters = (*l)->get_filters();
        for (auto f=filters.begin(); f!=filters.end(); f++)
        {
            // this listener should be skipped if one of its filters return true
            if ((*f)(*evt)) goto skip_listener;
        }

        // we don't continue if the event is consumed
        if ((*l)->on_event(*evt)) return true;

        skip_listener: continue;
    }

    return false;
}
