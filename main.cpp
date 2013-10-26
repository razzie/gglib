#include <iostream>
#include <string>
#include <locale>
#include "gg/types.hpp"
#include "gg/var.hpp"

#include "c_console.hpp"
#include "c_taskmgr.hpp"
#include "c_eventmgr.hpp"

gg::c_task_manager* taskmgr = nullptr;
gg::c_event_manager* eventmgr = nullptr;

class console_controller : public gg::console::controller
{
public:
    bool exec(std::string& cmd, gg::console::output& o)
    {
        if (cmd.compare("exit") == 0)
        {
            o.get_console().close();
            return true;
        }
        else if (cmd.compare("clear") == 0)
        {
            o.get_console().clear();
            return true;
        }
        else if (cmd.compare("red") == 0)
        {
            o.set_color({255,0,0});
            o << "some red text!";
            return true;
        }
        else if (cmd.compare("task") == 0)
        {
            taskmgr->get_thread("test thread")->add_task( taskmgr->create_task([]{ std::cout << "taasskkk!!" << std::endl; }) );
            o.set_color({0,0,255});
            o << "task added!";
            return true;
        }
        return false;
    }

    void complete(std::string& cmd, gg::console::output& o)
    {
        //o << "complete: " << cmd << "!";
    }
};

void drop_test(const gg::reference_counted* o)
{
    std::cout << "[drop_test] ref count: " << o->get_ref_count() << std::endl;
}

int main()
{
    setlocale(LC_ALL, "");

    try
    {
        //std::cout << gg::util::has_insert_op<console_controller>::value << std::endl;

        std::cout << gg::varlist {"lofasz", 123, 5.5f, false} << std::endl;

        std::cout << gg::var("abc").get<std::string>() << std::endl;
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }

    taskmgr = new gg::c_task_manager();
    eventmgr = new gg::c_event_manager();

    gg::task* t1 = taskmgr->create_wait_task(5000);
    gg::task* t2 = taskmgr->create_task([](){ std::cout << "aaaaaaaaa!" << std::endl; });
    t1->add_child(t2);
    taskmgr->create_thread("test thread")->add_task(t1);


    gg::event_type* evt_type = eventmgr->create_event_type("aaa_event");
    evt_type->add_listener([](const gg::event& evt) -> bool
        {
            std::cout << evt["text"].to_stream() << std::endl;
            return true;
        });
    eventmgr->push_event( new gg::event("aaa_event", {{"num", 123}, {"text", "lofasz!"}}) );


    drop_test( gg::auto_drop(taskmgr->create_wait_task(5000)) );


    gg::c_console con("test console", new console_controller());
    con.open();
    while (con.run(0));


    delete taskmgr;
    taskmgr = nullptr;
    delete eventmgr;
    eventmgr = nullptr;

    return 0;
}
