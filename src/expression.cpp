#include <algorithm>
#include <cctype>
#include "expression.hpp"
#include "gg/util.hpp"

using namespace gg;


static bool is_valid_leaf_expr(std::string expr)
{
    if (expr.empty() || util::is_numeric(expr)) return true;

    int dbl_apost_cnt = 0;
    auto it = expr.begin(), end = expr.end();

    for (; it != end; ++it)
    {
        if ((*it == '\\') && (it+1 != expr.end()) && (*(it+1) == '"'))
        {
            ++it; // it will jump to ", but we want to skip it
            continue;
        }

        if (*it == '"')
        {
            ++dbl_apost_cnt;
            continue;
        }

        if (dbl_apost_cnt == 1)
            continue;

        if (dbl_apost_cnt > 2)
            return false;

        if (dbl_apost_cnt == 2 && !std::isspace(*it))
            return false;
    }

    if (dbl_apost_cnt == 0 && util::contains_space(expr))
        return false;

    return true;
}

static void make_valid_leaf_expr(std::string& expr)
{
    if (expr.empty() || util::is_numeric(expr)) return;

    for (auto it = expr.begin(); it != expr.end(); ++it)
    {
        if ((*it == '\\') && (it+1 != expr.end()) && (*(it+1) == '"'))
        {
            it = expr.erase(it); // it will jump to ", but we want to skip it
            continue;
        }

        if (*it == '"')
        {
            it = expr.erase(it) - 1;
            continue;
        }
    }
}


expression::expression(expression* parent, std::string orig_expr, bool auto_complete)
 : m_parent(parent)
{
    std::string expr = util::trim(orig_expr);

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

        if (dbl_apost_cnt % 2) continue;// string mode

        if (*it == '(')
        {
            ++open_brackets;

            if (open_brackets == 1 && expr_mode == EXPR_NONE)
            {
                std::string name = std::string(expr.begin(), it);
                if (!auto_complete && !is_valid_leaf_expr(name))
                    throw expression_error("invalid expression: " + name);

                expr_mode = EXPR_INCOMPLETE;
                expr_begin = it + 1;
                make_valid_leaf_expr(name);
                this->set_name(name);
                continue;
            }

            continue;
        }

        if (*it == ')')
        {
            --open_brackets;

            if (open_brackets < 0)
            {
                if (auto_complete) { it = expr.erase(it) - 1; continue; }
                throw expression_error("invalid use of )");
            }

            else if (open_brackets == 0 && expr_mode == EXPR_INCOMPLETE)
            {
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
                new expression(this, std::string(expr_begin, it), auto_complete);
                expr_begin = it + 1;
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

    if (auto_complete)
    {
        if (expr_mode == EXPR_INCOMPLETE)
        {
            std::string child_expr = std::string(expr_begin, expr.end());
            if (dbl_apost_cnt % 2) child_expr += '"';
            if (open_brackets > 0) for (int i = open_brackets; i > 0; --i) child_expr += ')';
            new expression(this, child_expr, auto_complete);
        }
        else if (expr_mode == EXPR_NONE)
        {
            make_valid_leaf_expr(expr);
            this->set_name(expr);
        }
    }
    else
    {
        if (dbl_apost_cnt % 2)
            throw expression_error("missing \"");

        if (open_brackets > 0)
            throw expression_error("missing )");

        if (expr_mode == EXPR_NONE)
        {
            if (!is_valid_leaf_expr(expr))
                throw expression_error("invalid expression: " + expr);

            make_valid_leaf_expr(expr);
            this->set_name(expr);
        }
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
{
    if (m_parent != nullptr)
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

expression& expression::operator= (const expression& e)
{
    m_name = e.m_name;
    m_children.clear();
    m_children.insert(m_children.begin(), e.m_children.begin(), e.m_children.end());

    return *this;
}

expression& expression::operator= (expression&& e)
{
    m_name = std::move(e.m_name);
    m_children.clear();
    m_children = std::move(e.m_children);

    std::for_each(e.m_children.begin(), e.m_children.end(),
        [&](expression_ptr child)
        {
            child->m_parent = this;
        });

    return *this;
}

void expression::print(uint32_t level, std::ostream& o) const
{
    for (uint32_t i = 0; i < level; ++i) o << "  ";
    o << this->get_expression() << std::endl;
    std::for_each(m_children.cbegin(), m_children.cend(), [&](expression_ptr e) { e->print(level+1, o); });
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
    if (this->is_leaf())
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
            if (std::next(it, 1) != end) expr += ", ";
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

std::list<expression::expression_ptr>& expression::get_children()
{
    return m_children;
}

const std::list<expression::expression_ptr>& expression::get_children() const
{
    return m_children;
}

bool expression::is_leaf() const
{
    return m_children.empty();
}

bool expression::is_empty() const
{
    return (this->is_leaf() && util::trim(m_name).empty());
}

void expression::set_name(std::string name)
{
    if (!this->is_leaf() && util::contains_space(name))
        throw expression_error("non-leaf expressions cannot contain space");

    m_name = util::trim(name);
}

void expression::add_child(expression e)
{
    expression* expr = new expression(e);
    expr->m_parent = this;
    m_children.push_back(expression_ptr( expr ));
}

void expression::remove_child(std::list<expression_ptr>::iterator& it)
{
    if (it == m_children.end()) return;

    (*it)->m_parent = nullptr;
    it = std::prev(m_children.erase(it));
}

void expression::for_each(std::function<void(expression&)> func)
{
    func(*this);
    std::for_each(m_children.begin(), m_children.end(), [&](expression_ptr e) { e->for_each(func); });
}

void expression::for_each(std::function<void(const expression&)> func) const
{
    func(*this);
    std::for_each(m_children.cbegin(), m_children.cend(), [&](const expression_ptr e) { e->for_each(func); });
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
