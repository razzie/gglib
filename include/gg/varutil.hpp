#ifndef GG_VARUTIL_HPP_INCLUDED
#define GG_VARUTIL_HPP_INCLUDED

#include "gg/var.hpp"

namespace gg
{
    typedef std::function<var(varlist)> dynamic_function;

    namespace util
    {
        /*
         * Call function with dynamic argument list
         */
        template<typename R, typename... Args>
        R call_function(std::function<R(Args...)> func, varlist vl);

        template<typename R>
        R call_function(std::function<R()> func, varlist vl)
        {
            if (vl.size() > 0)
                throw std::runtime_error("too long argument list");

            return func();
        }

        template<typename R, typename Arg0, typename... Args>
        R call_function(std::function<R(Arg0, Args...)> func, varlist vl)
        {
            if (vl.size() == 0)
                throw std::runtime_error("too short argument list");

            Arg0 arg0 = vl[0].cast<Arg0>();
            vl.erase(vl.begin());
            std::function<R(Args... args)> lambda =
                [=](Args... args) -> R { return func(arg0, args...); };

            return call_function(lambda, vl);
        }


        /*
         * make_dynamic_function
         */
        template<typename R, typename... Args>
        dynamic_function make_dynamic_function(std::function<R(Args...)> stdfunc)
        {
            dynamic_function result =
                [=](varlist vl) -> var
                {
                    return var(call_function(stdfunc, vl));
                };
            return result;
        }

        template<typename... Args>
        dynamic_function make_dynamic_function(std::function<void(Args...)> stdfunc)
        {
            dynamic_function result =
                [=](varlist vl) -> var
                {
                    call_function(stdfunc, vl);
                    return var();
                };
            return result;
        }

        template<typename R, typename... Args>
        dynamic_function make_dynamic_function(R(*func)(Args...))
        {
            std::function<R(Args...)> stdfunc = func;
            return make_dynamic_function(stdfunc);
        }

        template<typename F>
        dynamic_function make_dynamic_function(F&& f)
        {
            return make_dynamic_function(make_function(f));
        }
    };
};

#endif // GG_VARUTIL_HPP_INCLUDED
