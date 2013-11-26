#ifndef GG_BUFFER_HPP_INCLUDED
#define GG_BUFFER_HPP_INCLUDED

#include <cstdint>
#include <vector>
#include <deque>
#include "gg/refcounted.hpp"
#include "gg/optional.hpp"

namespace gg
{
    class buffer : public reference_counted
    {
    public:
        static buffer* create();
        virtual ~buffer() {}
        virtual std::size_t available() const = 0;
        virtual void push(uint8_t) = 0;
        virtual void push(const uint8_t*, std::size_t) = 0;
        virtual void push(const std::vector<uint8_t>&) = 0;
        virtual optional<uint8_t> pop() = 0;
        virtual std::vector<uint8_t> pop(std::size_t) = 0;
    };
};

#endif // GG_BUFFER_HPP_INCLUDED
