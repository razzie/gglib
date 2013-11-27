#include "c_buffer.hpp"

using namespace gg;


buffer* buffer::create()
{
    return new c_buffer();
}

c_buffer::c_buffer()
{
}

c_buffer::~c_buffer()
{
}

std::size_t c_buffer::available() const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return m_data.size();
}

void c_buffer::advance(std::size_t len)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it_begin = m_data.begin(), it_end = std::next(it_begin, len);
    m_data.erase(it_begin, it_end);
}

std::vector<uint8_t> c_buffer::peek(std::size_t len) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    std::vector<uint8_t> r;

    auto it_begin = m_data.begin(), it_end = std::next(it_begin, len);
    r.insert(r.begin(), it_begin, it_end);

    return std::move(r);
}

void c_buffer::push(uint8_t byte)
{
    m_data.push_back(byte);
}

void c_buffer::push(const uint8_t* bytes, std::size_t len)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for(std::size_t i = 0; i < len; ++i)
        m_data.push_back(bytes[i]);
}

void c_buffer::push(const std::vector<uint8_t>& bytes)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_data.insert(m_data.end(), bytes.begin(), bytes.end());
}

optional<uint8_t> c_buffer::pop()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    if (m_data.empty())
        return optional<uint8_t>();

    uint8_t r = m_data.front();
    m_data.pop_front();
    return r;
}

std::vector<uint8_t> c_buffer::pop(std::size_t len)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    std::vector<uint8_t> r;

    auto it_begin = m_data.begin(), it_end = std::next(it_begin, len);
    r.insert(r.begin(), it_begin, it_end);
    m_data.erase(it_begin, it_end);

    return std::move(r);
}
