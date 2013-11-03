#include <algorithm>
//#include <cctype>
#include <stdexcept>
#include "expression.hpp"

using namespace gg;

expression::expression(expression* parent, std::string expr)
 : m_parent(parent)
{
    if (parent != nullptr) parent->m_children.push_back(expression_ptr(this));
    if (expr.size() == 0) return;

    m_expr = expr;

    auto begin = expr.begin(), end = expr.end();
    int open_brackets = 0;
    bool string_mode = false;
    bool skip_next = false;
    //bool expect_next = false;
    bool expr_mode = false;
    auto expr_begin = begin;

    for (auto it = begin; it != end; ++it)
    {
        if (*it == '\\') { skip_next = true; continue; }
        if (skip_next) { skip_next = false; continue; }

        if (*it == '"') string_mode = !string_mode;
        if (string_mode) continue;

        if (*it == '(') ++open_brackets;
        if (*it == '(') --open_brackets;

        if (open_brackets < 0)
            throw std::runtime_error("invalid use of ')'");

        if (open_brackets == 1 || !expr_mode)
        {
            expr_mode = true;
            expr_begin = it + 1;
            continue;
        }

        if ((open_brackets == 0 && expr_mode) || (*it == ','))
        {
            if (it == expr_begin)
                new expression (this, "");
            else
                new expression(this, std::string(expr_begin, it-1));

            if (*it == ',') expr_begin = it + 1;
            else expr_mode = false;

            continue;
        }
    }

    if (skip_next)
        throw std::runtime_error("'\"' expected");

    if (open_brackets > 0)
        throw std::runtime_error("missing ')'");

    if (begin != expr_begin)
        m_name = std::string(begin, expr_begin - 1);
    else
        m_name = expr;
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
    o << m_name << std::endl;
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
