#ifndef GG_TYPEINFO_HPP_INCLUDED
#define GG_TYPEINFO_HPP_INCLUDED

#include <typeinfo>
#include <string>

namespace gg
{
    class typeinfo
    {
        const std::type_info* m_type;

    public:
        typeinfo(const std::type_info&);
        typeinfo(const typeinfo&);
        typeinfo& operator= (const typeinfo&);
        ~typeinfo();

        bool operator== (const typeinfo&) const;
        bool operator!= (const typeinfo&) const;
        bool operator<  (const typeinfo&) const;
        bool operator<= (const typeinfo&) const;
        bool operator>  (const typeinfo&) const;
        bool operator>= (const typeinfo&) const;

        std::string name() const;
        operator const std::type_info& () const;
        size_t hash_code() const noexcept;

        template<class T>
        static std::string name_of() { return name_of(typeid(T)); }
        static std::string name_of(const std::type_info&);
    };
};

#endif // GG_TYPEINFO_HPP_INCLUDED
