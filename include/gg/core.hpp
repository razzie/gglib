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

    namespace meta
    {
        template<bool B, class T = void>
        using enable_if_t = typename std::enable_if<B,T>::type;


        template<class T>
        struct remove_class { };

        template<class C, class R, class... Args>
        struct remove_class<R(C::*)(Args...)> { using type = R(Args...); };

        template<class C, class R, class... Args>
        struct remove_class<R(C::*)(Args...) const> { using type = R(Args...); };

        template<class C, class R, class... Args>
        struct remove_class<R(C::*)(Args...) volatile> { using type = R(Args...); };

        template<class C, class R, class... Args>
        struct remove_class<R(C::*)(Args...) const volatile> { using type = R(Args...); };


        template<class T>
        struct get_signature_impl { using type = typename remove_class<
            decltype(&std::remove_reference<T>::type::operator())>::type; };

        template<class R, class... Args>
        struct get_signature_impl<R(Args...)> { using type = R(Args...); };

        template<class R, class... Args>
        struct get_signature_impl<R(&)(Args...)> { using type = R(Args...); };

        template<class R, class... Args>
        struct get_signature_impl<R(*)(Args...)> { using type = R(Args...); };

        template<class T>
        using get_signature = typename get_signature_impl<T>::type;


        namespace sfinae
        {
            class yes { char c[1]; };
            class no  { char c[2]; };
        };

        template<class T> T& lvalue_of_type();
        template<class T> T  rvalue_of_type();

        template<class T>
        struct has_insert_op
        {
            template<class U>
            static sfinae::yes test(char(*)[sizeof(
                lvalue_of_type<std::ostream>() << rvalue_of_type<U>()
            )]);

            template<class U>
            static sfinae::no test(...);

            enum { value = ( sizeof(sfinae::yes) == sizeof(test<T>(0)) ) };
            typedef std::integral_constant<bool, value> type;
        };

        template<class T>
        struct has_extract_op
        {
            template<class U>
            static sfinae::yes test(char(*)[sizeof(
                lvalue_of_type<std::istream>() >> lvalue_of_type<U>()
            )]);

            template<class U>
            static sfinae::no test(...);

            enum { value = ( sizeof(sfinae::yes) == sizeof(test<T>(0)) ) };
            typedef std::integral_constant<bool, value> type;
        };
    };

    template<class T>
    void ostream_insert(std::ostream& o, const T& t,
        meta::enable_if_t<meta::has_insert_op<T>::value>* = 0)
    {
        o << t;
    }

    template<class T>
    void ostream_insert(std::ostream& o, const T& t,
        meta::enable_if_t<!meta::has_insert_op<T>::value>* = 0)
    {
        o << "???";
    }

    template<class T>
    void istream_extract(std::istream& o, T& t,
        meta::enable_if_t<meta::has_extract_op<T>::value>* = 0)
    {
        o >> t;
    }

    template<class T>
    void istream_extract(std::istream& o, T& t,
        meta::enable_if_t<!meta::has_extract_op<T>::value>* = 0)
    {
    }

    template<class From, class To>
    meta::enable_if_t<std::is_convertible<From, To>::value, To>
    cast(const From& from)
    {
        return To(from);
    }

    template<class From, class To>
    meta::enable_if_t<!std::is_convertible<From, To>::value, To>
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
            var_impl(T t) : m_var(t), m_type(&typeid(T)) {}
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
        class view
        {
            friend std::ostream& operator<< (std::ostream& o, const gg::var::view& vw);
            const var& m_var;

        public:
            view(const var& var);
            view(const view& vw);
            ~view();
        };

        friend std::ostream& operator<< (std::ostream& o, const gg::var::view& vw);

        var();
        var(const var& v);
        var(var&& v);
        ~var();
        var& operator= (const var& v);
        var& operator= (var&& v);

        template<class T>
        var(T t) { m_var = new var_impl<T>(t); }
        var(const char* str) : var(std::string(str)) {}

        template<class T>
        var& operator= (const T& t)
        {
            if (m_var != nullptr) delete m_var;
            m_var = new var_impl<T>(t);
            return *this;
        }

        view to_stream() const;
        std::string to_string() const;
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
    };

    typedef std::vector<var> varlist;

    std::ostream& operator<< (std::ostream& o, const varlist& vl);

    template<class>
    class function;

    template<class R, class... Args>
    class function<R(Args...)>
    {
        std::function<R(Args...)> m_func;

        template<class _R, class... _Args>
        static _R _invoke(std::function<_R(_Args...)> func, varlist vl);

        template<class _R>
        static _R _invoke(std::function<_R()> func, varlist vl)
        {
            if (vl.size() > 0)
                throw std::runtime_error("too long argument list");

            return func();
        }

        template<class _R, class _Arg0, class... _Args>
        static _R _invoke(std::function<_R(_Arg0, _Args...)> func, varlist vl)
        {
            if (vl.size() == 0)
                throw std::runtime_error("too short argument list");

            _Arg0 arg0 = vl[0].cast<_Arg0>();
            vl.erase(vl.begin());
            std::function<_R(_Args... args)> lambda =
                [=](_Args... args) -> _R { return func(arg0, args...); };

            return _invoke(lambda, vl);
        }

        template<class _R, class... _Args>
        static _R _invoke(_R(*func)(_Args...))
        {
            return _invoke(std::function<_R(_Args...)>(func));
        }

        template<class _F>
        static typename std::result_of<meta::get_signature<_F>>::type
        _invoke(_F func)
        {
            return _invoke(std::function<meta::get_signature<_F>>(func));
        }

    public:
        using type = R(Args...);
        using return_type = R;

        function() {}
        function(const gg::function<R(Args...)>& func) : m_func(func.m_func) {}
        function(gg::function<R(Args...)>&& func) : m_func(std::move(func.m_func)) {}
        ~function() {}

        template<class F>
        function(F func) : m_func(func) {}
        function(R(*func)(Args...)) : m_func(func) {}
        function(std::function<R(Args...)> func) : m_func(func) {}

        template<class F>
        function<R(Args...)>& operator= (F func) { m_func = func; return *this; }
        function<R(Args...)>& operator= (R(*func)(Args...)) { m_func = func; return *this; }
        function<R(Args...)>& operator= (std::function<R(Args...)> func) { m_func = func; return *this; }

        function& operator= (const gg::function<R(Args...)>& func) { m_func = func.m_func; return *this; }
        function& operator= (gg::function<R(Args...)>&& func) { m_func = std::move(func.m_func); return *this; }

        R operator() (Args... args) const { return m_func(std::forward<Args>(args)...); }
        R invoke(varlist vl) const { return _invoke(m_func, vl); }
        operator bool() const { return static_cast<bool>(m_func); }
        operator std::function<R(Args...)>() const { return m_func; }
    };

    class dynamic_function
    {
        gg::function<var(varlist)> m_func;

        template<class R, class... Args>
        static gg::function<var(varlist)> convert(gg::function<R(Args...)> func)
        {
            return ([=](varlist vl)->var { return func.invoke(vl); });
        }

        template<class... Args>
        static gg::function<var(varlist)> convert(gg::function<void(Args...)> func)
        {
            return ([=](varlist vl)->var { func.invoke(vl); return var(); });
        }

    public:
        dynamic_function() {}
        dynamic_function(const dynamic_function& func) : m_func(func.m_func) {}
        dynamic_function(dynamic_function&& func) : m_func(std::move(func.m_func)) {}
        ~dynamic_function() {}

        template<class R, class... Args>
        dynamic_function(gg::function<R(Args...)> func)
         : m_func(convert(func)) {}

        template<class R, class... Args>
        dynamic_function(std::function<R(Args...)> func)
         : m_func(convert( gg::function<R(Args...)>(func) )) {}

        template<class R, class... Args>
        dynamic_function(R(*func)(Args...))
         : m_func(convert( gg::function<R(Args...)>(func) )) {}

        template<class F>
        dynamic_function(F func)
         : m_func(convert( gg::function<meta::get_signature<F>>(func) )) {}

        dynamic_function& operator= (const dynamic_function& func) { m_func = func.m_func; return *this; }
        dynamic_function& operator= (dynamic_function&& func) { m_func = std::move(func.m_func); return *this; }

        var operator() (varlist vl) const { return m_func(vl); }
        operator bool() const { return static_cast<bool>(m_func); }
        operator gg::function<var(varlist)>() const { return m_func; }
        operator std::function<var(varlist)>() const { return m_func; }
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

    template<class T, class =
        meta::enable_if_t<std::is_base_of<reference_counted,T>::value>>
    T* grab(T* o)
    {
        o->grab();
        return o;
    }

    template<class T, class =
        meta::enable_if_t<std::is_base_of<reference_counted,T>::value>>
    const T* grab(const T* o)
    {
        o->grab();
        return o;
    }

    template<class T, class =
        meta::enable_if_t<std::is_base_of<reference_counted,T>::value>>
    T* drop(T* o)
    {
        uint32_t refc = o->get_ref_count();
        o->drop();
        return (refc == 1) ? nullptr : o;
    }

    template<class T, class =
        meta::enable_if_t<std::is_base_of<reference_counted,T>::value>>
    const T* drop(const T* o)
    {
        uint32_t refc = o->get_ref_count();
        o->drop();
        return (refc == 1) ? nullptr : o;
    }

    template<class T, class =
        meta::enable_if_t<std::is_base_of<reference_counted,T>::value>>
    class auto_drop
    {
        T* m_obj;

    public:
        auto_drop(T* o) : m_obj(o) {}
        ~auto_drop() { m_obj->drop(); }
        operator T*() { return m_obj; }
    };

    template<class T>
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

        T get() const
        {
            if (!m_valid) throw std::runtime_error("getting value of invalid optional<>");
            return m_val;
        }

        operator T() const { return get(); }

        bool is_valid() const { return m_valid; }
        void set_valid(bool valid) { m_valid = valid; }
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

    struct color
    {
        uint8_t R, G, B;
    };
};

#endif // GG_CORE_HPP_INCLUDED
