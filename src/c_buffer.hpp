#ifndef C_BUFFER_HPP_INCLUDED
#define C_BUFFER_HPP_INCLUDED

#include <deque>
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

        byte_array peek(size_t len) const;
        byte_array peek(size_t start_pos, size_t len) const;
        size_t peek(uint8_t* buf, size_t len) const;
        size_t peek(size_t start_pos, uint8_t* buf, size_t len) const;

        void push(uint8_t byte);
        void push(const uint8_t* buf, size_t len);
        void push(const byte_array& buf);
        void push(const buffer* buf);
        void merge(buffer* buf);

        optional<uint8_t> pop();
        byte_array pop(size_t len);
        size_t pop(uint8_t* buf, size_t len);

        friend std::ostream& operator<< (std::ostream&, const buffer&);
        friend std::ostream& operator<< (std::ostream&, const buffer*);
    };
};

#endif // C_BUFFER_HPP_INCLUDED
