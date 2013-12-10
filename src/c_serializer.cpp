#include <string>
#include "c_serializer.hpp"
#include "c_buffer.hpp"

using namespace gg;


class safe_buffer : public buffer
{
    buffer* m_buf;
    size_t m_pos;

public:
    safe_buffer(buffer* buf) : m_buf(buf), m_pos(0) {}
    ~safe_buffer() {}

    void clear() { m_pos = 0; m_buf->clear(); }
    void push(uint8_t byte) { m_buf->push(byte); }
    void push(const uint8_t* buf, size_t len) { m_buf->push(buf, len); }
    void push(const std::vector<uint8_t>& buf) { m_buf->push(buf); }
    void push(const buffer* buf) { m_buf->push(buf); }
    void merge(buffer* buf) { m_buf->merge(buf); }

    size_t available() const
    {
        return m_buf->available() - m_pos;
    }

    void advance(size_t len)
    {
        m_pos += len;
    }

    std::vector<uint8_t> peek(size_t len) const
    {
        return std::move(m_buf->peek(m_pos, len));
    }

    std::vector<uint8_t> peek(size_t start_pos, size_t len) const
    {
        return std::move(m_buf->peek(start_pos + m_pos, len));
    }

    optional<uint8_t> pop()
    {
        auto v = m_buf->peek(m_pos++, 1);
        if (v.empty()) return v[0];
        else return {};
    }

    std::vector<uint8_t> pop(size_t len)
    {
        m_pos += len;
        return std::move(m_buf->peek(m_pos - len, len));
    }

    void finalize()
    {
        m_buf->advance(m_pos);
        m_pos = 0;
    }
};


static bool serialize_string(const var& v, buffer* buf)
{
    if (buf == nullptr || v.is_empty() || v.get_type() != typeid(std::string))
        return false;

    grab_guard bufgrab(buf);
    const std::string& str = v.get<std::string>();

    buf->push(reinterpret_cast<const uint8_t*>(str.c_str()), str.size()+1);

    return true;
}

static var deserialize_string(buffer* buf)
{
    if (buf == nullptr || buf->available() == 0)
        return {};

    grab_guard bufgrab(buf);
    std::string str;

    while (buf->available())
    {
        optional<uint8_t> b = buf->pop();
        if (!b.is_valid()) return {};

        char c = b.get();
        if (c == '\0') return str;
        else str += c;
    }

    return {};
}


c_serializer::c_serializer(application* app)
 : m_app(app)
{
    add_trivial_rule<int8_t>();
    add_trivial_rule<uint8_t>();
    add_trivial_rule<int16_t>();
    add_trivial_rule<uint16_t>();
    add_trivial_rule<int32_t>();
    add_trivial_rule<uint32_t>();
    add_trivial_rule<int64_t>();
    add_trivial_rule<uint64_t>();
    add_trivial_rule<float>();
    add_trivial_rule<double>();
    add_rule(typeid(std::string), serialize_string, deserialize_string);
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

bool c_serializer::serialize(const var& v, buffer* buf) const
{
    if (buf == nullptr) return false;

    size_t hash = typeinfo(v.get_type()).hash_code();

    grab_guard bufgrab(buf);
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    c_buffer tmpbuf;

    auto rule = m_rules.find(hash);
    if (rule != m_rules.end())
    {
        tmpbuf.push(reinterpret_cast<const uint8_t*>(&hash), sizeof(size_t));
        if (rule->second.m_sfunc(v, &tmpbuf)) // successful serialization
        {
            buf->merge(&tmpbuf);
            return true;
        }
    }

    return false;
}

var c_serializer::deserialize(buffer* buf) const
{
    if (buf == nullptr || buf->available() < sizeof(size_t)) return {};

    grab_guard bufgrab(buf);
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    safe_buffer sbuf(buf);

    size_t hash;
    auto v = sbuf.pop(sizeof(size_t));
    std::memcpy(&hash, v.data(), sizeof(size_t));

    auto rule = m_rules.find(hash);
    if (rule != m_rules.end())
    {
        var v( std::move(rule->second.m_dfunc(&sbuf)) );
        if (!v.is_empty())
        {
            sbuf.finalize();
            return std::move(v);
        }
    }

    return {};
}

varlist c_serializer::deserialize_all(buffer* buf) const
{
    grab_guard bufgrab(buf);
    varlist vl;

    for(;;)
    {
        gg::var v = std::move(deserialize(buf));

        if (v.is_empty()) break;
        else vl.push_back( std::move(v) );
    }

    return std::move(vl);
}
