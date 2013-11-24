#include "c_serializer.hpp"

using namespace gg;


c_serializer::c_storage::c_storage()
 : m_pos(m_data.begin())
{
}

c_serializer::c_storage::~c_storage()
{
}

std::size_t c_serializer::c_storage::get_size() const
{
    return m_data.size();
}

bool c_serializer::c_storage::is_empty() const
{
    return m_data.empty();
}

void c_serializer::c_storage::push(uint8_t byte)
{
    m_data.push_back(byte);
}

void c_serializer::c_storage::push(const uint8_t* bytes, std::size_t len)
{
    for(std::size_t i = 0; i < len; ++i)
        m_data.push_back(bytes[i]);
}

void c_serializer::c_storage::push(const std::vector<uint8_t>& bytes)
{
    m_data.insert(m_data.end(), bytes.begin(), bytes.end());
}

const uint8_t* c_serializer::c_storage::get() const
{
    return m_data.data();
}

optional<uint8_t> c_serializer::c_storage::pop()
{
    if (m_pos == m_data.end())
        return optional<uint8_t>();
    else
        return *(m_pos++);
}

std::vector<uint8_t> c_serializer::c_storage::pop(std::size_t len)
{
    std::vector<uint8_t> r;
    r.insert(r.begin(), m_pos, std::next(m_pos,len));
    return std::move(r);
}


c_serializer::c_serializer(application* app)
 : m_app(app)
{
}

c_serializer::~c_serializer()
{
}

application* c_serializer::get_app() const
{
    return m_app;
}

serializer::storage* c_serializer::create_storage() const
{
    return new c_storage();
}

void c_serializer::add_rule(typeinfo ti, serializer_func sfunc, deserializer_func dfunc)
{
    if (m_rules.count(ti) > 0)
        throw std::runtime_error("rule already added");

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto r = m_rules.insert( std::make_pair(ti, rule {sfunc, dfunc}) );

    if (!r.second)
        throw std::runtime_error("failed to add rule");
}

bool c_serializer::serialize(typeinfo ti, const var& v, storage& st) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto rule = m_rules.find(ti);
    if (rule != m_rules.end())
    {
        return rule->second.m_sfunc(v, st);
    }

    return false;
}

optional<var> c_serializer::deserialize(typeinfo ti, storage& st) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto rule = m_rules.find(ti);
    if (rule != m_rules.end())
    {
        return rule->second.m_dfunc(st);
    }

    return optional<var>();
}
