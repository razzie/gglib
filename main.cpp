#include "gglib.hpp"

gg::application* app;

int main()
{
    app = gg::application::create_instance("test", 0, 1);

    gg::console* con = app->create_console();
    con->open();

    gg::thread* th = app->get_task_manager()->create_thread("test");
    th->add_delayed_task( app->get_task_manager()->create_task([&]{ app->exit(); }), 5000);

    return app->start();
}
