#ifndef GG_SERIALIZER_HPP_INCLUDED
#define GG_SERIALIZER_HPP_INCLUDED

#include <cstdint>
#include <vector>
#include <cstring>
#include <typeinfo>
#include <functional>
#include <stdexcept>
#include "gg/var.hpp"
#include "gg/refcounted.hpp"
#include "gg/optional.hpp"
#include "gg/typeinfo.hpp"

namespace gg
{
    class application;

    class serializer
    {
    protected:
        virtual ~serializer() {}

    public:
        class storage : public reference_counted
        {
        public:
            virtual ~storage() {}
            virtual std::size_t get_size() const = 0;
            virtual bool is_empty() const = 0;
            virtual void push(uint8_t) = 0;
            virtual void push(const uint8_t*, std::size_t) = 0;
            virtual void push(const std::vector<uint8_t>&) = 0;
            virtual const uint8_t* get() const = 0;
            virtual optional<uint8_t> pop() = 0;
            virtual std::vector<uint8_t> pop(std::size_t) = 0;
        };

        typedef std::function<bool(const var&,storage&)> serializer_func;
        typedef std::function<optional<var>(storage&)> deserializer_func;

        virtual application* get_app() const = 0;
        virtual storage* create_storage() const = 0;
        virtual void add_rule(typeinfo, serializer_func, deserializer_func) = 0;
        virtual bool serialize(typeinfo, const var&, storage&) const = 0;
        virtual optional<var> deserialize(typeinfo, storage&) const = 0;

        template<class T>
        void add_rule(serializer_func s, deserializer_func d)
        {
            this->add_rule(typeid(T), s, d);
        }

        template<class T>
        void add_trivial_rule()
        {
            serializer_func s = [](const var& v, storage& st)
            {
                st.push(reinterpret_cast<const uint8_t*>(v.get_ptr<T>()), sizeof(T));
                return true;
            };

            deserializer_func d = [](storage& st)->optional<var>
            {
                if (st.get_size() < sizeof(T))
                    throw std::runtime_error("insufficient length of data for deserialization");

                T t;
                std::memcpy(&t, st.get(), sizeof(T));
                return var(std::move(t));
            };

            this->add_rule(typeid(T), s, d);
        }

        template<class T>
        bool serialize(const var& v, storage& st)
        {
            return this->serialize(typeid(T), v, st);
        }

        template<class T>
        optional<var> deserialize(storage& st)
        {
            return this->deserialize(typeid(T), st);
        }
    };
};

#endif // GG_SERIALIZER_HPP_INCLUDED
