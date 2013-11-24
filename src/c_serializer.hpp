#ifndef C_SERIALIZER_HPP_INCLUDED
#define C_SERIALIZER_HPP_INCLUDED

#include <map>
#include "tinythread.h"
#include "gg/serializer.hpp"

namespace gg
{
    class c_serializer : public serializer
    {
        struct rule
        {
            serializer_func m_sfunc;
            deserializer_func m_dfunc;
        };

        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::map<typeinfo, rule> m_rules;

    public:
        class c_storage : public storage
        {
            std::vector<uint8_t> m_data;
            std::vector<uint8_t>::iterator m_pos;

        public:
            c_storage();
            ~c_storage();
            std::size_t get_size() const;
            bool is_empty() const;
            void push(uint8_t);
            void push(const uint8_t*, std::size_t);
            void push(const std::vector<uint8_t>&);
            const uint8_t* get() const;
            optional<uint8_t> pop();
            std::vector<uint8_t> pop(std::size_t);
        };

        c_serializer(application* app);
        ~c_serializer();
        application* get_app() const;
        storage* create_storage() const;
        void add_rule(typeinfo, serializer_func, deserializer_func);
        bool serialize(typeinfo, const var&, storage&) const;
        optional<var> deserialize(typeinfo, storage&) const;
    };
};

#endif // C_SERIALIZER_HPP_INCLUDED
