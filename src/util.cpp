#include <vector>
#include <assert.h>
#include "gg/util.hpp"

using namespace gg;

std::string util::trim(std::string s, std::locale loc)
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

    return std::string(it_first, it_last);
}

std::wstring util::trim(std::wstring ws, std::locale loc)
{
    auto ws_begin = ws.begin(), ws_end = ws.end();
    auto it_first = ws_end, it_last = ws_end;

    for (auto it = ws_begin; it != ws_end; ++it)
    {
        if (!std::isspace(*it, loc))
        {
            if (it_first == ws_end) it_first = it;
            it_last = it + 1;
        }
    }

    return std::wstring(it_first, it_last);
}

std::string util::narrow(std::wstring s, std::locale loc)
{
    std::vector<char> result(4*s.size() + 1);
    wchar_t const* fromNext;
    char* toNext;
    mbstate_t state = {0};
    std::codecvt_base::result convResult
        = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t> >(loc)
        .out(state,&s[0], &s[s.size()], fromNext,
             &result[0], &result[result.size()], toNext);

    assert(fromNext == &s[s.size()]);
    assert(toNext != &result[result.size()]);
    assert(convResult == std::codecvt_base::ok);
    *toNext = '\0';

    return &result[0];
}

std::wstring util::widen(std::string s, std::locale loc)
{
    std::vector<wchar_t> result(s.size() + 1);
    char const* fromNext;
    wchar_t* toNext;
    mbstate_t state = {0};
    std::codecvt_base::result convResult
        = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t> >(loc)
        .in(state, &s[0], &s[s.size()], fromNext,
            &result[0], &result[result.size()], toNext);

    assert(fromNext == &s[s.size()]);
    assert(toNext != &result[result.size()]);
    assert(convResult == std::codecvt_base::ok);
    *toNext = L'\0';

    return &result[0];
}

bool util::is_integer(std::string s)
{
    if (s.empty()) return false;

    auto it = s.begin(), end = s.end();

    if (*it == '-') ++it;

    for (; it != end; ++it)
    {
        if (!std::isdigit(*it)) return false;
    }

    return true;
}

bool util::is_float(std::string s)
{
    if (s.empty()) return false;

    auto it = s.begin(), end = s.end();
    bool point_used = false;

    if (*it == '-') ++it;

    for (; it != end; ++it)
    {
        if ((!std::isdigit(*it) && *it != '.') ||
            (*it == '.' && point_used)) return false;

        if (*it == '.') point_used = true;
    }

    return true;
}

bool util::is_numeric(std::string s)
{
    return (util::is_float(s) || util::is_integer(s));
}

bool util::contains_space(std::string s)
{
    auto it = s.cbegin(), end = s.cend();
    bool found_char = false;
    bool found_space = false;

    for (; it != end; ++it)
    {
        if (!found_char && !std::isspace(*it)) { found_char = true; continue; }
        if (found_char && !found_space && std::isspace(*it)) { found_space = true; continue; }
        if (found_space && !std::isspace(*it)) return true;
    }

    return false;
}
