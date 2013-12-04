#ifndef GG_NETMGR_HPP_INCLUDED
#define GG_NETMGR_HPP_INCLUDED

#include <functional>
#include "gg/refcounted.hpp"
#include "gg/buffer.hpp"

namespace gg
{
    class application;
    class connection;

    class listener : public reference_counted
    {
    public:
        typedef std::function<void(connection*)> connection_open_callback;
        typedef std::function<void(connection*)> connection_close_callback;

        virtual ~listener() {}
        virtual uint16_t get_port() const = 0;
        virtual void set_connection_open_callback(connection_open_callback) = 0;
        virtual void set_connection_close_callback(connection_close_callback) = 0;
        virtual int send_to_all(buffer*) = 0;
        virtual int send_to_all(uint8_t*, size_t) = 0;
        virtual bool is_opened() = 0;
        virtual bool open() = 0;
        virtual bool close() = 0;
    };

    class connection : public reference_counted
    {
    public:
        typedef std::function<void(connection*,buffer*)> packet_callback;

        virtual ~connection() {}
        virtual listener* get_listener() = 0;
        virtual buffer* get_input_buffer() = 0;
        virtual buffer* get_output_buffer() = 0;
        virtual std::string get_address() const = 0;
        virtual uint16_t get_port() const = 0;
        virtual int send(buffer*) = 0;
        virtual int send(uint8_t*, size_t) = 0;
        virtual void set_packet_callback(packet_callback) = 0;
        virtual bool is_opened() = 0;
        virtual bool open() = 0;
        virtual bool close() = 0;
    };

    class network_manager
    {
    protected:
        virtual ~network_manager() {}

    public:
        virtual application* get_app() const = 0;
        virtual listener* open_tcp_listener(uint16_t port) const = 0;
        virtual listener* open_udp_listener(uint16_t port) const = 0;
        virtual connection* open_tcp_connection(std::string address, uint16_t port) const = 0;
        virtual connection* open_udp_connection(std::string address, uint16_t port) const = 0;
    };
};

#endif // GG_NETMGR_HPP_INCLUDED
