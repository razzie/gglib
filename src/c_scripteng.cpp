#include <cctype>
#include <stdexcept>
#include <algorithm>
#include "c_scripteng.hpp"
#include "managed_cout.hpp"
#include "threadglobal.hpp"

using namespace gg;


GG_USE_RECURSIVE_THREAD_GLOBAL(console*)

console* script_engine::get_invoker_console()
{
    optional<console*> con = recursive_thread_global<console*>::get();
    if (con.is_valid()) return con;
    else return nullptr;
}


c_script_engine::console_controller::console_controller(const c_script_engine* scripteng)
 : m_scripteng(scripteng)
{
}

c_script_engine::console_controller::~console_controller()
{
}

console::controller::exec_result
 c_script_engine::console_controller::exec(std::string& expr, console::output& out)
{
    if (!expr.empty() && expr[0] == '#')
        return exec_result::NO_EXEC;

    optional<var> r;

    try
    {
        recursive_thread_global<console*>::scope invoker(&out.get_console());
        expression e(expr);
        r = m_scripteng->parse_and_exec(expr, out);
    }
    catch (expression_error& e)
    {
        out << e.what();
        return exec_result::EXEC_FAIL;
    }

    return (r.is_valid() ? exec_result::EXEC_SUCCESS : exec_result::EXEC_FAIL);
}

void c_script_engine::console_controller::complete(std::string& expr, console::output& out)
{
    managed_cout::hook h(out);
    out.set_color({100,100,100});
    m_scripteng->auto_complete_expr(expr, true);
}


c_script_engine::c_script_engine()
{
    script_engine* eng = this;

    eng->add_function("close",
            [&] {
                console* con = script_engine::get_invoker_console();
                if (con == nullptr)
                    std::cout << "This function can only be used from a console" << std::endl;
                else
                    con->close();
            });

    eng->add_function("clear",
            [&] {
                console* con = script_engine::get_invoker_console();
                if (con == nullptr)
                    std::cout << "This function can only be used from a console" << std::endl;
                else
                    con->clear();
            });
}

c_script_engine::~c_script_engine()
{
}

void c_script_engine::add_function(std::string fn, dynamic_function func)
{
    if (m_functions.count(fn) == 1)
        throw std::runtime_error("command already registered");

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_functions.insert( std::make_pair(fn, func) );
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
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto pos = m_functions.find(fn);
    if (pos != m_functions.end())
    {
        managed_cout::hook h(output);
        return pos->second(vl);
    }

    return optional<var>();
}

optional<var> c_script_engine::exec(std::string fn, varlist vl, console::output& output) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto pos = m_functions.find(fn);
    if (pos != m_functions.end())
    {
        managed_cout::hook h(output);
        return pos->second(vl);
    }

    return optional<var>();
}

optional<var> c_script_engine::parse_and_exec(std::string expr, std::ostream& output) const
{
    managed_cout::hook h(output);
    return process_expression(expr);
}

optional<var> c_script_engine::parse_and_exec(std::string expr, console::output& output) const
{
    managed_cout::hook h(output);
    return process_expression(expr);
}

console::controller* c_script_engine::get_console_controller() const
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

std::vector<std::string> c_script_engine::find_matching_functions(std::string fn) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    fn = util::trim(fn);

    std::vector<std::string> matches;
    size_t len = fn.size();
    auto it = m_functions.begin(), end = m_functions.end();

    for (; it != end; ++it)
    {
        if (it->first.compare(0, len, fn) == 0)
            matches.push_back(it->first);
    }

    return matches;
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

    fn = util::trim(fn);

    util::on_return o([&]
    {
        if (print && matches.size() > 1)
        {
            for_each(matches.begin(), matches.end(),
                     [&](const std::string& s)
                     {
                         if (s.size() != 0)
                         {
                             std::cout << "\n> " << s;
                         }
                     });
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
    //if (fn_len == min_len || fn_len == max_len) return;
    if (fn_len == max_len) return true;
    if (fn_len == min_len) return false;

    auto begin = matches.begin(), end = matches.end();

    for (size_t pos = fn_len; pos <= min_len; ++pos)
    {
        char c = (matches[0])[pos];
        for (auto it = begin+1; it != end; ++it)
        {
            if ((*it)[pos] != c) return false;
        }
        fn += c;
    }

    return true;
}

void c_script_engine::auto_complete_expr(std::string& expr, bool print) const
{
    expression e(expr, true);

    if (e.is_leaf())
    {
        std::string name = e.get_name();

        if ( auto_complete(name, print) )
        {
            //e.set_name(name);
            //e.add_child({"\" \""});
            expr = name + "(  )";
        }
    }
    else
    {
        e.for_each([&](expression& e)
        {
            if (!e.is_leaf() && // it's an expression
                (e.get_parent() == nullptr || !e.get_name().empty())) // not an array arg
            {
                std::string name = e.get_name();
                auto_complete(name, print);
                e.set_name(name);
            }
        });

        expr = e.get_expression();
    }
}

optional<var> c_script_engine::process_expression(const expression& e) const
{
    std::string name = e.get_name();

    if (e.is_leaf())
    {
        return var(name);
    }
    else
    {
        if (!is_valid_fn_name(name) && !name.empty())
            return optional<var>();

        varlist vl;
        auto args = e.get_children();
        auto it = args.begin(), end = args.end();
        for (; it != end; ++it)
        {
            if ((*it)->is_empty()) continue;

            optional<var> v = process_expression(**it);
            if (v.is_valid()) vl.push_back(v);
            else return optional<var>();
        }

        if (name.empty())
        {
            return optional<var>(vl);
        }
        else
        {
            tthread::lock_guard<tthread::mutex> guard(m_mutex);
            auto pos = m_functions.find(name);
            if (pos != m_functions.end()) return pos->second(vl);
        }
    }

    return optional<var>();
}
