#ifndef GG_EXPRESSION_HPP_INCLUDED
#define GG_EXPRESSION_HPP_INCLUDED

#include <string>
#include <list>
#include <memory>
#include <exception>
#include "gg/types.hpp"

namespace gg
{
    class expression final
    {
    public:
        typedef std::shared_ptr<expression> expression_ptr;

        expression(std::string expr, bool auto_complete = false);
        expression(const expression& e);
        expression(expression&& e);
        ~expression();

        expression& operator= (const expression& e);
        expression& operator= (expression&& e);

        std::string get_name() const;
        std::string get_expression() const;
        expression* get_parent();
        const expression* get_parent() const;
        std::list<expression_ptr>& get_children();
        const std::list<expression_ptr>& get_children() const;
        bool is_leaf() const;
        bool is_empty() const;

        void set_name(std::string name);
        void add_child(expression e);
        void remove_child(std::list<expression_ptr>::iterator& it);

        void for_each(std::function<void(expression&)>);
        void for_each(std::function<void(const expression&)>) const;

        inline friend std::ostream& operator<< (std::ostream& o, const expression& e)
        {
            e.print(0,o);
            return o;
        }

    protected:
        expression(expression* parent, std::string expr, bool auto_complete);
        void print(uint32_t level, std::ostream& o) const;
        void get_expression(std::string&) const;

    private:
        expression* m_parent;
        std::string m_name;
        std::list<expression_ptr> m_children;
    };

    class expression_error : public std::exception
    {
        std::string m_error;

    public:
        expression_error(std::string) noexcept;
        expression_error(const expression_error&) noexcept;
        expression_error& operator= (const expression_error&) noexcept;
        virtual ~expression_error() noexcept;
        virtual const char* what() const noexcept;
    };
};

#endif // GG_EXPRESSION_HPP_INCLUDED
