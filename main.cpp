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
    gg::buffer* buf = gg::buffer::create();

    srl->serialize(123, buf);
    srl->serialize(test {4,5,6}, buf);
    std::cout << srl->deserialize(buf).to_stream() << " " << srl->deserialize(buf).to_stream() << std::endl;

    buf->drop();


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
