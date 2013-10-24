#ifndef C_TIMER_HPP_INCLUDED
#define C_TIMER_HPP_INCLUDED

#include <chrono>
#include "gg/timer.hpp"

namespace gg
{
    class c_timer : public timer
    {
        std::chrono::steady_clock::time_point m_start_time;
        uint32_t m_last_elapsed;

    public:
        c_timer();
        c_timer(const c_timer&);
        ~c_timer();
        uint32_t get_elapsed();
        uint32_t peek_elapsed() const;
        void reset();
    };
};

#endif // C_TIMER_HPP_INCLUDED
