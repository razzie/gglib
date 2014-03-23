#include "scope_callback.hpp"

using namespace gg;


scope_callback::scope_callback()
{
}

scope_callback::scope_callback(std::function<void()> func)
 : m_func(func)
{
}

scope_callback::~scope_callback()
{
    if (m_func) m_func();
}

scope_callback& scope_callback::operator= (std::function<void()> func)
{
    m_func = func; return *this;
}

void scope_callback::reset()
{
    m_func = nullptr;
}
