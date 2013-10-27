#ifndef C_SCRIPTENG_HPP_INCLUDED
#define C_SCRIPTENG_HPP_INCLUDED

#include <map>
#include "gg/scripteng.hpp"
#include "gg/console.hpp"
#include "tinythread.h"

namespace gg
{
    class c_script_engine : public script_engine
    {
    public:
        class console_controller : public console::controller
        {
            c_script_engine* m_scripteng;

        public:
            console_controller(c_script_engine*);
            ~console_controller();
            bool exec(std::string& cmd, console::output&);
            void complete(std::string& cmd, console::output&);
        };

    private:
        struct command
        {
            std::string m_name;
            std::vector<arg::type> m_args;
            callback m_cb;
        };

    private:
        tthread::mutex m_mutex;
        std::map<std::string, command> m_commands;
        console_controller* m_ctrl;

    public:
        c_script_engine();
        ~c_script_engine();
        void add_command(std::string cmd,
                         std::vector<arg::type> args,
                         callback cb);
        void remove_command(std::string cmd);
        bool exec(std::string cmd,
                  std::vector<arg> args,
                  std::ostream& output = std::cout) const;
        bool parse_and_exec(std::string cmd_line,
                            std::ostream& output = std::cout) const;
        console_controller* get_console_controller();

    private:
        bool is_valid_cmd_name(std::string cmd) const;
        command* find_command(std::string cmd);
    };
};

#endif // C_SCRIPTENG_HPP_INCLUDED
