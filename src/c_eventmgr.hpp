#ifndef C_EVENTMGR_HPP_INCLUDED
#define C_EVENTMGR_HPP_INCLUDED

#include "tinythread.h"
#include "gg/eventmgr.hpp"
#include "gg/optional.hpp"
#include "gg/serializer.hpp"
#include "c_taskmgr.hpp"

namespace gg
{
    class c_event : public event
    {
        mutable remote_application* m_orig;
        event_type m_type;
        attribute_list m_attributes;

    public:
        c_event(remote_application*, event_type, event::attribute_list&& = {});
        c_event(remote_application*, buffer*, const serializer*); // deserialize
        c_event(const c_event&);
        c_event(c_event&&);
        ~c_event();
        void set_originator(remote_application*);
        remote_application* get_originator() const;
        event_type get_type() const;
        const attribute_list& get_attributes() const;
        void add(std::string, var);
        const var& operator[] (std::string) const;
        const var& get_attribute(std::string) const;
        bool serialize(buffer* buf, const serializer*) const;
    };

    class c_event_manager : public event_manager
    {
        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::map<event_type, std::list<event_listener*>, event_type::comparator> m_listeners;
        c_thread m_thread;

    public:
        c_event_manager(application* app);
        ~c_event_manager();
        application* get_app() const;
        event_listener* add_listener(event_type, event_callback);
        void add_listener(event_type, event_listener*);
        void remove_listener(event_type, event_listener*);
        void push_event(event_type, event::attribute_list);
        void push_event(event_type, event::attribute_list, remote_application*);
        void push_event(c_event);
        bool trigger_event(event_type, event::attribute_list);
        bool trigger_event(event_type, event::attribute_list, remote_application*);
        bool trigger_event(const event*);
    };
};

#endif // C_EVENTMGR_HPP_INCLUDED
