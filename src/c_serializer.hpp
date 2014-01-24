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
            serializer_func_ex m_sfunc;
            deserializer_func_ex m_dfunc;
        };

        mutable tthread::recursive_mutex m_mutex;
        mutable application* m_app;
        std::map<size_t, rule> m_rules;

    public:
        c_serializer(application* app);
        ~c_serializer();
        application* get_app() const;
        void add_rule_ex(typeinfo, serializer_func_ex, deserializer_func_ex);
        void add_rule(typeinfo, serializer_func, deserializer_func);
        void remove_rule(typeinfo);
        bool serialize(const var&, buffer*) const;
        optional<var> deserialize(buffer*) const;
        varlist deserialize_all(buffer*) const;
    };
};

#endif // C_SERIALIZER_HPP_INCLUDED
