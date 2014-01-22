#ifndef C_IDGEN_HPP_INCLUDED
#define C_IDGEN_HPP_INCLUDED

#include <climits>
#include <random>
#include <set>
#include "tinythread.h"
#include "gg/id.hpp"

namespace gg
{
    class c_id_generator : public id_generator
    {
        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::set<id, id::comparator> m_ids;
        //std::random_device m_rd;
        mutable std::mt19937 m_gen;
        mutable std::uniform_int_distribution<uint32_t> m_dis;

    public:
        c_id_generator(application*);
        ~c_id_generator();
        application* get_app() const;
        id get_random_id() const;
        id get_unique_id();
        bool reserve_id(id);
        void release_id(id);
        bool is_unique(id) const;
    };
};

#endif // C_IDGEN_HPP_INCLUDED
