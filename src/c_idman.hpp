#ifndef C_IDMAN_HPP_INCLUDED
#define C_IDMAN_HPP_INCLUDED

#include <climits>
#include <random>
#include <set>
#include "tinythread.h"
#include "gg/idman.hpp"

namespace gg
{
    class c_id_manager : public id_manager
    {
        mutable tthread::mutex m_mutex;
        mutable application* m_app;
        std::set<id, id::comparator> m_ids;
        //std::random_device m_rd;
        mutable std::mt19937 m_gen;
        mutable std::uniform_int_distribution<uint32_t> m_dis;

    public:
        c_id_manager(application*);
        ~c_id_manager();
        application* get_app() const;
        id get_random_id() const;
        id get_unique_id();
        bool reserve_id(id);
        void release_id(id);
        bool is_unique(id) const;
    };
};

#endif // C_IDMAN_HPP_INCLUDED
