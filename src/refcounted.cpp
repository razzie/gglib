#include "tinythread.h"
#include "gg/refcounted.hpp"

using namespace gg;


struct reference_counted::refcounted_data
{
    tthread::mutex m_mut;
    uint32_t m_ref = 1;
};


reference_counted::reference_counted()
{
    m_refdata = new refcounted_data();
}

reference_counted::~reference_counted()
{
    delete m_refdata;
}

void reference_counted::grab() const
{
    m_refdata->m_mut.lock();
    ++(m_refdata->m_ref);
    m_refdata->m_mut.unlock();
}

void reference_counted::drop() const
{
    m_refdata->m_mut.lock();
    if (--(m_refdata->m_ref) == 0)
        delete this;
    else
        m_refdata->m_mut.unlock();
}

uint32_t reference_counted::get_ref_count() const
{
    return m_refdata->m_ref;
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
