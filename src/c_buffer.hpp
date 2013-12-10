#ifndef C_BUFFER_HPP_INCLUDED
#define C_BUFFER_HPP_INCLUDED

#include "tinythread.h"
#include "gg/buffer.hpp"

namespace gg
{
    class c_buffer : public buffer
    {
        mutable tthread::mutex m_mutex;
        std::deque<uint8_t> m_data;

    public:
        c_buffer();
        ~c_buffer();
        size_t available() const;
        void advance(size_t len);
        void clear();
        std::vector<uint8_t> peek(size_t len) const;
        std::vector<uint8_t> peek(size_t start_pos, size_t len) const;
        void push(uint8_t byte);
        void push(const uint8_t* buf, size_t len);
        void push(const std::vector<uint8_t>& buf);
        void push(const buffer* buf);
        void merge(buffer* buf);
        optional<uint8_t> pop();
        std::vector<uint8_t> pop(size_t len);
    };
};

#endif // C_BUFFER_HPP_INCLUDED
