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
