#ifndef GG_VAR_HPP_INCLUDED
#define GG_VAR_HPP_INCLUDED

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include <stdexcept>
#include "gg/misc.hpp"
#include "gg/cast.hpp"

namespace gg
{
    class var
    {
        class var_impl_base
        {
        public:
            virtual ~var_impl_base() {}
            virtual var_impl_base* clone() const = 0;
            virtual void* get_ptr() = 0;
            virtual const void* get_ptr() const = 0;
            virtual const std::type_info& get_type() const = 0;
            virtual void extract_to(std::ostream&) const = 0;
        };

        template<class T>
        class var_impl : public var_impl_base
        {
            T m_var;
            const std::type_info* m_type;

        public:
            template<class... Args>
            var_impl(Args... args) : m_var(std::forward<Args>(args)...), m_type(&typeid(T)) {}
            var_impl(const var_impl& v) : m_var(v.m_var), m_type(v.m_type) {}
            ~var_impl() {}
            var_impl_base* clone() const { return new var_impl<T>(m_var); }
            void* get_ptr() { return static_cast<void*>(&m_var); }
            const void* get_ptr() const { return static_cast<const void*>(&m_var); }
            const std::type_info& get_type() const { return *m_type; }
            void extract_to(std::ostream& o) const { ostream_insert(o, m_var); }
        };

        const var_impl_base* m_var = nullptr;

    public:
        var();
        var(const var& v);
        var(var&& v);
        ~var();

        template<class T>
        var(T t) : m_var(new var_impl<T>(t)) {}

        template<class T, class... Args>
        var(Args... args) : m_var(new var_impl<T>(std::forward<Args>(args)...)) {}

        template<class T>
        var& operator= (const T& t)
        {
            if (m_var != nullptr) delete m_var;
            m_var = new var_impl<T>(t);
            return *this;
        }

        var& operator= (const var& v);
        var& operator= (var&& v);
        const std::type_info& get_type() const;
        bool is_empty() const;

        template<class T>
        T& get()
        {
            if (m_var == nullptr)
                throw std::runtime_error("get() called on empty var");

            if (m_var->get_type() != typeid(T))
                throw std::runtime_error("var type mismatch");

            return *static_cast<T*>(m_var->get_ptr());
        }

        template<class T>
        const T& get() const
        {
            return get<T>();
        }

        template<class T>
        operator T() const { return this->get<T>(); }

        template<class T, class = meta::enable_if_t<std::is_same<T, var>::value>>
        var cast() const { return var(*this); }

        template<class T, class = meta::enable_if_t<!std::is_same<T, var>::value>>
        T cast() const
        {
            if (m_var == nullptr)
                throw std::runtime_error("casting empty var");

            if (m_var->get_type() == typeid(T))
                return *static_cast<const T*>(m_var->get_ptr());

            if (!meta::has_extract_op<T>::value)
                throw std::runtime_error("unable to cast");

            T result;
            std::stringstream ss;

            m_var->extract_to(ss);
            istream_extract(ss, result);

            return result;
        }

        class view
        {
            friend std::ostream& operator<< (std::ostream& o, const gg::var::view& vw);
            const var& m_var;

        public:
            view(const var& var);
            view(const view& vw);
            ~view();
        };

        view to_stream() const;
        std::string to_string() const;
        friend std::ostream& operator<< (std::ostream& o, const gg::var::view& vw);
    };

    typedef std::vector<var> varlist;

    std::ostream& operator<< (std::ostream& o, const varlist& vl);
};

#endif // GG_VAR_HPP_INCLUDED
