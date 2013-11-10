#include "gglib.hpp"

gg::application* app;

int main()
{
    app = gg::application::create_instance("test app", 0, 1);


    gg::console* con = app->create_console();
    con->open();


    app->get_script_engine()->add_function("exit_program", [&](int exit_code){ app->exit(exit_code); });

    app->get_script_engine()->add_function("is_integer",
            [](std::string i){ std::cout << (gg::util::is_integer(i) ? "true" : "false"); });

    app->get_script_engine()->add_function("is_float",
            [](std::string i){ std::cout << (gg::util::is_float(i) ? "true" : "false"); });

    app->get_script_engine()->add_function("add",
            [](int a, int b){ std::cout << a << "+" << b << "=" << a+b << "  "; return a+b; });


    gg::thread* thr = app->get_task_manager()->create_thread("test thread");

    gg::task* t = app->get_task_manager()->create_task(
                        [&] {
                            if (!con->is_opened()) con->open();
                            thr->add_delayed_task(grab(t), 3000);
                        });

    thr->add_task(grab(t));

    return app->start();
}
