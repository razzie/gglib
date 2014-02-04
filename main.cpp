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


    gg::serializer* srl = app->get_serializer();
    srl->add_trivial_rule<test>();


    gg::event_manager* evtmgr = app->get_event_manager();
    evtmgr->open_port(9999);
    evtmgr->add_listener("test_event_type", [](const gg::event& e)->bool
    {
        std::cout << "originator: " << e.get_originator()->get_address() << ":" << e.get_originator()->get_port() << "\n" << e << std::endl;
        return true;
    });

    gg::event_dispatcher* evtd = evtmgr->connect("127.0.0.1", 9999);
    evtd->push_event("test_event_type", {{"arg1", 123}, {"arg2", test {4,5,6}}, {"arg3", std::string("abc")}});
    evtd->drop();


    app->get_script_engine()->add_function("exit_program",
            [](int exit_code) { gg::console::get_invoker_console()->get_app()->exit(exit_code); });

    app->get_script_engine()->add_function("echo",
            [](std::string str) { return str; });

    app->get_script_engine()->add_function("is_integer",
            [](std::string i) { return (gg::util::is_integer(i) ? "true" : "false"); },
            true);

    app->get_script_engine()->add_function("is_float",
            [](std::string i) { return (gg::util::is_float(i) ? "true" : "false"); },
            true);

    app->get_script_engine()->add_function("sum",
            [](gg::varlist vl)
            {
                int sum = 0;
                for (auto& v : vl) sum += v.cast<int>();
                return sum;
            });


    gg::console* con = app->create_console();
    con->open();
    //con->on_close(std::bind(&gg::console::open, con));
    con->on_close([]{ gg::console::get_invoker_console()->get_app()->exit(0); });
    con->drop();


    return app->start();
}
