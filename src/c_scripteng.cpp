#include <cctype>
#include <stdexcept>
#include "c_scripteng.hpp"

using namespace gg;

c_script_engine::console_controller::console_controller(c_script_engine* scripteng)
 : m_scripteng(scripteng)
{
}

c_script_engine::console_controller::~console_controller()
{
}

bool c_script_engine::console_controller::exec(std::string& cmd, console::output&)
{
    return m_scripteng->parse_and_exec(cmd);
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

void c_script_engine::add_command(std::string cmd,
                                  std::vector<script_engine::arg::type> args,
                                  callback cb)
{
    if (m_commands.count(cmd) == 1)
        throw std::runtime_error("command already registered");

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_commands.insert( std::make_pair(cmd, command {cmd, args, cb}) );
}

void c_script_engine::remove_command(std::string cmd)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto pos = m_commands.find(cmd);

    if (pos != m_commands.end())
        m_commands.erase(pos);
}

bool c_script_engine::exec(std::string cmd,
                           std::vector<script_engine::arg> args,
                           std::ostream& output) const
{

}

bool c_script_engine::parse_and_exec(std::string cmd_line,
                                     std::ostream& output) const
{

}

c_script_engine::console_controller* c_script_engine::get_console_controller()
{
    return m_ctrl;
}

bool c_script_engine::is_valid_cmd_name(std::string cmd) const
{
    if (cmd.size() == 0) return false;

    auto it=cmd.begin(), end = cmd.end();
    if (!std::isalpha(*it) && (*it) != '_') return false;

    for (; it != end; it++)
        if (!std::isalnum(*it) && (*it) != '_') return false;

    return true;
}

c_script_engine::command* c_script_engine::find_command(std::string cmd)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto pos = m_commands.find(cmd);

    if (pos == m_commands.end())
        return nullptr;
    else
        return &pos->second;
}
