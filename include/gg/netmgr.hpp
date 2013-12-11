#ifndef GG_NETMGR_HPP_INCLUDED
#define GG_NETMGR_HPP_INCLUDED

#include <string>
#include <functional>
#include "gg/refcounted.hpp"
#include "gg/buffer.hpp"

namespace gg
{
    class application;
    class connection;

    class connection_handler : public reference_counted
    {
    public:
        virtual ~connection_handler() {}
        virtual void handle_connection_open(connection*) = 0;
        virtual void handle_connection_close(connection*) = 0;
    };

    class listener : public reference_counted
    {
    public:
        virtual ~listener() {}
        virtual uint16_t get_port() const = 0;
        virtual void set_connection_handler(connection_handler*) = 0;
        virtual connection_handler* get_connection_handler() const = 0;
        virtual void send_to_all(buffer*) = 0;
        virtual void send_to_all(uint8_t*, size_t) = 0;
        virtual bool is_opened() = 0;
        virtual bool open() = 0;
        virtual void close() = 0;
    };

    class packet_handler : public reference_counted
    {
    public:
        virtual ~packet_handler() {}
        virtual void handle_packet(connection*) = 0;
    };

    class connection : public reference_counted
    {
    public:
        virtual ~connection() {}
        virtual listener* get_listener() = 0;
        virtual buffer* get_input_buffer() = 0;
        virtual buffer* get_output_buffer() = 0;
        virtual std::string get_address() const = 0;
        virtual uint16_t get_port() const = 0;
        virtual void set_packet_handler(packet_handler*) = 0;
        virtual packet_handler* get_packet_handler() const = 0;
        virtual void send(buffer*) = 0;
        virtual void send(uint8_t*, size_t) = 0;
        virtual bool is_opened() = 0;
        virtual bool open() = 0;
        virtual void close() = 0;
    };

    class network_manager
    {
    protected:
        virtual ~network_manager() {}

    public:
        virtual application* get_app() const = 0;
        virtual listener* create_tcp_listener(uint16_t port) const = 0;
        virtual listener* create_udp_listener(uint16_t port) const = 0;
        virtual connection* create_tcp_connection(std::string address, uint16_t port) const = 0;
        virtual connection* create_udp_connection(std::string address, uint16_t port) const = 0;
    };
};

#endif // GG_NETMGR_HPP_INCLUDED
