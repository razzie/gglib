#ifndef C_SCRIPTENG_HPP_INCLUDED
#define C_SCRIPTENG_HPP_INCLUDED

#include <map>
#include "gg/scripteng.hpp"
#include "tinythread.h"

namespace gg
{
    class c_script_engine : public script_engine
    {
        class console_controller : public console::controller
        {
            c_script_engine* m_scripteng;

        public:
            console_controller(c_script_engine*);
            ~console_controller();
            bool exec(std::string& cmd, console::output&);
            void complete(std::string& cmd, console::output&);
        };

        tthread::mutex m_mutex;
        std::map<std::string, dynamic_function> m_commands;
        console_controller* m_ctrl;

    public:
        c_script_engine();
        ~c_script_engine();
        void add_function(std::string cmd, dynamic_function func);
        void remove_function(std::string cmd);
        var exec(std::string cmd, varlist vl, std::ostream& output = std::cout) const;
        var exec(std::string cmd, varlist vl, console::output& output) const;
        var parse_and_exec(std::string cmd_line, std::ostream& output = std::cout) const;
        var parse_and_exec(std::string cmd_line, console::output& output) const;
        console::controller* get_console_controller();

    private:
        bool is_valid_cmd_name(std::string cmd) const;
        //std::pair<std::string, dynamic_function>* find_command(std::string cmd);
    };
};

#endif // C_SCRIPTENG_HPP_INCLUDED
