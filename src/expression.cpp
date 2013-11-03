#include <algorithm>
#include <cctype>
#include <stdexcept>
#include "expression.hpp"

using namespace gg;

expression::expression(expression* parent, std::string expr)
 : m_parent(parent)
{
    int open_brackets = 0;
    bool string_mode = false;
    bool skip_next = false;
    auto begin = expr.begin(), end = expr.end();

    for (auto it = begin; it != end; ++it)
    {
        if (*it == '\\') { skip_next = true; continue; }
        if (skip_next) { skip_next = false; continue; }

        if (*it == '"') string_mode = !string_mode;
        if (string_mode) continue;

        if (*it == '(') ++open_brackets;
        if (*it == '(') --open_brackets;
    }
}

expression::expression(std::string expr)
 : expression(nullptr, expr)
{
}

expression::~expression()
{
    for_each(m_children.begin(), m_children.end(), [](expression* e){ delete e; });
}

std::string expression::get_name() const
{
    return m_name;
}

std::string expression::get_expression() const
{
    return m_expr;
}

std::vector<expression*> expression::get_children() const
{
    return m_children;
}
