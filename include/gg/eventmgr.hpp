#ifndef GG_EVENTMGR_HPP_INCLUDED
#define GG_EVENTMGR_HPP_INCLUDED

#include <cstdint>
#include <iostream>
#include <string>
#include <map>
#include <list>
#include <functional>
#include "gg/refcounted.hpp"
#include "gg/var.hpp"
#include "gg/enumerator.hpp"

namespace gg
{
    class application;
    class event_dispatcher;

    class event_type
    {
        std::string m_name;
        size_t m_hash;

    public:
        event_type(std::string);
        event_type(const char*);
        event_type(size_t);
        event_type(const event_type&);
        event_type(event_type&&);
        ~event_type();
        event_type& operator= (const event_type&);
        event_type& operator= (event_type&&);
        std::string get_name() const;
        size_t get_hash() const;
        operator std::string() const;
        operator size_t() const;

        struct comparator
        {
            comparator() = default;
            ~comparator() = default;
            bool operator() (const event_type&, const event_type&) const;
        };
    };

    class event
    {
    public:
        typedef std::map<std::string, var> attribute_list;
        typedef attribute_list::value_type attribute;

        virtual event_dispatcher* get_originator() const = 0;
        virtual event_type get_type() const = 0;
        virtual const attribute_list& get_attributes() const = 0;
        virtual const var& operator[] (std::string) const = 0;
        virtual const var& get_attribute(std::string) const = 0;

    protected:
        virtual ~event() {}
    };

    std::ostream& operator<< (std::ostream&, const event::attribute&);
    std::ostream& operator<< (std::ostream&, const event::attribute_list&);
    std::ostream& operator<< (std::ostream&, const event&);

    class event_listener : public reference_counted
    {
    public:
        typedef std::function<bool(const event&)> event_filter; // returns true if event should be skipped

        void add_filter(event_filter f) { m_filters.push_back(f); }
        const std::list<event_filter>& get_filters() const { return m_filters; }
        virtual ~event_listener() {}
        virtual bool on_event(const event&) = 0;

    protected:
        std::list<event_filter> m_filters;
    };

    typedef std::function<bool(const event&)> event_callback; // returns true if event is consumed

    class event_dispatcher : public reference_counted
    {
    protected:
        virtual ~event_dispatcher() {}

    public:
        virtual bool connect() = 0;
        virtual void disconnect() = 0;
        virtual bool is_connected() const = 0;
        virtual std::string get_address() const = 0;
        virtual uint16_t get_port() const = 0;
        virtual void push_event(event_type, event::attribute_list) = 0;
    };

    class event_manager
    {
    public:
        virtual application* get_app() const = 0;

        virtual bool open_port(uint16_t port) = 0;
        virtual void close_port(uint16_t port) = 0;
        virtual void close_ports() = 0;
        virtual event_dispatcher* create_event_dispatcher_alias() = 0;
        virtual event_dispatcher* connect(std::string addr, uint16_t port) = 0;
        virtual enumerator<event_dispatcher*> get_connections() = 0;

        virtual event_listener* add_listener(event_type, event_callback) = 0;
        virtual void add_listener(event_type, event_listener*) = 0;
        virtual void remove_listener(event_type, event_listener*) = 0;
        virtual void push_event(event_type, event::attribute_list) = 0;
        virtual bool trigger_event(event_type, event::attribute_list) = 0;

    protected:
        virtual ~event_manager() {}
    };
};

#endif // GG_EVENTMGR_HPP_INCLUDED
