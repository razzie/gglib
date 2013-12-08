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
        connection_handler* m_handler;
        bool m_open;
        bool m_tcp;

    public:
        c_listener(uint16_t port, bool is_tcp);
        ~c_listener();
        uint16_t get_port() const;
        void set_connection_handler(connection_handler*);
        void send_to_all(buffer*);
        void send_to_all(uint8_t*, size_t);
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
        packet_handler* m_handler;
        bool m_client;
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
        void set_packet_handler(packet_handler*);
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
