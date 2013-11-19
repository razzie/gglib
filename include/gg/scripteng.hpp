#ifndef GG_SCRIPTENG_HPP_INCLUDED
#define GG_SCRIPTENG_HPP_INCLUDED

#include "gg/core.hpp"
#include "gg/util.hpp"
#include "gg/console.hpp"

namespace gg
{
    class application;

    class script_engine
    {
    protected:
        virtual ~script_engine() {}

        template<typename T, typename _T = typename std::decay<T>::type>
        nulltype get_arg( std::string& args )
        {
            if (std::is_same<_T, varlist>::value) args.insert(0, ",( )");
            else if (std::is_arithmetic<_T>::value) args.insert(0, ",0");
            else if (!std::is_arithmetic<_T>::value) args.insert(0, ",\"\"");
            return {};
        }

        template<typename... Args>
        std::string get_args()
        {
            std::string args;

            struct { void operator() (...) {} } expand;
            expand( get_arg<Args>(args)... );

            args.erase(args.begin());
            args.insert(args.begin(), '(');
            args.insert(args.end(), ')');

            return args;
        }

    public:
        virtual application* get_app() const = 0;
        virtual void add_function(std::string fn, util::dynamic_function func, std::string args, bool hidden = false) = 0;
        virtual void remove_function(std::string fn) = 0;
        virtual optional<var> exec(std::string fn, varlist vl, std::ostream& output = std::cout) const = 0;
        virtual optional<var> parse_and_exec(std::string expr, std::ostream& output = std::cout) const = 0;
        virtual console::controller* get_console_controller() const = 0;

        template<typename R, typename... Args>
        void add_function(std::string fn, std::function<R(Args...)> func, bool hidden = false)
        {
            this->add_function(fn, util::make_dynamic_function(func), get_args<Args...>(), hidden);
        }

        template<typename R, typename... Args>
        void add_function(std::string fn, R(*func)(Args...), bool hidden = false)
        {
            this->add_function(fn, util::make_dynamic_function(func), get_args<Args...>(), hidden);
        }

        template<typename F>
        void add_function(std::string fn, F&& func, bool hidden = false)
        {
            this->add_function(fn, util::make_dynamic_function(func), get_args<meta::get_signature<F>>(), hidden);
        }
    };
};

#endif // GG_SCRIPTENG_HPP_INCLUDED
