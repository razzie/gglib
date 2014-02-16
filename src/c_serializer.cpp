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
    void push(const byte_array& buf) { m_buf->push(buf); }
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

    byte_array peek(size_t len) const
    {
        return std::move(m_buf->peek(m_pos, len));
    }

    byte_array peek(size_t start_pos, size_t len) const
    {
        return std::move(m_buf->peek(start_pos + m_pos, len));
    }

    size_t peek(uint8_t* buf, size_t len) const
    {
        return m_buf->peek(m_pos, buf, len);
    }

    size_t peek(size_t start_pos, uint8_t* buf, size_t len) const
    {
        return m_buf->peek(start_pos + m_pos, buf, len);
    }

    optional<uint8_t> pop()
    {
        auto v = m_buf->peek(m_pos++, 1);
        if (!v.empty()) return v[0];
        else return {};
    }

    byte_array pop(size_t len)
    {
        m_pos += len;
        return std::move(m_buf->peek(m_pos - len, len));
    }

    size_t pop(uint8_t* buf, size_t len)
    {
        size_t rc = m_buf->peek(m_pos, buf, len);
        m_pos += len;
        return rc;
    }

    void finalize()
    {
        m_buf->advance(m_pos);
        m_pos = 0;
    }
};


bool gg::serialize_varlist(const var& v, buffer* buf, const serializer* s)
{
    if (buf == nullptr || s == nullptr || v.get_type() != typeid(varlist)) return false;

    const varlist& vl = v.get<varlist>();
    uint16_t vlsize = vl.size();

    buf->push(reinterpret_cast<const uint8_t*>(&vlsize), sizeof(uint16_t));

    for (const var& subv : vl)
    {
        if (!s->serialize(subv, buf)) return false;
    }

    return true;
}

optional<var> gg::deserialize_varlist(buffer* buf, const serializer* s)
{
    if (buf == nullptr || s == nullptr || buf->available() < 2) return {};

    varlist vl;
    uint16_t vlsize;

    buf->pop(reinterpret_cast<uint8_t*>(&vlsize), sizeof(uint16_t));

    for (uint16_t i = 0; i < vlsize; ++i)
    {
        optional<var> v = s->deserialize(buf);
        if (!v.is_valid()) return {};
        vl.push_back(std::move(*v));
    }

    return std::move(vl);
}


bool gg::serialize_string(const var& v, buffer* buf)
{
    if (buf == nullptr || v.get_type() != typeid(std::string)) return false;

    grab_guard bufgrab(buf);
    const std::string& str = v.get<std::string>();
    uint16_t len = str.length();

    buf->push(reinterpret_cast<uint8_t*>(&len), sizeof(uint16_t));
    buf->push(reinterpret_cast<const uint8_t*>(str.c_str()), len);

    return true;
}

optional<var> gg::deserialize_string(buffer* buf)
{
    if (buf == nullptr || buf->available() < 2) return {};

    grab_guard bufgrab(buf);

    uint16_t len;
    buf->pop(reinterpret_cast<uint8_t*>(&len), sizeof(uint16_t));

    if (buf->available() < len) return {};

    auto data = buf->pop(len);
    return std::move( std::string(data.begin(), data.end()) );
}


static bool serialize_void(const var& v, buffer* buf)
{
    if (buf == nullptr || v.get_type() != typeid(void)) return false;
    else return true;
}

static optional<var> deserialize_void(buffer* buf)
{
    if (buf == nullptr) return {};
    else return var();
}


#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

static uint64_t pack754(long double f, unsigned bits, unsigned expbits)
{
    long double fnorm;
    int shift;
    long long sign, exp, significand;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (f == 0.0) return 0; // get this special case out of the way

    // check sign and begin normalization
    if (f < 0) { sign = 1; fnorm = -f; }
    else { sign = 0; fnorm = f; }

    // get the normalized form of f and track the exponent
    shift = 0;
    while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
    while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
    fnorm = fnorm - 1.0;

    // calculate the binary form (non-float) of the significand data
    significand = fnorm * ((1LL<<significandbits) + 0.5f);

    // get the biased exponent
    exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

    // return the final answer
    return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

static long double unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (i == 0) return 0.0;

    // pull the significand
    result = (i&((1LL<<significandbits)-1)); // mask
    result /= (1LL<<significandbits); // convert back to float
    result += 1.0f; // add the one back on

    // deal with the exponent
    bias = (1<<(expbits-1)) - 1;
    shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; shift--; }
    while(shift < 0) { result /= 2.0; shift++; }

    // sign it
    result *= (i>>(bits-1))&1? -1.0: 1.0;

    return result;
}

bool gg::serialize_float(const var& v, buffer* buf)
{
    if (buf == nullptr || v.get_type() != typeid(float))
        return false;

    uint64_t data = pack754_32(v.get<float>());
    buf->push(reinterpret_cast<uint8_t*>(&data), sizeof(uint64_t));

    return true;
}

optional<var> gg::deserialize_float(buffer* buf)
{
    if (buf == nullptr || buf->available() < sizeof(uint64_t))
        return {};

    uint64_t data;
    buf->pop(reinterpret_cast<uint8_t*>(&data), sizeof(uint64_t));

    float f = unpack754_32(data);
    return f;
}

bool gg::serialize_double(const var& v, buffer* buf)
{
    if (buf == nullptr || v.get_type() != typeid(double))
        return false;

    uint64_t data = pack754_64(v.get<double>());
    buf->push(reinterpret_cast<uint8_t*>(&data), sizeof(uint64_t));

    return true;
}

optional<var> gg::deserialize_double(buffer* buf)
{
    if (buf == nullptr || buf->available() < sizeof(uint64_t))
        return {};

    uint64_t data;
    buf->pop(reinterpret_cast<uint8_t*>(&data), sizeof(uint64_t));

    double d = unpack754_64(data);
    return d;
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
    //add_trivial_rule<float>();
    //add_trivial_rule<double>();
    add_rule_ex(typeid(varlist), serialize_varlist, deserialize_varlist);
    add_rule(typeid(std::string), serialize_string, deserialize_string);
    add_rule(typeid(void), serialize_void, deserialize_void);
    add_rule(typeid(float), serialize_float, deserialize_float);
    add_rule(typeid(double), serialize_double, deserialize_double);
}

c_serializer::~c_serializer()
{
}

application* c_serializer::get_app() const
{
    return m_app;
}

void c_serializer::add_rule_ex(typeinfo ti, serializer_func_ex sfunc, deserializer_func_ex dfunc)
{
    if (m_rules.count(ti.get_hash()) > 0)
        throw std::runtime_error("rule already added");

    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    auto r = m_rules.insert( std::make_pair(ti.get_hash(), rule {ti, sfunc, dfunc}) );

    if (!r.second)
        throw std::runtime_error("failed to add rule");
}

void c_serializer::add_rule(typeinfo ti, serializer_func sfunc, deserializer_func dfunc)
{
    add_rule_ex(ti,
        [=](const var& v, buffer* buf, const serializer*)->bool { return sfunc(v, buf); },
        [=](buffer* buf, const serializer*)->var { return dfunc(buf); });
}

void c_serializer::remove_rule(typeinfo ti)
{
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    auto pos = m_rules.find(ti.get_hash());
    if (pos != m_rules.end())
    {
        m_rules.erase(pos);
    }
}

bool c_serializer::serialize(const var& v, buffer* buf) const
{
    if (buf == nullptr) return false;

    size_t hash = typeinfo(v.get_type()).get_hash();

    grab_guard bufgrab(buf);
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    c_buffer tmpbuf;

    auto rule = m_rules.find(hash);
    if (rule != m_rules.end())
    {
        tmpbuf.push(reinterpret_cast<const uint8_t*>(&hash), sizeof(size_t));
        if (rule->second.m_sfunc(v, &tmpbuf, this)) // successful serialization
        {
            buf->merge(&tmpbuf);
            return true;
        }
    }

    return false;
}

optional<var> c_serializer::deserialize(buffer* buf) const
{
    if (buf == nullptr || buf->available() < sizeof(size_t)) return {};

    grab_guard bufgrab(buf);
    tthread::lock_guard<tthread::recursive_mutex> guard(m_mutex);

    safe_buffer sbuf(buf);

    size_t hash;
    auto v = sbuf.pop(sizeof(size_t));
    std::memcpy(&hash, v.data(), sizeof(size_t));

    auto rule = m_rules.find(hash);
    if (rule != m_rules.end())
    {
        optional<var> v = std::move(rule->second.m_dfunc(&sbuf, this));
        if (v.is_valid())
        {
            sbuf.finalize();
            return std::move(v.get());
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
        optional<var> v = std::move(deserialize(buf));
        if (v.is_valid()) vl.push_back( std::move(v.get()) );
        else break;
    }

    return std::move(vl);
}
