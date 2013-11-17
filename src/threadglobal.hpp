#ifndef GG_THREADGLOBAL_HPP_INCLUDED
#define GG_THREADGLOBAL_HPP_INCLUDED

#include <stack>
#include "tinythread.h"
#include "gg/core.hpp"

namespace gg
{
    template<typename T>
    class thread_global
    {
        tthread::mutex sm_mutex;
        std::map<tthread::thread::id, T> sm_value;

    public:
        thread_global() = default;
        ~thread_global() = default;

        void set(T t)
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            sm_value[tthread::this_thread::get_id()] = t;
        }

        void unset()
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            sm_value.erase(tthread::this_thread::get_id());
        }

        optional<T> get()
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            auto pos = sm_value.find(tthread::this_thread::get_id());
            if (pos != sm_value.end()) return optional<T>(pos->second);
            else return optional<T>();
        }
    };

    template<typename T>
    class recursive_thread_global
    {
        tthread::mutex sm_mutex;
        std::map<tthread::thread::id, std::stack<T>> sm_values;

    public:
        recursive_thread_global() = default;
        ~recursive_thread_global() = default;

        void begin(T t)
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            sm_values[tthread::this_thread::get_id()].push(t);
        }

        void end()
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            std::stack<T>& v = sm_values[tthread::this_thread::get_id()];
            if (!v.empty()) v.pop();
        }

        optional<T> get()
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            std::stack<T>& v = sm_values[tthread::this_thread::get_id()];
            if (!v.empty()) return v.top();
            else return optional<T>();
        }

        class scope
        {
            recursive_thread_global* m_ptr;

        public:
            scope(recursive_thread_global* ptr, T t) : m_ptr(ptr) { m_ptr->begin(t); }
            scope(const scope&) = delete;
            scope(scope&&) = delete;
            ~scope() { m_ptr->end(); }
        };
    };
};

#endif // GG_THREADGLOBAL_HPP_INCLUDED
