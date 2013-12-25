#ifndef GG_EVENTMGR_HPP_INCLUDED
#define GG_EVENTMGR_HPP_INCLUDED

#include <iostream>
#include <map>
#include <list>
#include <initializer_list>
#include <functional>
#include "gg/refcounted.hpp"
#include "gg/var.hpp"

namespace gg
{
    class application;
    class event_manager;

    class event_type
    {
    public:
        event_type(std::string);
        event_type(const char*);
        event_type(size_t);
        event_type(const event_type&);
        event_type(event_type&&);
        ~event_type();
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

    private:
        std::string m_name;
        size_t m_hash;
    };

    class event
    {
    public:
        typedef std::map<std::string, var> attribute_list;
        typedef attribute_list::value_type attribute;

        virtual event_manager* get_event_manager() const = 0;
        virtual event_type get_type() const = 0;
        virtual const var& operator[] (std::string) const = 0;
        virtual const var& get_attribute(std::string) const = 0;
        virtual const attribute_list& get_attributes() const = 0;

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

    class event_manager
    {
    public:
        typedef std::function<bool(const event&)> event_callback; // returns true if event is consumed

        virtual application* get_app() const = 0;
        virtual void add_event_type(event_type) = 0;
        virtual void remove_event_type(event_type) = 0;
        virtual event_listener* add_listener(event_type, event_callback) = 0;
        virtual void add_listener(event_type, event_listener*) = 0;
        virtual void remove_listener(event_type, event_listener*) = 0;
        virtual void push_event(event_type, std::initializer_list<event::attribute>) = 0;
        virtual bool trigger_event(event_type, std::initializer_list<event::attribute>) const = 0;

    protected:
        virtual ~event_manager() {}
    };
};

#endif // GG_EVENTMGR_HPP_INCLUDED
