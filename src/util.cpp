#include "gg/util.hpp"

using namespace gg;
using namespace gg::util;


struct delimiter_is_space : std::ctype<char>
{
    std::ctype_base::mask m_rc[table_size];

    delimiter_is_space(char delim)
     : std::ctype<char>(get_table(static_cast<unsigned char>(delim)))
    {
    }

    const std::ctype_base::mask* get_table(unsigned char delim)
    {
        m_rc[delim] = std::ctype_base::space;
        m_rc['\n'] = std::ctype_base::space;
        return &m_rc[0];
    }
};

static std::istream& __delimiter(std::istream& i, char d)
{
    i.imbue(std::locale(i.getloc(), new delimiter_is_space(d)));
    return i;
}

istream_manipulator<char> util::delimiter(char d)
{
    return istream_manipulator<char>(__delimiter, d);
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
