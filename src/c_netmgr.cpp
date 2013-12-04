#include "c_netmgr.hpp"

using namespace gg;


c_listener::c_listener(uint16_t port, bool is_tcp)
 : m_port(port)
 , m_open(false)
 , m_tcp(is_tcp)
{

}

c_listener::~c_listener()
{

}

uint16_t c_listener::get_port() const
{
    return m_port;
}

void c_listener::set_connection_open_callback(connection_open_callback cb)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_open_cb = cb;
}

void c_listener::set_connection_close_callback(connection_close_callback cb)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_close_cb = cb;
}

int c_listener::send_to_all(buffer* buf)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    for (connection* c : m_conns) c->send(buf);
}

int c_listener::send_to_all(uint8_t* buf, size_t len)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    for (connection* c : m_conns) c->send(buf, len);
}

bool c_listener::is_opened()
{
    return m_open;
}

bool c_listener::open()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

}

bool c_listener::close()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

}



c_connection::c_connection(std::string address, uint16_t port, bool is_tcp)
 : m_listener(nullptr)
 , m_address(address)
 , m_port(port)
 , m_open(false)
 , m_tcp(is_tcp)
{

}

c_connection::c_connection(listener* l, SOCKADDR_STORAGE sockaddr)
 : m_listener(l)
 , m_sockaddr(sockaddr)
{

}

c_connection::~c_connection()
{

}

listener* c_connection::get_listener()
{
    return m_listener;
}

buffer* c_connection::get_input_buffer()
{
    return m_input_buf;
}

buffer* c_connection::get_output_buffer()
{
    return m_output_buf;
}

std::string c_connection::get_address() const
{
    return m_address;
}

uint16_t c_connection::get_port() const
{
    return m_port;
}

int c_connection::send(buffer* buf)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

}

int c_connection::send(uint8_t* buf, size_t len)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

}

void c_connection::set_packet_callback(packet_callback cb)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_packet_cb = cb;
}

bool c_connection::is_opened()
{
    return m_open;
}

bool c_connection::open()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

}

bool c_connection::close()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

}



c_network_manager::c_network_manager(application* app)
 : m_app(app)
{
}

c_network_manager::~c_network_manager()
{
}

application* c_network_manager::get_app() const
{
    return m_app;
}

listener* c_network_manager::open_tcp_listener(uint16_t port) const
{
    return new c_listener(port, true);
}

listener* c_network_manager::open_udp_listener(uint16_t port) const
{
    return new c_listener(port, false);
}

connection* c_network_manager::open_tcp_connection(std::string address, uint16_t port) const
{
    return new c_connection(address, port, true);
}

connection* c_network_manager::open_udp_connection(std::string address, uint16_t port) const
{
    return new c_connection(address, port, false);
}
