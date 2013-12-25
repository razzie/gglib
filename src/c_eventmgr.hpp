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
        mutable event_manager* m_evtmgr;
        event_type m_type;
        attribute_list m_attributes;

    public:
        c_event(event_type, std::initializer_list<attribute> = {});
        ~c_event();
        event_manager* get_event_manager() const;
        event_type get_type() const;
        void add(std::string, var);
        const var& operator[] (std::string) const;
        const var& get_attribute(std::string) const;
        const attribute_list& get_attributes() const;

        static bool serialize(const var& v, buffer* buf, const serializer*);
        static optional<var> deserialize(buffer* buf, const serializer*);
    };

    class c_event_manager : public event_manager
    {
        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::map<event_type, std::list<event_listener*>, event_type::comparator> m_evt_types;
        c_thread m_thread;

    public:
        c_event_manager(application* app);
        ~c_event_manager();
        application* get_app() const;
        void add_event_type(event_type);
        void remove_event_type(event_type);
        event_listener* add_listener(event_type, event_callback);
        void add_listener(event_type, event_listener*);
        void remove_listener(event_type, event_listener*);
        void push_event(event_type, std::initializer_list<event::attribute>);
        bool trigger_event(event_type, std::initializer_list<event::attribute>) const;
        bool trigger_event(const event*) const;
    };
};

#endif // C_EVENTMGR_HPP_INCLUDED
