#ifndef GG_REFCOUNTED_HPP_INCLUDED
#define GG_REFCOUNTED_HPP_INCLUDED

#include <cstdint>
#include <type_traits>

namespace gg
{
    class reference_counted
    {
        struct refcounted_data;
        refcounted_data* m_data;

    public:
        reference_counted();
        virtual ~reference_counted();
        void grab() const;
        void drop() const;
        uint32_t get_ref_count() const;
    };

    template<class T>
    using reference_counted_type =
        typename std::enable_if<std::is_base_of<reference_counted, T>::value>::type;

    template<class T, class = reference_counted_type<T>>
    T* grab(T* o)
    {
        o->grab();
        return o;
    }

    template<class T, class = reference_counted_type<T>>
    const T* grab(const T* o)
    {
        o->grab();
        return o;
    }

    template<class T, class = reference_counted_type<T>>
    T* drop(T* o)
    {
        uint32_t refc = o->get_ref_count();
        o->drop();
        return (refc == 1) ? nullptr : o;
    }

    template<class T, class = reference_counted_type<T>>
    const T* drop(const T* o)
    {
        uint32_t refc = o->get_ref_count();
        o->drop();
        return (refc == 1) ? nullptr : o;
    }

    template<class T, class = reference_counted_type<T>>
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
