#ifndef C_SCRIPTENG_HPP_INCLUDED
#define C_SCRIPTENG_HPP_INCLUDED

#include <map>
#include <vector>
#include <initializer_list>
#include "gg/scripteng.hpp"
#include "gg/console.hpp"

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
        std::map<std::string, std::vector<arg_type>> m_commands;
        console_controller* m_ctrl;

    public:
        c_script_engine();
        ~c_script_engine();
        void add_command(std::string cmd, std::initializer_list<arg_type> args);
        void remove_command(std::string cmd);
        bool exec(std::string cmd, std::initializer_list<arg> args) const;
        bool parse_and_exec(std::string cmd_line) const;
        console_controller* get_console_controller();
    };
};

#endif // C_SCRIPTENG_HPP_INCLUDED
