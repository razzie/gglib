#include "gglib.hpp"


struct test
{
    int a, b, c;
};

std::ostream& operator<< (std::ostream& o, const test& t)
{
    o << "{" << t.a << "," << t.b << "," << t.c << "}";
    return o;
}


int main()
{
    gg::application* app = gg::application::create_instance("test app", 0, 1);


    gg::event_manager* evtmgr = app->get_event_manager();
    evtmgr->add_listener("test_event_type", [](const gg::event& e)->bool
    {
        std::cout << "originator: " << e.get_originator()->get_address() << " : " << e.get_originator()->get_port()
                  << "\n" << e.get_attributes() << std::endl;
        return true;
    });
    evtmgr->open_port(9999);
    gg::event_dispatcher* evtd = evtmgr->connect("127.0.0.1", 9999);
    //evtd->push_event("test_event_type", {{"arg1", 123}, {"arg2", std::string("abc")}, {"arg3", test {4,5,6}}});
    //evtd->drop();



    gg::console* con = app->create_console();
    con->open();


    app->get_script_engine()->add_function("exit_program",
            [](int exit_code) { gg::console::get_invoker_console()->get_app()->exit(exit_code); });

    app->get_script_engine()->add_function("echo",
            [](std::string str) { return str; });

    app->get_script_engine()->add_function("is_integer",
            [](std::string i){ return (gg::util::is_integer(i) ? "true" : "false"); },
            true);

    app->get_script_engine()->add_function("is_float",
            [](std::string i){ return (gg::util::is_float(i) ? "true" : "false"); },
            true);

    app->get_script_engine()->add_function("sum",
            [](gg::varlist vl)
            {
                int sum = 0;
                for (auto v : vl) sum += v.cast<int>();
                return sum;
            });


    con->on_close(std::bind(&gg::console::open, con));

    return app->start();
}
