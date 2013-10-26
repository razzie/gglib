#ifndef TIMER_HPP_INCLUDED
#define TIMER_HPP_INCLUDED

#include "gg/types.hpp"
#include "gg/refcounted.hpp"

namespace gg
{
    class timer : public reference_counted
    {
    public:
        virtual ~timer() {};
        virtual uint32_t get_elapsed() = 0;
        virtual uint32_t peek_elapsed() const = 0;
        virtual void reset() = 0;
    };
};

#endif // TIMER_HPP_INCLUDED
