#ifndef GG_EXPRESSION_HPP_INCLUDED
#define GG_EXPRESSION_HPP_INCLUDED

#include <string>
#include <vector>
//#include <exception>
//#include "gg/types.hpp"

namespace gg
{
    class expression
    {
        expression* m_parent;
        std::string m_name;
        std::string m_expr;
        std::vector<expression*> m_children;

    protected:
        expression(expression* parent, std::string expr);

    public:
        expression(std::string expr);
        ~expression();

        const expression* get_parent() const;
        std::string get_name() const;
        std::string get_expression() const;
        std::vector<expression*> get_children() const;
    };
};

#endif // GG_EXPRESSION_HPP_INCLUDED
