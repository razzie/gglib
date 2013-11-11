#include <algorithm>
#include <cctype>
#include "expression.hpp"
#include "gg/util.hpp"

using namespace gg;


static bool is_valid_string_expr(std::string expr)
{
    if (expr.empty()) return false;

    int dbl_apost_cnt = 0;
    auto it = expr.begin(), end = expr.end();

    for (; it != end; ++it)
    {
        if (*it == '"') { ++dbl_apost_cnt; continue; }
        if (dbl_apost_cnt == 1) continue;
        if (dbl_apost_cnt > 2) return false;
        if (!std::isspace(*it)) return false;
    }

    return true;
}

static void make_valid_string_expr(std::string& expr)
{
    if (expr.empty()) { expr = "\"\""; return; }

    std::string tmp = "\"";
    std::for_each(expr.begin(), expr.end(), [&](char c){ if (c != '"') tmp += c; });
    tmp += "\"";

    std::swap(tmp, expr);
    return;
}


expression::expression(expression* parent, std::string orig_expr, bool auto_complete)
 : m_parent(parent)
{
    std::string expr = util::trim(orig_expr);
    if (expr.size() == 0) return;

    int open_brackets = 0;
    int dbl_apost_cnt = 0;
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
        if (expr_mode == EXPR_NONE) // probably we're a leaf
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
        }
        else
        {
            if ((*it == '\\') && (it+1 != expr.end()) && (*(it+1) == '"'))
            {
                ++it; // it will jump to ", and then we skip it
                continue;
            }

            if (*it == '"')
            {
                ++dbl_apost_cnt;
                continue;
            }
        }

        if (dbl_apost_cnt % 2) continue;// string mode

        if (*it == '(')
        {
            ++open_brackets;

            if (open_brackets == 1 && expr_mode == EXPR_NONE)
            {
                expr_mode = EXPR_INCOMPLETE;
                expr_begin = it + 1;
                m_name = util::trim( std::string(expr.begin(), it) );
                if (util::contains_space(m_name))
                    throw expression_error("expression names cannot contain space");
                continue;
            }

            continue;
        }

        if (*it == ')')
        {
            --open_brackets;

            if (open_brackets < 0)
                throw expression_error("invalid use of )");

            else if (open_brackets == 0 && expr_mode == EXPR_INCOMPLETE)
            {
                if (it == expr_begin)
                    new expression (this, "", auto_complete);
                else
                    new expression(this, std::string(expr_begin, it), auto_complete);
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
                    new expression (this, "", auto_complete);
                else
                    new expression(this, std::string(expr_begin, it), auto_complete);

                expr_begin = it + 1;
                //expr_mode = EXPR_INCOMPLETE;
                dbl_apost_cnt = 0;
                continue;
            }

            continue;
        }

        if (expr_mode == EXPR_COMPLETE && !std::isspace(*it))
        {
            if (auto_complete) { it = expr.erase(it) - 1; continue; }
            else throw expression_error("character found after expression");
        }
    }

    if (dbl_apost_cnt == 1)
    {
        if (auto_complete) expr += '"';
        else throw expression_error("missing \"");
    }

    if (open_brackets > 0)
    {
        if (auto_complete) expr += ')';
        else throw expression_error("missing )");
    }

    if (expr_mode == EXPR_NONE)
    {
        if (!util::is_numeric(expr) && !is_valid_string_expr(orig_expr))
        {
            if (auto_complete) make_valid_string_expr(expr);
            else throw expression_error("invalid string expression: " + orig_expr);
        }
        this->m_is_leaf = true;
        this->m_name = expr;
    }
    else
    {
        this->m_is_leaf = false;
    }

    if (parent != nullptr) parent->m_children.push_back(expression_ptr(this));
}

expression::expression(std::string expr, bool auto_complete)
 : expression(nullptr, expr, auto_complete)
{
}

expression::expression(const expression& e)
 : m_parent(e.m_parent)
 , m_name(e.m_name)
 , m_is_leaf(e.m_is_leaf)
{
    std::for_each(e.m_children.begin(), e.m_children.end(),
        [&](expression_ptr child)
        {
            m_children.push_back(expression_ptr( new expression(*child) ));
        });
}

expression::expression(expression&& e)
 : m_parent(e.m_parent)
 , m_name(std::move(e.m_name))
 , m_children(std::move(e.m_children))
 , m_is_leaf(e.m_is_leaf)
{
    if (m_parent)
    {
        std::for_each(m_parent->m_children.begin(), m_parent->m_children.end(),
            [&](expression_ptr child)
            {
                if (child.get() == &e)
                {
                    child = expression_ptr(this);
                }
            });
    }
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
    //return m_children.empty();
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
    if (is_leaf())
    {
        if (!util::is_numeric(m_name)) expr += '"' + m_name + '"';
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
