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

    class c_remote_event_manager : public remote_event_manager, public packet_handler
    {
        mutable tthread::mutex m_mutex;
        event_manager* m_evtmgr;
        c_connection m_conn;

    public:
        c_remote_event_manager(event_manager* evtmgr, std::string addr, uint16_t port);
        ~c_remote_event_manager();
        bool connect();
        void disconnect();
        bool is_connected() const;
        event_listener* add_listener(event_type, event_callback);
        void add_listener(event_type, event_listener*);
        void remove_listener(event_type, event_listener*);
        void push_event(event_type, std::initializer_list<event::attribute>);

        // inherited from packet_handler
        void handle_packet(connection*);
    };

    class c_event_manager : public event_manager, public connection_handler, public packet_handler
    {
        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::map<event_type, std::list<event_listener*>, event_type::comparator> m_evt_types;
        std::list<c_listener*> m_ports;
        c_thread m_thread;

    public:
        c_event_manager(application* app);
        ~c_event_manager();
        application* get_app() const;
        bool open_port(uint16_t port);
        void close_port(uint16_t port);
        void close_ports();
        remote_event_manager* get_remote_event_manager(std::string addr, uint16_t port);
        void add_event_type(event_type);
        void remove_event_type(event_type);
        event_listener* add_listener(event_type, event_callback);
        void add_listener(event_type, event_listener*);
        void remove_listener(event_type, event_listener*);
        void push_event(event_type, std::initializer_list<event::attribute>);
        bool trigger_event(event_type, std::initializer_list<event::attribute>) const;
        bool trigger_event(const event*) const;

        // inherited from connection_handler and packet_handler
        void handle_connection_open(connection*);
        void handle_connection_close(connection*);
        void handle_packet(connection*);
    };
};

#endif // C_EVENTMGR_HPP_INCLUDED
