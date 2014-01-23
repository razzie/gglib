#ifndef C_EVENTMGR_HPP_INCLUDED
#define C_EVENTMGR_HPP_INCLUDED

#include "tinythread.h"
#include "gg/eventmgr.hpp"
#include "gg/optional.hpp"
#include "gg/serializer.hpp"
#include "c_taskmgr.hpp"
#include "c_netmgr.hpp"

namespace gg
{
    class c_event : public event
    {
        mutable event_dispatcher* m_orig;
        event_type m_type;
        attribute_list m_attributes;

    public:
        c_event(event_dispatcher*, event_type, std::initializer_list<attribute> = {});
        c_event(event_dispatcher*, buffer*, const serializer*); // deserialize
        c_event(const c_event&);
        c_event(c_event&&);
        ~c_event();
        void set_originator(event_dispatcher*);
        event_dispatcher* get_originator() const;
        event_type get_type() const;
        const attribute_list& get_attributes() const;
        void add(std::string, var);
        const var& operator[] (std::string) const;
        const var& get_attribute(std::string) const;
        bool serialize(buffer* buf, const serializer*) const;
    };

    class c_event_manager : public event_manager, public connection_handler, public packet_handler
    {
        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::map<event_type, std::list<event_listener*>, event_type::comparator> m_listeners;
        std::list<c_listener*> m_ports;
        std::list<event_dispatcher*> m_conns;
        c_thread m_thread;

    public:
        c_event_manager(application* app);
        ~c_event_manager();
        application* get_app() const;

        bool open_port(uint16_t port);
        void close_port(uint16_t port);
        void close_ports();
        event_dispatcher* connect(std::string addr, uint16_t port);
        enumerator<event_dispatcher*> get_connections();
        operator event_dispatcher*(); // use this as an event_dispatcher

        event_listener* add_listener(event_type, event_callback);
        void add_listener(event_type, event_listener*);
        void remove_listener(event_type, event_listener*);
        void push_event(event_type, std::initializer_list<event::attribute>);
        void push_event(event_type, std::initializer_list<event::attribute>, event_dispatcher*);
        bool trigger_event(event_type, std::initializer_list<event::attribute>);
        bool trigger_event(event_type, std::initializer_list<event::attribute>, event_dispatcher*);
        bool trigger_event(const event*);

        // inherited from connection_handler and packet_handler
        void handle_connection_open(connection*);
        void handle_connection_close(connection*);
        void handle_packet(connection*);
    };
};

#endif // C_EVENTMGR_HPP_INCLUDED
