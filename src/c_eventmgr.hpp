#ifndef C_EVENTMGR_HPP_INCLUDED
#define C_EVENTMGR_HPP_INCLUDED

#include "tinythread.h"
#include "gg/eventmgr.hpp"
#include "c_taskmgr.hpp"

namespace gg
{
    class c_event : public event
    {
        std::string m_name;
        std::map<std::string,var> m_attributes;

    public:
        c_event(std::string name);
        c_event(std::string name, std::initializer_list<attribute> il);
        ~c_event();
        std::string get_name() const;
        void add(std::string key, var value);
        void add(std::initializer_list<attribute> il);
        var& operator[] (const std::string& attr);
        const var& operator[] (const std::string& attr) const;
    };

    class c_event_manager;

    class c_event_type : public event_type
    {
        friend class c_event_manager;

        std::string m_name;
        std::list<event_listener*> m_listeners;
        tthread::mutex m_mutex;
        c_event_manager* m_parent_mgr;

    public:
        c_event_type(std::string name, c_event_manager*);
        c_event_type(c_event_type&&);
        ~c_event_type();
        std::string get_name() const;
        void add_listener(event_listener*);
        void add_listener(event_callback);
        void remove_listener(event_listener*);
    };

    class c_event_manager : public event_manager
    {
        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::map<std::string, c_event_type> m_evt_types;
        c_thread m_thread;

    public:
        c_event_manager(application* app);
        ~c_event_manager();
        application* get_app() const;
        c_event_type* create_event_type(std::string name);
        c_event_type* get_event_type(std::string name);
        event_listener* create_event_listener(event_callback) const;
        void push_event(event*);
        bool trigger_event(event*);
    };
};

#endif // C_EVENTMGR_HPP_INCLUDED
