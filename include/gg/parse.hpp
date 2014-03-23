#ifndef GG_PARSE_HPP_INCLUDED
#define GG_PARSE_HPP_INCLUDED

#include <iosfwd>
#include <sstream>
#include <string>
#include <tuple>
#include "gg/streamutil.hpp"
#include "gg/optional.hpp"

namespace gg
{
    template<class Arg>
    std::tuple<Arg> parse(std::istream& i, optional<char> d = {})
    {
        Arg a;
        if (d) i << delimiter(*d);
        if (!istream_extract(i, a)) throw std::runtime_error("can't extract arg");
        return std::tuple<Arg> { a };
    }

    template<class Arg1, class Arg2, class... Args>
    std::tuple<Arg1, Arg2, Args...> parse(std::istream& i, optional<char> d = {})
    {
        if (d) i << delimiter(*d);
        auto a = parse<Arg1>(i);
        auto b = parse<Arg2, Args...>(i);
        return std::tuple_cat(a,b);
    }

    template<class... Args>
    std::tuple<Args...> parse(std::string str, optional<char> d = {})
    {
        std::stringstream ss(str);
        if (d) ss << delimiter(*d);
        return parse<Args...>(ss);
    }
};

#endif // GG_PARSE_HPP_INCLUDED
