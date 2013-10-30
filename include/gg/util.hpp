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
class var;
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
