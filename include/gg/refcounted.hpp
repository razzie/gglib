#ifndef GG_REFCOUNTED_HPP_INCLUDED
#define GG_REFCOUNTED_HPP_INCLUDED

#include "tinythread.h"
#include "gg/types.hpp"

namespace gg
{
    class reference_counted
    {
        mutable tthread::mutex m_mut;
        mutable uint32_t m_ref = 1;

    public:
        virtual ~reference_counted() {}
        void grab() const { m_mut.lock(); ++m_ref; m_mut.unlock(); }
        void drop() const { m_mut.lock(); if (--m_ref == 0) delete this; else m_mut.unlock(); }
        uint32_t get_ref_count() const { return m_ref; }
    };

    template<class T>
    typename std::enable_if<std::is_base_of<reference_counted,T>::value>::type*
    grab(T* o)
    {
        o->grab();
        return o;
    }

    template<class T>
    const typename std::enable_if<std::is_base_of<reference_counted,T>::value>::type*
    grab(const T* o)
    {
        o->grab();
        return o;
    }

    template<class T>
    typename std::enable_if<std::is_base_of<reference_counted,T>::value>::type*
    drop(T* o)
    {
        uint32_t refc = o->get_ref_count();
        o->drop();
        return (refc == 1) ? nullptr : o;
    }

    template<class T>
    const typename std::enable_if<std::is_base_of<reference_counted,T>::value>::type*
    drop(const T* o)
    {
        uint32_t refc = o->get_ref_count();
        o->drop();
        return (refc == 1) ? nullptr : o;
    }

    template<class T,
             class = typename std::enable_if<std::is_base_of<reference_counted, T>::value>::type >
    class auto_drop
    {
        T* m_obj;

    public:
        auto_drop(T* o) : m_obj(o) {}
        ~auto_drop() { m_obj->drop(); }
        operator T*() { return m_obj; }
    };
};

#endif // GG_REFCOUNTED_HPP_INCLUDED
