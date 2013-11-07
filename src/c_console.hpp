#ifndef CCONSOLE_H_INCLUDED
#define CCONSOLE_H_INCLUDED

#include <iostream>
#include <sstream>
#include <list>
#define _WIN32_WINNT	0x0501
#define _WIN32_IE       0x0501
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include "gg/types.hpp"
#include "gg/console.hpp"
#include "c_taskmgr.hpp"

// I've included the following for people without dwmapi.h
typedef struct _DWM_BLURBEHIND {
	DWORD dwFlags;
	BOOL  fEnable;
	HRGN  hRgnBlur;
	BOOL  fTransitionOnMaximized;
} DWM_BLURBEHIND, *PDWM_BLURBEHIND;
#define DWM_BB_ENABLE					0x00000001
#define DWM_BB_BLURREGION				0x00000002
#define DWM_BB_TRANSITIONONMAXIMIZED	0x00000004

#define DTT_TEXTCOLOR   (1UL << 0)
#define DTT_BORDERCOLOR (1UL << 1)
#define DTT_BORDERSIZE  (1UL << 5)
#define DTT_GLOWSIZE    (1UL << 11)
#define DTT_COMPOSITED  (1UL << 13)
#define DTT_CALCRECT    (1UL << 9)

#define TMT_TEXTGLOWSIZE    0x0979 // 2425
#define TMT_FONT            0x00d2 // 210

//#define TEXT_BODYTEXT   0x0004 // 4
//#define TEXT_LABEL      0x0008 // 8

#define TEXTCOLOR_SYSTEM (0xff000000)

typedef int (WINAPI *DTT_CALLBACK_PROC)
(
  HDC hdc,
  LPWSTR pszText,
  int cchText,
  LPRECT prc,
  UINT dwFlags,
  LPARAM lParam
);

typedef struct _DTTOPTS {
  DWORD             dwSize;
  DWORD             dwFlags;
  COLORREF          crText;
  COLORREF          crBorder;
  COLORREF          crShadow;
  int               iTextShadowType;
  POINT             ptShadowOffset;
  int               iBorderSize;
  int               iFontPropId;
  int               iColorPropId;
  int               iStateId;
  BOOL              fApplyOverlay;
  int               iGlowSize;
  DTT_CALLBACK_PROC pfnDrawTextCallback;
  LPARAM            lParam;
} DTTOPTS, *PDTTOPTS;

namespace gg
{

class c_console : public console
{
/* private structures & classes */
private:
    enum alignment : int
    {
        TP_LEFT   = 0x0001,
        TP_TOP    = 0x0002,
        TP_RIGHT  = 0x0004,
        TP_BOTTOM = 0x0008,
        TP_CENTER = 0x0016
    };

    class c_output : public output
    {
        friend class c_console;

        mutable tthread::mutex m_mutex;
        c_console& m_console;
        std::stringstream m_stream;
        gg::color m_color;
        bool m_right;
        bool m_visible;

    public:
        c_output(c_output&&);
        c_output(c_console&, gg::color c = {0,0,0},
                 bool r = false, bool v = true);
        ~c_output();
        console& get_console() const;
        void show();
        void hide();
        void set_color(gg::color c);
        gg::color get_color() const ;
        void align_left();
        void align_right();
        bool is_empty() const;
        output& operator<< (const gg::var&);
        std::string get_string() const;
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

    class set_scoped_invoker
    {
        console* m_con;

    public:
        set_scoped_invoker(console* con);
        set_scoped_invoker(const set_scoped_invoker&) = delete;
        set_scoped_invoker(set_scoped_invoker&&) = delete;
        ~set_scoped_invoker();
    };

/* private variables */
private:
    tthread::recursive_mutex m_mutex;
    std::string m_name;
	bool m_open;
	std::list<c_output*> m_outp;
	std::string m_cmd;
	size_t m_cmdpos;
	controller* m_ctrl;
	c_thread* m_thread;
    HINSTANCE m_hInst;
    WNDCLASSEX m_wndClassEx;
	HWND m_hWnd;
	HTHEME m_hTheme;
	HFONT m_hFont;
	render_context m_rendctx;

	static std::map<tthread::thread::id, std::vector<console*>> m_invokers;
	static tthread::mutex m_invokers_mutex;

/* private functions */
private:
    void async_open();
    void async_close();
    bool prepare_render_context(render_context* ctx);
    void finish_render_context(render_context* ctx);
	void paint(const render_context* ctx);
	RECT calc_caret_rect(const render_context* ctx, std::string text,
                         int max_width, int pos = -1);
	int wrap_text(const render_context* ctx, std::string& text, const RECT* rect);
	int draw_text(const render_context* ctx, std::string text, const RECT* rect,
                  int align = alignment::TP_BOTTOM, COLORREF color = TEXTCOLOR_SYSTEM);
	LRESULT handle_wnd_message(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void cmd_async_exec();
	void cmd_complete();

/* public functions */
public:
    c_console(std::string name, controller* ctrl);
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

	static void push_invoker(console* con);
	static void pop_invoker();
	static console* get_invoker();
};

};

#endif // CCONSOLE_H_INCLUDED
