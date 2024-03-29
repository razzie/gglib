#ifndef C_NETMGR_HPP_INCLUDED
#define C_NETMGR_HPP_INCLUDED

#include <set>
#include <sstream>
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
        std::set<connection*> m_conns;
        connection_handler* m_handler;
        volatile bool m_open;
        bool m_tcp;
        c_thread m_thread;
        std::ostream* m_err;

        void error_close();

    public:
        c_listener(uint16_t port, bool is_tcp);
        c_listener(const c_listener&) = delete;
        c_listener(c_listener&&) = delete;
        ~c_listener();
        uint16_t get_port() const;
        void set_connection_handler(connection_handler*);
        void set_connection_handler(std::function<void(connection*, bool is_opened)>);
        connection_handler* get_connection_handler() const;
        enumerator<connection*> get_connections();
        void send_to_all(buffer*);
        void send_to_all(uint8_t*, size_t);
        bool is_opened() const;
        bool open();
        void close();
        void set_error_stream(std::ostream&);
        bool run(uint32_t); // inherited from gg::task
        void detach_connection(connection*);
    };

    class c_connection : public connection, public task
    {
        mutable tthread::recursive_mutex m_mutex;
        c_listener* m_listener;
        SOCKET m_socket;
        SOCKADDR_STORAGE m_sockaddr;
        std::string m_address;
        uint16_t m_port;
        buffer* m_input_buf;
        buffer* m_output_buf;
        packet_handler* m_packet_handler;
        connection_handler* m_conn_handler;
        volatile bool m_open;
        bool m_tcp;
        c_thread m_thread;
        std::ostream* m_err;

        void error_close();
        bool flush_output_buffer();

    public:
        c_connection(std::string address, uint16_t port, bool is_tcp);
        c_connection(c_listener*, SOCKET, SOCKADDR_STORAGE*, bool is_tcp);
        c_connection(const c_connection&) = delete;
        c_connection(c_connection&&) = delete;
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
        void set_error_stream(std::ostream&);
        bool run(uint32_t); // inherited from gg::task
    };

    class c_network_manager : public network_manager
    {
        mutable application* m_app;

    public:
        c_network_manager(application*);
        ~c_network_manager();
        application* get_app() const;
        std::string get_hostname() const;
        bool is_big_endian() const;
        bool is_little_endian() const;
        listener* create_tcp_listener(uint16_t port) const;
        listener* create_udp_listener(uint16_t port) const;
        connection* create_tcp_connection(std::string address, uint16_t port) const;
        connection* create_udp_connection(std::string address, uint16_t port) const;
    };
};

#endif // C_NETMGR_HPP_INCLUDED
