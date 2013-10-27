#ifndef GG_TYPES_HPP_INCLUDED
#define GG_TYPES_HPP_INCLUDED

#include <iostream>
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

    #define DEBUG_FUNCTION_SCOPE function_scope __fs( \
                std::string(typeid(*this).name()) + "::" + __FUNCTION__);

    class function_scope
    {
        std::string m_fname;

    public:
        function_scope(std::string fn) : m_fname(fn)
        {
            std::cout << m_fname << "::begin" << std::endl;
        }
        ~function_scope()
        {
            std::cout << m_fname << "::end" << std::endl;
        }
    };
};

#endif // GG_TYPES_HPP_INCLUDED
