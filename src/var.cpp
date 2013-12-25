#include <algorithm>
#include "gg/var.hpp"

using namespace gg;

template class std::vector<var>;


var::view::view(const var& var) : m_var(var)
{
}

var::view::view(const view& vw) : m_var(vw.m_var)
{
}

var::view::~view()
{
}

var::var()
{
}

var::var(const var& v)
{
    if (v.m_var != nullptr)
        m_var = v.m_var->clone();
}

var::var(var&& v)
{
    std::swap(m_var, v.m_var);
}

var::~var()
{
    if (m_var != nullptr)
        delete m_var;
}

var& var::operator= (const var& v)
{
    if (m_var != nullptr) { delete m_var; m_var = nullptr; }
    if (v.m_var != nullptr) m_var = v.m_var->clone();
    return *this;
}

var& var::operator= (var&& v)
{
    std::swap(m_var, v.m_var);
    return *this;
}

var::view var::to_stream() const
{
    return view(*this);
}

std::string var::to_string() const
{
    std::stringstream ss;
    ss << to_stream();
    return ss.str();
}

const std::type_info& var::get_type() const
{
    if (m_var != nullptr)
        return m_var->get_type();
    else
        return typeid(void);
}

bool var::is_empty() const
{
    return (m_var == nullptr);
}

void var::clear()
{
    if (m_var != nullptr)
    {
        delete m_var;
        m_var = nullptr;
    }
}

std::ostream& gg::operator<< (std::ostream& o, const gg::var::view& vw)
{
    if (vw.m_var.m_var == nullptr) o << "(empty)";
    else vw.m_var.m_var->extract_to(o);
    return o;
}

std::ostream& gg::operator<< (std::ostream& o, const varlist& vl)
{
    //if (vl.empty()) return o;
    auto it = vl.begin();

    o << "[" << (it++)->to_stream();
    std::for_each(it, vl.end(), [&](const var& v){ o << ", " << v.to_stream(); });
    o << "]";

    return o;
}
