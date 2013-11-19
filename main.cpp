#include "gglib.hpp"

void exit_program(int exit_code)
{
    gg::console* con = gg::console::get_invoker_console();
    if (con == nullptr) std::exit(exit_code);
    else con->get_app()->exit(exit_code);
}

int main()
{
    gg::application* app = gg::application::create_instance("test app", 0, 1);


    gg::console* con = app->create_console();
    con->open();


    app->get_script_engine()->add_function("exit_program", exit_program);

    app->get_script_engine()->add_function("echo", [](std::string str) { return str; }, true);

    app->get_script_engine()->add_function("is_integer",
            [](std::string i){ return (gg::util::is_integer(i) ? "true" : "false"); });

    app->get_script_engine()->add_function("is_float",
            [](std::string i){ return (gg::util::is_float(i) ? "true" : "false"); });

    app->get_script_engine()->add_function("sum",
            [](gg::varlist vl)
            {
                int sum = 0;
                std::for_each(vl.begin(), vl.end(), [&](gg::var& v){ sum += v.cast<int>(); });
                return sum;
            });


    con->on_close(std::bind(&gg::console::open, con));

    return app->start();
}
