#include <fstream>
#include <stdexcept>
#include "c_iniparser.hpp"

using namespace gg;


c_ini_parser::c_group::c_group(std::string name)
 : m_name(name)
{
}

c_ini_parser::c_group::c_group(std::string name, std::list<entry>&& entries)
 : m_name(name)
 , m_entries(std::move(entries))
{
}

c_ini_parser::c_group::c_group(c_ini_parser::c_group&& g)
 : m_name(std::move(g.m_name))
 , m_entries(std::move(g.m_entries))
{
}

c_ini_parser::c_group::~c_group()
{
}

std::string c_ini_parser::c_group::get_name() const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return m_name;
}

void c_ini_parser::c_group::set_name(std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_name = name;
}

ini_parser::entry& c_ini_parser::c_group::operator[] (std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_entries.begin(), end = m_entries.end();
    for (; it != end; ++it)
        if (it->key == key) return *it;

    m_entries.push_back({key, ""});
    return m_entries.back();
}

ini_parser::entry* c_ini_parser::c_group::get_entry(std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_entries.begin(), end = m_entries.end();
    for (; it != end; ++it)
        if (it->key == key) return &(*it);

    return nullptr;
}

const ini_parser::entry* c_ini_parser::c_group::get_entry(std::string key) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_entries.begin(), end = m_entries.end();
    for (; it != end; ++it)
        if (it->key == key) return &(*it);

    return nullptr;
}

std::list<ini_parser::entry>& c_ini_parser::c_group::get_entries()
{
    return m_entries;
}

const std::list<ini_parser::entry>& c_ini_parser::c_group::get_entries() const
{
    return m_entries;
}

void c_ini_parser::c_group::save(std::ostream& out)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return; //TBD
}


ini_parser* ini_parser::create(std::string file)
{
    return new c_ini_parser(file);
}

ini_parser* ini_parser::create(const std::istream& in)
{
    return new c_ini_parser(in);
}


c_ini_parser::c_ini_parser(std::string file)
 : c_ini_parser(std::fstream(file, std::ios_base::in))
{
}

c_ini_parser::c_ini_parser(const std::istream& in)
{
    //TBD
}

c_ini_parser::~c_ini_parser()
{
}

c_ini_parser::c_group& c_ini_parser::operator[] (std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_groups.begin(), end = m_groups.end();
    for (; it != end; ++it)
        if (it->m_name == name) return *it;

    m_groups.push_back(name);
    return m_groups.back();
}

c_ini_parser::c_group* c_ini_parser::get_group(std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_groups.begin(), end = m_groups.end();
    for (; it != end; ++it)
        if (it->m_name == name) return &(*it);

    return nullptr;
}

const c_ini_parser::c_group* c_ini_parser::get_group(std::string name) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_groups.begin(), end = m_groups.end();
    for (; it != end; ++it)
        if (it->m_name == name) return &(*it);

    return nullptr;
}

c_ini_parser::c_group* c_ini_parser::create_group(std::string)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return nullptr; //TDB
}

void c_ini_parser::remove_group(std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_groups.begin(), end = m_groups.end();
    for (; it != end; ++it)
    {
        if (it->m_name == name)
        {
            it = std::prev(m_groups.erase(it), 1);
            continue;
        }
    }
}

void c_ini_parser::remove_group(group* g)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_groups.begin(), end = m_groups.end();
    for (; it != end; ++it)
    {
        if (&(*it) == g)
        {
            it = std::prev(m_groups.erase(it), 1);
            continue;
        }
    }
}

void c_ini_parser::save(std::string file)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return; //TBD
}

void c_ini_parser::save(std::ostream& out)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return; //TBD
}
