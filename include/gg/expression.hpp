#ifndef GG_EXPRESSION_HPP_INCLUDED
#define GG_EXPRESSION_HPP_INCLUDED

#include <string>
#include <list>
#include <memory>

namespace gg
{
    class expression
    {
    public:
        typedef std::shared_ptr<expression> expression_ptr;

        static expression* create(std::string expr, bool auto_complete = false);
        virtual ~expression() {};

        virtual void set_name(std::string name) = 0;
        virtual std::string get_name() const = 0;
        virtual std::string get_expression() const = 0;

        virtual expression* get_parent() = 0;
        virtual const expression* get_parent() const = 0;

        virtual std::list<expression_ptr>& get_children() = 0;
        virtual const std::list<expression_ptr>& get_children() const = 0;
        virtual void add_child(expression& e) = 0;
        virtual void remove_child(std::list<expression_ptr>::iterator& it) = 0;
        virtual bool is_leaf() const = 0;
        virtual bool is_empty() const = 0;

        virtual void for_each(std::function<void(expression&)>) = 0;
        virtual void for_each(std::function<void(const expression&)>) const = 0;

        friend std::ostream& operator<< (std::ostream& o, const expression& e);
    };

    class expression_error : public std::exception
    {
    public:
        virtual ~expression_error() noexcept {};
        virtual const char* what() const noexcept = 0;
    };
};

#endif // GG_EXPRESSION_HPP_INCLUDED
