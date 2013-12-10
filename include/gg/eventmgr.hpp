#ifndef GG_EVENTMGR_HPP_INCLUDED
#define GG_EVENTMGR_HPP_INCLUDED

#include <map>
#include <list>
#include <functional>
#include "gg/refcounted.hpp"
#include "gg/var.hpp"

namespace gg
{
    class application;

    class event : public reference_counted
    {
    public:
        typedef std::map<std::string,var>::value_type attribute;

        static event* create(std::string name, std::initializer_list<attribute> il = {});
        virtual ~event() {}
        virtual std::string get_name() const = 0;
        virtual void add(std::string key, var value) = 0;
        virtual void add(std::initializer_list<attribute> il) = 0;
        virtual var& operator[] (std::string attr) = 0;
        virtual const var& operator[] (std::string attr) const = 0;
    };

    typedef std::function<bool(const event&)> event_filter; // returns true if event should be skipped
    typedef std::function<bool(const event&)> event_callback; // return true if event is consumed

    class event_listener : public reference_counted
    {
        std::list<event_filter> m_filters;

    public:
        void add_filter(event_filter f) { m_filters.push_back(f); }
        const std::list<event_filter>& get_filters() const { return m_filters; }
        virtual ~event_listener() {}
        virtual bool on_event(const event&) = 0;
    };

    class event_type
    {
    protected:
        virtual ~event_type() {}

    public:
        virtual std::string get_name() const = 0;
        virtual void add_listener(event_listener*) = 0;
        virtual void add_listener(event_callback) = 0;
        virtual void remove_listener(event_listener*) = 0;
    };

    class event_manager
    {
    protected:
        virtual ~event_manager() {}

    public:
        virtual application* get_app() const = 0;
        virtual event_type* create_event_type(std::string name) = 0;
        virtual event_type* get_event_type(std::string name) = 0;
        virtual event_listener* create_event_listener(event_callback) const = 0;
        virtual void push_event(event*) = 0;
        virtual bool trigger_event(event*) = 0;
    };
};

#endif // GG_EVENTMGR_HPP_INCLUDED
