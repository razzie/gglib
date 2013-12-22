#include "gg/util.hpp"

using namespace gg;
using namespace gg::util;


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
