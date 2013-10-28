#include <vector>
#include "gg/util.hpp"
#include "c_console.hpp"

using namespace gg;

extern "C"
{
    HRESULT WINAPI DwmExtendFrameIntoClientArea(HWND hWnd, const MARGINS *pMarInset);
    HRESULT WINAPI DwmIsCompositionEnabled(BOOL *pfEnabled);
    HRESULT WINAPI DwmEnableBlurBehindWindow(HWND hWnd, const DWM_BLURBEHIND *pBlurBehind);

    HRESULT WINAPI DrawThemeTextEx(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText,
                                    int iCharCount, DWORD dwFlags, LPRECT pRect, const DTTOPTS *pOptions);
}

/*HMODULE hDwmAPI;
HRESULT (WINAPI *DwmExtendFrameIntoClientArea)(HWND hWnd, const MARGINS *pMarInset);
HRESULT (WINAPI *DwmIsCompositionEnabled)(BOOL *pfEnabled);
HRESULT (WINAPI *DwmEnableBlurBehindWindow)(HWND hWnd, const DWM_BLURBEHIND *pBlurBehind);

HMODULE hUxtAPI;
HRESULT (WINAPI *DrawThemeTextEx)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText,
            int iCharCount, DWORD dwFlags, LPRECT pRect, const DTTOPTS *pOptions);

void init() __attribute__((constructor));
void init()
{
    hDwmAPI = LoadLibrary("dwmapi.dll");
    if (hDwmAPI)
    {
        *(FARPROC *)&DwmIsCompositionEnabled      = GetProcAddress(hDwmAPI, "DwmIsCompositionEnabled");
        *(FARPROC *)&DwmExtendFrameIntoClientArea = GetProcAddress(hDwmAPI, "DwmExtendFrameIntoClientArea");
        *(FARPROC *)&DwmEnableBlurBehindWindow    = GetProcAddress(hDwmAPI, "DwmEnableBlurBehindWindow");
    }

    hUxtAPI = LoadLibrary("uxtheme.dll");
    if (hUxtAPI)
    {
        *(FARPROC *)&DrawThemeTextEx = GetProcAddress(hUxtAPI, "DrawThemeTextEx");
    }
}*/

bool enable_glass(HWND hWnd)
{
    //if(hDwmAPI)
    //{
        BOOL bEnabled = FALSE;
        MARGINS Margins = { -1 };
        if(SUCCEEDED(DwmIsCompositionEnabled(&bEnabled)) && bEnabled)
        {
            if(SUCCEEDED(DwmExtendFrameIntoClientArea(hWnd, &Margins)))
            {
                DWM_BLURBEHIND blurBehind = {
                    DWM_BB_ENABLE | DWM_BB_TRANSITIONONMAXIMIZED,
                    TRUE, NULL, TRUE,
                };
                DwmEnableBlurBehindWindow(hWnd, &blurBehind);
                return true;
            }
        }
    //}
    return false;
}

LRESULT CALLBACK c_console::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    c_console* con = (c_console*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (con == NULL)
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    else
    {
        return con->handle_wnd_message(uMsg, wParam, lParam);
    }
}


c_console::c_output::c_output(c_output&& o)
 : m_console(o.m_console)/*, m_stream(std::move(o.m_stream))*/
 , m_color(o.m_color), m_right(o.m_right), m_visible(o.m_visible)
{
    m_stream << o.m_stream.str();
}

c_console::c_output::c_output(c_console& con, gg::color clr, bool r, bool v)
 : m_console(con), m_color(clr), m_right(r), m_visible(v)
{
}

c_console::c_output::~c_output()
{
    m_mutex.lock();
    m_console.remove_output(this);
    m_mutex.unlock();
}

console& c_console::c_output::get_console() const
{
    return m_console;
}

void c_console::c_output::show()
{
    m_visible = true;
}

void c_console::c_output::hide()
{
    m_visible = false;
}

void c_console::c_output::set_color(gg::color c)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_color = c;
}

gg::color c_console::c_output::get_color() const
{
    return m_color;
}

void c_console::c_output::align_left()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_right = false;
}

void c_console::c_output::align_right()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_right = true;
}

bool c_console::c_output::is_empty() const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    return (m_stream.rdbuf()->in_avail() == 0);
}

console::output& c_console::c_output::operator<< (const gg::var& v)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    m_stream << v.to_stream();
    //m_console.force_update();

    return *this;
}

std::string c_console::c_output::get_string() const
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    return m_stream.str();
}


c_console::c_console(std::string name, controller* ctrl)
 : m_name(name)
 , m_open(false)
 , m_cmdpos(0)
 , m_ctrl(ctrl)
 , m_thread("console '" + name + "' thread")
{
    if (m_ctrl != nullptr) m_ctrl->grab();

	INITCOMMONCONTROLSEX iccex = {
		sizeof(INITCOMMONCONTROLSEX), ICC_TAB_CLASSES,
	};

	m_hInst = GetModuleHandle(0);

	InitCommonControlsEx(&iccex);

	ZeroMemory(&m_wndClassEx, sizeof(WNDCLASSEX));
	m_wndClassEx.cbSize = sizeof(WNDCLASSEX);
	m_wndClassEx.lpfnWndProc   = WndProc;
	m_wndClassEx.cbWndExtra    = sizeof(LONG);
	m_wndClassEx.hInstance     = m_hInst;
	m_wndClassEx.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	m_wndClassEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	m_wndClassEx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	m_wndClassEx.lpszClassName = "CompositedWindow::Window";
	m_wndClassEx.hIconSm       = m_wndClassEx.hIcon;
}

c_console::~c_console()
{
    if (m_open) close();
    if (m_ctrl != nullptr) m_ctrl->drop();
    clear();
}

void c_console::set_controller(controller* ctrl)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    if (m_ctrl != nullptr) m_ctrl->drop();
    m_ctrl = ctrl;
}

console::controller* c_console::get_controller() const
{
    return m_ctrl;
}

void c_console::open()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    if (m_open) return; // already opened
    m_open = true;

	if(!RegisterClassEx(&m_wndClassEx)) return;

	m_hWnd = CreateWindowEx(WS_EX_APPWINDOW, m_wndClassEx.lpszClassName,
		TEXT(m_name.c_str()), WS_OVERLAPPEDWINDOW, 200, 200,
		400, 400, HWND_DESKTOP, NULL, m_hInst, NULL);

	if(!m_hWnd) return;

	enable_glass(m_hWnd);

	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

    LOGFONT lfont;
	m_hTheme = OpenThemeData(m_hWnd, L"CompositedWindow::Window");
    GetThemeFont(m_hTheme, (HDC)NULL, 0, 0, TMT_FONT, &lfont);
    m_hFont = CreateFontIndirect(&lfont);

	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);
	//SendMessage(m_hWnd, WM_PAINT, 0, 0);

	//m_thread.add_task(grab(this));
}

void c_console::close()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    if (!m_open) return;
    m_open = false;

    CloseThemeData(m_hTheme);
    DestroyWindow(m_hWnd);
}

bool c_console::is_opened() const
{
    return m_open;
}

bool c_console::run(uint32_t)
{
    MSG Msg;

    if (!m_open) return false;

    if (GetMessage(&Msg, NULL, 0, 0) != 0)
    {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);

		return true;
    }

    return false;
}

console::output* c_console::create_output()
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    c_output* out = new c_output(*this);
    m_outp.push_back(out);
    return out;
}

void c_console::remove_output(console::output* o)
{
    tthread::lock_guard<tthread::mutex> guard(m_mutex);

    for (auto it=m_outp.begin(); it!=m_outp.end(); it++)
    {
        if (*it == o)
        {
            m_outp.erase(it);
            return;
        }
    }
}

void c_console::clear()
{
    std::list<c_output*> tmp_outp;

    m_mutex.lock();
    std::swap(tmp_outp, m_outp);
    m_mutex.unlock();

    for (auto it=tmp_outp.begin(); it!=tmp_outp.end(); it=tmp_outp.erase(it))
    {
        (*it)->drop();
    }
}

LRESULT c_console::handle_wnd_message(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /*if (!m_mutex.try_lock())
    {
        SendMessage(m_hWnd, uMsg, wParam, lParam);
        return 0;
    }*/

    //m_mutex.unlock();
    //tthread::lock_guard<tthread::mutex> guard(m_mutex);

	switch(uMsg)
    {
		case WM_CREATE:
			break;

        case WM_SIZE:
            force_update();
            break;

        case WM_ERASEBKGND:
            return 0; // don't call DefWindowProc here

        case WM_PAINT:
            if (prepare_render_context(&m_rendctx))
            {
                paint(&m_rendctx);
                finish_render_context(&m_rendctx);
            }
            break;

		case WM_LBUTTONDOWN:
			return DefWindowProc(m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);

        case WM_LBUTTONDBLCLK:
            return DefWindowProc(m_hWnd, WM_NCLBUTTONDBLCLK, HTCAPTION, lParam);

        case WM_CHAR:
            switch (wParam)
            {
                case VK_RETURN:
                    // execute command
                    if ((lParam & (1<<30)) == 0) // first press
                        this->cmd_async_exec();
                    break;

                case VK_TAB:
                    // auto command completion
                    if ((lParam & (1<<30)) == 0) // first press
                        this->cmd_complete();
                    break;

                case VK_BACK:
                    if (m_cmdpos > 0)
                    {
                        m_cmd.erase(m_cmdpos-1, 1);
                        m_cmdpos--;
                    }
                    break;

                default:
                    if (isprint(wParam))
                    {
                        m_cmd.insert(m_cmdpos, 1, /*(wchar_t)*/wParam);
                        m_cmdpos++;
                    }
                    break;
            }
            force_update();
            break;

        case WM_KEYDOWN:
            switch (wParam)
            {
                case VK_LEFT:
                    if (m_cmdpos > 0) m_cmdpos--;
                    break;

                case VK_RIGHT:
                    if (m_cmdpos < m_cmd.length()) m_cmdpos++;
                    break;
            }
            force_update();
            break;

        case WM_QUIT:
		case WM_DESTROY:
			if (m_open) close();
			break;
	}

	return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}

bool c_console::prepare_render_context(render_context* ctx)
{
    const char c = ' ';

    memset(ctx, 0, sizeof(render_context));

    ctx->hwnd = m_hWnd;
    ctx->theme = m_hTheme;
    ctx->primary = GetDC(m_hWnd);
    ctx->secondary = CreateCompatibleDC(ctx->primary);
    if (!ctx->primary || !ctx->secondary) goto prepare_ctx_cleanup;

    GetClientRect(m_hWnd, &ctx->wndrect);

    ctx->bminfo.bmiHeader.biHeight = -(ctx->wndrect.bottom - ctx->wndrect.top);
    ctx->bminfo.bmiHeader.biWidth = ctx->wndrect.right - ctx->wndrect.left;
    ctx->bminfo.bmiHeader.biPlanes = 1;
    ctx->bminfo.bmiHeader.biSize = sizeof(BITMAPINFO);
    ctx->bminfo.bmiHeader.biBitCount = 32;
    ctx->bminfo.bmiHeader.biCompression = BI_RGB;

    ctx->dttopts.dwSize = sizeof(DTTOPTS);
    ctx->dttopts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE | DTT_TEXTCOLOR | DTT_BORDERSIZE | DTT_BORDERCOLOR;
    //ctx->dttopts.crText = RGB(255,0,0);
    ctx->dttopts.crText = GetThemeSysColor(m_hTheme, COLOR_WINDOWTEXT);
    //ctx->dttopts.iGlowSize = 12;
    GetThemeInt(m_hTheme, 0, 0, TMT_TEXTGLOWSIZE, &ctx->dttopts.iGlowSize);
    ctx->dttopts.iBorderSize = 2;
    ctx->dttopts.crBorder = RGB(255,255,255);

    ctx->font = m_hFont;
    ctx->bitmap = CreateDIBSection(ctx->secondary, &ctx->bminfo, DIB_RGB_COLORS, NULL, NULL, 0);
    if (!ctx->bitmap) goto prepare_ctx_cleanup;

    SelectObject(ctx->secondary, ctx->bitmap);
    SelectObject(ctx->secondary, ctx->font);

    SIZE csize;
    GetTextExtentPoint32(ctx->secondary, &c, 1, &csize);
    ctx->cwidth = csize.cx;
    ctx->cheight = csize.cy;

    return true;

prepare_ctx_cleanup:
    if (ctx->secondary) DeleteDC(ctx->secondary);
    if (ctx->primary) ReleaseDC(m_hWnd, ctx->primary);

    memset(ctx, 0, sizeof(render_context));

    return false;
}

void c_console::finish_render_context(render_context* ctx)
{
    BitBlt(ctx->primary, ctx->wndrect.left, ctx->wndrect.top, ctx->wndrect.right - ctx->wndrect.left, ctx->wndrect.bottom - ctx->wndrect.top,
           ctx->secondary, 0, 0, SRCCOPY);

    DeleteObject(ctx->bitmap);
    DeleteObject(ctx->font);

    DeleteDC(ctx->secondary);
    ReleaseDC(m_hWnd, ctx->primary);

    return;
}

RECT c_console::calc_caret_rect(const render_context* ctx, std::string text, int max_width, int pos)
{
    std::string::iterator it = text.begin(), end = text.end();
    int curr_pos = 0, curr_width = 0, height = 0;
    SIZE s;
    char c;

    for (; it != end && (pos == -1 || curr_pos < pos); it++, curr_pos++)
    {
        c = *it;
        GetTextExtentPoint32(ctx->secondary, &c, 1, &s);
        curr_width += s.cx;

        if (curr_width >= max_width)
        {
            height += s.cy;
            curr_width = 0;
        }
    }

    if (it == end)
    {
        s.cx = ctx->cwidth;
        s.cy = ctx->cheight;
    }
    else
    {
        c = *(it++);
        GetTextExtentPoint32(ctx->secondary, &c, 1, &s);
    }

    return {(LONG)curr_width, (LONG)height, (LONG)(curr_width+s.cx), (LONG)(height+s.cy)};
}

int c_console::wrap_text(const render_context* ctx, std::string& text, const RECT* rect)
{
    std::string tmpstr;
    std::string::iterator it = text.begin(), end = text.end();
    size_t height = 0, curr_width = 0, max_width = rect->right - rect->left;
    char c;
    SIZE s;

    //c = ' ';
    //GetTextExtentPoint32(ctx->secondary, &c, 1, &s);
    //height = s.cy;
    height = ctx->cheight;

    for (; it != end; it++)
    {
        c = *it;
        GetTextExtentPoint32(ctx->secondary, &c, 1, &s);
        curr_width += s.cx;

        if (curr_width >= max_width)
        {
            tmpstr.append("\n");
            height += s.cy;
            curr_width = 0;
        }

        tmpstr.append(&c, 1);
    }

    //text = tmpstr;
    std::swap(text, tmpstr);

    return height;
}

int c_console::draw_text(const render_context* ctx, std::string text, const RECT* rect,
        int align, COLORREF color)
{
    int height;
    RECT bounds;
    DTTOPTS tmpopts = ctx->dttopts;
    DWORD flags = DT_NOCLIP;

    if (color != TEXTCOLOR_SYSTEM)
        tmpopts.crText = color;

    if (rect == NULL) // we should use window size in this case
        CopyRect(&bounds, &ctx->wndrect);
    else
        CopyRect(&bounds, rect);

    height = wrap_text(ctx, text, &bounds);

    if (align & alignment::TP_LEFT) {
        flags |= DT_LEFT;
    }
    if (align & alignment::TP_TOP) {
        flags |= DT_TOP;
    }
    if (align & alignment::TP_BOTTOM) {
        flags |= DT_BOTTOM;
        bounds.top = bounds.bottom - height;
    }
    if (align & alignment::TP_RIGHT) {
        flags |= DT_RIGHT;
    }
    if (align & alignment::TP_CENTER) {
        flags |= DT_VCENTER | DT_CENTER;
        bounds.top += (bounds.bottom - bounds.top)/2 - height/2;
    }

    DrawThemeTextEx(ctx->theme, ctx->secondary, 0, 0, util::widen(text).c_str(), -1, flags, &bounds, &tmpopts);
    //FrameRect(ctx->secondary, &bounds, (HBRUSH)GetStockObject(WHITE_BRUSH)); // debug

    return height;
}

void c_console::paint(const render_context* ctx)
{
    #define DRAWTEXT(outp) \
        if (outp.m_visible) \
        { \
            tthread::lock_guard<tthread::mutex> guard(outp.m_mutex); \
            pos.bottom -= draw_text(ctx, outp.m_stream.str(), &pos, \
                                    alignment::TP_BOTTOM | (outp.m_right ? alignment::TP_RIGHT : alignment::TP_LEFT), \
                                    RGB(outp.m_color.R, outp.m_color.G, outp.m_color.B)) + 2; \
        } \

    RECT pos, caret;
    auto it = m_outp.rbegin(), end = m_outp.rend();
    unsigned cmd_height;

    cmd_height = draw_text(ctx, m_cmd, &ctx->wndrect, alignment::TP_BOTTOM, RGB(32,32,32));
    caret = calc_caret_rect(ctx, m_cmd, ctx->wndrect.right, m_cmdpos);

    CopyRect(&pos, &ctx->wndrect);
    pos.bottom -= ((cmd_height==0) ? ctx->cheight : cmd_height) + 2;

    caret.top += pos.bottom + 2;
    caret.bottom += pos.bottom + 2;
    FrameRect(ctx->secondary, &caret, (HBRUSH)GetStockObject(WHITE_BRUSH));

    for (; it != end && pos.bottom > 0; it++)
    {
        DRAWTEXT((**it));
    }

    return;

    #undef DRAWTEXT
}

void c_console::force_update()
{
    //InvalidateRect(m_hWnd, NULL, TRUE);
    //UpdateWindow(m_hWnd);
    SendMessage(m_hWnd, WM_PAINT, 0, 0);
}

c_console::cmd_async_exec_task::cmd_async_exec_task(
    std::string cmd,
    c_console::c_output* cmd_outp,
    c_console::c_output* exec_outp,
    console::controller* ctrl,
    c_thread* t)
 : m_cmd(cmd)
 , m_cmd_outp(cmd_outp)
 , m_exec_outp(exec_outp)
 , m_ctrl(ctrl)
 , m_thread(t)
{
}

c_console::cmd_async_exec_task::~cmd_async_exec_task()
{
}

bool c_console::cmd_async_exec_task::run(uint32_t)
{
    *m_cmd_outp << m_cmd;

    try
    {
        if (m_ctrl->exec(m_cmd, *m_exec_outp)) // successfull command
            m_cmd_outp->set_color({0,100,0});
        else
            m_cmd_outp->set_color({100,0,0});

        if (m_exec_outp->is_empty()) m_exec_outp->hide();
    }
    catch (std::exception& e)
    {
        *m_exec_outp << "\nexception: " << e.what();
    }
    catch (...)
    {
        *m_exec_outp << "\nexception: unknown";
    }

    m_cmd_outp->drop();
    m_exec_outp->drop();

    delete m_thread;
    return true;
}

void c_console::cmd_async_exec()
{
    if (m_ctrl != nullptr &&
        m_cmd.size() > 0 && m_cmd[0] != '#')
    {
        c_thread* t = new c_thread("async cmd exec");
        c_output* cmd_outp = static_cast<c_output*>(create_output());
        c_output* exec_outp = static_cast<c_output*>(create_output());
        cmd_outp->grab();
        exec_outp->grab();
        t->add_task( new cmd_async_exec_task(m_cmd, cmd_outp, exec_outp, m_ctrl, t) );
    }
    else
    {
        *create_output() << m_cmd;
    }

    m_cmd.erase();
    m_cmdpos = 0;
}

void c_console::cmd_complete()
{
    if (m_ctrl != nullptr)
    {
        c_output* o = static_cast<c_output*>(create_output());
        m_ctrl->complete(m_cmd, *o);
        if (o->is_empty()) o->hide();
    }
    m_cmdpos = m_cmd.length();
}
