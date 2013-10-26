#ifndef TASKMAN_HPP_INCLUDED
#define TASKMAN_HPP_INCLUDED

#include <list>
#include "gg/types.hpp"
#include "gg/refcounted.hpp"

namespace gg
{
    class task : public reference_counted
    {
        std::string m_name;
        std::list<task*> m_children;

    public:
        task() : m_name("unknown") {}
        task(std::string name) : m_name(name) {}
        virtual ~task() {}

        void rename(std::string name) { m_name = name; }
        void add_child(task* t) { m_children.push_back(t); }
        const std::list<task*>& get_children() const { return m_children; }

        virtual std::string get_name() const { return std::string("unknown"); }
        virtual bool run(uint32_t elapsed) = 0; // returns true if task is finished
    };

    class thread
    {
    protected:
        virtual ~thread() {}

    public:
        virtual std::string get_name() const = 0;
        virtual void add_task(task*) = 0;
        virtual void add_delayed_task(task*, uint32_t delay_ms) = 0;
        virtual void suspend() = 0;
        virtual void resume() = 0;
    };

    class task_manager
    {
    protected:
        virtual ~task_manager() {}

    public:
        virtual thread* create_thread(std::string name) = 0;
        virtual thread* get_thread(std::string name) = 0;

        virtual task* create_wait_task(uint32_t wait_ms) const = 0;
        virtual task* create_periodic_task(bool(*)(uint32_t)) const = 0;
        virtual task* create_periodic_task(bool(*)(void)) const = 0;
        virtual task* create_task(void(*)(void)) const = 0;
    };
};

#endif // TASKMAN_HPP_INCLUDED
