#include "gglib.hpp"

gg::application* app;

int main()
{
    app = gg::application::create_instance("test app", 0, 1);


    gg::console* con = app->create_console();
    con->open();


    app->get_script_engine()->add_function("exit_program", [&](int exit_code){ app->exit(exit_code); });

    app->get_script_engine()->add_function("square", [](int a){ std::cout << a*a; });


    /*gg::thread* thr = app->get_task_manager()->create_thread("test thread");

    gg::task* t = app->get_task_manager()->create_task(
                        [&] {
                            if (!con->is_opened()) con->open();

                            t->grab();
                            thr->add_delayed_task(t, 3000);
                        });

    t->grab();
    thr->add_task(t);*/

    return app->start();
}
