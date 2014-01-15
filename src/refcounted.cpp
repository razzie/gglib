#include "tinythread.h"
#include "gg/refcounted.hpp"

using namespace gg;


reference_counted::reference_counted()
 : m_ref_count(1)
{
}

reference_counted::~reference_counted()
{
}

void reference_counted::grab() const
{
    ++m_ref_count;
}

void reference_counted::drop() const
{
    if (--m_ref_count == 0)
        delete this;
}

uint32_t reference_counted::get_ref_count() const
{
    return m_ref_count;
}


grab_guard::grab_guard(const reference_counted* obj)
 : m_obj(obj)
{
    m_obj->grab();
}

grab_guard::~grab_guard()
{
    m_obj->drop();
}
