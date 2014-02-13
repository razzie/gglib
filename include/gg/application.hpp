#ifndef GG_APPLICATION_HPP_INCLUDED
#define GG_APPLICATION_HPP_INCLUDED

#include <iostream>
#include <cstdint>
#include <string>
#include <functional>
#include "gg/var.hpp"
#include "gg/optional.hpp"
#include "gg/enumerator.hpp"
#include "gg/eventmgr.hpp"

namespace gg
{
    class application;

    class remote_application : public reference_counted
    {
    protected:
        ~remote_application() {}

    public:
        virtual application* get_app() const = 0;
        virtual bool connect() = 0;
        virtual void disconnect() = 0;
        virtual bool is_connected() const = 0;
        virtual std::string get_name() const = 0;
        virtual std::string get_address() const = 0;
        virtual uint16_t get_port() const = 0;
        virtual const var& get_auth_data() const = 0;
        virtual void push_event(event_type, event::attribute_list) = 0;
        virtual optional<var> exec(std::string fn, varlist vl, std::ostream& output) const = 0;
        virtual optional<var> parse_and_exec(std::string expr, std::ostream& output) const = 0;
    };

    class authentication_handler : public reference_counted
    {
    protected:
        ~authentication_handler() {}

    public:
        virtual bool authenticate(remote_application* app, const var& auth_data) = 0;
    };

    //class event_manager;
    class task_manager;
    class logger;
    class serializer;
    class script_engine;
    class network_manager;
    class id_manager;
    class console;

    class application : public reference_counted
    {
    protected:
        virtual ~application() {}

    public:
        static application* create(std::string name);

        virtual std::string get_name() const = 0;
        virtual int start() = 0;
        virtual void exit(int exit_code = 0) = 0;
        virtual bool open_port(uint16_t port, authentication_handler*) = 0;
        virtual bool open_port(uint16_t port, std::function<bool(remote_application*, const var&)>) = 0;
        virtual void close_port(uint16_t port) = 0;
        virtual void close_ports() = 0;
        virtual remote_application* connect(std::string address, uint16_t port, var auth_data) = 0;
        virtual enumerator<remote_application*> get_remote_applications() = 0;

        virtual event_manager*   get_event_manager() = 0;
        virtual task_manager*    get_task_manager() = 0;
        virtual logger*          get_logger() = 0;
        virtual serializer*      get_serializer() = 0;
        virtual script_engine*   get_script_engine() = 0;
        virtual network_manager* get_network_manager() = 0;
        virtual id_manager*      get_id_manager() = 0;
        virtual console*         create_console() = 0;
        virtual console*         create_console(std::string name, std::string welcome_text = {}) = 0;
    };
};

#endif // GG_APPLICATION_HPP_INCLUDED
