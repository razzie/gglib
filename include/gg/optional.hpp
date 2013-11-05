#ifndef GG_OPTIONAL_HPP_INCLUDED
#define GG_OPTIONAL_HPP_INCLUDED

#include <utility>

namespace gg
{
    template<typename T>
    class optional
    {
        T m_val;
        bool m_valid;

    public:
        optional()
         : m_valid(false) {}

        optional(T t)
         : m_val(t), m_valid(true) {}

        optional(const optional& o)
         : m_val(o.m_val), m_valid(o.m_valid) {}

        optional(optional&& o)
         : m_val(std::move(o.m_val)), m_valid(o.m_valid) {}

        ~optional() {}

        optional& operator= (T t)
        {
            m_val = t;
            m_valid = true;
            return *this;
        }

        optional& operator= (const optional& o)
        {
            m_val = o.m_val;
            m_valid = o.m_valid;
            return *this;
        }

        optional& operator= (optional&& o)
        {
            m_val = std::move(o.m_val);
            m_valid = o.m_valid;
            return *this;
        }

        bool is_valid() const
        {
            return m_valid;
        }

        operator T() const
        {
            if (!m_valid) throw std::runtime_error("getting value of invalid optional<>");
            return m_val;
        }
    };
};

#endif // GG_OPTIONAL_HPP_INCLUDED
