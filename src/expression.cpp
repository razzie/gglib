#include <algorithm>
//#include <cctype>
//#include <stdexcept>
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
            throw expression_error("invalid use of ')'");

        if (open_brackets == 1 && !expr_mode)
        {
            expr_mode = true;
            expr_begin = it + 1;
            m_name = std::string(begin, it);
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
        throw expression_error("'\"' expected");

    if (open_brackets > 0)
        throw expression_error("missing ')'");

    if (begin == expr_begin)
        m_name = expr;

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
