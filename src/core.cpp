#include <cxxabi.h>
#include <typeindex>
#include "tinythread.h"
#include "gg/core.hpp"

using namespace gg;

struct reference_counted::refcounted_data
{
    tthread::mutex m_mut;
    uint32_t m_ref = 1;
};

reference_counted::reference_counted()
{
    m_data = new refcounted_data();
}

reference_counted::~reference_counted()
{
    delete m_data;
}

void reference_counted::grab() const
{
    m_data->m_mut.lock();
    ++(m_data->m_ref);
    m_data->m_mut.unlock();
}

void reference_counted::drop() const
{
    m_data->m_mut.lock();
    if (--(m_data->m_ref) == 0)
        delete this;
    else
        m_data->m_mut.unlock();
}

uint32_t reference_counted::get_ref_count() const
{
    return m_data->m_ref;
}


typeinfo::typeinfo(const std::type_info& ti)
 : m_type(&ti)
{
}

typeinfo::typeinfo(const typeinfo& ti)
 : m_type(ti.m_type)
{
}

typeinfo& typeinfo::operator= (const typeinfo& ti)
{
    m_type = ti.m_type;
    return *this;
}

typeinfo::~typeinfo()
{
}

bool typeinfo::operator== (const typeinfo& ti) const
{
    return (*m_type == *ti.m_type);
}

bool typeinfo::operator!= (const typeinfo& ti) const
{
    return (*m_type != *ti.m_type);
}

bool typeinfo::operator< (const typeinfo& ti) const
{
    return (std::type_index(*m_type) < std::type_index(*ti.m_type));
}

bool typeinfo::operator<= (const typeinfo& ti) const
{
    return (std::type_index(*m_type) <= std::type_index(*ti.m_type));
}

bool typeinfo::operator> (const typeinfo& ti) const
{
    return (std::type_index(*m_type) > std::type_index(*ti.m_type));
}

bool typeinfo::operator>= (const typeinfo& ti) const
{
    return (std::type_index(*m_type) >= std::type_index(*ti.m_type));
}

std::string typeinfo::name() const
{
    return typeinfo::name_of(*m_type);
}

std::string typeinfo::name_of(const std::type_info& ti)
{
    std::string tname;
    char* buf = NULL;
    int status = 0;

    if (NULL != (buf = abi::__cxa_demangle(ti.name(), NULL, NULL, &status)))
    {
        tname = buf;
        free (buf);
    }

    return tname;
}

typeinfo::operator const std::type_info& () const
{
    return *m_type;
}


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

std::ostream& gg::operator<< (std::ostream& o, const gg::var::view& vw)
{
    if (vw.m_var.m_var == nullptr) o << "(empty)";
    else vw.m_var.m_var->extract_to(o);
    return o;
}

std::ostream& gg::operator<< (std::ostream& o, const varlist& vl)
{
    if (vl.empty()) return o;
    auto it = vl.begin();

    o << "[" << (it++)->to_stream();
    std::for_each(it, vl.end(), [&](const var& v){ o << ", " << v.to_stream(); });
    o << "]";

    return o;
}
