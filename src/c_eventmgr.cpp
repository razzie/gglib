#include "c_eventmgr.hpp"

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


c_event_type::c_event_type(std::string name, c_event_manager* mgr)
 : m_name(name)
 , m_parent_mgr(mgr)
{
}

c_event_type::c_event_type(c_event_type&& e)
 : m_name(std::move(e.m_name))
 , m_listeners(std::move(e.m_listeners))
 , m_parent_mgr(e.m_parent_mgr)
{
}

c_event_type::~c_event_type()
{
    //std::lock_guard<std::mutex> guard(m_mutex);
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

void c_event_type::add_listener(event_listener* l)
{
    //std::lock_guard<std::mutex> guard(m_mutex);
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    l->grab();
    m_listeners.push_back(l);
}

void c_event_type::add_listener(event_callback cb)
{
    event_listener* l = m_parent_mgr->create_event_listener(cb);
    this->add_listener(l);
}

void c_event_type::remove_listener(event_listener* l)
{
    //std::lock_guard<std::mutex> guard(m_mutex);
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


c_event_manager::c_event_manager()
 : m_thread("event manager")
{
}

c_event_manager::~c_event_manager()
{
}

c_event_type* c_event_manager::create_event_type(std::string name)
{
    auto ret = m_evt_types.insert( std::make_pair(name, c_event_type(name,this)) );

    if (!ret.second)
        throw gg::exception("failed to create event type");

    return &ret.first->second;
}

c_event_type* c_event_manager::get_event_type(std::string name)
{
    auto it = m_evt_types.find(name);
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
    if (nullptr == this->get_event_type(evt->get_name()))
        throw gg::exception("unknown event type");

    m_thread.add_task( new event_task(this,evt) );
}

bool c_event_manager::trigger_event(event* evt)
{
    c_event_type* evt_type = this->get_event_type(evt->get_name());

    if (nullptr == evt_type)
        throw gg::exception("unknown event type");

    //std::lock_guard<std::mutex> guard(evt_type->m_mutex);
    tthread::lock_guard<tthread::mutex> guard(evt_type->m_mutex);

    auto_drop evt_releaser(evt); // it will call evt->drop() when this function returns

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
