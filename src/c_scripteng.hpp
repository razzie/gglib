#ifndef C_SCRIPTENG_HPP_INCLUDED
#define C_SCRIPTENG_HPP_INCLUDED

#include "gg/scripteng.hpp"
#include "expression.hpp"
#include "tinythread.h"

namespace gg
{
    class c_script_engine : public script_engine
    {
        class console_controller : public console::controller
        {
            const c_script_engine* m_scripteng;

        public:
            console_controller(const c_script_engine*);
            ~console_controller();
            exec_result exec(std::string& expr, console::output&);
            void complete(std::string& expr, console::output&);
        };

        struct function_container
        {
            util::dynamic_function m_func;
            expression m_sign;
            bool m_is_hidden;
        };

        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::map<std::string, function_container> m_functions;
        console_controller* m_ctrl;

    public:
        c_script_engine(application* app);
        ~c_script_engine();
        application* get_app() const;
        void add_function(std::string fn, util::dynamic_function func, std::string args, bool hidden = false);
        void remove_function(std::string fn);
        optional<var> exec(std::string fn, varlist vl, std::ostream& output = std::cout) const;
        optional<var> parse_and_exec(std::string expr, std::ostream& output = std::cout) const;
        console::controller* get_console_controller() const;

    private:
        bool is_valid_fn_name(std::string fn) const;
        std::vector<std::string> find_matching_functions(std::string fn) const;
        bool auto_complete(std::string& fn, bool print = false) const;
        bool auto_complete(std::string& fn, std::vector<std::string> matches, bool print = false) const;
        void auto_complete_expr(std::string& expr, bool print = false) const;
        optional<var> process_expression(const expression& e) const;
    };
};

#endif // C_SCRIPTENG_HPP_INCLUDED
