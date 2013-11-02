#ifndef GG_SCRIPTENG_HPP_INCLUDED
#define GG_SCRIPTENG_HPP_INCLUDED

#include <iostream>
#include <vector>
#include "gg/types.hpp"
#include "gg/var.hpp"
#include "gg/console.hpp"

namespace gg
{
    class script_engine
    {
    protected:
        virtual ~script_engine() {}

    public:
        virtual void add_function(std::string cmd, dynamic_function func) = 0;
        virtual void remove_function(std::string cmd) = 0;

        template<typename R, typename... Args>
        void add_function(std::string cmd, std::function<R(Args...)> func)
        {
            this->add_function(cmd, util::make_dynamic_function(func));
        }

        template<typename R, typename... Args>
        void add_function(std::string cmd, R(*func)(Args...))
        {
            this->add_function(cmd, util::make_dynamic_function(func));
        }

        template<typename F>
        void add_function(std::string cmd, F&& func)
        {
            this->add_function(cmd, util::make_dynamic_function(func));
        }

        virtual var exec(std::string cmd, varlist vl, std::ostream& output = std::cout) const = 0;
        virtual var exec(std::string cmd, varlist vl, console::output& output) const = 0;
        virtual var parse_and_exec(std::string cmd_line, std::ostream& output = std::cout) const = 0;
        virtual var parse_and_exec(std::string cmd_line, console::output& output) const = 0;
    };
};

#endif // GG_SCRIPTENG_HPP_INCLUDED
