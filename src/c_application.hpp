#ifndef C_APPLICATION_HPP_INCLUDED
#define C_APPLICATION_HPP_INCLUDED

#include "gg/application.hpp"
#include "tinythread.h"

namespace gg
{
    class c_event_manager;
    class c_task_manager;
    class c_serializer;
    class c_script_engine;
    class c_network_manager;
    class c_id_manager;

    class c_application : public application
    {
        std::string m_name;
        uint32_t m_ver_major;
        uint32_t m_ver_minor;
        c_event_manager* m_eventmgr;
        c_task_manager* m_taskmgr;
        c_serializer* m_serializer;
        c_script_engine* m_scripteng;
        c_network_manager* m_netmgr;
        c_id_manager* m_idman;
        tthread::condition_variable m_cond;
        mutable tthread::mutex m_cond_mutex;
        int m_exit_code;

    public:
        c_application(std::string name, uint32_t ver_major, uint32_t ver_minor);
        ~c_application();

        std::string get_name() const;
        uint32_t get_major_version() const;
        uint32_t get_minor_version() const;

        event_manager*   get_event_manager();
        task_manager*    get_task_manager();
        logger*          get_logger();
        serializer*      get_serializer();
        script_engine*   get_script_engine();
        network_manager* get_network_manager();
        id_manager*      get_id_manager();
        console*         create_console();
        console*         create_console(std::string name, std::string welcome_text = {});

        int  start();
        void exit(int exit_code = 0);
    };
};

#endif // C_APPLICATION_HPP_INCLUDED
