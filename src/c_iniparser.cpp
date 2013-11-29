#include <fstream>
#include <stdexcept>
#include "c_iniparser.hpp"

using namespace gg;

/*
 * c_section
 */
c_ini_parser::c_section::c_section(std::string name)
 : m_name(name)
{
}

c_ini_parser::c_section::c_section(std::string name, std::list<entry*>&& entries)
 : m_name(name)
 , m_entries(std::move(entries))
{
}

c_ini_parser::c_section::c_section(c_ini_parser::c_section&& s)
 : m_name(std::move(s.m_name))
 , m_entries(std::move(s.m_entries))
{
}

c_ini_parser::c_section::~c_section()
{
    for (auto e : m_entries)
        delete static_cast<c_entry*>(e);
}

std::string c_ini_parser::c_section::get_name() const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return m_name;
}

void c_ini_parser::c_section::set_name(std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_name = name;
}

ini_parser::entry& c_ini_parser::c_section::operator[] (std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto e : m_entries)
        if (e->get_key() == key) return *e;

    m_entries.push_back(new c_entry(this, key));
    return *(m_entries.back());
}

ini_parser::entry* c_ini_parser::c_section::get_entry(std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto e : m_entries)
        if (e->get_key() == key) return e;

    return nullptr;
}

const ini_parser::entry* c_ini_parser::c_section::get_entry(std::string key) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto e : m_entries)
        if (e->get_key() == key) return e;

    return nullptr;
}

ini_parser::entry* c_ini_parser::c_section::add_entry(std::string key, std::string value)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_entries.push_back(new c_entry(this, key, value));
    return m_entries.back();
}

void c_ini_parser::c_section::remove_entry(std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_entries.begin(), end = m_entries.end();
    for (; it != end; ++it)
    {
        if ((*it)->get_key() == key)
        {
            it = std::prev(m_entries.erase(it), 1);
            continue;
        }
    }
}

void c_ini_parser::c_section::remove_entry(ini_parser::entry* e)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_entries.begin(), end = m_entries.end();
    for (; it != end; ++it)
    {
        if (*it == e)
        {
            it = std::prev(m_entries.erase(it), 1);
            continue;
        }
    }
}

std::list<ini_parser::entry*>& c_ini_parser::c_section::get_entries()
{
    return m_entries;
}

const std::list<ini_parser::entry*>& c_ini_parser::c_section::get_entries() const
{
    return m_entries;
}


/*
 * c_entry
 */
c_ini_parser::c_entry::c_entry(section* s, std::string key, std::string value)
 : m_section(s)
 , m_key(key)
 , m_value(value)
{
}

c_ini_parser::c_entry::~c_entry()
{
}

ini_parser::section* c_ini_parser::c_entry::get_section()
{
    return m_section;
}

const ini_parser::section* c_ini_parser::c_entry::get_section() const
{
    return m_section;
}

std::string c_ini_parser::c_entry::get_key() const
{
    return m_key;
}

void c_ini_parser::c_entry::set_key(std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_key = key;
}

std::string c_ini_parser::c_entry::get_value() const
{
    return m_value;
}

void c_ini_parser::c_entry::set_value(std::string value)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_value = value;
}


ini_parser* ini_parser::create(std::string file)
{
    return new c_ini_parser(file);
}

ini_parser* ini_parser::create(std::istream& in)
{
    return new c_ini_parser(in);
}


/*
 * static helper functions
 */
static bool is_section(std::string line)
{
    bool first_char_is_open_bracket = false;
    bool last_char_is_end_bracket = false;

    for (char c : line)
    {
        if (c == ' ') continue;

        if (c == '[' && !first_char_is_open_bracket)
        {
            first_char_is_open_bracket = true;
            continue;
        }

        if (c != '[' && !first_char_is_open_bracket) return false;

        last_char_is_end_bracket = (c == ']');
    }

    if (!last_char_is_end_bracket) return false;

    return true;
}

static std::string get_key(std::string line)
{
    size_t comm_pos = line.find(';');
    size_t eq_pos = line.find('=');

    if (comm_pos < eq_pos) eq_pos = comm_pos;

    return line.substr(0, eq_pos - 1);
}

static std::string get_value(std::string line)
{
    size_t comm_pos = line.find(';');
    size_t eq_pos = line.find('=');

    if (comm_pos < eq_pos) return {};

    return line.substr(eq_pos + 1, comm_pos - 1);
}


/*
 * c_ini_parser
 */
void c_ini_parser::parse(std::istream& in)
{
    std::string line;
    c_section* curr_section = nullptr;

    while (true)
    {
        line.clear();
        std::getline(in, line);

        if (in.eof()) break;
        if (line.empty()) continue;

        if (is_section(line))
        {
            size_t open_pos = line.find('['), close_pos = line.find(']');
            curr_section = new c_section(line.substr(open_pos + 1, close_pos - 1));
            m_sections.push_back(curr_section);
            continue;
        }
        else
        {
            if (curr_section == nullptr)
            {
                curr_section = new c_section("");
                m_sections.push_back(curr_section);
            }
            curr_section->add_entry(get_key(line), get_value(line));
            continue;
        }
    }
}

c_ini_parser::c_ini_parser(std::string file)
{
    std::fstream f(file, std::ios_base::in);
    this->parse(f);
}

c_ini_parser::c_ini_parser(std::istream& in)
{
    this->parse(in);
}

c_ini_parser::~c_ini_parser()
{
}

ini_parser::entry* c_ini_parser::get_entry(std::string section_name, std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
        if (s->get_name() == section_name) return s->get_entry(key);

    return nullptr;
}

const ini_parser::entry* c_ini_parser::get_entry(std::string section_name, std::string key) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
        if (s->get_name() == section_name) return s->get_entry(key);

    return nullptr;
}

ini_parser::entry* c_ini_parser::add_entry(std::string section_name, std::string key, std::string value)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
        if (s->get_name() == section_name) return s->add_entry(key, value);

    return nullptr;
}

void c_ini_parser::remove_entry(std::string section_name, std::string key)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
    {
        if (s->get_name() == section_name)
        {
            s->remove_entry(key);
            return;
        }
    }
}

void c_ini_parser::remove_entry(ini_parser::entry* e)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
    {
        if (s == e->get_section())
        {
            e->get_section()->remove_entry(e);
            return;
        }
    }
}

ini_parser::section& c_ini_parser::operator[] (std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
        if (s->get_name() == name) return *s;

    m_sections.push_back(new c_section(name));
    return *(m_sections.back());
}

ini_parser::section* c_ini_parser::get_section(std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
        if (s->get_name() == name) return s;

    return nullptr;
}

const ini_parser::section* c_ini_parser::get_section(std::string name) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
        if (s->get_name() == name) return s;

    return nullptr;
}

ini_parser::section* c_ini_parser::create_section(std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
        if (s->get_name() == name) return s;

    m_sections.push_back(new c_section(name));
    return m_sections.back();
}

void c_ini_parser::remove_section(std::string name)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_sections.begin(), end = m_sections.end();
    for (; it != end; ++it)
    {
        if ((*it)->get_name() == name)
        {
            it = std::prev(m_sections.erase(it), 1);
            continue;
        }
    }
}

void c_ini_parser::remove_section(section* g)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_sections.begin(), end = m_sections.end();
    for (; it != end; ++it)
    {
        if (*it == g)
        {
            it = std::prev(m_sections.erase(it), 1);
            continue;
        }
    }
}

std::list<ini_parser::section*>& c_ini_parser::get_sections()
{
    return m_sections;
}

const std::list<ini_parser::section*>& c_ini_parser::get_sections() const
{
    return m_sections;
}

void c_ini_parser::save(std::string file)
{
    std::fstream f(file, std::ios_base::in);
    this->save(f);
}

void c_ini_parser::save(std::ostream& out)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto s : m_sections)
    {
        out << "[" << s->get_name() << "]\n";

        for (auto e : s->get_entries())
        {
            out << e->get_key() << " = " << e->get_value() << "\n";
        }
    }

    out.flush();
}
