#ifndef GG_SERIALIZER_HPP_INCLUDED
#define GG_SERIALIZER_HPP_INCLUDED

#include <cstdint>
#include <typeinfo>
#include <functional>
#include <vector>
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
            virtual void add(uint8_t) = 0;
            virtual void add(uint8_t*, std::size_t) = 0;
            virtual void add(std::vector<uint8_t>) = 0;
            //virtual uint8_t* get(std::size_t = 1) = 0;
            virtual std::vector<uint8_t> get(std::size_t) = 0;
            virtual uint8_t operator[] (std::size_t) = 0;
        };

        typedef std::function<void(var,storage&)> constructor;
        typedef std::function<var(storage&)> destructor;

        virtual storage* create_storage() const = 0;
        virtual void add_rule(typeinfo, constructor, destructor) = 0;
        virtual void add_trivial_rule(typeinfo) = 0;
        virtual bool serialize(typeinfo, uint8_t*, storage&) const = 0;
        virtual optional<var> deserialize(typeinfo, storage&) const = 0;

        template<class T>
        void add_rule(constructor ctor, destructor dtor)
        {
            this->add_rule(typeid(T), ctor, dtor);
        }

        template<class T>
        void add_trivial_rule()
        {
            this->add_trivial_rule(typeid(T));
        }

        template<class T>
        bool serialize(T* p, storage& st)
        {
            return this->serialize(typeid(T), static_cast<uint8_t*>(p), st);
        }

        template<class T>
        optional<var> deserialize(storage& st)
        {
            return this->deserialize(typeid(T), st);
        }
    };
};

#endif // GG_SERIALIZER_HPP_INCLUDED
