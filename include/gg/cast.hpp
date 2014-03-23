#ifndef GG_CAST_HPP_INCLUDED
#define GG_CAST_HPP_INCLUDED

#include <type_traits>
#include <stdexcept>
#include "gg/streamutil.hpp"

namespace gg
{
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
        if (!meta::has_insert_op<From>::value
            || !meta::has_extract_op<To>::value)
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

#endif // GG_CAST_HPP_INCLUDED
