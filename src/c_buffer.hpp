#ifndef C_BUFFER_HPP_INCLUDED
#define C_BUFFER_HPP_INCLUDED

#include "gg/buffer.hpp"

namespace gg
{
    class c_buffer : public buffer
    {
        std::deque<uint8_t> m_data;

    public:
        c_buffer();
        ~c_buffer();
        std::size_t available() const;
        void push(uint8_t);
        void push(const uint8_t*, std::size_t);
        void push(const std::vector<uint8_t>&);
        optional<uint8_t> pop();
        std::vector<uint8_t> pop(std::size_t);
    };
};

#endif // C_BUFFER_HPP_INCLUDED
