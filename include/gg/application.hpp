#ifndef GG_APPLICATION_HPP_INCLUDED
#define GG_APPLICATION_HPP_INCLUDED

namespace gg
{
    class event_manager;
    class task_manager;
    class logger;
    class serializer;
    class script_engine;
    class network_manager;
    class id_generator;
    class console;

    class application : public reference_counted
    {
    public:
        static application* create_instance(std::string name, uint32_t ver_major, uint32_t ver_minor);
        virtual ~application() {}

        virtual std::string get_name() const = 0;
        virtual uint32_t get_major_version() const = 0;
        virtual uint32_t get_minor_version() const = 0;

        virtual event_manager*   get_event_manager() = 0;
        virtual task_manager*    get_task_manager() = 0;
        virtual logger*          get_logger() = 0;
        virtual serializer*      get_serializer() = 0;
        virtual script_engine*   get_script_engine() = 0;
        virtual network_manager* get_network_manager() = 0;
        virtual id_generator*    get_id_generator() = 0;
        virtual console*         create_console() = 0;
        virtual console*         create_console(std::string name, std::string welcome_text = {}) = 0;

        virtual int  start() = 0;
        virtual void exit(int exit_code = 0) = 0;
    };
};

#endif // GG_APPLICATION_HPP_INCLUDED
