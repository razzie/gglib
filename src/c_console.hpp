#ifndef CCONSOLE_H_INCLUDED
#define CCONSOLE_H_INCLUDED

#include "win32_aero.hpp"
#include "gg/core.hpp"
#include "gg/console.hpp"
#include "c_taskmgr.hpp"

namespace gg
{

class c_console : public console
{
/* private structures & classes */
private:
    struct render_context
    {
        HWND hwnd;
        HDC primary;
        HDC secondary;
        RECT wndrect;
        HTHEME theme;
        BITMAPINFO bminfo;
        DTTOPTS dttopts;
        HBITMAP bitmap;
        HFONT font;
        unsigned cwidth;
        unsigned cheight;
    };

    enum alignment : unsigned char
    {
        H_LEFT   = 0x0001,
        H_CENTER = 0x0002,
        H_RIGHT  = 0x0004,
        V_TOP    = 0x0008,
        V_CENTER = 0x0010,
        V_BOTTOM = 0x0020
    };

    class c_output : public output, public std::streambuf
    {
        friend class c_console;

        mutable tthread::mutex m_mutex;
        c_console* m_console;
        std::string m_text;
        gg::color m_color;
        unsigned char m_align;
        bool m_visible;
        mutable bool m_dirty;
        mutable std::wstring m_wrapped_text;
        mutable RECT m_last_bounds;
        mutable unsigned m_last_height;

    public:
        c_output(c_output&&);
        c_output(c_console* con);
        c_output(c_console* con, gg::color c, int align, bool visible);
        ~c_output();
        console& get_console() const;

        void show();
        void hide();
        void flag_dirty();
        bool is_dirty() const;
        void draw(const render_context* ctx, RECT* bounds, int caret_pos = -1) const;

        void set_color(gg::color c);
        gg::color get_color() const ;

        void align_left();
        void align_center();
        void align_right();
        void valign_top();
        void valign_center();
        void valign_bottom();

        std::string to_string() const;
        bool is_empty() const;
        void erase();

    protected:
        // inherited from std::streambuf
        virtual int overflow (int c =  std::char_traits<char>::eof());
    };

    class main_task : public task
    {
        c_console* m_con;

    public:
        main_task(c_console* con);
        main_task(const main_task&) = delete;
        main_task(main_task&&) = delete;
        ~main_task();
        bool run(uint32_t unused);
    };

    class cmd_async_exec_task : public task
    {
        std::string m_cmd;
        c_output* m_cmd_outp;
        c_output* m_exec_outp;
        c_console* m_con;
        controller* m_ctrl;
        c_thread* m_thread;

    public:
        cmd_async_exec_task(std::string cmd,
                            c_output* cmd_outp,
                            c_output* exec_outp,
                            c_console* con,
                            c_thread* t);
        ~cmd_async_exec_task();
        bool run(uint32_t);
    };

/* private variables */
private:
    tthread::recursive_mutex m_mutex;
    std::string m_name;
	bool m_open;
	std::list<c_output*> m_outp;
	std::string m_cmd;
	std::string::iterator m_cmd_pos;
	std::vector<std::string> m_cmd_history;
	std::vector<std::string>::iterator m_cmd_history_pos;
	controller* m_ctrl;
	c_thread* m_thread;
    HINSTANCE m_hInst;
    WNDCLASSEX m_wndClassEx;
	HWND m_hWnd;
	HTHEME m_hTheme;
	HFONT m_hFont;
	render_context m_rendctx;

/* private functions */
private:
    void async_open();
    void async_close();
	void cmd_async_exec();
	void cmd_complete();
    bool prepare_render_context(render_context* ctx);
    void finish_render_context(render_context* ctx);
	void paint(const render_context* ctx);
	LRESULT handle_wnd_message(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* public functions */
public:
    c_console(std::string name, controller* ctrl);
    c_console(const c_console&) = delete;
    c_console(c_console&&) = delete;
    ~c_console();
    void set_controller(controller* ctrl);
    controller* get_controller() const;
    void open();
    void close();
    bool is_opened() const;
    bool run();
	void update();
    output* create_output();
    void remove_output(output*);
    void clear();
};

};

#endif // CCONSOLE_H_INCLUDED
