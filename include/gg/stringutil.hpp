#ifndef GG_STRINGUTIL_HPP_INCLUDED
#define GG_STRINGUTIL_HPP_INCLUDED

#include <cassert>
#include <string>
#include <vector>
#include <locale>

namespace gg
{
    template<class T>
    int strcmpi(std::basic_string<T> s1, std::basic_string<T> s2,
                std::locale loc = std::locale())
    {
        if (s1.size() != s2.size()) return (s1.size() - s2.size());

        for (size_t i = 0; i < s1.size(); ++i)
        {
            int c1 = std::tolower(s1[i], loc);
            int c2 = std::tolower(s2[i], loc);
            if (c1 != c2) return (c1 - c2);
        }

        return 0;
    }

    template<class T>
    int strncmpi(std::basic_string<T> s1, std::basic_string<T> s2, size_t n,
                 std::locale loc = std::locale())
    {
        if (s1.size() < n || s2.size() < n) return (s1.size() - s2.size());

        for (size_t i = 0; i < n; ++i)
        {
            int c1 = std::tolower(s1[i], loc);
            int c2 = std::tolower(s2[i], loc);
            if (c1 != c2) return (c1 - c2);
        }

        return 0;
    }

    template<class T>
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

    template<class FROM, class TO>
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

    template<class T>
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

    template<class T>
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

    template<class T>
    bool is_numeric(std::basic_string<T> s,
                    std::locale loc = std::locale())
    {
        return (is_float(s, loc) || is_integer(s, loc));
    }

    template<class T>
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
};

#endif // GG_STRINGUTIL_HPP_INCLUDED
