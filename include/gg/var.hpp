#ifndef VAR_HPP_INCLUDED
#define VAR_HPP_INCLUDED

#include <vector>
#include "gg/types.hpp"
#include "gg/util.hpp"

namespace gg
{
    class var
    {
        /*
         * var_impl_base abstract class for holding dynamic data
         */
        class var_impl_base
        {
        public:
            virtual ~var_impl_base() {}
            virtual var_impl_base* clone() const = 0;
            virtual const void* get_ptr() const = 0;
            virtual const std::type_info& get_type() const = 0;
            virtual void extract_to(std::ostream&) const = 0;
        };

        /*
         * var_impl<T> implements var_impl_base abstract class
         */
        template<class T>
        class var_impl : public var_impl_base
        {
            T m_var;
            const std::type_info* m_type;

        public:
            var_impl(T t)
             : m_var(t), m_type(&typeid(T))
            {
            }

            var_impl(const var_impl& v)
             : m_var(v.m_var), m_type(v.m_type)
            {
            }

            ~var_impl()
            {
            }

            var_impl_base* clone() const
            {
                return new var_impl<T>(m_var);
            }

            const void* get_ptr() const
            {
                return static_cast<const void*>(&m_var);
            }

            const std::type_info& get_type() const
            {
                return *m_type;
            }

            void extract_to(std::ostream& o) const
            {
                util::ostream_insert(o, m_var);
            }
        };

        const var_impl_base* m_var = nullptr;

        void extract_to(std::ostream& o) const
        {
            if (m_var != nullptr)
                m_var->extract_to(o);
            else
                o << "(empty)";
        }

    public:
        /*
         * view class for ostream insertion operator support
         */
        class view
        {
            const var& m_var;
        public:
            view(const var& var) : m_var(var) {}
            view(const view& vw) : m_var(vw.m_var) {}
            ~view() {}
            void extract_to(std::ostream& o) const { m_var.extract_to(o); }
        };

        /*
         * var public method implementations
         */
        var() {}

        var(const var& _v)
        {
            //if (m_var != nullptr) delete m_var;
            m_var = _v.m_var->clone();
        }

        var(var&& _v)
        {
            //if (m_var != nullptr) delete m_var;
            std::swap(m_var, _v.m_var);
        }

        template<class T>
        var(T t)
        {
            m_var = new var_impl<T>(t);
        }

        var(const char* str)
         : var(std::string(str))
        {
        }

        ~var()
        {
            if (m_var != nullptr) delete m_var;
        }

        var& operator= (const var& _v)
        {
            if (m_var != nullptr) { delete m_var; m_var = nullptr; }
            m_var = _v.m_var->clone();

            return *this;
        }

        var& operator= (var&& _v)
        {
            if (m_var != nullptr) { delete m_var; m_var = nullptr; }
            std::swap(m_var, _v.m_var);

            return *this;
        }

        template<class T>
        var& operator= (const T& t)
        {
            if (m_var != nullptr) delete m_var;
            m_var = new var_impl<T>(t);

            return *this;
        }

        template<class T>
        const T& get() const
        {
            if (m_var == nullptr)
                throw std::runtime_error("casting empty var");

            if (m_var->get_type() != typeid(T))
                throw std::runtime_error("casting var to different type");

            return *static_cast<const T*>(m_var->get_ptr());
        }

        template<class T>
        operator T() const
        {
            return this->get<T>();
        }

        template<class T>
        T cast() const
        {
            if (m_var == nullptr)
                throw std::runtime_error("casting empty var");

            if (m_var->get_type() == typeid(T))
                return *static_cast<const T*>(m_var->get_ptr());

            if (!util::has_extract_op<T>::value)
                throw std::runtime_error("unable to cast");

            T result;
            std::stringstream ss;

            m_var->extract_to(ss);
            util::istream_extract(ss, result);

            return result;
        }

        view to_stream() const
        {
            return view(*this);
        }

        const std::type_info& get_type() const
        {
            if (m_var != nullptr)
                return m_var->get_type();
            else
                return typeid(void);
        }
    };

    typedef std::vector<var> varlist;

    namespace util
    {
        /*
         * Dynamic function argument list
         */
        template <typename R, typename... Args>
        R callfunc (std::function<R(Args...)> func, varlist vl);

        template <typename R>
        R callfunc (std::function<R()> func, varlist vl)
        {
            if (vl.size() > 0)
                throw std::runtime_error("argument list too long");

            return func();
        }

        template <typename R, typename Arg0, typename... Args>
        R callfunc (std::function<R(Arg0, Args...)> func, varlist vl)
        {
            if (vl.size() == 0)
                throw std::runtime_error("argument list too short");

            Arg0 arg0 = vl[0].cast<Arg0>();
            vl.erase(vl.begin());
            std::function<R(Args... args)> lambda =
                [=](Args... args) -> R { return func(arg0, args...); };

            return callfunc (lambda, vl);
        }

        template <typename R, typename... Args>
        std::function<var(varlist)> adaptfunc (std::function<R(Args...)> stdfunc)
        {
            std::function<var(varlist)> result =
                ([=](varlist vl) -> var {
                 return var(callfunc(stdfunc, vl));
                 });
            return result;
        }

        template <typename R, typename... Args>
        std::function<var(varlist)> adaptfunc (R(*func)(Args...))
        {
            std::function<R(Args...)> stdfunc = func;
            return adaptfunc(stdfunc);
        }
    };
};

std::ostream& operator<< (std::ostream& o, const gg::var::view& vw);
std::ostream& operator<< (std::ostream& o, const gg::varlist& vl);

#endif // VAR_HPP_INCLUDED
