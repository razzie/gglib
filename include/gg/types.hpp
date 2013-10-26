#ifndef GG_TYPES_HPP_INCLUDED
#define GG_TYPES_HPP_INCLUDED

#include <cstdint>
#include <string>

namespace gg
{
    class named_object
    {
    public:
        virtual ~named_object() {}
        virtual std::string get_name() const = 0;
    };

    struct buffer
    {
        char* m_ptr;
        std::size_t m_len;
    };

    struct color
    {
        uint8_t R, G, B;
    };
};

#endif // GG_TYPES_HPP_INCLUDED
