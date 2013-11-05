#include <cctype>
#include <stdexcept>
#include <algorithm>
#include "c_scripteng.hpp"
#include "managed_cout.hpp"

using namespace gg;

c_script_engine::console_controller::console_controller(c_script_engine* scripteng)
 : m_scripteng(scripteng)
{
}

c_script_engine::console_controller::~console_controller()
{
}

bool c_script_engine::console_controller::exec(std::string& fn, console::output& out)
{
    optional<var> r = m_scripteng->parse_and_exec(fn, out);
    return (r.is_valid());
}

void c_script_engine::console_controller::complete(std::string& fn, console::output&)
{
    expression e(fn);
    e.for_each([&](expression& e)
    {
        if (!e.is_leaf())
            m_scripteng->auto_complete(e.get_name());
    });
    fn = e.get_expression();
}


c_script_engine::c_script_engine()
{
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

    return optional<var>();
}

optional<var> c_script_engine::parse_and_exec(std::string expr, console::output& output) const
{

    return optional<var>();
}

console::controller* c_script_engine::get_console_controller()
{
    return static_cast<console::controller*>(m_ctrl);
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

    std::vector<std::string> matches;
    size_t cmd_len = fn.size();
    auto it = m_functions.begin(), end = m_functions.end();

    for (; it != end; ++it)
    {
        if (fn.compare(0, cmd_len, it->first) == 0)
            matches.push_back(it->first);
    }

    if (matches.size() == 0)
        throw std::runtime_error("no matching function");

    return matches;
}

void c_script_engine::auto_complete(std::string& fn, bool print) const
{
    auto_complete(fn, find_matching_functions(fn), print);
    return;
}

void c_script_engine::auto_complete(std::string& fn, std::vector<std::string> matches, bool print) const
{
    if (matches.size() == 0) return;

    if (matches.size() == 1)
    {
        fn = matches[0];
        return;
    }

    util::on_return o([&]
    {
        if (print && matches.size() > 1)
        {
            for_each(matches.begin(), matches.end(),
                     [&](const std::string& s)
                     {
                         if (s.size() != 0)
                         {
                             std::cout << "> " << s << std::endl;
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

    size_t cmd_len = fn.size();
    if (cmd_len == min_len || cmd_len == max_len) return;

    auto begin = matches.begin(), end = matches.end();

    for (size_t pos = cmd_len; pos <= min_len; ++pos)
    {
        char c = (matches[0])[pos];
        for (auto it = begin+1; it != end; ++it)
        {
            if ((*it)[pos] != c) return;
        }
    }
}

optional<var> c_script_engine::process_expression(const expression& e) const
{
    std::string val = util::trim( e.get_name() );

    if (e.is_leaf())
    {
        return var(val);
    }
    else
    {
        if (!is_valid_fn_name(val)) return optional<var>();

        varlist vl;
    }
}
