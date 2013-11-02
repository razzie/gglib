#ifndef C_SCRIPTENG_HPP_INCLUDED
#define C_SCRIPTENG_HPP_INCLUDED

#include <map>
#include <vector>
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

        mutable tthread::mutex m_mutex;
        std::map<std::string, dynamic_function> m_functions;
        console_controller* m_ctrl;

    public:
        c_script_engine();
        ~c_script_engine();
        void add_function(std::string cmd, dynamic_function func);
        void remove_function(std::string cmd);
        bool exec(std::string cmd, varlist vl, std::ostream& output = std::cout, var* ret = nullptr) const;
        bool exec(std::string cmd, varlist vl, console::output& output, var* ret = nullptr) const;
        bool parse_and_exec(std::string expr, std::ostream& output = std::cout, var* ret = nullptr) const;
        bool parse_and_exec(std::string expr, console::output& output, var* ret = nullptr) const;
        console::controller* get_console_controller();

    private:
        bool is_valid_cmd_name(std::string cmd) const;
        std::vector<std::string> find_matching_commands(std::string cmd) const;
        void auto_complete(std::string& cmd) const;
        void auto_complete(std::string& cmd, std::vector<std::string> matches) const;
    };
};

#endif // C_SCRIPTENG_HPP_INCLUDED
