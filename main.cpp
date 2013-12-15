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


class network_handler : public gg::packet_handler, public gg::connection_handler
{
    gg::application* m_app;

public:
    network_handler(gg::application* app)
     : m_app(app)
    {
    }

    ~network_handler()
    {
    }

    void handle_packet(gg::connection* c)
    {
        gg::varlist vl = m_app->get_serializer()->deserialize_all(c->get_input_buffer());
        std::cout << "packet: " << vl << std::endl;
    }

    void handle_connection_open(gg::connection* c)
    {
        std::cout << "connection opened: " << c->get_address() << ":" << c->get_port() << std::endl;
        c->set_packet_handler(this);
    }

    void handle_connection_close(gg::connection* c)
    {
        std::cout << "connection closed: " << c->get_address() << ":" << c->get_port() << std::endl;
    }
};


int main()
{
    gg::application* app = gg::application::create_instance("test app", 0, 1);


    network_handler h(app);
    gg::listener* l = app->get_network_manager()->create_tcp_listener(9999);
    l->set_connection_handler(&h);
    l->open();


    gg::serializer* srl = app->get_serializer();
    gg::buffer* buf = gg::buffer::create();
    srl->serialize(123, buf);
    srl->serialize(std::string("abc"), buf);
    srl->add_trivial_rule<test>();
    srl->serialize(test {4,5,6}, buf);

    gg::connection* c = app->get_network_manager()->create_tcp_connection("127.0.0.1", 9999);
    c->set_connection_handler(&h);
    c->open();
    c->send(buf);
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
