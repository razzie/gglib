#include <fstream>
#include <stdexcept>
#include "c_iniparser.hpp"

using namespace gg;


c_ini_parser::c_parse_result::c_group::c_group()
{
}

c_ini_parser::c_parse_result::c_group::c_group(std::list<entry>&& entries)
 : m_entries(std::move(entries))
{
}

c_ini_parser::c_parse_result::c_group::~c_group()
{
}

ini_parser::parse_result::entry*
c_ini_parser::c_parse_result::c_group::operator[] (std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_entries.begin(), end = m_entries.end();
    for (; it != end; ++it)
        if (it->key == key) return &(*it);

    return nullptr;
}

const ini_parser::parse_result::entry*
c_ini_parser::c_parse_result::c_group::operator[] (std::string key) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_entries.begin(), end = m_entries.end();
    for (; it != end; ++it)
        if (it->key == key) return &(*it);

    return nullptr;
}

ini_parser::parse_result::entry*
c_ini_parser::c_parse_result::c_group::get_entry(std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_entries.begin(), end = m_entries.end();
    for (; it != end; ++it)
        if (it->key == key) return &(*it);

    return nullptr;
}

const ini_parser::parse_result::entry*
c_ini_parser::c_parse_result::c_group::get_entry(std::string key) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_entries.begin(), end = m_entries.end();
    for (; it != end; ++it)
        if (it->key == key) return &(*it);

    return nullptr;
}

std::list<ini_parser::parse_result::entry>&
c_ini_parser::c_parse_result::c_group::get_entries()
{
    return m_entries;
}

const std::list<ini_parser::parse_result::entry>&
c_ini_parser::c_parse_result::c_group::get_entries() const
{
    return m_entries;
}

void c_ini_parser::c_parse_result::c_group::save(std::ostream& out)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return; //TBD
}


c_ini_parser::c_parse_result::c_parse_result(std::string file)
{
    //TBD
}

c_ini_parser::c_parse_result::c_parse_result(std::istream& in)
{
    //TBD
}

c_ini_parser::c_parse_result::~c_parse_result()
{
    //TBD
}

c_ini_parser::c_parse_result::c_group*
c_ini_parser::c_parse_result::get_group(std::string)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return nullptr; //TBD
}

const c_ini_parser::c_parse_result::c_group*
c_ini_parser::c_parse_result::get_group(std::string) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return nullptr; //TBD
}

c_ini_parser::c_parse_result::c_group*
c_ini_parser::c_parse_result::create_group(std::string)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return nullptr; //TDB
}

void c_ini_parser::c_parse_result::remove_group(std::string)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return; //TDB
}

void c_ini_parser::c_parse_result::remove_group(group*)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return; //TBD
}

void c_ini_parser::c_parse_result::save(std::string file)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return; //TBD
}

void c_ini_parser::c_parse_result::save(std::ostream& out)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return; //TBD
}


c_ini_parser::c_ini_parser(application* app)
 : m_app(app)
{
}

c_ini_parser::~c_ini_parser()
{
}

application* c_ini_parser::get_app() const
{
    return m_app;
}

c_ini_parser::c_parse_result*
c_ini_parser::open(std::string file) const
{
    return new c_parse_result(file);
}

c_ini_parser::c_parse_result*
c_ini_parser::open(std::istream& in) const
{
    return new c_parse_result(in);
}
