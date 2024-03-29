#include <algorithm>
#include <cctype>
#include "c_scripteng.hpp"
#include "c_logger.hpp"
#include "scope_callback.hpp"
#include "gg/application.hpp"
#include "gg/stringutil.hpp"

using namespace gg;


c_script_engine::console_controller::console_controller(const c_script_engine* scripteng)
 : m_scripteng(scripteng)
{
    m_scripteng->get_app()->application::grab();
}

c_script_engine::console_controller::~console_controller()
{
    m_scripteng->get_app()->application::drop();
}

console::controller::exec_result
 c_script_engine::console_controller::exec(std::string& expr, console::output& out)
{
    if (!expr.empty() && expr[0] == '#')
        return exec_result::NO_EXEC;

    optional<var> r;

    try
    {
        c_expression e(expr);
        r = m_scripteng->parse_and_exec(expr, out);

        if (r && !r->is_empty() && out.is_empty() && !e.is_leaf())
        {
            out << r;
        }
    }
    catch (expression_error& e)
    {
        out << e.what();
        return exec_result::EXEC_FAIL;
    }

    return (r ? exec_result::EXEC_SUCCESS : exec_result::EXEC_FAIL);
}

void c_script_engine::console_controller::complete(std::string& expr, console::output& out)
{
    logger::scoped_hook __hook(out);
    out.set_color({100,100,100});
    m_scripteng->auto_complete_expr(expr, true);
}


bool c_script_engine::fn_name_comparator::operator() (const std::string& s1, const std::string& s2) const
{
    return (strcmpi(s1, s2) < 0);
}


c_script_engine::c_script_engine(application* app)
 : m_app(app)
 , m_show_hidden(false)
{
    script_engine* eng = this;

    eng->add_function("close",
            [&] {
                console* con = console::get_invoker_console();
                if (con == nullptr)
                    *c_logger::get_instance() << "This function can only be used from a console" << std::endl;
                else
                    con->close();
            },
            true);

    eng->add_function("clear",
            [&] {
                console* con = console::get_invoker_console();
                if (con == nullptr)
                    *c_logger::get_instance() << "This function can only be used from a console" << std::endl;
                else
                    con->clear();
            },
            true);

    eng->add_function("show_hidden", [&] { this->show_hidden_functions(); }, true);
    eng->add_function("hide_hidden", [&] { this->hide_hidden_functions(); }, true);

    eng->add_function("show_all",
            [&] {
                tthread::lock_guard<tthread::mutex> guard(m_mutex);
                for (auto& it : m_functions)
                {
                    if (!it.second.m_is_hidden || m_show_hidden)
                    {
                        std::cout << it.second.m_sign.get_expression();
                        if (it.second.m_is_hidden) std::cout << " *";
                        std::cout << std::endl;
                    }
                }
            },
            true);
}

c_script_engine::~c_script_engine()
{
}

application* c_script_engine::get_app() const
{
    return m_app;
}

void c_script_engine::add_function(std::string fn, dynamic_function func, std::string args, bool hidden)
{
    if (m_functions.count(fn) > 0)
        throw std::runtime_error("function already registered");

    if (!is_valid_fn_name(fn))
        throw std::runtime_error("invalid function name");

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_functions.insert( std::make_pair(fn, function_container {func, fn+args, hidden}) );
}

void c_script_engine::remove_function(std::string fn)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto pos = m_functions.find(fn);

    if (pos != m_functions.end())
        m_functions.erase(pos);
}

optional<var> c_script_engine::exec(std::string fn, varlist vl, std::ostream& output) const
{
    dynamic_function func;

    m_mutex.lock();
    auto pos = m_functions.find(fn);
    if (pos != m_functions.end()) func = pos->second.m_func;
    m_mutex.unlock();

    if (func)
    {
        logger::scoped_hook __hook(output);
        return func(vl);
    }

    return {};
}

optional<var> c_script_engine::parse_and_exec(std::string expr, std::ostream& output) const
{
    logger::scoped_hook __hook(output);
    return process_expression(c_expression(expr));
}

console::controller* c_script_engine::create_console_controller() const
{
    return new c_script_engine::console_controller(this);
}

bool c_script_engine::is_valid_fn_name(std::string fn) const
{
    if (fn.size() == 0) return false;

    auto it=fn.begin(), end = fn.end();
    if (!std::isalpha(*it) && (*it) != '_') return false;

    for (; it != end; ++it)
        if (!std::isalnum(*it) && (*it) != '_') return false;

    return true;
}

void c_script_engine::show_hidden_functions()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_show_hidden = true;
}

void c_script_engine::hide_hidden_functions()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_show_hidden = false;
}

std::vector<std::string> c_script_engine::find_matching_functions(std::string fn) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    fn = trim(fn);

    std::vector<std::string> matches;
    size_t len = fn.size();
    auto it = m_functions.begin(), end = m_functions.end();

    for (; it != end; ++it)
    {
        if (it->second.m_is_hidden && !m_show_hidden)
            continue;

        if (strncmpi(it->first, fn, len) == 0)
            matches.push_back(it->first);
    }

    return std::move(matches);
}

bool c_script_engine::auto_complete(std::string& fn, bool print) const
{
    return auto_complete(fn, find_matching_functions(fn), print);
}

bool c_script_engine::auto_complete(std::string& fn, std::vector<std::string> matches, bool print) const
{
    if (matches.empty()) return false;

    if (matches.size() == 1)
    {
        fn = matches[0];
        return true;
    }

    fn = trim(fn);

    scope_callback o([&]
    {
        if (print && matches.size() > 1)
        {
            for_each(matches.begin(), matches.end(),
                     [&](const std::string& s)
                     {
                         if (s.size() != 0)
                         {
                             *c_logger::get_instance() << "\n> " << s;
                         }
                     });
            //*c_logger::get_instance() << std::flush;
        }
    });

    size_t max_len = 0;
    size_t min_len = ~0;
    for_each(matches.begin(), matches.end(),
             [&](const std::string& s)
             {
                 size_t len = s.size();
                 if (len > max_len) max_len = len;
                 if (len < min_len) min_len = len;
             });

    size_t fn_len = fn.size();
    if (fn_len == max_len) return true;
    if (fn_len == min_len) return false;

    auto begin = matches.begin(), end = matches.end();

    for (size_t pos = fn_len; pos <= min_len; ++pos)
    {
        char c = (matches[0])[pos];
        for (auto it = begin+1; it != end; ++it)
        {
            if (std::tolower((*it)[pos]) != std::tolower(c)) return false;
        }
        fn += c;
    }

    return true;
}

static void fill_expr_by_sign(expression& e, const c_expression& sg)
{
    auto e_children = e.get_children();
    auto sg_children = sg.get_children();

    size_t e_children_cnt = e_children.count();
    size_t sg_children_cnt = sg_children.count();

    if (e_children_cnt < sg_children_cnt)
    {
        sg_children.advance(e_children_cnt);
        for (; e_children.has_next(); e_children.next()); // jump to the end
        for (; sg_children.has_next(); sg_children.next()) e_children.insert( *(sg_children.get()) );
    }
    else if (e_children_cnt > sg_children_cnt)
    {
        e_children.advance(sg_children_cnt);
        for (; e_children.has_next(); e_children.next()) e_children.erase();
    }
}

void c_script_engine::auto_complete_expr(std::string& expr, bool print) const
{
    c_expression e(expr, true);

    if (e.is_leaf())
    {
        std::string name = e.get_name();

        if ( auto_complete(name, print) )
        {
            e.set_name(name);

            tthread::lock_guard<tthread::mutex> guard(m_mutex);
            auto pos = m_functions.find(name);
            if (pos != m_functions.end())
            {
                fill_expr_by_sign(e, pos->second.m_sign);
                e.set_as_expression();
            }
        }
    }
    else
    {
        e.for_each([&](expression& e)
        {
            if (!e.is_leaf() && // it's an expression
                (e.is_root() || !e.get_name().empty())) // not an array arg
            {
                std::string name = e.get_name();
                auto_complete(name, print);
                e.set_name(name);

                tthread::lock_guard<tthread::mutex> guard(m_mutex);
                auto pos = m_functions.find(name);
                if (pos != m_functions.end()) fill_expr_by_sign(e, pos->second.m_sign);
            }
        });
    }

    expr = e.get_expression();
}

optional<var> c_script_engine::process_expression(const expression& e) const
{
    std::string name = e.get_name();

    if (e.is_leaf() && !e.is_root())
    {
        return var(name);
    }
    else
    {
        if (!is_valid_fn_name(name) && !name.empty())
            return {};

        varlist vl;

        for (auto args = e.get_children(); args.has_next(); args.next())
        {
            optional<var> v = process_expression(**args.get());
            if (v) vl.push_back(*v);
            else return {};
        }

        if (name.empty())
        {
            return optional<var>(vl);
        }
        else
        {
            dynamic_function func;

            //tthread::lock_guard<tthread::mutex> guard(m_mutex);
            m_mutex.lock();
            auto pos = m_functions.find(name);
            if (pos != m_functions.end()) func = pos->second.m_func;
            m_mutex.unlock();

            if (func) return func(vl);
        }
    }

    return {};
}
