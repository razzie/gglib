#ifndef C_APPLICATION_HPP_INCLUDED
#define C_APPLICATION_HPP_INCLUDED

#include <map>
#include <set>
#include "gg/application.hpp"
#include "gg/netmgr.hpp"
#include "gg/idman.hpp"
#include "tinythread.h"

namespace gg
{
    class c_application;

    class c_remote_application : public remote_application, public packet_handler, public connection_handler
    {
        mutable c_application* m_app;
        mutable tthread::recursive_mutex m_mutex;
        connection* m_conn;
        remote_application::connection_handler* m_conn_handler;
        volatile bool m_auth_ok;
        std::string m_name;
        var m_auth_data;
        std::map<typeinfo, request_handler*> m_req_handlers;
        mutable std::map<id, var> m_responses;
        bool m_remote_events;
        bool m_remote_exec;
        std::ostream* m_err;
        uint8_t m_packet_err;

    protected:
        bool send_var(const var& data) const;
        bool handle_request(var& data) const;
        bool wait_for_authentication(uint32_t timeout) const;

    public:
        c_remote_application(c_application*, std::string address, uint16_t port, var auth_data);
        c_remote_application(c_application*, connection*);
        ~c_remote_application();
        application* get_app() const;
        std::string get_name() const;
        std::string get_address() const;
        uint16_t get_port() const;
        const var& get_auth_data() const;
        bool connect();
        void disconnect();
        bool is_connected() const;
        void set_connection_handler(remote_application::connection_handler*);
        void set_connection_handler(std::function<void(remote_application*, bool is_connection)>);
        remote_application::connection_handler* get_connection_handler() const;
        void add_request_handler(typeinfo, request_handler*);
        void add_request_handler(typeinfo, std::function<bool(var&)>);
        void remove_request_handler(typeinfo);
        optional<var> send_request(var data, uint32_t timeout) const;
        void send_async_request(var data, uint32_t timeout, std::function<void(optional<var>)> callback) const;
        void push_event(event_type, event::attribute_list) const;
        optional<var> exec(std::string fn, varlist vl, std::ostream&) const;
        optional<var> parse_and_exec(std::string expr, std::ostream&) const;
        void enable_remote_events();
        void disable_remote_events();
        void enable_remote_exec();
        void disable_remote_exec();
        void set_error_stream(std::ostream&);

        // inherited from packet_handler
        void handle_packet(connection*);
        // inherited from connection_handler
        void handle_connection_open(connection*);
        void handle_connection_close(connection*);
    };

    class c_event_manager;
    class c_task_manager;
    class c_serializer;
    class c_script_engine;
    class c_network_manager;
    class c_id_manager;

    class c_application : public application, public connection_handler
    {
        static atomic<uint32_t> sm_inst_cnt;
        static void(*sm_init_cb)(void);
        mutable tthread::mutex m_mutex;
        std::string m_name;
        c_event_manager* m_eventmgr;
        c_task_manager* m_taskmgr;
        c_serializer* m_serializer;
        c_script_engine* m_scripteng;
        c_network_manager* m_netmgr;
        c_id_manager* m_idman;
        tthread::condition_variable m_cond;
        mutable tthread::mutex m_cond_mutex;
        int m_exit_code;
        std::map<listener*, authentication_handler*> m_ports;
        std::set<remote_application*> m_clients;

    protected:
        friend class c_remote_application;

        void add_client(remote_application*);
        void remove_client(remote_application*);
        authentication_handler* get_auth_handler(listener*);

    public:
        typedef void(*init_callback)(void);
        static void set_init_callback(init_callback);

        c_application(std::string name);
        ~c_application();

        std::string get_name() const;
        int start();
        void exit(int exit_code = 0);
        bool open_port(uint16_t port, authentication_handler*);
        bool open_port(uint16_t port, std::function<bool(remote_application*, const var&)>);
        void close_port(uint16_t port);
        void close_ports();
        remote_application* connect(std::string address, uint16_t port, var auth_data);
        enumerator<remote_application*> get_remote_applications();

        event_manager*   get_event_manager();
        task_manager*    get_task_manager();
        logger*          get_logger();
        serializer*      get_serializer();
        script_engine*   get_script_engine();
        network_manager* get_network_manager();
        id_manager*      get_id_manager();
        console*         create_console();
        console*         create_console(std::string name, std::string welcome_text = {});

        // inherited from connection_handler
        void handle_connection_open(connection*);
        void handle_connection_close(connection*);
    };
};

#endif // C_APPLICATION_HPP_INCLUDED
