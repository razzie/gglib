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
            typeinfo m_type;
            serializer_func m_sfunc;
            deserializer_func m_dfunc;
        };

        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::map<size_t, rule> m_rules;

    public:
        c_serializer(application* app);
        ~c_serializer();
        application* get_app() const;
        void add_rule(typeinfo, serializer_func, deserializer_func);
        bool serialize(const var&, buffer*) const;
        optional<var> deserialize(typeinfo, buffer*) const;
    };
};

#endif // C_SERIALIZER_HPP_INCLUDED
