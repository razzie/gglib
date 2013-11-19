#ifndef GG_UTIL_HPP_INCLUDED
#define GG_UTIL_HPP_INCLUDED

#include <assert.h>
#include "gg/core.hpp"

namespace gg
{
namespace util
{
    template<typename T>
    std::basic_string<T> trim(std::basic_string<T> s,
                              std::locale loc = std::locale())
    {
        auto s_begin = s.begin(), s_end = s.end();
        auto it_first = s_end, it_last = s_end;

        for (auto it = s_begin; it != s_end; ++it)
        {
            if (!std::isspace(*it, loc))
            {
                if (it_first == s_end) it_first = it;
                it_last = it + 1;
            }
        }

        return std::basic_string<T>(it_first, it_last);
    }

    template<typename FROM, typename TO>
    std::basic_string<TO> convert_string(std::basic_string<FROM> s,
                                         std::locale loc = std::locale())
    {
        std::vector<TO> result(s.size() + 1);
        FROM const* fromNext;
        TO* toNext;
        mbstate_t state = {0};
        std::codecvt_base::result convResult
            = std::use_facet<std::codecvt<TO, FROM, std::mbstate_t> >(loc)
            .in(state,&s[0], &s[s.size()], fromNext,
                &result[0], &result[result.size()], toNext);

        assert(fromNext == &s[s.size()]);
        assert(toNext != &result[result.size()]);
        assert(convResult == std::codecvt_base::ok);
        *toNext = '\0';

        return &result[0];
    }

    template<typename T>
    bool is_integer(std::basic_string<T> s,
                    std::locale loc = std::locale())
    {
        if (s.empty()) return false;

        auto it = s.begin(), end = s.end();

        if (*it == '-') ++it;

        for (; it != end; ++it)
        {
            if (!std::isdigit(*it, loc)) return false;
        }

        return true;
    }

    template<typename T>
    bool is_float(std::basic_string<T> s,
                  std::locale loc = std::locale())
    {
        if (s.empty()) return false;

        auto it = s.begin(), end = s.end();
        bool point_used = false;

        if (*it == '-') ++it;

        for (; it != end; ++it)
        {
            if ((!std::isdigit(*it, loc) && *it != '.') ||
                (*it == '.' && point_used)) return false;

            if (*it == '.') point_used = true;
        }

        return true;
    }

    template<typename T>
    bool is_numeric(std::basic_string<T> s,
                    std::locale loc = std::locale())
    {
        return (is_float(s, loc) || is_integer(s, loc));
    }

    template<typename T>
    bool contains_space(std::basic_string<T> s,
                        std::locale loc = std::locale())
    {
        auto it = s.cbegin(), end = s.cend();
        bool found_char = false;
        bool found_space = false;

        for (; it != end; ++it)
        {
            if (!found_char && !std::isspace(*it, loc)) { found_char = true; continue; }
            if (found_char && !found_space && std::isspace(*it, loc)) { found_space = true; continue; }
            if (found_space && !std::isspace(*it, loc)) return true;
        }

        return false;
    }


    class on_return
    {
        std::function<void()> m_func;

    public:
        on_return(std::function<void()> func) : m_func(func) {}
        on_return(const on_return&) = delete;
        on_return(on_return&&) = delete;
        ~on_return() { m_func(); }
    };
};
};

#endif // GG_UTIL_HPP_INCLUDED
