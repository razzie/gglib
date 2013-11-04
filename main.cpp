#include <iostream>
#include <string>
#include <locale>
#include "gg/types.hpp"
#include "gg/var.hpp"
#include "gg/varutil.hpp"
#include "c_console.hpp"
#include "c_taskmgr.hpp"
#include "c_eventmgr.hpp"
#include "managed_cout.hpp"
#include "expression.hpp"

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

class add_class
{
public:
    int add(int a, int b)
    {
        return a + b;
    }
};

int main()
{
    setlocale(LC_ALL, "");

    try
    {
        //std::cout << gg::util::has_insert_op<console_controller>::value << std::endl;

        std::cout << gg::varlist {"lofasz", 123, 5.5f, false} << std::endl;

        std::cout << gg::var("abc").get<std::string>() << std::endl;

        gg::expression e("aaa(bbb,ccc(ddd))");
        std::cout << e << std::endl << e.get_expression() << std::endl;
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


    drop_test( gg::auto_drop<gg::task>(taskmgr->create_wait_task(5000)) );


    add_class aa;
    std::function<int(int,int)> add_func = std::bind(&add_class::add, &aa, std::placeholders::_1, std::placeholders::_2);
    std::cout << gg::util::call_function(add_func, gg::varlist {(int)3, (int)8} ) << std::endl;

    /*int(*add_func2)(int,int) = [](int a, int b)->int { return a+b; };
    gg::var add_func2_result = gg::util::call_function(gg::util::adaptfunc(add_func2), gg::varlist {(int)3, (int)8} );
    std::cout << add_func2_result.to_stream() << std::endl;*/

    std::cout << gg::util::call_function(
                    gg::util::make_function([](int a, int b)->int { return a+b; }),
                    gg::varlist { (int)3, (int)8 })
              << std::endl;

    //std::cout << gg::util::call_function([](int a, int b)->int { return a+b; }, gg::varlist { (int)3, (int)8 }) << std::endl;


    gg::c_console con("test console", gg::auto_drop<console_controller>(new console_controller()) );
    con.open();

    gg::managed_cout::get_instance()->enable();
    {
        gg::console::output* o = con.create_output();
        //gg::auto_drop<gg::console::output> drop_o(o);
        gg::managed_cout::hook h(*o);
        std::cout << "Hello world hooked!" << std::endl;
    }


    while (con.run());


    delete taskmgr;
    taskmgr = nullptr;
    delete eventmgr;
    eventmgr = nullptr;

    return 0;
}
