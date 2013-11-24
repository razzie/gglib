#ifndef C_EXPRESSION_HPP_INCLUDED
#define C_EXPRESSION_HPP_INCLUDED

#include "gg/expression.hpp"

namespace gg
{
    class c_expression : public expression
    {
    public:
        c_expression(std::string expr, bool auto_complete = false);
        c_expression(const expression& e);
        c_expression(const c_expression& e);
        c_expression(c_expression&& e);
        ~c_expression();

        c_expression& operator= (const c_expression& e);
        c_expression& operator= (c_expression&& e);

        std::string get_name() const;
        std::string get_expression() const;
        c_expression* get_parent();
        const c_expression* get_parent() const;
        std::list<expression_ptr>& get_children();
        const std::list<expression_ptr>& get_children() const;
        bool is_leaf() const;
        bool is_empty() const;

        void set_name(std::string name);
        void add_child(expression& e);
        void remove_child(std::list<expression_ptr>::iterator& it);

        void for_each(std::function<void(expression&)>);
        void for_each(std::function<void(const expression&)>) const;

        /*inline friend std::ostream& operator<< (std::ostream& o, const c_expression& e)
        {
            e.print(0,o);
            return o;
        }*/

    protected:
        c_expression(c_expression* parent, std::string expr, bool auto_complete);
        void print(uint32_t level, std::ostream& o) const;
        void get_expression(std::string&) const;

    private:
        c_expression* m_parent;
        std::string m_name;
        std::list<expression_ptr> m_children;
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
