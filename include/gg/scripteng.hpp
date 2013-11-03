#ifndef GG_SCRIPTENG_HPP_INCLUDED
#define GG_SCRIPTENG_HPP_INCLUDED

#include <iostream>
#include "gg/types.hpp"
#include "gg/var.hpp"
#include "gg/varutil.hpp"
#include "gg/console.hpp"

namespace gg
{
    class script_engine
    {
    protected:
        virtual ~script_engine() {}

    public:
        virtual void add_function(std::string fn, dynamic_function func) = 0;
        virtual void remove_function(std::string fn) = 0;

        template<typename R, typename... Args>
        void add_function(std::string fn, std::function<R(Args...)> func)
        {
            this->add_function(fn, util::make_dynamic_function(func));
        }

        template<typename R, typename... Args>
        void add_function(std::string fn, R(*func)(Args...))
        {
            this->add_function(fn, util::make_dynamic_function(func));
        }

        template<typename F>
        void add_function(std::string fn, F&& func)
        {
            this->add_function(fn, util::make_dynamic_function(func));
        }

        virtual bool exec(std::string fn, varlist vl, std::ostream& output = std::cout, var* ret = nullptr) const = 0;
        virtual bool exec(std::string fn, varlist vl, console::output& output, var* ret = nullptr) const = 0;
        virtual bool parse_and_exec(std::string expr, std::ostream& output = std::cout, var* ret = nullptr) const = 0;
        virtual bool parse_and_exec(std::string expr, console::output& output, var* ret = nullptr) const = 0;
    };
};

#endif // GG_SCRIPTENG_HPP_INCLUDED
