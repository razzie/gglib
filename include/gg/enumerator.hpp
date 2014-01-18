#ifndef GG_ENUMERATOR_HPP_INCLUDED
#define GG_ENUMERATOR_HPP_INCLUDED

#include <functional>
#include <type_traits>
#include <stdexcept>
#include "gg/optional.hpp"

namespace gg
{
    template<class T>
    class const_enumerator
    {
    protected:
        class enumerator_impl_base
        {
        public:
            virtual ~enumerator_impl_base() {}
            virtual void next() = 0;
            virtual void advance(size_t) = 0;
            virtual bool is_finished() const = 0;
            virtual void reset() = 0;
            virtual void erase() = 0;
            virtual void insert(const T&) = 0;
            virtual void insert(T&&) = 0;
            virtual size_t count() const = 0;
            virtual optional<T> get() = 0;
            virtual optional<T> get() const = 0;
            virtual enumerator_impl_base* clone() const = 0;
        };

        template<class iterator> using compatible_iterator =
            typename std::enable_if<std::is_same<T, typename iterator::value_type>::value>::type;

        template<class iterator,
                 class = compatible_iterator<iterator>>
        class enumerator_impl : public enumerator_impl_base
        {
            iterator m_begin;
            iterator m_end;
            iterator m_current;

        public:
            enumerator_impl(iterator begin, iterator end) : m_begin(begin), m_end(end), m_current(begin) {}
            enumerator_impl(const enumerator_impl& e) : m_begin(e.m_begin), m_end(e.m_end), m_current(e.m_current) {}
            ~enumerator_impl() {}
            enumerator_impl_base* clone() const { return new enumerator_impl(*this); }
            void next() { std::advance(m_current, 1); }
            void advance(size_t n) { std::advance(m_current, n); }
            bool is_finished() const { return (m_current == m_end); }
            void reset() { m_current = m_begin; }
            void erase() { throw std::runtime_error("erase error"); }
            void insert(const T&) { throw std::runtime_error("insert error"); }
            void insert(T&&) { throw std::runtime_error("insert error"); }
            size_t count() const { return std::distance(m_begin, m_end); }
            optional<T> get() { if (!is_finished()) return *m_current; else return {}; }
            optional<T> get() const { if (!is_finished()) return *m_current; else return {}; }
        };

        template<class container> using compatible_container =
            typename std::enable_if<std::is_same<T, typename container::value_type>::value>::type;

        template<class container> using const_container =
            typename std::enable_if<std::is_const<container>::value>::type;

        template<class container> using non_const_container =
            typename std::enable_if<!std::is_const<container>::value>::type;

        template<class container, class iterator = typename container::iterator,
                 class = compatible_container<container>,
                 class = compatible_iterator<iterator>>
        class container_enumerator_impl : public enumerator_impl_base
        {
            container* m_cont;
            iterator m_current;
            iterator m_next;

        public:
            container_enumerator_impl(container& c) : m_cont(&c), m_current(m_cont->begin()), m_next(std::next(m_current, 1)) {}
            container_enumerator_impl(const container_enumerator_impl& c) : m_cont(c.m_cont), m_current(c.m_current), m_next(c.m_next) {}
            ~container_enumerator_impl() {}
            enumerator_impl_base* clone() const { return new container_enumerator_impl(*this); }
            void next() { if (!is_finished()) { m_current = m_next; std::advance(m_next, 1); } }
            void advance(size_t n) { std::advance(m_current, n); m_next = std::next(m_current, 1); }
            bool is_finished() const { return (m_current == m_cont->end()); }
            void reset() { m_current = m_cont->begin(); m_next = std::next(m_current, 1); }
            void erase() { if (!is_finished()) { m_current = m_next = m_cont->erase(m_current); } }
            void insert(const T& t) { m_next = m_cont->insert(m_next, t); }
            void insert(T&& t) { m_next = m_cont->insert(m_next, t); }
            size_t count() const { return m_cont->size(); }
            optional<T> get() { if (!is_finished() && m_current != m_next) return *m_current; else return {}; }
            optional<T> get() const { if (!is_finished() && m_current != m_next) return *m_current; else return {}; }
        };

        template<class container, class iterator = typename container::const_iterator,
                 class = compatible_container<container>,
                 class = compatible_iterator<iterator>>
        class const_container_enumerator_impl : public enumerator_impl_base
        {
            container* m_cont;
            iterator m_current;

        public:
            const_container_enumerator_impl(container& c) : m_cont(&c), m_current(m_cont->begin()) {}
            const_container_enumerator_impl(const const_container_enumerator_impl& c) : m_cont(c.m_cont), m_current(c.m_current) {}
            ~const_container_enumerator_impl() {}
            enumerator_impl_base* clone() const { return new const_container_enumerator_impl(*this); }
            void next() { if (!is_finished()) { std::advance(m_current, 1); } }
            void advance(size_t n) { std::advance(m_current, n); }
            bool is_finished() const { return (m_current == m_cont->end()); }
            void reset() { m_current = m_cont->begin(); }
            void erase() { throw std::runtime_error("erase error"); }
            void insert(const T&) { throw std::runtime_error("insert error"); }
            void insert(T&&) { throw std::runtime_error("insert error"); }
            size_t count() const { return m_cont->size(); }
            optional<T> get() { if (!is_finished()) return *m_current; else return {}; }
            optional<T> get() const { if (!is_finished()) return *m_current; else return {}; }
        };

        enumerator_impl_base* m_enum;

    public:
        template<class iterator>
        const_enumerator(iterator begin, iterator end) : m_enum(new enumerator_impl<iterator>(begin, end)) {}

        template<class container>
        const_enumerator(container& cont, non_const_container<container>* = 0)
         : m_enum(new container_enumerator_impl<container>(cont)) {}

        template<class container>
        const_enumerator(container& cont, const_container<container>* = 0)
         : m_enum(new const_container_enumerator_impl<container>(cont)) {}

        const_enumerator() : m_enum(nullptr) {}
        const_enumerator(const const_enumerator& e) { m_enum = ((e.m_enum == nullptr) ? nullptr : e.m_enum->clone()); }
        const_enumerator(const_enumerator&& e) { m_enum = e.m_enum; e.m_enum = nullptr; }
        ~const_enumerator() { if (m_enum != nullptr) delete m_enum; }

        const_enumerator& operator= (const const_enumerator& e)
        { if (m_enum != nullptr) delete m_enum; m_enum = (e.m_enum == nullptr) ? nullptr : e.m_enum->clone(); }
        const_enumerator& operator= (const_enumerator&& e)
        { if (m_enum != nullptr) delete m_enum; m_enum = e.m_enum; e.m_enum = nullptr; }

        void next() { if (m_enum != nullptr) m_enum->next(); else throw std::runtime_error("empty enumerator"); }
        void advance(size_t n) { if (m_enum != nullptr) m_enum->advance(n); else throw std::runtime_error("empty enumerator"); }
        bool is_finished() const { if (m_enum != nullptr) return m_enum->is_finished(); else throw std::runtime_error("empty enumerator"); }
        void reset() { if (m_enum != nullptr) m_enum->reset(); else throw std::runtime_error("empty enumerator"); }
        size_t count() const { if (m_enum != nullptr) return m_enum->count(); else return 0; }
        optional<T> get() { if (m_enum != nullptr) return m_enum->get(); else return {}; }
        optional<T> get() const { if (m_enum != nullptr) return m_enum->get(); else return {}; }
    };

    template<class T>
    class enumerator : public const_enumerator<T>
    {
    public:
        template<class iterator>
        enumerator(iterator begin, iterator end) : const_enumerator<T>(begin, end) {}

        template<class container>
        enumerator(container& cont) : const_enumerator<T>(cont) {}

        enumerator() {}
        enumerator(const enumerator& e) { this->m_enum = ((e.m_enum == nullptr) ? nullptr : e.m_enum->clone()); }
        enumerator(enumerator&& e) { this->m_enum = e.m_enum; e.m_enum = nullptr; }
        ~enumerator() { /* implemented in const_enumerator already */ }

        enumerator& operator= (const enumerator& e)
        { if (this->m_enum != nullptr) delete this->m_enum; this->m_enum = (e.m_enum == nullptr) ? nullptr : e.m_enum->clone(); }
        enumerator& operator= (enumerator&& e)
        { if (this->m_enum != nullptr) delete this->m_enum; this->m_enum = e.m_enum; e.m_enum = nullptr; }

        void erase() { if (this->m_enum != nullptr) this->m_enum->erase(); else throw std::runtime_error("empty enumerator"); }
        void insert(const T& t) { if (this->m_enum != nullptr) this->m_enum->insert(t); else throw std::runtime_error("empty enumerator"); }
        void insert(T&& t) { if (this->m_enum != nullptr) this->m_enum->insert(t); else throw std::runtime_error("empty enumerator"); }
    };
};

#endif // GG_ENUMERATOR_HPP_INCLUDED
