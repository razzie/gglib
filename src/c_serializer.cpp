#include "c_serializer.hpp"

using namespace gg;


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

void c_serializer::add_rule(typeinfo ti, serializer_func sfunc, deserializer_func dfunc)
{
    if (m_rules.count(ti.hash_code()) > 0)
        throw std::runtime_error("rule already added");

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto r = m_rules.insert( std::make_pair(ti.hash_code(), rule {ti, sfunc, dfunc}) );

    if (!r.second)
        throw std::runtime_error("failed to add rule");
}

bool c_serializer::serialize(typeinfo ti, const var& v, buffer* buf) const
{
    if (buf == nullptr)
        return false;

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto rule = m_rules.find(ti.hash_code());
    if (rule != m_rules.end())
    {
        return rule->second.m_sfunc(v, buf);
    }

    return false;
}

optional<var> c_serializer::deserialize(typeinfo ti, buffer* buf) const
{
    if (buf == nullptr)
        return optional<var>();

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto rule = m_rules.find(ti.hash_code());
    if (rule != m_rules.end())
    {
        return rule->second.m_dfunc(buf);
    }

    return optional<var>();
}
