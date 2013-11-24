#ifndef GG_MISC_HPP_INCLUDED
#define GG_MISC_HPP_INCLUDED

#include <cstdint>
#include <type_traits>

namespace gg
{
    struct nulltype {};

    namespace meta
    {
        template<bool B, class T = void>
        using enable_if_t = typename std::enable_if<B,T>::type;
    };

    struct color
    {
        uint8_t R, G, B;
    };
};

#endif // GG_MISC_HPP_INCLUDED
