#ifndef C_NETMGR_HPP_INCLUDED
#define C_NETMGR_HPP_INCLUDED

#include <list>
#include <winsock2.h>
#include "tinythread.h"
#include "gg/netmgr.hpp"
#include "c_taskmgr.hpp"

namespace gg
{
    class c_listener : public listener, public task
    {
        mutable tthread::recursive_mutex m_mutex;
        SOCKET m_socket;
        SOCKADDR_STORAGE m_sockaddr;
        uint16_t m_port;
        std::list<connection*> m_conns;
        connection_handler* m_handler;
        volatile bool m_open;
        bool m_tcp;
        c_thread m_thread;

        void error_close();

    public:
        c_listener(uint16_t port, bool is_tcp);
        ~c_listener();
        uint16_t get_port() const;
        void set_connection_handler(connection_handler*);
        void set_connection_handler(std::function<void(connection*, bool is_opened)>);
        connection_handler* get_connection_handler() const;
        void send_to_all(buffer*);
        void send_to_all(uint8_t*, size_t);
        bool is_opened() const;
        bool open();
        void close();
        bool run(uint32_t); // inherited from gg::task
    };

    class c_connection : public connection, public task
    {
        mutable tthread::recursive_mutex m_mutex;
        listener* m_listener;
        SOCKET m_socket;
        SOCKADDR_STORAGE m_sockaddr;
        std::string m_address;
        uint16_t m_port;
        buffer* m_input_buf;
        buffer* m_output_buf;
        packet_handler* m_packet_handler;
        connection_handler* m_conn_handler;
        bool m_client;
        volatile bool m_open;
        bool m_tcp;
        c_thread m_thread;

        void error_close();
        bool flush_output_buffer();

    public:
        c_connection(std::string address, uint16_t port, bool is_tcp);
        c_connection(listener*, SOCKET, bool is_tcp);
        ~c_connection();
        listener* get_listener();
        buffer* get_input_buffer();
        buffer* get_output_buffer();
        std::string get_address() const;
        uint16_t get_port() const;
        void send(buffer*);
        void send(uint8_t*, size_t);
        void set_packet_handler(packet_handler*);
        void set_packet_handler(std::function<void(connection*)>);
        packet_handler* get_packet_handler() const;
        void set_connection_handler(connection_handler*);
        connection_handler* get_connection_handler() const;
        bool is_opened() const;
        bool open();
        void close();
        bool run(uint32_t); // inherited from gg::task
    };

    class c_network_manager : public network_manager
    {
        mutable application* m_app;

    public:
        c_network_manager(application*);
        ~c_network_manager();
        application* get_app() const;
        listener* create_tcp_listener(uint16_t port) const;
        listener* create_udp_listener(uint16_t port) const;
        connection* create_tcp_connection(std::string address, uint16_t port) const;
        connection* create_udp_connection(std::string address, uint16_t port) const;
    };
};

#endif // C_NETMGR_HPP_INCLUDED
