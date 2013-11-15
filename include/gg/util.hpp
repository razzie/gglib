#ifndef GG_UTIL_HPP_INCLUDED
#define GG_UTIL_HPP_INCLUDED

#include "gg/core.hpp"

namespace gg
{
namespace util
{
    std::string trim(std::string, std::locale = std::locale());
    std::wstring trim(std::wstring, std::locale = std::locale());

    std::string narrow(std::wstring, std::locale = std::locale());
    std::wstring widen(std::string, std::locale = std::locale());

    bool is_integer(std::string);
    bool is_float(std::string);
    bool is_numeric(std::string);
    bool contains_space(std::string);


    class on_return
    {
        std::function<void()> m_func;

    public:
        on_return(std::function<void()> func) : m_func(func) {}
        on_return(const on_return&) = delete;
        on_return(on_return&&) = delete;
        ~on_return() { m_func(); }
    };


    template<typename F>
    std::function<meta::get_signature<F>> make_function(F &&f)
    {
        return std::function<meta::get_signature<F>>( std::forward<F>(f) );
    }

    template<typename R>
    std::function<R()> make_function(R(*func)())
    {
        return std::function<R()> (func);
    }

    template<typename R, typename... Args>
    std::function<R(Args...)> make_function(R(*func)(Args...))
    {
        return std::function<R(Args...)> (func);
    }

    typedef std::function<var(varlist)> dynamic_function;

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

#endif // GG_UTIL_HPP_INCLUDED
