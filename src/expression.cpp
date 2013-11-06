#include <algorithm>
#include <cctype>
#include "expression.hpp"
#include "gg/util.hpp"

using namespace gg;


expression::expression(expression* parent, std::string expr)
 : m_parent(parent)
{
    expr = util::trim(expr);

    if (expr.size() == 0) return;

    int open_brackets = 0;
    int dbl_apost_cnt = 0;
    auto expr_begin = expr.begin();
    enum
    {
        EXPR_NONE,
        EXPR_FOUND,
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

        if (*it == '"')
        {
            ++dbl_apost_cnt;
            it = expr.erase(it) - 1;
            continue;
        }

        if (dbl_apost_cnt > 2)
            throw expression_error("too many \" marks");

        if (dbl_apost_cnt == 1) // string mode
            continue;

        if (dbl_apost_cnt == 2 && expr_mode == EXPR_INCOMPLETE && !isspace(*it))
            throw expression_error("character outside of \" marks");

        if (dbl_apost_cnt == 0 && expr_mode == EXPR_INCOMPLETE && isspace(*it))
            throw expression_error("expression with spaces should be placed between \" marks");

        if (expr_mode == EXPR_FOUND && !isspace(*it)) expr_mode = EXPR_INCOMPLETE;

        if (*it == '(')
        {
            ++open_brackets;
            if (open_brackets == 1 && expr_mode == EXPR_NONE)
            {
                expr_mode = EXPR_FOUND;
                expr_begin = it + 1;
                m_name = util::trim( std::string(expr.begin(), it) );
                continue;
            }
            continue;
        }

        if (*it == ')')
        {
             --open_brackets;
            if (open_brackets < 0)
                throw expression_error("invalid use of )");
            else if (open_brackets == 0 &&
                     (expr_mode == EXPR_FOUND || expr_mode == EXPR_INCOMPLETE))
            {
                if (it == expr_begin)
                    new expression (this, "");
                else
                    new expression(this, std::string(expr_begin, it));
                expr_mode = EXPR_COMPLETE;
                continue;
            }
            continue;
        }

        if (*it == ',')
        {
            if (open_brackets == 0)
                throw expression_error(", found before expression");
            else if (open_brackets == 1)
            {
                if (it == expr_begin)
                    new expression (this, "");
                else
                    new expression(this, std::string(expr_begin, it));
                expr_begin = it + 1;
                dbl_apost_cnt = 0;
                continue;
            }
            continue;
        }

        if (expr_mode == EXPR_COMPLETE && !isspace(*it))
            throw expression_error("character found after expression: " + *it);
    }

    if (dbl_apost_cnt == 1)
        throw expression_error("missing \"");

    if (open_brackets > 0)
        throw expression_error("missing )");

    if (expr_mode == EXPR_NONE)
        this->m_name = expr;

    this->m_is_leaf = (expr_mode == EXPR_NONE);

    if (parent != nullptr) parent->m_children.push_back(expression_ptr(this));
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
    for (uint32_t i = 0; i < level; ++i) o << "  ";
    o << this->get_expression() << std::endl;
    std::for_each(m_children.cbegin(), m_children.cend(), [&](expression_ptr e) { e->print(level+1, o); });
}

bool expression::is_leaf() const
{
    return m_is_leaf;
}

void expression::for_each(std::function<void(expression&)> func)
{
    func(*this);
    std::for_each(m_children.begin(), m_children.end(), [&](expression_ptr e) { func(*e); });
}

void expression::for_each(std::function<void(const expression&)> func) const
{
    func(*this);
    std::for_each(m_children.cbegin(), m_children.cend(), [&](const expression_ptr e) { func(*e); });
}

std::string& expression::get_name()
{
    return m_name;
}

std::string expression::get_name() const
{
    return m_name;
}

std::string expression::get_expression() const
{
    std::string expr;
    this->get_expression(expr);
    return expr;
}

void expression::get_expression(std::string& expr) const
{
    if (m_is_leaf)
    {
        //if (util::contains_space(expr)) expr += '"' + m_name + '"';
        if (!util::is_numeric(expr)) expr += '"' + m_name + '"';
        else expr += m_name;
    }
    else
    {
        expr += m_name;
        expr += '(';
        auto it = m_children.cbegin(), end = m_children.cend();
        for (; it != end; ++it)
        {
            (*it)->get_expression(expr);
            if ((it + 1) != end) expr += ", ";
        }
        expr += ')';
    }
}

expression* expression::get_parent()
{
    return m_parent;
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
 : m_error(std::string("expression error: ") + error)
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
