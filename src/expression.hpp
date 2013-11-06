#ifndef GG_EXPRESSION_HPP_INCLUDED
#define GG_EXPRESSION_HPP_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include <exception>
#include "gg/types.hpp"

namespace gg
{
    class expression final
    {
    public:
        typedef std::shared_ptr<expression> expression_ptr;

        expression(std::string expr, bool repair_mode = false);
        expression(const expression&) = delete;
        expression(expression&&) = delete;
        ~expression();

        std::string& get_name();
        std::string get_name() const;
        std::string get_expression() const;
        expression* get_parent();
        const expression* get_parent() const;
        const std::vector<expression_ptr>& get_children() const;
        bool is_leaf() const;
        void for_each(std::function<void(expression&)>);
        void for_each(std::function<void(const expression&)>) const;

        inline friend std::ostream& operator<< (std::ostream& o, const expression& e)
        {
            e.print(0,o);
            return o;
        }

    protected:
        expression(expression* parent, std::string expr, bool repair_mode);
        void print(uint32_t level, std::ostream& o) const;
        void get_expression(std::string&) const;

    private:
        expression* m_parent;
        std::string m_name;
        std::vector<expression_ptr> m_children;
        bool m_is_leaf;
    };

    class expression_error : public std::exception
    {
        std::string m_error;

    public:
        expression_error(std::string) noexcept;
        expression_error(const expression_error&) noexcept;
        expression_error& operator= (const expression_error&) noexcept;
        virtual ~expression_error() noexcept;
        virtual const char* what() const noexcept;
    };
};

#endif // GG_EXPRESSION_HPP_INCLUDED
