#include <algorithm>
#include <cctype>
//#include <stdexcept>
#include "expression.hpp"

using namespace gg;


expression::expression(expression* parent, std::string expr)
 : m_parent(parent)
{
    std::cout << "starting new expression; expr: " << expr << std::endl;

    if (expr.size() == 0) return;

    m_expr = expr;

    int open_brackets = 0;
    bool string_mode = false;
    auto expr_begin = expr.begin();
    enum
    {
        EXPR_NONE,
        EXPR_INCOMPLETE,
        EXPR_COMPLETE
    }
    expr_mode = EXPR_NONE;

    for (auto it = expr.begin(); it != expr.end(); ++it)
    {
        if ((*it == '\\') && (it+1 != expr.end()) && (*(it+1) == '"'))
        {
            it = expr.erase(it); // it will jump to ", but we want to skip it
            continue;
        }

        if (*it == '"') string_mode = !string_mode;
        if (string_mode) continue;

        if (*it == '(')
        {
            ++open_brackets;
            if (open_brackets == 1 && expr_mode == EXPR_NONE)
            {
                expr_mode = EXPR_INCOMPLETE;
                expr_begin = it + 1;
                m_name = std::string(expr.begin(), it);
                continue;
            }
            continue;
        }

        if (*it == ')')
        {
             --open_brackets;
            if (open_brackets < 0)
                throw expression_error("invalid use of ')'");
            else if (open_brackets == 0 && expr_mode == EXPR_INCOMPLETE)
            {
                if (it == expr_begin)
                    new expression (this, "");
                else
                    new expression(this, std::string(expr_begin, it-1));
                expr_mode = EXPR_COMPLETE;
                continue;
            }
            continue;
        }

        if (*it == ',')
        {
            if (open_brackets == 0)
                throw expression_error("',' found before expression");
            else if (open_brackets == 1)
            {
                if (it == expr_begin)
                    new expression (this, "");
                else
                    new expression(this, std::string(expr_begin, it-1));
                expr_begin = it + 1;
                continue;
            }
            continue;
        }

        if (expr_mode == EXPR_COMPLETE && !isspace(*it))
            throw expression_error("character found after expression: " + *it);
    }

    if (open_brackets > 0)
        throw expression_error("missing ')'");

    if (expr_mode == EXPR_NONE)
        m_name = expr;

    if (parent != nullptr) parent->m_children.push_back(expression_ptr(this));

    std::cout << "new expression created; name: " << m_name << ", expr: " << m_expr << std::endl;
}

expression::expression(std::string expr)
 : expression(nullptr, expr)
{
}

expression::~expression()
{
}

void expression::print(uint32_t level, std::ostream& o) const
{
    for (uint32_t i = 0; i < level; ++i) o << " ";
    o << m_expr << std::endl;
    for_each(m_children.begin(), m_children.end(), [&](expression_ptr e) { e->print(level+1, o); });
}

void expression::print(std::ostream& o) const
{
    this->print(0, o);
}

std::string expression::get_name() const
{
    return m_name;
}

std::string expression::get_expression() const
{
    return m_expr;
}

const expression* expression::get_parent() const
{
    return m_parent;
}

const std::vector<expression::expression_ptr>& expression::get_children() const
{
    return m_children;
}


expression_error::expression_error(std::string error) noexcept
 : m_error(error)
{
}

expression_error::expression_error(const expression_error& e) noexcept
 : m_error(e.m_error)
{
}

expression_error& expression_error::operator= (const expression_error& e) noexcept
{
    m_error = e.m_error;
    return *this;
}

expression_error::~expression_error() noexcept
{
}

const char* expression_error::what() const noexcept
{
    return m_error.c_str();
}
