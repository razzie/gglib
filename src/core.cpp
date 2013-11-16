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

    if (abi::__cxa_demangle(ti.name(), buf, 0, &status))
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
