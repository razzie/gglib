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

bool c_script_engine::console_controller::exec(std::string& cmd, console::output& out)
{
    return m_scripteng->parse_and_exec(cmd, out);
}

void c_script_engine::console_controller::complete(std::string& cmd, console::output&)
{
    // TBD
}


c_script_engine::c_script_engine()
{
}

c_script_engine::~c_script_engine()
{
}

void c_script_engine::add_function(std::string cmd, dynamic_function func)
{
    if (m_functions.count(cmd) == 1)
        throw std::runtime_error("command already registered");

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_functions.insert( std::make_pair(cmd, func) );
}

void c_script_engine::remove_function(std::string cmd)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto pos = m_functions.find(cmd);

    if (pos != m_functions.end())
        m_functions.erase(pos);
}

bool c_script_engine::exec(std::string cmd, varlist vl, std::ostream& output, var* ret) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto pos = m_functions.find(cmd);

    if (pos != m_functions.end())
    {
        managed_cout::hook h(output);
        var v = pos->second(vl);
        if (ret != nullptr) std::swap(v, *ret);
        return true;
    }

    return false;
}

bool c_script_engine::exec(std::string cmd, varlist vl, console::output& output, var* ret) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto pos = m_functions.find(cmd);

    if (pos != m_functions.end())
    {
        managed_cout::hook h(output);
        var v = pos->second(vl);
        if (ret != nullptr) std::swap(v, *ret);
        return true;
    }

    return false;
}

bool c_script_engine::parse_and_exec(std::string expr, std::ostream& output, var* ret) const
{

}

bool c_script_engine::parse_and_exec(std::string expr, console::output& output, var* ret) const
{

}

console::controller* c_script_engine::get_console_controller()
{
    return static_cast<console::controller*>(m_ctrl);
}

bool c_script_engine::is_valid_cmd_name(std::string cmd) const
{
    if (cmd.size() == 0) return false;

    auto it=cmd.begin(), end = cmd.end();
    if (!std::isalpha(*it) && (*it) != '_') return false;

    for (; it != end; ++it)
        if (!std::isalnum(*it) && (*it) != '_') return false;

    return true;
}

std::vector<std::string> c_script_engine::find_matching_commands(std::string cmd) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    std::vector<std::string> matches;
    size_t cmd_len = cmd.size();
    auto it = m_functions.begin(), end = m_functions.end();

    for (; it != end; ++it)
    {
        if (cmd.compare(0, cmd_len, it->first) == 0)
            matches.push_back(it->first);
    }

    return matches;
}

void c_script_engine::auto_complete(std::string& cmd) const
{
    auto_complete(cmd, find_matching_commands(cmd));
    return;
}

void c_script_engine::auto_complete(std::string& cmd, std::vector<std::string> matches) const
{
    if (matches.size() == 0) return;

    if (matches.size() == 1)
    {
        cmd = matches[0];
        return;
    }

    size_t max_len = 0;
    size_t min_len = ~0;
    for_each(matches.begin(), matches.end(),
             [&](const std::string& s)
             {
                 size_t len = s.size();
                 if (len > max_len) max_len = len;
                 if (len < min_len) min_len = len;
             });

    size_t cmd_len = cmd.size();
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
