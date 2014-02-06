#ifndef ENUMUTIL_HPP_INCLUDED
#define ENUMUTIL_HPP_INCLUDED

#include <functional>
#include <iterator>
#include "gg/enumerator.hpp"

namespace gg
{
    template<class iterator, class T>
    class conversion_iterator : public std::iterator<std::forward_iterator_tag, T>
    {
    public:
        typedef T value_type;

    private:
        iterator m_iter;
        std::function<value_type&(typename iterator::value_type&)> m_conv;

    public:
        conversion_iterator(iterator it, std::function<value_type&(typename iterator::value_type&)> conv)
         : m_iter(it), m_conv(conv) {}
        conversion_iterator(const conversion_iterator& it)
         : m_iter(it.m_iter), m_conv(it.m_conv) {}
        ~conversion_iterator() {}

        conversion_iterator& operator= (const conversion_iterator& it)
        { m_iter = it.m_iter; m_conv = it.m_conv; return *this; }

        conversion_iterator& operator++ () { ++m_iter; return *this; }
        conversion_iterator& operator++ (int) { m_iter++; return *this; }

        bool operator== (const conversion_iterator& it) const { return (m_iter == it.m_iter); }
        bool operator!= (const conversion_iterator& it) const { return (m_iter != it.m_iter); }

        value_type& operator* () { return m_conv(*m_iter); }
        value_type* operator-> () { return &m_conv(*m_iter); }
        const value_type& operator* () const { return m_conv(*m_iter); }
        const value_type* operator-> () const { return &m_conv(*m_iter); }

        iterator get_internal_iterator() { return m_iter; }
    };

    template<class container, class T>
    class conversion_container
    {
    public:
        typedef T value_type;
        typedef conversion_iterator<typename container::iterator, value_type> iterator;

    private:
        container m_cont;
        std::function<value_type&(typename container::iterator::value_type&)> m_conv;

    public:
        conversion_container(container&& cont, std::function<value_type&(typename container::iterator::value_type&)> conv)
         : m_cont(cont), m_conv(conv) {}
        conversion_container(const conversion_container& cont)
         : m_cont(cont.m_cont), m_conv(cont.m_conv) {}
        conversion_container(conversion_container&& cont)
         : m_cont(std::move(cont.m_cont)), m_conv(std::move(cont.m_conv)) {}
        ~conversion_container() {}

        conversion_container& operator= (const conversion_container& cont)
        { m_cont.clear(); m_cont.insert(cont.m_cont.begin(), cont.m_cont.end()); m_conv = cont.m_conv; return *this; }
        conversion_container& operator= (conversion_container&& cont)
        { std::swap(m_cont, cont.m_cont); m_conv = std::move(cont.m_conv); return *this; }

        iterator begin() { return iterator(m_cont.begin(), m_conv); }
        iterator end() { return iterator(m_cont.end(), m_conv); }
        iterator insert(iterator it, value_type val) { return iterator(m_cont.insert(it.get_internal_iterator(), val), m_conv); }
        iterator erase(iterator it) { return iterator(m_cont.erase(it.get_internal_iterator()), m_conv); }
        size_t size() const { return m_cont.size(); }
        void clear() { m_cont.clear(); }
    };
};

#endif // ENUMUTIL_HPP_INCLUDED
