#include <cxxabi.h>
#include <stdlib.h>
#include "gg/typeinfo.hpp"

using namespace gg;

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
    return m_type->before(*ti.m_type);
}

bool typeinfo::operator<= (const typeinfo& ti) const
{
    return (*this < ti || *this == ti);
}

bool typeinfo::operator> (const typeinfo& ti) const
{
    return (ti.m_type->before(*m_type));
}

bool typeinfo::operator>= (const typeinfo& ti) const
{
    return (*this > ti || *this == ti);
}

std::string typeinfo::name() const
{
    return typeinfo::name_of(*m_type);
}

typeinfo::operator const std::type_info& () const
{
    return *m_type;
}

size_t typeinfo::hash_code() const noexcept
{
    return m_type->hash_code();
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
