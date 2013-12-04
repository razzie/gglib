#ifndef C_NETMGR_HPP_INCLUDED
#define C_NETMGR_HPP_INCLUDED

#include <list>
#include <winsock2.h>
#include "tinythread.h"
#include "gg/netmgr.hpp"

namespace gg
{
    class c_listener : public listener
    {
        mutable tthread::mutex m_mutex;
        SOCKADDR_STORAGE m_sockaddr;
        uint16_t m_port;
        std::list<connection*> m_conns;
        connection_open_callback m_open_cb;
        connection_close_callback m_close_cb;
        bool m_open;
        bool m_tcp;

    public:
        c_listener(uint16_t port, bool is_tcp);
        ~c_listener();
        uint16_t get_port() const;
        void set_connection_open_callback(connection_open_callback);
        void set_connection_close_callback(connection_close_callback);
        int send_to_all(buffer*);
        int send_to_all(uint8_t*, size_t);
        bool is_opened();
        bool open();
        bool close();
    };

    class c_connection : public connection
    {
        mutable tthread::mutex m_mutex;
        listener* m_listener;
        SOCKADDR_STORAGE m_sockaddr;
        std::string m_address;
        uint16_t m_port;
        buffer* m_input_buf;
        buffer* m_output_buf;
        packet_callback m_packet_cb;
        bool m_open;
        bool m_tcp;

    public:
        c_connection(std::string address, uint16_t port, bool is_tcp);
        c_connection(listener*, SOCKADDR_STORAGE);
        ~c_connection();
        listener* get_listener();
        buffer* get_input_buffer();
        buffer* get_output_buffer();
        std::string get_address() const;
        uint16_t get_port() const;
        int send(buffer*);
        int send(uint8_t*, size_t);
        void set_packet_callback(packet_callback);
        bool is_opened();
        bool open();
        bool close();
    };

    class c_network_manager : public network_manager
    {
        mutable application* m_app;

    public:
        c_network_manager(application*);
        ~c_network_manager();
        application* get_app() const;
        listener* open_tcp_listener(uint16_t port) const;
        listener* open_udp_listener(uint16_t port) const;
        connection* open_tcp_connection(std::string address, uint16_t port) const;
        connection* open_udp_connection(std::string address, uint16_t port) const;
    };
};

#endif // C_NETMGR_HPP_INCLUDED
