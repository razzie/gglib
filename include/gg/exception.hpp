#ifndef GG_EXCEPTION_HPP_INCLUDED
#define GG_EXCEPTION_HPP_INCLUDED

#include <exception>
#include "gg/types.hpp"

namespace gg
{
    class exception : public std::exception
    {
        const char* m_what;

    public:
        exception(const char* what) : m_what(what) {}
        virtual ~exception() noexcept = default;
        virtual const char* what() const noexcept { return m_what; }
    };
};

#endif // GG_EXCEPTION_HPP_INCLUDED
