#ifndef GG_CORE_HPP_INCLUDED
#define GG_CORE_HPP_INCLUDED

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <initializer_list>
#include <locale>
#include <functional>
#include <type_traits>
#include <stdexcept>

namespace gg
{
    struct nulltype {};

    struct color
    {
        uint8_t R, G, B;
    };

    class reference_counted
    {
        struct refcounted_data;
        refcounted_data* m_data;

    public:
        reference_counted();
        virtual ~reference_counted();
        void grab() const;
        void drop() const;
        uint32_t get_ref_count() const;
    };

    template<class T,
        class = typename std::enable_if<
            std::is_base_of<reference_counted,T>::value
        >::type >
    T* grab(T* o)
    {
        o->grab();
        return o;
    }

    template<class T,
        class = typename std::enable_if<
            std::is_base_of<reference_counted,T>::value
        >::type >
    const T* grab(const T* o)
    {
        o->grab();
        return o;
    }

    template<class T,
        class = typename std::enable_if<
            std::is_base_of<reference_counted,T>::value
        >::type >
    T* drop(T* o)
    {
        uint32_t refc = o->get_ref_count();
        o->drop();
        return (refc == 1) ? nullptr : o;
    }

    template<class T,
        class = typename std::enable_if<
            std::is_base_of<reference_counted,T>::value
        >::type >
    const T* drop(const T* o)
    {
        uint32_t refc = o->get_ref_count();
        o->drop();
        return (refc == 1) ? nullptr : o;
    }

    template<class T,
        class = typename std::enable_if<
            std::is_base_of<reference_counted,T>::value
        >::type >
    class auto_drop
    {
        T* m_obj;

    public:
        auto_drop(T* o) : m_obj(o) {}
        ~auto_drop() { m_obj->drop(); }
        operator T*() { return m_obj; }
    };

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

        void set_valid(bool valid)
        {
            m_valid = valid;
        }

        T get() const
        {
            if (!m_valid) throw std::runtime_error("getting value of invalid optional<>");
            return m_val;
        }

        operator T() const
        {
            return get();
        }
    };

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
        static std::string name_of(const std::type_info&);
        operator const std::type_info& () const;
    };

    namespace meta
    {
        namespace sfinae
        {
            class yes { char c[1]; };
            class no  { char c[2]; };
        };

        template<typename T> T& lvalue_of_type();
        template<typename T> T  rvalue_of_type();


        template<typename T>
        struct remove_class { };

        template<typename C, typename R, typename... Args>
        struct remove_class<R(C::*)(Args...)> { using type = R(Args...); };

        template<typename C, typename R, typename... Args>
        struct remove_class<R(C::*)(Args...) const> { using type = R(Args...); };

        template<typename C, typename R, typename... Args>
        struct remove_class<R(C::*)(Args...) volatile> { using type = R(Args...); };

        template<typename C, typename R, typename... Args>
        struct remove_class<R(C::*)(Args...) const volatile> { using type = R(Args...); };


        template<typename T>
        struct get_signature_impl { using type = typename remove_class<
            decltype(&std::remove_reference<T>::type::operator())>::type; };

        template<typename R, typename... Args>
        struct get_signature_impl<R(Args...)> { using type = R(Args...); };

        template<typename R, typename... Args>
        struct get_signature_impl<R(&)(Args...)> { using type = R(Args...); };

        template<typename R, typename... Args>
        struct get_signature_impl<R(*)(Args...)> { using type = R(Args...); };

        template<typename T>
        using get_signature = typename get_signature_impl<T>::type;


        template<typename T>
        struct has_insert_op
        {
            template<typename U>
            static sfinae::yes test(char(*)[sizeof(
                lvalue_of_type<std::ostream>() << rvalue_of_type<U>()
            )]);

            template<typename U>
            static sfinae::no test(...);

            enum { value = ( sizeof(sfinae::yes) == sizeof(test<T>(0)) ) };
            typedef std::integral_constant<bool, value> type;
        };


        template<typename T>
        struct has_extract_op
        {
            template<typename U>
            static sfinae::yes test(char(*)[sizeof(
                lvalue_of_type<std::istream>() >> lvalue_of_type<U>()
            )]);

            template<typename U>
            static sfinae::no test(...);

            enum { value = ( sizeof(sfinae::yes) == sizeof(test<T>(0)) ) };
            typedef std::integral_constant<bool, value> type;
        };
    };

    template<typename T>
    void ostream_insert(std::ostream& o, const T& t,
        typename std::enable_if<meta::has_insert_op<T>::value>::type* = 0)
    {
        o << t;
    }

    template<typename T>
    void ostream_insert(std::ostream& o, const T& t,
        typename std::enable_if<!meta::has_insert_op<T>::value>::type* = 0)
    {
        o << "???";
    }

    template<typename T>
    void istream_extract(std::istream& o, T& t,
        typename std::enable_if<meta::has_extract_op<T>::value>::type* = 0)
    {
        o >> t;
    }

    template<typename T>
    void istream_extract(std::istream& o, T& t,
        typename std::enable_if<!meta::has_extract_op<T>::value>::type* = 0)
    {
    }

    template<typename From, typename To>
    typename std::enable_if<std::is_convertible<From, To>::value, To>::type
    cast(const From& from)
    {
        return To(from);
    }

    template<typename From, typename To>
    typename std::enable_if<!std::is_convertible<From, To>::value, To>::type
    cast(const From& from)
    {
        if (!meta::has_insert_op<From>::value
            || !meta::has_extract_op<To>::value)
        {
            throw std::runtime_error("unable to cast");
        }

        To result;
        std::stringstream ss;

        ostream_insert(ss, from);
        istream_extract(ss, result);

        return result;
    }

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
        template<typename T>
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
                ostream_insert(o, m_var);
            }
        };

        const var_impl_base* m_var = nullptr;

    public:
        /*
         * view class for ostream insertion operator support
         */
        class view
        {
            friend std::ostream& operator<< (std::ostream& o, const gg::var::view& vw);

            const var& m_var;

        public:
            view(const var& var) : m_var(var) {}
            view(const view& vw) : m_var(vw.m_var) {}
            ~view() {}
        };

        inline friend std::ostream& operator<< (std::ostream& o, const gg::var::view& vw)
        {
            if (vw.m_var.m_var == nullptr) o << "(empty)";
            else vw.m_var.m_var->extract_to(o);
            return o;
        }

        /*
         * var public method implementations
         */
        var() {}

        var(const var& v)
        {
            if (v.m_var != nullptr) m_var = v.m_var->clone();
        }

        var(var&& v)
        {
            std::swap(m_var, v.m_var);
        }

        template<typename T>
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

        var& operator= (const var& v)
        {
            if (m_var != nullptr) { delete m_var; m_var = nullptr; }
            if (v.m_var != nullptr) m_var = v.m_var->clone();

            return *this;
        }

        var& operator= (var&& v)
        {
            std::swap(m_var, v.m_var);

            return *this;
        }

        template<typename T>
        var& operator= (const T& t)
        {
            if (m_var != nullptr) delete m_var;
            m_var = new var_impl<T>(t);

            return *this;
        }

        template<typename T>
        const T& get() const
        {
            if (m_var == nullptr)
                throw std::runtime_error("casting empty var");

            if (m_var->get_type() != typeid(T))
                throw std::runtime_error("casting var to different type");

            return *static_cast<const T*>(m_var->get_ptr());
        }

        template<typename T>
        operator T() const
        {
            return this->get<T>();
        }

        template<typename T,
            typename = typename std::enable_if<!std::is_same<T, var>::value>::type >
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

        template<typename T,
            typename = typename std::enable_if<std::is_same<T, var>::value>::type >
        var cast() const
        {
            return var(*this);
        }

        view to_stream() const
        {
            return view(*this);
        }

        std::string to_string() const
        {
            std::stringstream ss;
            ss << to_stream();
            return ss.str();
        }

        const std::type_info& get_type() const
        {
            if (m_var != nullptr)
                return m_var->get_type();
            else
                return typeid(void);
        }

        bool is_empty() const
        {
            return (m_var == nullptr);
        }
    };

    typedef std::vector<var> varlist;

    std::ostream& operator<< (std::ostream& o, const varlist& vl);

    template<typename = void>
    class function
    {
    public:
        function() = delete;
        ~function() = delete;

        template<typename R, typename... Args>
        static R invoke(std::function<R(Args...)> func, varlist vl);

        template<typename R>
        static R invoke(std::function<R()> func, varlist vl)
        {
            if (vl.size() > 0)
                throw std::runtime_error("too long argument list");

            return func();
        }

        template<typename R, typename Arg0, typename... Args>
        static R invoke(std::function<R(Arg0, Args...)> func, varlist vl)
        {
            if (vl.size() == 0)
                throw std::runtime_error("too short argument list");

            Arg0 arg0 = vl[0].cast<Arg0>();
            vl.erase(vl.begin());
            std::function<R(Args... args)> lambda =
                [=](Args... args) -> R { return func(arg0, args...); };

            return invoke(lambda, vl);
        }

        template<typename R, typename... Args>
        static R invoke(R(*func)(Args...))
        {
            return invoke(std::function<R(Args...)>(func));
        }

        template<typename F>
        static typename std::result_of<meta::get_signature<F>>::type
        invoke(F func)
        {
            return invoke(std::function<meta::get_signature<F>>(func));
        }
    };

    template<typename R, typename... Args>
    class function<R(Args...)>
    {
        std::function<R(Args...)> m_func;

    public:
        function() {}
        function(const gg::function<R(Args...)>& func) : m_func(func.m_func) {}
        function(gg::function<R(Args...)>&& func) : m_func(std::move(func.m_func)) {}
        ~function() {}

        template<typename F>
        function(F func) : m_func(func) {}
        function(R(*func)(Args...)) : m_func(func) {}
        function(std::function<R(Args...)> func) : m_func(func) {}

        template<typename F>
        function<R(Args...)>& operator= (F func) { m_func = func; return *this; }
        function<R(Args...)>& operator= (R(*func)(Args...)) { m_func = func; return *this; }
        function<R(Args...)>& operator= (std::function<R(Args...)> func) { m_func = func; return *this; }

        function& operator= (const gg::function<R(Args...)>& func) { m_func = func.m_func; return *this; }
        function& operator= (gg::function<R(Args...)>&& func) { m_func = std::move(func.m_func); return *this; }

        R operator() (Args... args) const { return m_func(std::forward<Args>(args)...); }
        R invoke(varlist vl) const { return gg::function<>::invoke(m_func, vl); }
    };

    class dynamic_function
    {
        gg::function<var(varlist)> m_func;

        template<typename R, typename... Args>
        static gg::function<var(varlist)> convert(gg::function<R(Args...)> func)
        {
            return ([=](varlist vl)->var { return func.invoke(vl); });
        }

        template<typename... Args>
        static gg::function<var(varlist)> convert(gg::function<void(Args...)> func)
        {
            return ([=](varlist vl)->var { func.invoke(vl); return var(); });
        }

    public:
        dynamic_function() {}
        dynamic_function(const dynamic_function& func) : m_func(func.m_func) {}
        dynamic_function(dynamic_function&& func) : m_func(std::move(func.m_func)) {}
        ~dynamic_function() {}

        template<typename R, typename... Args>
        dynamic_function(gg::function<R(Args...)> func)
         : m_func(convert(func)) {}

        template<typename R, typename... Args>
        dynamic_function(std::function<R(Args...)> func)
         : m_func(convert( gg::function<R(Args...)>(func) )) {}

        template<typename R, typename... Args>
        dynamic_function(R(*func)(Args...))
         : m_func(convert( gg::function<R(Args...)>(func) )) {}

        template<typename F>
        dynamic_function(F func)
         : m_func(convert( gg::function<meta::get_signature<F>>(func) )) {}

        dynamic_function& operator= (const dynamic_function& func) { m_func = func.m_func; return *this; }
        dynamic_function& operator= (dynamic_function&& func) { m_func = std::move(func.m_func); return *this; }

        var operator() (varlist vl) const { return m_func(vl); }
    };
};

#endif // GG_CORE_HPP_INCLUDED
