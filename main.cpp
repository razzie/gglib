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


std::ostream& operator<< (std::ostream& o, const gg::remote_application* rem_app)
{
    if (rem_app == nullptr) o << "[this machine]";
    else o << "[" << rem_app->get_address() << ":" << rem_app->get_port() << "]";
    return o;
}


int main()
{
    gg::application* app = gg::application::create("test app");
    app->open_port(9999, nullptr);


    float f1, f2, f3;
    std::tie(f1, f2, f3) = gg::util::parse<float,float,float>("123;456;-1.23", ';');
    std::cout << f1 << "; " << f2 << "; " << f3 << std::endl;


    gg::serializer* srl = app->get_serializer();
    srl->add_trivial_rule<test>();


    gg::event_manager* evtmgr = app->get_event_manager();
    evtmgr->add_listener("test_event_type", [](const gg::event& e)->bool
    {
        std::cout << e.get_originator() << ": " << e << std::endl;
        return true;
    });


    gg::remote_application* rem_app = app->connect("127.0.0.1", 9999, {});
    if (rem_app->connect())
    {
        std::cout << "Successfully connected!" << std::endl;
        rem_app->push_event("test_event_type", {{"arg1", 123}, {"arg2", test {4,5,6}}, {"arg3", std::string("abc")}});
    }
    rem_app->drop();


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

    app->get_script_engine()->add_function("color",
            [](unsigned R, unsigned G, unsigned B)
            {
                gg::console::output* o = gg::console::get_invoker_output();
                o->set_color( {(uint8_t)R, (uint8_t)G, (uint8_t)B} );
                *o << "(R: " << R << ", G: " << G << ", B: " << B << ")";
            });


    gg::console* con = app->create_console();
    con->open();
    //con->on_close(std::bind(&gg::console::open, con));
    con->on_close([]{ gg::console::get_invoker_console()->get_app()->exit(0); });
    con->drop();


    return app->start();
}
