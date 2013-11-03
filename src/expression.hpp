#ifndef GG_EXPRESSION_HPP_INCLUDED
#define GG_EXPRESSION_HPP_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include "gg/types.hpp"

namespace gg
{
    class expression final
    {
    public:
        typedef std::shared_ptr<expression> expression_ptr;

        expression(std::string expr);
        ~expression();

        std::string get_name() const;
        std::string get_expression() const;
        const expression* get_parent() const;
        const std::vector<expression_ptr>& get_children() const;
        void print(std::ostream& o = std::cout) const;

    protected:
        expression(expression* parent, std::string expr);
        void print(uint32_t level, std::ostream& o) const;

    private:
        expression* m_parent;
        std::string m_name;
        std::string m_expr;
        std::vector<expression_ptr> m_children;
    };
};

#endif // GG_EXPRESSION_HPP_INCLUDED
