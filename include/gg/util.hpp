#ifndef GG_UTIL_HPP_INCLUDED
#define GG_UTIL_HPP_INCLUDED

#include <iostream>
#include <sstream>
#include <locale>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include "gg/types.hpp"

namespace gg
{
namespace util
{
    /*
     * string & wstring helpers
     */
    std::string trim(std::string, std::locale = std::locale());
    std::wstring trim(std::wstring, std::locale = std::locale());

    std::string narrow(std::wstring, std::locale = std::locale());
    std::wstring widen(std::string, std::locale = std::locale());

    bool is_integer(std::string);
    bool is_float(std::string);


    /*
     * remove class of member function pointer
     */
    template<typename T>
    struct remove_class { };

    template<typename C, typename R, typename... Args>
    struct remove_class<R(C::*)(Args...)> { using type = R(Args...); };

    template<typename C, typename R, typename... Args>
    struct remove_class<R(C::*)(Args...) const> { using type = R(Args...); };

    template<typename C, typename R, typename... Args>
    struct remove_class<R(C::*)(Args...) volatile> { using type = R(Args...); };

    template<typename C, typename R, typename... Args>
    struct remove_class<R(C::*)(Args...) const volatile> { using type = R(Args...); };


    /*
     * get signature of lambda
     */
    template<typename T>
    struct get_signature_impl { using type = typename remove_class<
        decltype(&std::remove_reference<T>::type::operator())>::type; };

    template<typename R, typename... Args>
    struct get_signature_impl<R(Args...)> { using type = R(Args...); };

    template<typename R, typename... Args>
    struct get_signature_impl<R(&)(Args...)> { using type = R(Args...); };

    template<typename R, typename... Args>
    struct get_signature_impl<R(*)(Args...)> { using type = R(Args...); };

    template<typename T>
    using get_signature = typename get_signature_impl<T>::type;


    /*
     * make_function
     */
    template<typename F>
    std::function<get_signature<F>> make_function(F &&f)
    {
        return std::function<get_signature<F>>( std::forward<F>(f) );
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

    /*
     * SFINAE helpers
     */
    template<class T> T& lvalue_of_type();
    template<class T> T  rvalue_of_type();

    namespace sfinae
    {
        class yes { char c[1]; };
        class no  { char c[2]; };
    }


    /*
     * ostream insertion helpers
     */
    template<class T>
    struct has_insert_op
    {
        template<class U>
        static sfinae::yes test(char(*)[sizeof(
            lvalue_of_type<std::ostream>() << rvalue_of_type<U>()
        )]);

        template<class U>
        static sfinae::no test(...);

        enum { value = ( sizeof(sfinae::yes) == sizeof(test<T>(0)) ) };
        typedef std::integral_constant<bool, value> type;
    };

    template<class T>
    void ostream_insert(std::ostream& o, const T& t,
        typename std::enable_if<has_insert_op<T>::value>::type* = 0)
    {
        o << t;
    }

    template<class T>
    void ostream_insert(std::ostream& o, const T& t,
        typename std::enable_if<!has_insert_op<T>::value>::type* = 0)
    {
        o << "???";
    }


    /*
     * istream extraction helpers
     */
    template<class T>
    struct has_extract_op
    {
        template<class U>
        static sfinae::yes test(char(*)[sizeof(
            lvalue_of_type<std::istream>() >> rvalue_of_type<U>()
        )]);

        template<class U>
        static sfinae::no test(...);

        enum { value = ( sizeof(sfinae::yes) == sizeof(test<T>(0)) ) };
        typedef std::integral_constant<bool, value> type;
    };

    template<class T>
    void istream_extract(std::istream& o, T& t,
        typename std::enable_if<has_extract_op<T>::value>::type* = 0)
    {
        o >> t;
    }

    template<class T>
    void istream_extract(std::istream& o, T& t,
        typename std::enable_if<!has_extract_op<T>::value>::type* = 0)
    {
    }


    /*
     * cast utilities
     */
    template<class From, class To>
    typename std::enable_if<std::is_convertible<From, To>::value, To>::type
    cast(const From& from)
    {
        return To(from);
    }

    template<class From, class To>
    typename std::enable_if<!std::is_convertible<From, To>::value, To>::type
    cast(const From& from)
    {
        if (!has_insert_op<From>::value
            || !has_extract_op<To>::value)
        {
            throw std::runtime_error("unable to cast");
        }

        To result;
        std::stringstream ss;

        ostream_insert(ss, from);
        istream_extract(ss, result);

        return result;
    }
};
};

#endif // GG_UTIL_HPP_INCLUDED
