#ifndef C_EXPRESSION_HPP_INCLUDED
#define C_EXPRESSION_HPP_INCLUDED

#include <list>
#include "gg/expression.hpp"

namespace gg
{
    class c_expression : public expression
    {
    public:
        c_expression(std::string expr, bool auto_complete = false);
        c_expression(const c_expression& e);
        c_expression(c_expression&& e);
        ~c_expression();
        void set_name(std::string name);
        std::string get_name() const;
        std::string get_expression() const;
        void set_as_expression();
        bool is_root() const;
        bool is_leaf() const;
        enumerator<expression*> get_children();
        enumerator<expression*> get_children() const;
        void for_each(std::function<void(expression&)>);
        void for_each(std::function<void(const expression&)>) const;

    protected:
        c_expression(c_expression* parent, std::string expr, bool auto_complete);

    private:
        std::string m_name;
        std::list<grab_ptr<expression*, true>> m_children;
        bool m_expr;
        bool m_root;
    };

    class c_expression_error : public expression_error
    {
        std::string m_error;

    public:
        c_expression_error(std::string) noexcept;
        c_expression_error(const c_expression_error&) noexcept;
        c_expression_error& operator= (const c_expression_error&) noexcept;
        virtual ~c_expression_error() noexcept;
        virtual const char* what() const noexcept;
    };
};

#endif // C_EXPRESSION_HPP_INCLUDED
