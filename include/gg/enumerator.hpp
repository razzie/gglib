#ifndef GG_ENUMERATOR_HPP_INCLUDED
#define GG_ENUMERATOR_HPP_INCLUDED

#include <iterator>
#include <functional>
#include <type_traits>
#include "gg/optional.hpp"

namespace gg
{
    template<class T>
    class enumerator
    {
        class enumerator_impl_base
        {
        public:
            virtual ~enumerator_impl_base() {}
            virtual void next() = 0;
            virtual bool has_next() const = 0;
            virtual void reset() = 0;
            virtual optional<T> get() = 0;
            virtual enumerator_impl_base* clone() const = 0;
        };

        template<class iterator>
        using compatible_iterator = typename std::enable_if<std::is_same<T, typename iterator::value_type>::value>::type;

        template<class iterator, class = compatible_iterator<iterator>>
        class enumerator_impl : public enumerator_impl_base
        {
            iterator m_begin;
            iterator m_end;
            iterator m_current;
            std::function<void(T&)> m_get_callback;

        public:
            enumerator_impl(iterator begin, iterator end, std::function<void(T&)> get_cb = {})
             : m_begin(begin), m_end(end), m_current(begin), m_get_callback(get_cb) {}
            enumerator_impl(const enumerator_impl& e)
             : m_begin(e.m_begin), m_end(e.m_end), m_current(e.m_current), m_get_callback(e.m_get_callback) {}
            ~enumerator_impl() {}
            enumerator_impl_base* clone() const { return new enumerator_impl<iterator>(*this); }
            void next() { if (has_next()) std::advance(m_current, 1); }
            bool has_next() const { return (m_current != m_end); }
            void reset() { m_current = m_begin; }
            optional<T> get()
            {
                if (!has_next()) return {};
                else if (m_get_callback) { m_get_callback(*m_current); return *m_current; }
                else { return *m_current; }
            }
        };

        enumerator_impl_base* m_enum;

    public:
        template<class iterator, class = compatible_iterator<iterator>>
        enumerator(iterator begin, iterator end, std::function<void(T&)> get_cb = {})
         : m_enum(new enumerator_impl<iterator>(begin, end, get_cb)) {}

        enumerator(const enumerator& e) : m_enum((e.m_enum == nullptr) ? nullptr : e.m_enum->clone()) {}
        enumerator(enumerator&& e) : m_enum(e.m_enum) { e.m_enum = nullptr; }
        ~enumerator() { if (m_enum != nullptr) delete m_enum; }

        enumerator& operator= (const enumerator& e)
        { if (m_enum != nullptr) delete m_enum; m_enum = (e.m_enum == nullptr) ? nullptr : e.m_enum->clone(); }
        enumerator& operator= (enumerator&& e)
        { if (m_enum != nullptr) delete m_enum; m_enum = e.m_enum; e.m_enum = nullptr; }

        void next() { if (m_enum != nullptr) m_enum->next(); }
        bool has_next() const { if (m_enum != nullptr) return m_enum->has_next(); else return false; }
        void reset() { if (m_enum != nullptr) m_enum->reset(); }
        optional<T> get() { if (m_enum != nullptr) return m_enum->get(); else return {}; }
    };
};

#endif // GG_ENUMERATOR_HPP_INCLUDED
