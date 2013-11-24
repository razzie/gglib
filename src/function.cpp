#include "gg/function.hpp"

using namespace gg;

dynamic_function::dynamic_function()
{
}

dynamic_function::dynamic_function(const dynamic_function& func)
 : m_func(func.m_func)
{
}

dynamic_function::dynamic_function(dynamic_function&& func)
 : m_func(std::move(func.m_func))
{
}

dynamic_function::~dynamic_function()
{
}

dynamic_function& dynamic_function::operator= (const dynamic_function& func)
{
    m_func = func.m_func;
    return *this;
}

dynamic_function& dynamic_function::operator= (dynamic_function&& func)
{
    m_func = std::move(func.m_func);
    return *this;
}

var dynamic_function::operator() (varlist vl) const
{
    return m_func(vl);
}

dynamic_function::operator bool() const
{
    return static_cast<bool>(m_func);
}

dynamic_function::operator gg::function<var(varlist)>() const
{
    return m_func;
}

dynamic_function::operator std::function<var(varlist)>() const
{
    return m_func;
}
