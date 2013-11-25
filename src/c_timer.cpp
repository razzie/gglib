#include "c_timer.hpp"

using namespace gg;
using namespace std::chrono;


timer* timer::create()
{
    return new c_timer();
}

c_timer::c_timer()
{
    this->reset();
}

c_timer::~c_timer()
{
}

c_timer::c_timer(const c_timer& t)
 : m_start_time(t.m_start_time)
 , m_last_elapsed(t.m_last_elapsed)
{
}

uint32_t c_timer::get_elapsed()
{
    uint32_t elapsed_from_start =
        duration_cast<milliseconds>(steady_clock::now() - m_start_time).count();

    uint32_t elapsed_now = elapsed_from_start - m_last_elapsed;

    m_last_elapsed = elapsed_from_start;

    return elapsed_now;
}

uint32_t c_timer::peek_elapsed() const
{
    uint32_t elapsed_from_start =
        duration_cast<milliseconds>(steady_clock::now() - m_start_time).count();

    return elapsed_from_start - m_last_elapsed;
}

void c_timer::reset()
{
    m_start_time = steady_clock::now();
    m_last_elapsed = 0;
}
