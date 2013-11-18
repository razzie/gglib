#include "gglib.hpp"

gg::application* app;

int add(int a, int b)
{
    return a + b;
}

int sum(gg::varlist vl)
{
    int sum = 0;
    std::for_each(vl.begin(), vl.end(), [&](gg::var& v){ sum += v.cast<int>(); });
    return sum;
}

void exit_program(int exit_code)
{
    gg::script_engine::get_invoker_console()->get_app()->exit(exit_code);
}

int main()
{
    app = gg::application::create_instance("test app", 0, 1);


    gg::console* con = app->create_console();
    con->open();


    app->get_script_engine()->add_function("exit_program", exit_program);

    app->get_script_engine()->add_function("echo", [](std::string str) { return str; }, true);

    app->get_script_engine()->add_function("add", add);
    app->get_script_engine()->add_function("sum", sum);

    app->get_script_engine()->add_function("is_integer",
            [](std::string i){ return (gg::util::is_integer(i) ? "true" : "false"); });

    app->get_script_engine()->add_function("is_float",
            [](std::string i){ return (gg::util::is_float(i) ? "true" : "false"); });

    /*app->get_script_engine()->add_function("sum",
            [](gg::varlist vl)
            {
                int sum = 0;
                std::for_each(vl.begin(), vl.end(), [&](gg::var& v){ sum += v.cast<int>(); });
                std::cout << sum << std::endl;
                return sum;
            });*/


    /*gg::thread* thr = app->get_task_manager()->create_thread("test thread");

    gg::task* t = app->get_task_manager()->create_task(
                        [&] {
                            //std::cerr << "checking console for being opened... "
                            //    << (con->is_opened() ? "true" : "false") << std::endl;
                            if (!con->is_opened()) con->open();
                            thr->add_delayed_task(grab(t), 3000);
                        });

    thr->add_task(grab(t));*/

    return app->start();
}
