#define _WIN32_WINNT 0x0501
#include <ws2tcpip.h>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include "c_netmgr.hpp"
#include "c_buffer.hpp"

using namespace gg;


static uint16_t get_port_from_sockaddr(SOCKADDR_STORAGE* sockaddr)
{
    switch (sockaddr->ss_family)
    {
        case AF_INET:
            return reinterpret_cast<SOCKADDR_IN*>(sockaddr)->sin_port;
        case AF_INET6:
            return reinterpret_cast<SOCKADDR_IN6*>(sockaddr)->sin6_port;
        default:
            return 0;
    }
}

static std::string get_addr_from_sockaddr(SOCKADDR_STORAGE* sockaddr)
{
    /*char str[INET6_ADDRSTRLEN];

    switch (sockaddr->ss_family)
    {
        case AF_INET:
            InetNtop(AF_INET, reinterpret_cast<SOCKADDR_IN*>(sockaddr)->sin_addr, str, sizeof(str));
            return str;
        case AF_INET6:
            InetNtop(AF_INET6, reinterpret_cast<SOCKADDR_IN6*>(sockaddr)->sin6_addr, str, sizeof(str));
            return str;
        default:
            return {};
    }*/

    char str[NI_MAXHOST];
    str[0] = '\0';
    getnameinfo(reinterpret_cast<SOCKADDR*>(sockaddr), sizeof(SOCKADDR_STORAGE),
                str, sizeof(str), NULL, 0, NI_NUMERICHOST);
    return str;
}


class wsa_init
{
    WSADATA m_wsaData;
    int m_status;

public:
    wsa_init()
    {
        // Initialize Winsock
        m_status = WSAStartup(MAKEWORD(2,2), &m_wsaData);
        if (m_status != 0)
        {
            std::cout << "WSAStartup failed: " << m_status << std::endl;
        }
    }

    ~wsa_init()
    {
        WSACleanup();
    }

    int get_status() const
    {
        return m_status;
    }
};

static wsa_init __wsa_init;


class func_connection_handler : public connection_handler
{
    std::function<void(connection*, bool)> m_func;

public:
    func_connection_handler(std::function<void(connection*, bool)> f) : m_func(f) {}
    ~func_connection_handler() {}
    void handle_connection_open(connection* c) { m_func(c, true); }
    void handle_connection_close(connection* c) { m_func(c, false); }
};

class func_packet_handler : public packet_handler
{
    std::function<void(connection*)> m_func;

public:
    func_packet_handler(std::function<void(connection*)> f) : m_func(f) {}
    ~func_packet_handler() {}
    void handle_packet(connection* c) { m_func(c); }
};


c_listener::c_listener(uint16_t port, bool is_tcp)
 : m_socket(INVALID_SOCKET)
 , m_port(port)
 , m_handler(nullptr)
 , m_open(false)
 , m_tcp(is_tcp)
 , m_thread("listener thread")
 , m_err(&std::cout)
{
}

c_listener::~c_listener()
{
    close();
    if (m_handler != nullptr) m_handler->drop();
}

uint16_t c_listener::get_port() const
{
    return m_port;
}

void c_listener::set_connection_handler(connection_handler* h)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    if (m_handler != nullptr) m_handler->drop();
    if (h != nullptr) h->grab();
    m_handler = h;
}

void c_listener::set_connection_handler(std::function<void(connection*, bool is_opened)> f)
{
    connection_handler* h = new func_connection_handler(f);
    this->set_connection_handler(h);
    h->drop();
}

connection_handler* c_listener::get_connection_handler() const
{
    return m_handler;
}

enumerator<connection*> c_listener::get_connections()
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    std::list<grab_ptr<connection, true>> tmplist;
    for (connection* conn : m_conns) tmplist.push_back(conn);

    conversion_container<decltype(tmplist), connection*> convlist(
        std::move(tmplist), [](grab_ptr<connection, true>& it)->connection*& { return it; });

    return std::move(convlist);
}

void c_listener::send_to_all(buffer* buf)
{
    grab_guard bufgrab(buf);
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    for (connection* c : m_conns) c->send(buf);
}

void c_listener::send_to_all(uint8_t* buf, size_t len)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    for (connection* c : m_conns) c->send(buf, len);
}

bool c_listener::is_opened() const
{
    return m_open;
}

bool c_listener::open()
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    if (m_open) return false;

    char port_str[6];
    std::sprintf(port_str, "%d", m_port);
    struct addrinfo hints, *result = NULL, *ptr = NULL;

    std::memset(&m_sockaddr, 0, sizeof(SOCKADDR_STORAGE));

    std::memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = m_tcp ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_protocol = m_tcp ? IPPROTO_TCP : IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    if (getaddrinfo(NULL, port_str, &hints, &result) != 0)
    {
        *m_err << "getaddrinfo error: " << WSAGetLastError() << std::endl;
        return false;
    }

    m_socket = INVALID_SOCKET;

    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        m_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (m_socket == INVALID_SOCKET)
        {
            *m_err << "socket error: " << WSAGetLastError() << std::endl;
            continue;
        }

        // Setup the TCP listening socket
        if (bind(m_socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(m_socket);
            *m_err << "bind error: " << WSAGetLastError() << std::endl;
            continue;
        }
    }

    if (m_socket == INVALID_SOCKET)
    {
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);
    m_open = true;
    m_thread.add_task(this);

    return true;
}

void c_listener::close()
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    if (!m_open) return;

    error_close();
    return;
}

void c_listener::set_error_stream(std::ostream& err)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    m_err = &err;
}

void c_listener::error_close()
{
    for (connection* c : m_conns)
    {
        try
        {
            c->close(); // handle_connection_close will be called
            c->drop();
        }
        catch (std::exception& e) { *m_err << e.what() << std::endl; }
        catch (...) {}
    }

    m_conns.clear();
    closesocket(m_socket);
    m_open = false;
}

bool c_listener::run(uint32_t)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    if (!m_open) return true; // returning true to finish task

    SOCKADDR_STORAGE addr;
    int addrlen = sizeof(SOCKADDR_STORAGE);

    if (m_tcp) // we are TCP
    {
        if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR)
        {
            *m_err << "listen error: " << WSAGetLastError() << std::endl;
            error_close();
            return true;
        }

        // accept a client socket
        SOCKET sock = accept(m_socket, reinterpret_cast<struct sockaddr*>(&addr), &addrlen);
        if (sock == INVALID_SOCKET)
        {
            *m_err << "accept error: " << WSAGetLastError() << std::endl;
            error_close();
            return true;
        }

        try
        {
            connection* conn = new c_connection(this, sock, &addr, m_tcp);
            m_conns.insert(conn);
        }
        catch (std::exception& e) { *m_err << e.what() << std::endl; }
        catch (...) {}
    }
    else // we are UDP
    {
        char buf[2048];
        int rc = recvfrom(m_socket, buf, sizeof(buf), 0, reinterpret_cast<struct sockaddr*>(&addr), &addrlen);
        if (rc == SOCKET_ERROR)
        {
            *m_err << "recvfrom error: " << WSAGetLastError() << std::endl;
            error_close();
            return true;
        }
        else if (rc > 0)
        {
            // received a datagram
            try
            {
                // faking a new connection
                connection* conn = new c_connection(this, m_socket, &addr, m_tcp);
                // triggering the packet handler callback to process the datagram
                packet_handler* ph = conn->get_packet_handler();
                if (ph != nullptr)
                {
                    conn->get_input_buffer()->push(reinterpret_cast<uint8_t*>(buf), rc);
                    ph->handle_packet(conn);
                }
                // faking connection drop
                conn->close();
                conn->drop();
            }
            catch (std::exception& e) { *m_err << e.what() << std::endl; }
            catch (...) {}
        }
    }

    return false; // returning false to keep the task persistent
}

void c_listener::detach_connection(connection* conn)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    m_conns.erase(conn);
}


c_connection::c_connection(std::string address, uint16_t port, bool is_tcp)
 : m_listener(nullptr)
 , m_address(address)
 , m_port(port)
 , m_input_buf(new c_buffer())
 , m_output_buf(new c_buffer())
 , m_packet_handler(nullptr)
 , m_conn_handler(nullptr)
 , m_open(false)
 , m_tcp(is_tcp)
 , m_thread("connection thread")
 , m_err(&std::cout)
{
}

c_connection::c_connection(c_listener* l, SOCKET sock, SOCKADDR_STORAGE* addr, bool is_tcp)
 : m_listener(l)
 , m_socket(sock)
 , m_sockaddr(*addr)
 , m_input_buf(new c_buffer())
 , m_output_buf(new c_buffer())
 , m_packet_handler(nullptr)
 , m_conn_handler(nullptr)
 , m_open(true)
 , m_tcp(is_tcp)
 , m_thread("connection thread")
 , m_err(&std::cout)
{
    /*int addrlen = sizeof(SOCKADDR_STORAGE);
    if (getpeername(m_socket, reinterpret_cast<struct sockaddr*>(&m_sockaddr), &addrlen) == SOCKET_ERROR)
        throw std::runtime_error("unable to initialize connection by socket");*/

    m_address = get_addr_from_sockaddr(&m_sockaddr);
    m_port = get_port_from_sockaddr(&m_sockaddr);

    if (m_listener && m_listener->get_connection_handler() != nullptr)
        m_listener->get_connection_handler()->handle_connection_open(this);

    m_thread.add_task(this);
}

c_connection::~c_connection()
{
    close();
    if (m_packet_handler != nullptr) m_packet_handler->drop();
    if (m_conn_handler != nullptr) m_conn_handler->drop();
    // we need to wait a bit to let the networking thread finish
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(100));
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

void c_connection::send(buffer* buf)
{
    grab_guard bufgrab(buf);
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    m_output_buf->push(buf);
}

void c_connection::send(uint8_t* buf, size_t len)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    m_output_buf->push(buf, len);
}

void c_connection::set_packet_handler(packet_handler* h)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    if (m_packet_handler != nullptr) m_packet_handler->drop();
    if (h != nullptr) h->grab();
    m_packet_handler = h;
}

void c_connection::set_packet_handler(std::function<void(connection*)> f)
{
    packet_handler* h = new func_packet_handler(f);
    this->set_packet_handler(h);
    h->drop();
}

packet_handler* c_connection::get_packet_handler() const
{
    return m_packet_handler;
}

void c_connection::set_connection_handler(connection_handler* h)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    if (m_conn_handler != nullptr) m_conn_handler->drop();
    if (h != nullptr) h->grab();
    m_conn_handler = h;
}

connection_handler* c_connection::get_connection_handler() const
{
    return m_conn_handler;
}

bool c_connection::is_opened() const
{
    return m_open;
}

bool c_connection::open()
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    // can't reopen if already opened or is a client
    if (m_open || (!m_open && m_listener != nullptr)) return false;

    char port_str[6];
    std::sprintf(port_str, "%d", m_port);
    struct addrinfo hints, *result = NULL, *ptr = NULL;

    std::memset(&m_sockaddr, 0, sizeof(SOCKADDR_STORAGE));

    std::memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = m_tcp ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_protocol = m_tcp ? IPPROTO_TCP : IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    if (getaddrinfo(m_address.c_str(), port_str, &hints, &result) != 0)
    {
        *m_err << "getaddrinfo error: " << WSAGetLastError() << std::endl;
        return false;
    }

    m_socket = INVALID_SOCKET;

    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        // Create a SOCKET for connecting to server
        m_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (m_socket == INVALID_SOCKET)
        {
            *m_err << "socket error: " << WSAGetLastError() << std::endl;
            continue;
        }

        if (m_tcp)
        {
            // Connect to server.
            if (connect(m_socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
            {
                *m_err << "connect error: " << WSAGetLastError() << std::endl;
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                continue;
            }
        }
        else
        {
            if (bind(m_socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
            {
                *m_err << "bind error: " << WSAGetLastError() << std::endl;
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                continue;
            }
        }
    }

    if (m_socket == INVALID_SOCKET)
    {
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);
    m_open = true;
    m_thread.add_task(this);

    if (m_listener && m_listener->get_connection_handler() != nullptr)
        m_listener->get_connection_handler()->handle_connection_open(this);

    if (m_conn_handler != nullptr)
        m_conn_handler->handle_connection_open(this);

    return true;
}

void c_connection::close()
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    if (!m_open) return;

    flush_output_buffer();
    error_close();
    return;
}

void c_connection::set_error_stream(std::ostream& err)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    m_err = &err;
}

void c_connection::error_close()
{
    if (m_listener != nullptr && m_listener->get_connection_handler() != nullptr)
        m_listener->get_connection_handler()->handle_connection_close(this);

    if (m_conn_handler != nullptr)
        m_conn_handler->handle_connection_close(this);

    if (m_listener != nullptr)
        m_listener->detach_connection(this);

    if (m_tcp) closesocket(m_socket);

    m_open = false;
}

bool c_connection::flush_output_buffer()
{
    if (m_open && m_output_buf->available())
    {
        // sending data queued in output buffer
        int rc;
        size_t len = m_output_buf->available();
        char buf[2048];
        len = m_output_buf->pop(reinterpret_cast<uint8_t*>(buf), (len > sizeof(buf)) ? sizeof(buf) : len);
        size_t bytes_sent = 0;

        for (; bytes_sent < len;)
        {
            if (m_tcp)
                rc = ::send(m_socket, &buf[bytes_sent], len, 0);
            else
                rc = ::sendto(m_socket, &buf[bytes_sent], len, 0,
                              reinterpret_cast<struct sockaddr*>(&m_sockaddr),
                              sizeof(SOCKADDR_STORAGE));

            if (rc == SOCKET_ERROR)
            {
                *m_err << "send error: " << WSAGetLastError() << std::endl;
                error_close();
                return false;
            }

            len -= rc;
            bytes_sent += rc;
        }
    }

    return true;
}

bool c_connection::run(uint32_t)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);
    if (!m_open) return true; // returning true to finish task

    // sending data queued in output buffer
    if (!flush_output_buffer()) return true;

    if (m_tcp) // in UDP case the listener will receive the traffic
    {
        // waiting for incoming data
        FD_SET in;
        FD_ZERO(&in);
        FD_SET(m_socket, &in);
        TIMEVAL timeout = {0, 10000};

        int rc = select(0, &in, NULL, NULL, &timeout);
        if (rc == SOCKET_ERROR)
        {
            *m_err << "select error: " << WSAGetLastError() << std::endl;
            error_close();
            return true;
        }

        if (FD_ISSET(m_socket, &in) == 0)
        {
            return false; // skipping recv
        }

        char buf[2048];
        rc = recv(m_socket, buf, sizeof(buf), 0);
        if (rc == SOCKET_ERROR)
        {
            *m_err << "recv error: " << WSAGetLastError() << std::endl;
            error_close();
            return true;
        }
        else if (rc == 0)
        {
            // connection closed from remote end
            error_close();
            return true;
        }
        else
        {
            // incoming data
            m_input_buf->push(reinterpret_cast<uint8_t*>(buf), rc);
            if (m_packet_handler != nullptr) m_packet_handler->handle_packet(this);
        }
    }

    return false;  // returning false to keep the task persistent
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

std::string c_network_manager::get_hostname() const
{
    char buf[1024];
    int rc = gethostname(buf, sizeof(buf)-1);
    if (rc != SOCKET_ERROR) return std::string(buf);
    else return {};
}

listener* c_network_manager::create_tcp_listener(uint16_t port) const
{
    return new c_listener(port, true);
}

listener* c_network_manager::create_udp_listener(uint16_t port) const
{
    return new c_listener(port, false);
}

connection* c_network_manager::create_tcp_connection(std::string address, uint16_t port) const
{
    return new c_connection(address, port, true);
}

connection* c_network_manager::create_udp_connection(std::string address, uint16_t port) const
{
    return new c_connection(address, port, false);
}
