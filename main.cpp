#include "gglib.hpp"

int main()
{
    gg::application* app = gg::application::create_instance("test app", 0, 1);


    gg::serializer* srl = app->get_serializer();
    gg::buffer* buf = gg::buffer::create();
    srl->serialize(123, buf);
    gg::optional<gg::var> v = srl->deserialize(buf);
    buf->drop();

    if (v.is_valid())
        std::cout << v.get().to_stream() << std::endl;
    else
        std::cout << "invalid result" << std::endl;


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
