#ifndef GG_THREADGLOBAL_HPP_INCLUDED
#define GG_THREADGLOBAL_HPP_INCLUDED

#include <stack>
#include "tinythread.h"
#include "gg/core.hpp"

#define GG_USE_THREAD_GLOBAL(T) \
    template<> tthread::mutex gg::thread_global<T>::sm_mutex{}; \
    template<> std::map<tthread::thread::id, T> gg::thread_global<T>::sm_value{};

#define GG_USE_RECURSIVE_THREAD_GLOBAL(T) \
    template<> tthread::mutex gg::recursive_thread_global<T>::sm_mutex{}; \
    template<> std::map<tthread::thread::id, std::stack<T>> gg::recursive_thread_global<T>::sm_values{};

namespace gg
{
    template<typename T>
    class thread_global
    {
        static tthread::mutex sm_mutex;
        static std::map<tthread::thread::id, T> sm_value;

    public:
        thread_global() = delete;
        ~thread_global() = delete;

        static void set(T t)
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            sm_value[tthread::this_thread::get_id()] = t;
        }

        static void unset()
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            sm_value.erase(tthread::this_thread::get_id());
        }

        static optional<T> get()
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
        static tthread::mutex sm_mutex;
        static std::map<tthread::thread::id, std::stack<T>> sm_values;

    public:
        recursive_thread_global() = delete;
        ~recursive_thread_global() = delete;

        static void begin(T t)
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            sm_values[tthread::this_thread::get_id()].push(t);
        }

        static void end()
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            std::stack<T>& v = sm_values[tthread::this_thread::get_id()];
            if (!v.empty()) v.pop();
        }

        static optional<T> get()
        {
            tthread::lock_guard<tthread::mutex> guard(sm_mutex);
            std::stack<T>& v = sm_values[tthread::this_thread::get_id()];
            if (!v.empty()) return v.top();
            else return optional<T>();
        }

        class scope
        {
        public:
            scope(T t) { begin(t); }
            scope(const scope&) = delete;
            scope(scope&&) = delete;
            ~scope() { end(); }
        };
    };
};

#endif // GG_THREADGLOBAL_HPP_INCLUDED
