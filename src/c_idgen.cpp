#include <stdexcept>
#include "c_idgen.hpp"
#include "gg/var.hpp"
#include "gg/optional.hpp"
#include "gg/serializer.hpp"
#include "gg/application.hpp"

using namespace gg;

id::id(uint32_t _id)
 : m_id(_id)
{
}

id::id(std::string)
{
    throw std::runtime_error("id initialization from hex unimplemented");
}

id::id(const id& _id)
 : m_id(_id.m_id)
{
}

id::~id()
{
}

id& id::operator= (const id& _id)
{
    m_id = _id.m_id;
    return *this;
}

bool id::operator== (const id& _id) const
{
    return (m_id == _id.m_id);
}

bool id::operator!= (const id& _id) const
{
    return (m_id != _id.m_id);
}

id::operator uint32_t() const
{
    return m_id;
}

std::string id::get_hex() const
{
    static const char hex_table[] = "0123456789ABCDEF";

    auto tmp_id = m_id;
    std::string hex;

    for (size_t i = 0; i < sizeof(tmp_id)*2; ++i)
        hex.append(&hex_table[(tmp_id << 16) % 16], 1);

    return hex;
}

bool id::comparator::operator() (const id& a, const id& b) const
{
    return (static_cast<uint32_t>(a) < static_cast<uint32_t>(b));
}

bool serialize_id(const var& v, buffer* buf)
{
    if (buf == nullptr || v.get_type() != typeid(id))
        return false;

    uint32_t _id = static_cast<uint32_t>(v.get<id>());
    buf->push(reinterpret_cast<uint8_t*>(&_id), sizeof(uint32_t));

    return true;
}

optional<var> deserialize_id(buffer* buf)
{
    if (buf == nullptr || buf->available() < sizeof(uint32_t)) return {};

    uint32_t _id;
    buf->pop(reinterpret_cast<uint8_t*>(&_id), sizeof(uint32_t));

    return var(id(_id));
}


c_id_generator::c_id_generator(application* app)
 : m_app(app)
 //, m_gen(m_rd())
 , m_dis(0, UINT_MAX)
{
    m_app->get_serializer()->add_rule<id>(serialize_id, deserialize_id);
}

c_id_generator::~c_id_generator()
{
}

application* c_id_generator::get_app() const
{
    return m_app;
}

id c_id_generator::get_random_id() const
{
    return m_dis(m_gen);
}

id c_id_generator::get_unique_id()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    id _id = get_random_id();
    for (int tries = 0; m_ids.count(_id) > 0; _id = get_random_id(), ++tries)
    {
        if (tries > 1000)
            throw std::runtime_error("couldn't generate unique id");
    }

    m_ids.insert(_id);
    return _id;
}

bool c_id_generator::reserve_id(id _id)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    auto ret = m_ids.insert(_id);
    return ret.second;
}

void c_id_generator::release_id(id _id)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_ids.erase(_id);
}

bool c_id_generator::is_unique(id _id) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return (m_ids.count(_id) > 0);
}
