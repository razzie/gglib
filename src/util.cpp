#include <cstdlib>
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


static std::ostream& __format(std::ostream& os, const char* fmt)
{
    int i = 0;
    while (fmt[i] != 0)
    {
        if (fmt[i] != '%')
        {
            os << fmt[i];
            i++;
        }
        else
        {
            i++;
            if (fmt[i] == '%')
            {
                os << fmt[i];
                i++;
            }
            else
            {
                bool ok = true;
                int istart = i;
                bool more = true;
                int width = 0;
                int precision = 6;
                std::ios_base::fmtflags flags;
                char fill = ' ';
                bool alternate = false;
                while (more)
                {
                    switch (fmt[i])
                    {
                    case '+':
                        flags |= std::ios::showpos;
                        break;
                    case '-':
                        flags |= std::ios::left;
                        break;
                    case '0':
                        flags |= std::ios::internal;
                        fill = '0';
                        break;
                    case '#':
                        alternate = true;
                        break;
                    case ' ':
                        break;
                    default:
                        more = false;
                        break;
                    }
                    if (more) i++;
                }
                if (std::isdigit(fmt[i]))
                {
                    width = std::atoi(fmt+i);
                    do i++;
                    while (std::isdigit(fmt[i]));
                }
                if (fmt[i] == '.')
                {
                    i++;
                    precision = std::atoi(fmt+i);
                    while (std::isdigit(fmt[i])) i++;
                }
                switch (fmt[i])
                {
                case 'd':
                    flags |= std::ios::dec;
                    break;
                case 'x':
                    flags |= std::ios::hex;
                    if (alternate) flags |= std::ios::showbase;
                    break;
                case 'X':
                    flags |= std::ios::hex | std::ios::uppercase;
                    if (alternate) flags |= std::ios::showbase;
                    break;
                case 'o':
                    flags |= std::ios::hex;
                    if (alternate) flags |= std::ios::showbase;
                    break;
                case 'f':
                    flags |= std::ios::fixed;
                    if (alternate) flags |= std::ios::showpoint;
                    break;
                case 'e':
                    flags |= std::ios::scientific;
                    if (alternate) flags |= std::ios::showpoint;
                    break;
                case 'E':
                    flags |= std::ios::scientific | std::ios::uppercase;
                    if (alternate) flags |= std::ios::showpoint;
                    break;
                case 'g':
                    if (alternate) flags |= std::ios::showpoint;
                    break;
                case 'G':
                    flags |= std::ios::uppercase;
                    if (alternate) flags |= std::ios::showpoint;
                    break;
                default:
                    ok = false;
                    break;
                }
                i++;
                if (fmt[i] != 0) ok = false;
                if (ok)
                {
                    os.unsetf(std::ios::adjustfield | std::ios::basefield |
                              std::ios::floatfield);
                    os.setf(flags);
                    os.width(width);
                    os.precision(precision);
                    os.fill(fill);
                }
                else i = istart;
            }
        }
    }
    return os;
}

ostream_manipulator<const char*> util::format(const char* fmt)
{
    return ostream_manipulator<const char*>(__format, fmt);
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
