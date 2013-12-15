#include <iomanip>
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

size_t c_buffer::available() const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    return m_data.size();
}

void c_buffer::advance(size_t len)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it_begin = m_data.begin(), it_end = std::next(it_begin, len);
    m_data.erase(it_begin, it_end);
}

void c_buffer::clear()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_data.clear();
}

buffer::byte_array c_buffer::peek(size_t len) const
{
    return std::move(peek((size_t)0, len));
}

buffer::byte_array c_buffer::peek(size_t start_pos, size_t len) const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    byte_array r;

    auto it_begin = std::next(m_data.begin(), start_pos), it_end = std::next(it_begin, len);
    r.insert(r.begin(), it_begin, it_end);

    return std::move(r);
}

size_t c_buffer::peek(uint8_t* buf, size_t len) const
{
    if (buf == nullptr || len == 0) return 0;

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_data.begin(), end = m_data.end();
    size_t i = 0;

    for (; it != end && i < len; ++it, ++i)
    {
        buf[i] = *it;
    }

    return i;
}

size_t c_buffer::peek(size_t start_pos, uint8_t* buf, size_t len) const
{
    if (buf == nullptr || len == 0) return 0;

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    if (start_pos > m_data.size()) return 0;

    auto it = std::next(m_data.begin(), start_pos), end = m_data.end();
    size_t i = 0;

    for (; it != end && i < len; ++it, ++i)
    {
        buf[i] = *it;
    }

    return i;
}

void c_buffer::push(uint8_t byte)
{
    m_data.push_back(byte);
}

void c_buffer::push(const uint8_t* buf, size_t len)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for(size_t i = 0; i < len; ++i)
        m_data.push_back(buf[i]);
}

void c_buffer::push(const byte_array& buf)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);
    m_data.insert(m_data.end(), buf.begin(), buf.end());
}

void c_buffer::push(const buffer* _buf)
{
    const c_buffer* buf = static_cast<const c_buffer*>(_buf);
    if (buf == nullptr) return;

    grab_guard bufgrab(buf);
    tthread::lock_guard<tthread::mutex> guard1(m_mutex);
    tthread::lock_guard<tthread::mutex> guard2(buf->m_mutex);

    m_data.insert(m_data.end(), buf->m_data.begin(), buf->m_data.end());
}

void c_buffer::merge(buffer* _buf)
{
    c_buffer* buf = static_cast<c_buffer*>(_buf);
    if (buf == nullptr) return;

    grab_guard bufgrab(buf);
    tthread::lock_guard<tthread::mutex> guard1(m_mutex);
    tthread::lock_guard<tthread::mutex> guard2(buf->m_mutex);

    m_data.insert(m_data.end(),
                  std::make_move_iterator(buf->m_data.begin()),
                  std::make_move_iterator(buf->m_data.end()));
}

optional<uint8_t> c_buffer::pop()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    if (m_data.empty()) return {};

    uint8_t r = m_data.front();
    m_data.pop_front();
    return r;
}

buffer::byte_array c_buffer::pop(size_t len)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    byte_array r;

    auto it_begin = m_data.begin(), it_end = std::next(it_begin, len);
    r.insert(r.begin(), it_begin, it_end);
    m_data.erase(it_begin, it_end);

    return std::move(r);
}

size_t c_buffer::pop(uint8_t* buf, size_t len)
{
    if (buf == nullptr || len == 0) return 0;

    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    auto it = m_data.begin(), end = m_data.end();
    size_t i = 0;

    for (; it != end && i < len; ++it, ++i)
    {
        buf[i] = *it;
    }

    m_data.erase(m_data.begin(), it);

    return i;
}


std::ostream& gg::operator<< (std::ostream& o, const buffer& _buf)
{
    std::ios state(NULL);
    const c_buffer* buf = static_cast<const c_buffer*>(&_buf);
    int i = 0;

    state.copyfmt(o);
    o << std::setfill('0') << std::hex;
    for (uint8_t c : buf->m_data)
        o << std::setw(2) << (int)c
          << ((++i % 8 == 0) ? "\n" : " ");
    o.copyfmt(state);

    return o;
}

std::ostream& gg::operator<< (std::ostream& o, const buffer* buf)
{
    return o << *buf;
}
