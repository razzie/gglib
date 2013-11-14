#ifndef GG_SCRIPTENG_HPP_INCLUDED
#define GG_SCRIPTENG_HPP_INCLUDED

#include <iostream>
#include "gg/types.hpp"
#include "gg/optional.hpp"
#include "gg/var.hpp"
#include "gg/varutil.hpp"
#include "gg/console.hpp"

namespace gg
{
    class script_engine
    {
    protected:
        virtual ~script_engine() {}

        template<typename T, typename _T = typename std::decay<T>::type>
        nulltype get_arg( std::string& args,
            typename std::enable_if<std::is_arithmetic<_T>::value>::type* = 0 )
        {
            std::cerr << "NUM: " << typeid(T).name() << std::endl;
            args.insert(0, ", 0");
            return {};
        }

        template<typename T, typename _T = typename std::decay<T>::type>
        nulltype get_arg( std::string& args,
            typename std::enable_if<!std::is_arithmetic<_T>::value
             && !std::is_same<_T, varlist>::value>::type* = 0 )
        {
            std::cerr << "STR: " << typeid(T).name() << std::endl;
            args.insert(0, ", \"  \"");
            return {};
        }

        template<typename T, typename _T = typename std::decay<T>::type>
        nulltype get_arg( std::string& args,
            typename std::enable_if<std::is_same<_T, varlist>::value>::type* = 0 )
        {
            std::cerr << "LIST: " << typeid(T).name() << std::endl;
            args.insert(0, ", (  )");
            return {};
        }

        template<typename... Args>
        std::string get_args()
        {
            std::cerr << "GETTING ARGS" << std::endl;
            std::string args;

            struct { void operator() (...) {} } expand;
            expand( get_arg<Args>(args)... );

            args.erase(0, 2);
            args.insert(args.begin(), '(');
            args.insert(args.end(), ')');

            return args;
        }

    public:
        virtual void add_function(std::string fn, dynamic_function func, std::string args) = 0;
        virtual void remove_function(std::string fn) = 0;

        template<typename R, typename... Args>
        void add_function(std::string fn, std::function<R(Args...)> func)
        {
            this->add_function(fn, util::make_dynamic_function(func), get_args<Args...>());
        }

        template<typename R, typename... Args>
        void add_function(std::string fn, R(*func)(Args...))
        {
            this->add_function(fn, util::make_dynamic_function(func), get_args<Args...>());
        }

        template<typename F>
        void add_function(std::string fn, F&& func)
        {
            this->add_function(fn, util::make_dynamic_function(func), get_args<util::get_signature<F>>());
        }

        virtual optional<var> exec(std::string fn, varlist vl, std::ostream& output = std::cout) const = 0;
        virtual optional<var> parse_and_exec(std::string expr, std::ostream& output = std::cout) const = 0;
        virtual console::controller* get_console_controller() const = 0;
        static console* get_invoker_console();
    };
};

#endif // GG_SCRIPTENG_HPP_INCLUDED
