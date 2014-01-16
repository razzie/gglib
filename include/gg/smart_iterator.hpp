#ifndef GG_SMART_ITERATOR_HPP_INCLUDED
#define GG_SMART_ITERATOR_HPP_INCLUDED

#include <functional>
#include <type_traits>

namespace gg
{
    template<class iterator, class T = typename iterator::value_type>
    class smart_iterator
    {
        iterator m_iter;
        std::function<T&(typename iterator::value_type&)> m_get;
        std::function<void(iterator&)> m_incr;

    public:
        typedef T value_type;

        smart_iterator(iterator iter, std::function<T&(typename iterator::value_type&)> get, std::function<void(iterator&)> incr)
         : m_iter(iter), m_get(get), m_incr(incr) {}
        smart_iterator(iterator iter, std::function<T&(typename iterator::value_type&)> get)
         : m_iter(iter), m_get(get), m_incr([](iterator& it)->void{ std::advance(it, 1); }) {}
        smart_iterator(const smart_iterator& it)
         : m_iter(it.m_iter), m_get(it.m_get), m_incr(it.m_incr) {}
        smart_iterator(smart_iterator&& it)
         : m_iter(std::move(it.m_iter)), m_get(std::move(it.m_get)), m_incr(std::move(it.m_incr)) {}
        ~smart_iterator() {}

        smart_iterator& operator= (const smart_iterator& it)
        { m_iter = it.m_iter; m_get = it.m_get; m_incr = it.m_incr; return *this; }
        smart_iterator& operator= (smart_iterator&& it)
        { m_iter = std::move(it.m_iter); m_get = std::move(it.m_get); m_incr = std::move(it.m_incr); return *this; }

        T& operator* () { return m_get(*m_iter); }
        T* operator-> () { return &m_get(*m_iter); }
        bool operator== (const smart_iterator& it) { return (m_iter == it.m_iter); }
        bool operator!= (const smart_iterator& it) { return (m_iter != it.m_iter); }
        smart_iterator& operator++ () { m_incr(m_iter); return *this; }
    };

};

#endif // GG_SMART_ITERATOR_HPP_INCLUDED
