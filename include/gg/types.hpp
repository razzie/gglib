#ifndef TYPES_HPP_INCLUDED
#define TYPES_HPP_INCLUDED

#include <cstdint>
#include <memory>
#include <exception>

namespace gg
{
    class exception : public std::exception
    {
        const char* m_what;

    public:
        exception(const char* what) : m_what(what) {}
        virtual ~exception() noexcept = default;
        virtual const char* what() const noexcept { return m_what; }
    };

    class named_object
    {
    public:
        virtual ~named_object() {}
        virtual std::string get_name() const = 0;
    };

    class reference_counted
    {
        mutable uint32_t m_ref = 1;

    public:
        virtual ~reference_counted() {}
        void grab() const { m_ref++; }
        void drop() const { m_ref--; if (m_ref == 0) delete this; }
        uint32_t get_ref_count() const { return m_ref; }
    };

    template<class T>
    T* grab(const typename std::enable_if<std::is_base_of<reference_counted,T>::value>::type* o)
    {
        o->grab();
        return o;
    }

    template<class T>
    T* drop(const typename std::enable_if<std::is_base_of<reference_counted,T>::value>::type* o)
    {
        uint32_t refc = o->get_ref_count();
        o->drop();
        return (refc == 1) ? nullptr : o;
    }

    class auto_drop
    {
        const reference_counted* m_obj;

    public:
        auto_drop(const reference_counted* o) : m_obj(o) {}
        ~auto_drop() { m_obj->drop(); }

        template<class T, class = typename std::enable_if<std::is_base_of<reference_counted,T>::value>::type >
        operator const T*() const { return dynamic_cast<const T*>(m_obj); }
    };

    /*struct buffer
    {
        void* m_ptr;
        std::size_t m_len;
    };*/

    struct color
    {
        uint8_t R, G, B;
    };
};

#endif // TYPES_HPP_INCLUDED
