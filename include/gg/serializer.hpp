#ifndef GG_SERIALIZER_HPP_INCLUDED
#define GG_SERIALIZER_HPP_INCLUDED

#include <cstdint>
#include <cstring>
#include <functional>
#include "gg/var.hpp"
#include "gg/refcounted.hpp"
#include "gg/optional.hpp"
#include "gg/buffer.hpp"
#include "gg/typeinfo.hpp"

namespace gg
{
    class application;

    class serializer
    {
    protected:
        virtual ~serializer() {}

    public:
        typedef std::function<bool(const var&,buffer*)> serializer_func;
        typedef std::function<var(buffer*)> deserializer_func;

        virtual application* get_app() const = 0;
        virtual void add_rule(typeinfo, serializer_func, deserializer_func) = 0;
        virtual bool serialize(const var&, buffer*) const = 0;
        virtual var deserialize(buffer*) const = 0;

        template<class T>
        void add_rule(serializer_func s, deserializer_func d)
        {
            this->add_rule(typeid(T), s, d);
        }

        template<class T>
        void add_trivial_rule()
        {
            serializer_func s = [](const var& v, buffer* buf)->bool
            {
                if (buf == nullptr)
                    return false;

                buf->push(reinterpret_cast<const uint8_t*>(v.get_ptr<T>()), sizeof(T));
                return true;
            };

            deserializer_func d = [](buffer* buf)->var
            {
                if (buf == nullptr || (buf->available() < sizeof(T)))
                    return {};

                T t;
                std::memcpy(&t, buf->pop(sizeof(T)).data(), sizeof(T));
                return std::move(t);
            };

            this->add_rule(typeid(T), s, d);
        }
    };
};

#endif // GG_SERIALIZER_HPP_INCLUDED
