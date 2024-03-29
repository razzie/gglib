#ifndef GG_ENUMERATOR_HPP_INCLUDED
#define GG_ENUMERATOR_HPP_INCLUDED

#include <functional>
#include <iterator>
#include <type_traits>
#include <stdexcept>
#include "gg/refcounted.hpp"
#include "gg/optional.hpp"

namespace gg
{
    template<class container, class T, bool is_reference = false,
             bool is_const = std::is_const<container>::value>
    class container_wrapper
    {
        template<class P>
        struct is_ptr
        {
            enum { value = std::is_pointer<P>::value || is_grab_ptr<P>::value };
        };

        template<class U>
        using make_ref = typename std::conditional<is_ptr<U>::value, U, U&>::type;

    public:
        typedef T value_type;
        typedef make_ref<value_type> value_type_ref;

        typedef typename container::value_type container_value_type;
        typedef make_ref<container_value_type> container_value_type_ref;

        typedef decltype(((container*)0)->begin()) container_iterator;
        typedef typename std::remove_reference<container>::type container_noref;
        typedef typename std::decay<container>::type container_decay;

        typedef std::function<value_type_ref(container_value_type_ref)> extractor;

        class iterator : public std::iterator<std::forward_iterator_tag, value_type>
        {
        private:
            container_iterator m_iter;
            extractor* m_extr;

            friend class container_wrapper;

            iterator(container_iterator it, extractor& extr)
             : m_iter(it), m_extr(&extr) {}

        public:
            iterator(const iterator& it) : m_iter(it.m_iter), m_extr(it.m_extr) {}
            ~iterator() {}

            iterator& operator= (const iterator& it) { m_iter = it.m_iter; m_extr = it.m_extr; return *this; }
            iterator& operator++ () { ++m_iter; return *this; }
            iterator& operator++ (int) { m_iter++; return *this; }

            bool operator== (const iterator& it) const { return (m_iter == it.m_iter); }
            bool operator!= (const iterator& it) const { return (m_iter != it.m_iter); }

            value_type_ref operator* () { return (*m_extr)(*m_iter); }
            const value_type_ref operator* () const { return (*m_extr)(*m_iter); }
            value_type* operator-> () { return &(*m_extr)(*m_iter); }
            const value_type* operator-> () const { return &(*m_extr)(*m_iter); }

            container_iterator get_internal_iterator() { return m_iter; }
        };

    private:
        container_noref* m_cont;
        extractor m_extr;

        template<class U = container_value_type_ref, class R = value_type_ref>
        static R default_extractor(U it)
        {
            static_assert(std::is_convertible<U, R>::value, "value type is not convertible to return type");
            return static_cast<R>(it);
        }

        template<bool is_const_iterator>
        typename std::enable_if<!is_const_iterator, iterator>::type
        _insert(iterator it, value_type val)
        {
            return iterator(m_cont->insert(it.get_internal_iterator(), val), m_extr);
        }

        template<bool is_const_iterator>
        typename std::enable_if<!is_const_iterator, iterator>::type
        _erase(iterator it)
        {
            return iterator(m_cont->erase(it.get_internal_iterator()), m_extr);
        }

        template<bool is_const_iterator>
        typename std::enable_if<is_const_iterator, iterator>::type
        _insert(iterator it, value_type val)
        {
            throw std::runtime_error("container is const");
            //return it;
        }

        template<bool is_const_iterator>
        typename std::enable_if<is_const_iterator, iterator>::type
        _erase(iterator it)
        {
            throw std::runtime_error("container is const");
            //return it;
        }

    public:
        container_wrapper(container_decay&& cont, extractor extr = {})
        {
            if (is_reference) throw std::runtime_error("can't take reference of xvalue");
            m_cont = new container_decay(cont);
            if (!m_extr) m_extr = default_extractor<container_value_type_ref, value_type_ref>;
        }

        container_wrapper(container_noref& cont, extractor extr = {})
        {
            if (is_reference) m_cont = &cont;
            else m_cont = new container_decay(cont.begin(), cont.end());
            if (!m_extr) m_extr = default_extractor<container_value_type_ref, value_type_ref>;
        }

        container_wrapper(const container_wrapper& cont)
        {
            if (is_reference) m_cont = cont.m_cont;
            else m_cont = new container_decay(cont.m_cont->begin(), cont.m_cont->end());
            m_extr = cont.m_extr;
        }

        container_wrapper(container_wrapper&& cont)
        {
            if (is_reference) m_cont = cont.m_cont;
            else m_cont = new container_decay(std::move(*cont.m_cont));
            m_extr = cont.m_extr; // can't move as the original would crash then
        }

        ~container_wrapper()
        {
            if (!is_reference) delete m_cont;
        }

        size_t size() const { return m_cont->size(); }
        void clear() { m_cont->clear(); }

        iterator begin() { return iterator(m_cont->begin(), m_extr); }
        iterator end() { return iterator(m_cont->end(), m_extr); }

        iterator insert(iterator it, value_type val) { return _insert<is_const>(it, val); }
        iterator erase(iterator it) { return _erase<is_const>(it); }
    };


    template<class T>
    class enumerator
    {
        template<class U>
        using ptr_of = typename std::conditional<std::is_pointer<U>::value, U, U*>::type;

        class enumerator_impl_base
        {
        public:
            virtual ~enumerator_impl_base() {}
            virtual enumerator_impl_base* clone() const = 0;
            virtual void next() = 0;
            virtual void advance(size_t) = 0;
            virtual bool has_next() const = 0;
            virtual void reset() = 0;
            virtual void erase() = 0;
            virtual void insert(const T&) = 0;
            virtual void insert(T&&) = 0;
            virtual size_t count() const = 0;
            virtual ptr_of<T> get_ptr() = 0;
            virtual const ptr_of<T> get_ptr() const = 0;
        };

        template<class U, class = typename std::enable_if<std::is_pointer<U>::value>>
        static U make_ptr(U u) { return u; }

        template<class U, class R, class = typename std::enable_if<!std::is_pointer<U>::value>>
        static R* make_ptr(U& u) { return static_cast<R*>(&u); }

        template<class U = T, class = typename std::enable_if<std::is_pointer<U>::value>>
        static optional<U> ptr_to_opt(T p) { return p; }

        template<class U = T, class = typename std::enable_if<!std::is_pointer<U>::value>>
        static optional<U> ptr_to_opt(T* p) { return *p; }

        template<class container, bool is_reference, bool is_const>
        class enumerator_impl : public enumerator_impl_base
        {
        private:
            typedef container_wrapper<container, T, is_reference, is_const> container_type;
            typedef decltype(((container_type*)0)->begin()) container_iterator;

            mutable container_type m_cont;
            container_iterator m_current;
            container_iterator m_next;

        public:
            enumerator_impl(container_type cont) : m_cont(cont), m_current(m_cont.begin()), m_next(std::next(m_current, 1)) {}
            enumerator_impl(const enumerator_impl* e) : m_cont(e.m_cont), m_current(e.m_current), m_next(e.m_next) {}
            enumerator_impl_base* clone() const { return new enumerator_impl(*this); }
            void next() { if (has_next()) { m_current = m_next; std::advance(m_next, 1); } }
            void advance(size_t n) { std::advance(m_current, n); m_next = std::next(m_current, 1); }
            bool has_next() const { return (m_current != m_cont.end()); }
            void reset() { m_current = m_cont.begin(); m_next = std::next(m_current, 1); }
            void erase() { if (has_next()) { m_current = m_next = m_cont.erase(m_current); } }
            void insert(const T& t) { m_next = m_cont.insert(m_next, t); }
            void insert(T&& t) { m_next = m_cont.insert(m_next, t); }
            size_t count() const { return m_cont.size(); }
            ptr_of<T> get_ptr() { if (has_next() && m_current != m_next) return make_ptr(*m_current); else return nullptr; }
            const ptr_of<T> get_ptr() const { if (has_next() && m_current != m_next) return make_ptr(*m_current); else return nullptr; }
        };

        enumerator_impl_base* m_enum;

    public:
        template<class container, bool is_reference, bool is_const>
        enumerator(container_wrapper<container, T, is_reference, is_const> cont)
         : m_enum(new enumerator_impl<container, is_reference, is_const>(cont)) {}

        enumerator() : m_enum(nullptr) {}
        enumerator(const enumerator& e) { m_enum = ((e.m_enum == nullptr) ? nullptr : e.m_enum->clone()); }
        enumerator(enumerator&& e) { m_enum = e.m_enum; e.m_enum = nullptr; }
        ~enumerator() { if (m_enum != nullptr) delete m_enum; }

        enumerator& operator= (const enumerator& e)
        { if (m_enum != nullptr) delete m_enum; m_enum = (e.m_enum == nullptr) ? nullptr : e.m_enum->clone(); }
        enumerator& operator= (enumerator&& e)
        { if (m_enum != nullptr) delete m_enum; m_enum = e.m_enum; e.m_enum = nullptr; }

        void next() { if (m_enum != nullptr) m_enum->next(); else throw std::runtime_error("empty enumerator"); }
        void advance(size_t n) { if (m_enum != nullptr) m_enum->advance(n); else throw std::runtime_error("empty enumerator"); }
        bool has_next() const { if (m_enum != nullptr) return m_enum->has_next(); else throw std::runtime_error("empty enumerator"); }
        void reset() { if (m_enum != nullptr) m_enum->reset(); else throw std::runtime_error("empty enumerator"); }
        void erase() { if (this->m_enum != nullptr) this->m_enum->erase(); else throw std::runtime_error("empty enumerator"); }
        void insert(const T& t) { if (this->m_enum != nullptr) this->m_enum->insert(t); else throw std::runtime_error("empty enumerator"); }
        void insert(T&& t) { if (this->m_enum != nullptr) this->m_enum->insert(t); else throw std::runtime_error("empty enumerator"); }
        size_t count() const { if (m_enum != nullptr) return m_enum->count(); else return 0; }
        optional<T> get() { if (m_enum != nullptr) return ptr_to_opt(m_enum->get_ptr()); else return {}; }
        optional<T> get() const { if (m_enum != nullptr) return ptr_to_opt(m_enum->get_ptr()); else return {}; }
        optional<T> operator* () { if (m_enum != nullptr) return ptr_to_opt(m_enum->get_ptr()); else return {}; }
        optional<T> operator* () const { if (m_enum != nullptr) return ptr_to_opt(m_enum->get_ptr()); else return {}; }
        ptr_of<T> operator-> () { if (m_enum != nullptr) return m_enum->get_ptr(); else return nullptr; }
        const ptr_of<T> operator-> () const { if (m_enum != nullptr) return m_enum->get_ptr(); else return nullptr; }
    };


    template<class T, class container>
    enumerator<T> make_enumerator(container cont, typename container_wrapper<container, T>::extractor extr = {})
    {
        container_wrapper<container, T, false> cw(cont, extr);
        return std::move(cw);
    }

    template<class T, class container>
    enumerator<T> make_ref_enumerator(container& cont, typename container_wrapper<container, T>::extractor extr = {})
    {
        static_assert(!std::is_const<container>::value, "container is const");
        container_wrapper<container, T, true> cw(cont, extr);
        return std::move(cw);
    }

    template<class T, class container>
    enumerator<T> make_const_enumerator(container cont, typename container_wrapper<container, T>::extractor extr = {})
    {
        container_wrapper<container, T, false, true> cw(cont, extr);
        return std::move(cw);
    }

    template<class T, class container>
    enumerator<T> make_const_ref_enumerator(container& cont, typename container_wrapper<container, T>::extractor extr = {})
    {
        container_wrapper<container, T, true, true> cw(cont, extr);
        return std::move(cw);
    }
};

#endif // GG_ENUMERATOR_HPP_INCLUDED
