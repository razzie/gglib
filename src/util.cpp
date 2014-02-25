#include "gg/util.hpp"

using namespace gg;
using namespace gg::util;


struct delimiter_is_space : std::ctype<char>
{
    char m_delim;
    std::vector<std::ctype_base::mask> m_rc;

//public:
    delimiter_is_space(char delim)
     : std::ctype<char>(get_table())
     , m_delim(delim)
     , m_rc(table_size, std::ctype_base::mask())
    {
        m_rc[m_delim] = std::ctype_base::space;
        m_rc['\n'] = std::ctype_base::space;
    }

    const std::ctype_base::mask* get_table()
    {
        return m_rc.data();
    }
};

std::istream& gg::util::operator<< (std::istream& i, const delimiter& d)
{
    i.imbue(std::locale(i.getloc(), new delimiter_is_space(d.delim)));
    return i;
}


scope_callback::scope_callback()
{
}

scope_callback::scope_callback(std::function<void()> func)
 : m_func(func)
{
}

scope_callback::~scope_callback()
{
    if (m_func) m_func();
}

scope_callback& scope_callback::operator= (std::function<void()> func)
{
    m_func = func; return *this;
}

void scope_callback::reset()
{
    m_func = nullptr;
}


template int strcmpi<char>(std::basic_string<char>, std::basic_string<char>, std::locale);
template int strcmpi<wchar_t>(std::basic_string<wchar_t>, std::basic_string<wchar_t>, std::locale);

template int strncmpi<char>(std::basic_string<char>, std::basic_string<char>, size_t, std::locale);
template int strncmpi<wchar_t>(std::basic_string<wchar_t>, std::basic_string<wchar_t>, size_t, std::locale);

template std::basic_string<char> trim<char>(std::basic_string<char>, std::locale);
template std::basic_string<wchar_t> trim<wchar_t>(std::basic_string<wchar_t>, std::locale);

template std::basic_string<char> convert_string<char>(std::basic_string<char>, std::locale);
template std::basic_string<wchar_t> convert_string<wchar_t>(std::basic_string<wchar_t>, std::locale);

template bool is_integer<char>(std::basic_string<char>, std::locale);
template bool is_integer<wchar_t>(std::basic_string<wchar_t>, std::locale);

template bool is_float<char>(std::basic_string<char>, std::locale);
template bool is_float<wchar_t>(std::basic_string<wchar_t>, std::locale);

template bool is_numeric<char>(std::basic_string<char>, std::locale);
template bool is_numeric<wchar_t>(std::basic_string<wchar_t>, std::locale);

template bool contains_space<char>(std::basic_string<char>, std::locale);
template bool contains_space<wchar_t>(std::basic_string<wchar_t>, std::locale);
