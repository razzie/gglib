#include "c_console.hpp"
#include "c_timer.hpp"
#include "threadglobal.hpp"
#include "gg/application.hpp"
#include "gg/util.hpp"
#include "gg/taskmgr.hpp"

using namespace gg;

struct invoker_data
{
    console* con;
    console::output* outp;
};

static recursive_thread_global<invoker_data> s_invokers;


console* console::get_invoker_console()
{
    optional<invoker_data> i = s_invokers.get();
    if (i) return i->con;
    else return nullptr;
}

console::output* console::get_invoker_output()
{
    optional<invoker_data> i = s_invokers.get();
    if (i) return i->outp;
    else return nullptr;
}


LRESULT CALLBACK c_console::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    c_console* con = reinterpret_cast<c_console*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (con == NULL)
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    else
    {
        return con->handle_wnd_message(uMsg, wParam, lParam);
    }
}


c_console::c_console(application* app, std::string name,
                     controller* ctrl, std::string welcome_text)
 : m_app(app)
 , m_name(name)
 , m_open(false)
 , m_input(true)
 , m_arg_fill(true)
 , m_cmd_pos(m_cmd.end())
 , m_cmd_history_pos(m_cmd_history.end())
 , m_ctrl(ctrl)
 , m_thread("console thread")
 , m_welcome(true)
 , m_welcome_text(nullptr)
{
    if (m_app != nullptr) m_app->grab();
    if (m_ctrl != nullptr) m_ctrl->grab();

	INITCOMMONCONTROLSEX iccex = {
		sizeof(INITCOMMONCONTROLSEX), ICC_TAB_CLASSES,
	};

	m_hInst = GetModuleHandle(0);

	InitCommonControlsEx(&iccex);

	ZeroMemory(&m_wndClassEx, sizeof(WNDCLASSEX));
	m_wndClassEx.cbSize        = sizeof(WNDCLASSEX);
	m_wndClassEx.style         = CS_DBLCLKS;
	m_wndClassEx.lpfnWndProc   = WndProc;
	m_wndClassEx.cbWndExtra    = sizeof(LONG);
	m_wndClassEx.hInstance     = m_hInst;
	m_wndClassEx.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	m_wndClassEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	m_wndClassEx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	m_wndClassEx.lpszClassName = "CompositedWindow::Window";
	m_wndClassEx.hIconSm       = m_wndClassEx.hIcon;

	//if(!RegisterClassEx(&m_wndClassEx)) return;
    RegisterClassEx(&m_wndClassEx);

    m_welcome_text << welcome_text;
    m_welcome_text.align_center();
    m_welcome_text.valign_center();
}

c_console::~c_console()
{
    if (m_open) close();
    clear();
    if (m_ctrl != nullptr) m_ctrl->drop();
    if (m_app != nullptr) m_app->drop();
}

application* c_console::get_app() const
{
    return m_app;
}

std::string c_console::get_name() const
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    return m_name;
}

void c_console::set_name(std::string name)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_name = name;
    if (m_open) SetWindowText(m_hWnd, TEXT(m_name.c_str()));
}

console::controller* c_console::get_controller() const
{
    return m_ctrl;
}

void c_console::set_controller(controller* ctrl)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    if (m_ctrl != nullptr) m_ctrl->drop();
    m_ctrl = ctrl;
}

void c_console::enable_input()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_input = true;
}

void c_console::disable_input()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_input = false;
}

void c_console::enable_argument_fill_helper()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_arg_fill = true;
}

void c_console::disable_argument_fill_helper()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_arg_fill = false;
}

void c_console::open()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    if (m_open) return; // already opened

	m_thread.add_task(std::bind(&c_console::async_open, this));
	m_thread.add_persistent_task([&](uint32_t)
    {
        if (!run())
        {
            async_close();
            return true;
        }

        return false;
    });

	this->grab();
}

void c_console::close()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    if (!m_open) return; // already closed
    m_open = false;
}

void c_console::async_open()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    if (m_open) return; // already opened
    m_open = true;

	m_hWnd = CreateWindowEx(WS_EX_APPWINDOW, m_wndClassEx.lpszClassName,
		TEXT(m_name.c_str()), WS_OVERLAPPEDWINDOW, 200, 200,
		400, 400, HWND_DESKTOP, NULL, m_hInst, NULL);

	if(!m_hWnd) return;

	enable_glass(m_hWnd);

	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

	m_hTheme = OpenThemeData(m_hWnd, L"CompositedWindow::Window");

    LOGFONT lfont;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lfont);
    lfont.lfHeight = 16;
    lfont.lfWidth = 0;
    strcpy(lfont.lfFaceName, "Consolas");
    m_hFont = CreateFontIndirect(&lfont);

	ShowWindow(m_hWnd, SW_SHOW);
	update();
}

void c_console::async_close()
{
    //tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_mutex.lock();
    DeleteObject(m_hFont);
    CloseThemeData(m_hTheme);
    DestroyWindow(m_hWnd);
    m_mutex.unlock();

    if (m_close_cb)
    {
        recursive_thread_global<invoker_data>::scope invoker(&s_invokers, {this, nullptr});
        m_close_cb();
    }

    this->drop();
}

bool c_console::is_opened() const
{
    return m_open;
}

void c_console::on_close(std::function<void()> callback)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_close_cb = callback;
}

bool c_console::run()
{
    if (!m_open) return false;

    MSG Msg;

    if (GetMessage(&Msg, NULL, 0, 0) != 0)
    {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);

		return true;
    }

    return false;
}

void c_console::update()
{
    InvalidateRect(m_hWnd, NULL, TRUE);
    PostMessage(m_hWnd, WM_PAINT, 0, 0);
}

console::output* c_console::create_output()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    c_output* out = new c_output(this);
    out->grab();
    m_outp.push_back(out);

    m_welcome = false;

    return out;
}

void c_console::remove_output(console::output* o)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    auto it = m_outp.begin(), end = m_outp.end();
    for (; it != end; ++it)
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

    for (c_output* o : tmp_outp) o->drop();
    tmp_outp.clear();
}

LRESULT c_console::handle_wnd_message(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!m_mutex.try_lock())
    {
        ReplyMessage( DefWindowProc(m_hWnd, uMsg, wParam, lParam) );
        PostMessage(m_hWnd, uMsg, wParam, lParam);
        SwitchToThread();
        return 0;
    }

    util::scope_callback([&]{ m_mutex.unlock(); });

	switch(uMsg)
    {
        case WM_PAINT:
            if (prepare_render_context(&m_rendctx))
            {
                paint(&m_rendctx);
                finish_render_context(&m_rendctx);
            }
            break;

        case WM_CHAR:
            // break if input is disable
            if (!m_input) break;

            // handling CTRL + C
            if (/*GetKeyState(VK_CONTROL) && wParam == 'c'*/ wParam == 3) // EndOfText
            {
                m_cmd.clear();
                m_cmd_pos = m_cmd.end();
                update();
                break;
            }

            // handling currently typed command
            switch (wParam)
            {
                case VK_RETURN:
                    // execute command
                    if ((lParam & (1<<30)) == 0) // first press
                    {
                        cmd_async_exec();
                        update();
                    }
                    break;

                case VK_TAB:
                    // auto command completion
                    if ((lParam & (1<<30)) == 0) // first press
                    {
                        cmd_complete();
                        update();
                    }
                    break;

                default:
                    if (isprint(wParam))
                    {
                        // special case if arg_fill_helper is on
                        if (m_arg_fill && m_cmd_pos != m_cmd.end() && *m_cmd_pos == '0' && std::isdigit(wParam) &&
                            (m_cmd_pos == m_cmd.begin() || *(m_cmd_pos-1) == '(' || *(m_cmd_pos-1) == ',' || *(m_cmd_pos-1) == ' '))
                        {
                            *m_cmd_pos = wParam;
                            ++m_cmd_pos;
                        }
                        else
                        {
                            m_cmd_pos = m_cmd.insert(m_cmd_pos, wParam) + 1;
                        }
                        update();
                    }
                    break;
            }
            break;

        case WM_KEYDOWN:
            m_welcome = false;

            // break if input is disable
            if (!m_input) break;

            switch (wParam)
            {
                case VK_BACK:
                    if (m_cmd_pos != m_cmd.begin())
                    {
                        m_cmd_pos = m_cmd.erase(m_cmd_pos - 1);
                        update();
                    }
                    break;

                case VK_DELETE:
                    if (m_cmd_pos != m_cmd.end())
                    {
                        m_cmd_pos = m_cmd.erase(m_cmd_pos);
                        update();
                    }
                    break;

                case VK_HOME:
                    m_cmd_pos = m_cmd.begin();
                    break;

                case VK_END:
                    m_cmd_pos = m_cmd.end();
                    break;

                case VK_LEFT:
                    if (m_cmd_pos != m_cmd.begin())
                    {
                        --m_cmd_pos;
                        update();
                    }
                    break;

                case VK_RIGHT:
                    if (m_cmd_pos != m_cmd.end())
                    {
                        ++m_cmd_pos;
                        update();
                    }
                    break;

                case VK_UP:
                    if (m_cmd_history_pos != m_cmd_history.begin())
                    {
                        m_cmd = *(--m_cmd_history_pos);
                        m_cmd_pos = m_cmd.begin();
                        arg_fill_helper();
                        update();
                    }
                    break;

                case VK_DOWN:
                    if (m_cmd_history_pos == m_cmd_history.end() - 1)
                    {
                        m_cmd.erase();
                        m_cmd_pos = m_cmd.end();
                        update();
                    }
                    else if (m_cmd_history_pos != m_cmd_history.end())
                    {
                        m_cmd = *(++m_cmd_history_pos);
                        m_cmd_pos = m_cmd.begin();
                        arg_fill_helper();
                        update();
                    }
                    break;
            }
            break;

        case WM_CREATE:
            break;

        case WM_ERASEBKGND:
            //return 0; // don't call DefWindowProc here
            break;

        case WM_SIZE:
            for (auto o : m_outp) o->flag_dirty();
            update();
            break;

        case WM_LBUTTONDOWN:
            return DefWindowProc(m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);

        case WM_LBUTTONDBLCLK:
            return DefWindowProc(m_hWnd, WM_NCLBUTTONDBLCLK, HTCAPTION, lParam);

        case WM_QUIT:
		case WM_DESTROY:
			if (m_open) m_open = false; //close();
			break;

        default:
            return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}

	return 0;
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

    /*LOGFONT lfont;
    GetObject(m_hFont, sizeof(LOGFONT), &lfont);
    ctx->font = CreateFontIndirect(&lfont);*/
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
    //DeleteObject(ctx->font);

    DeleteDC(ctx->secondary);
    ReleaseDC(m_hWnd, ctx->primary);

    return;
}

void c_console::paint(const render_context* ctx)
{
    RECT bounds = ctx->wndrect;

    if (m_welcome)
    {
        m_welcome_text.draw(ctx, &bounds);
        return;
    }

    if (m_input)
    {
        c_output::draw(m_cmd, ctx, &bounds, std::distance(m_cmd.begin(), m_cmd_pos));
    }

    for (auto it = m_outp.rbegin(); it != m_outp.rend() && bounds.bottom > 0; ++it)
    {
        (*it)->draw(ctx, &bounds);
    }
}

void c_console::cmd_async_exec()
{
    auto async_exec = [](std::string m_cmd, c_output* m_cmd_outp, c_output* m_exec_outp,
                         c_console* m_con, controller* m_ctrl)
    {
        recursive_thread_global<invoker_data>::scope invoker(&s_invokers, {m_con, m_exec_outp});

        m_con->grab();
        *m_cmd_outp << m_cmd;

        try
        {
            controller::exec_result r = m_ctrl->exec(m_cmd, *m_exec_outp);
            switch (r)
            {
            case controller::exec_result::EXEC_SUCCESS:
                m_cmd_outp->set_color({0,100,0});
                break;
            case controller::exec_result::EXEC_FAIL:
                m_cmd_outp->set_color({100,0,0});
                break;
            default:
                break;
            }
        }
        catch (std::exception& e) { *m_exec_outp << "exception: " << e.what(); }
        catch (...) { *m_exec_outp << "exception: unknown"; }

        if (m_exec_outp->is_empty()) { m_exec_outp->hide(); m_con->update(); }
        //if (m_exec_outp->is_empty()) { m_exec_outp->drop(); m_con->update(); }

        m_cmd_outp->drop();
        m_exec_outp->drop();
        m_con->drop();
    };

    if (m_ctrl != nullptr && !m_cmd.empty())
    {
        c_output* cmd_outp = static_cast<c_output*>(create_output());
        c_output* exec_outp = static_cast<c_output*>(create_output());
        std::function<void()> async_exec_bound = std::bind(async_exec, m_cmd, cmd_outp, exec_outp, this, m_ctrl);

        async_invoke(async_exec_bound);
    }
    else
    {
        output* o = create_output();
        *o << m_cmd;
        o->drop();
    }

    if (!m_cmd.empty())
    {
        //m_cmd_history_pos = m_cmd_history.insert(m_cmd_history_pos, std::move(m_cmd)) + 1;
        m_cmd_history.push_back(std::move(m_cmd));
        m_cmd_history_pos = m_cmd_history.end();
    }

    m_cmd.erase();
    m_cmd_pos = m_cmd.end();
}

void c_console::cmd_complete()
{
    if (m_ctrl != nullptr)
    {
        output* o = create_output();

        size_t dist = static_cast<size_t>(std::distance(m_cmd.begin(), m_cmd_pos));

        try
        {
            recursive_thread_global<invoker_data>::scope invoker(&s_invokers, {this, o});
            m_ctrl->complete(m_cmd, *o);
        }
        catch (std::exception& e) { *o << "\nexception: " << e.what(); }
        catch (...) { *o << "\nexception: unknown"; }

        m_cmd_pos = std::next(m_cmd.begin(), (dist < m_cmd.size()) ? dist : m_cmd.size());

        o->drop();

        if (o->is_empty()) o->hide();
        //if (o->is_empty()) o->drop();
    }

    arg_fill_helper();
}

void c_console::arg_fill_helper()
{
    if (m_ctrl != nullptr && m_arg_fill)
    {
        for (; m_cmd_pos != m_cmd.end(); ++m_cmd_pos)
        {
            if (*m_cmd_pos == '(' || *m_cmd_pos == ',')
            {
                ++m_cmd_pos;
                if (m_cmd_pos != m_cmd.end())
                {
                    if (*m_cmd_pos == ' ') for (; m_cmd_pos != m_cmd.end() && *m_cmd_pos == ' '; ++m_cmd_pos);
                    if (*m_cmd_pos == '"') ++m_cmd_pos;
                }
                break;
            }
        }
    }
    else
    {
        m_cmd_pos = m_cmd.end();
    }
}


static RECT calc_caret_rect(HDC hdc, std::wstring text, int pos = -1, int* line_width = nullptr)
{
    auto it = text.begin(), end = text.end();
    int curr_pos = 0, curr_width = 0, height = 0;
    wchar_t c;
    SIZE s;
    RECT caret { 0 };
    bool caret_initialized = false;

    for (; it != end; ++it, ++curr_pos)
    {
        c = *it;
        GetTextExtentPoint32W(hdc, &c, 1, &s);

        if (c == L'\n')
        {
            height += s.cy;
            if (pos >= 0 && curr_pos >= pos) break;
            curr_width = 0;
        }

        if (curr_pos == pos)
        {
            if (it+1 == end) c = L' ';
            else c = *(it+2);

            GetTextExtentPoint32W(hdc, &c, 1, &s);

            caret.top = height;
            caret.bottom = height + s.cy;
            caret.left = curr_width;
            caret.right = curr_width + s.cx;

            caret_initialized = true;
        }

        curr_width += s.cx;
    }

    if (!caret_initialized)
    {
        if (it == end || it+1 == end) c = L' ';
        else c = *(it+2);

        GetTextExtentPoint32W(hdc, &c, 1, &s);

        caret.top = height;
        caret.bottom = height + s.cy;
        caret.left = curr_width;
        caret.right = curr_width + s.cx;
    }

    if (line_width != nullptr) *line_width = curr_width;

    return caret;
}

static int wrap_text(HDC hdc, std::wstring& text, const RECT* rect)
{
    size_t height = 0;
    size_t curr_width = 0;
    size_t max_width = rect->right - rect->left;
    wchar_t c;
    SIZE s;

    c = L' ';
    GetTextExtentPoint32W(hdc, &c, 1, &s);
    height = s.cy;

    for (auto it = text.begin(); it != text.end(); ++it)
    {
        c = *it;
        GetTextExtentPoint32W(hdc, &c, 1, &s);
        curr_width += s.cx;

        if (curr_width >= max_width)
        {
            it = text.insert(it, '\n');
            height += s.cy;
            curr_width = 0;
        }

        if (c == '\n')
        {
            height += s.cy;
            curr_width = 0;
        }
    }

    return height;
}


c_console::c_output::c_output(c_output&& o)
 : std::ostream(this)
 , m_console(o.m_console)
 , m_text(std::move(o.m_text))
 , m_color(o.m_color)
 , m_align(o.m_align)
 , m_visible(o.m_visible)
 , m_dirty(true)
{
}

c_console::c_output::c_output(c_console* con)
 : std::ostream(this)
 , m_console(con)
 , m_color({0,0,0})
 , m_align(alignment::H_LEFT | alignment::V_BOTTOM)
 , m_visible(true)
 , m_dirty(true)
{
}

c_console::c_output::c_output(c_console* con, color c, int align, bool visible)
 : std::ostream(this)
 , m_console(con)
 , m_color(c)
 , m_align(align)
 , m_visible(visible)
 , m_dirty(true)
{
}

c_console::c_output::~c_output()
{
    m_mutex.lock();
    if (m_console != nullptr) m_console->remove_output(this);
    m_mutex.unlock();
}

console& c_console::c_output::get_console() const
{
    if (m_console == nullptr)
        throw std::runtime_error("no parent console");

    return *m_console;
}

void c_console::c_output::show()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_visible = true;
}

void c_console::c_output::hide()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_visible = false;
}

void c_console::c_output::set_color(color c)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_color = c;
}

console::output::color c_console::c_output::get_color() const
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    return m_color;
}

void c_console::c_output::align_left()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    m_align |= alignment::H_LEFT;
    m_align &= ~alignment::H_CENTER;
    m_align &= ~alignment::H_RIGHT;
}

void c_console::c_output::align_center()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    m_align &= ~alignment::H_LEFT;
    m_align |= alignment::H_CENTER;
    m_align &= ~alignment::H_RIGHT;
}

void c_console::c_output::align_right()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    m_align &= ~alignment::H_LEFT;
    m_align &= ~alignment::H_CENTER;
    m_align |= alignment::H_RIGHT;
}

void c_console::c_output::valign_top()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    m_align |= alignment::V_TOP;
    m_align &= ~alignment::V_CENTER;
    m_align &= ~alignment::V_BOTTOM;
}

void c_console::c_output::valign_center()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    m_align &= ~alignment::V_TOP;
    m_align |= alignment::V_CENTER;
    m_align &= ~alignment::V_BOTTOM;
}

void c_console::c_output::valign_bottom()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    m_align &= ~alignment::V_TOP;
    m_align &= ~alignment::V_CENTER;
    m_align |= alignment::V_BOTTOM;
}

void c_console::c_output::print(std::string str)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_text += str;
}

std::string c_console::c_output::to_string() const
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    return m_text;
}

bool c_console::c_output::is_empty() const
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    return (m_text.empty());
}

void c_console::c_output::erase()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_text.erase();
}

void c_console::c_output::flag_dirty()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_dirty = true;
}

bool c_console::c_output::is_dirty() const
{
    return m_dirty;
}

void c_console::c_output::draw(const render_context* ctx, RECT* bounds, int caret_pos) const
{
    if (!m_visible) return;

    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    if (m_dirty)
    {
        m_wrapped_text = util::convert_string<char, wchar_t>(m_text);
        m_last_height = wrap_text(ctx->secondary, m_wrapped_text, bounds);

        m_dirty = false;
    }

    RECT rect;
    DTTOPTS tmpopts = ctx->dttopts;
    DWORD flags = DT_NOCLIP;

    tmpopts.crText = RGB(m_color.R, m_color.G, m_color.B);

    if (bounds == NULL) // we should use window size in this case
        CopyRect(&rect, &ctx->wndrect);
    else
        CopyRect(&rect, bounds);

    m_last_bounds = rect;

    if (m_align & alignment::H_LEFT) {
        flags |= DT_LEFT;
    }
    else if (m_align & alignment::H_CENTER) {
        flags |= DT_CENTER;
    }
    else if (m_align & alignment::H_RIGHT) {
        flags |= DT_RIGHT;
    }

    if (m_align & alignment::V_TOP) {
        flags |= DT_TOP;
        bounds->top += m_last_height + 2;
    }
    else if (m_align & alignment::V_CENTER) {
        flags |= DT_VCENTER;
        rect.top += (rect.bottom - rect.top)/2 - m_last_height/2;
    }
    else if (m_align & alignment::V_BOTTOM) {
        flags |= DT_BOTTOM;
        rect.top = rect.bottom - m_last_height;
        bounds->bottom -= m_last_height + 2;
    }

    DrawThemeTextEx(ctx->theme, ctx->secondary, 0, 0, m_wrapped_text.c_str(), -1, flags, &rect, &tmpopts);
    DrawThemeTextEx(ctx->theme, ctx->secondary, 0, 0, m_wrapped_text.c_str(), -1, flags, &rect, &tmpopts);
    //FrameRect(ctx->secondary, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH)); // debug

    if (caret_pos >= 0)
    {
        int line_width = 0, h_offset = 0, v_offset = 0;
        RECT caret = calc_caret_rect(ctx->secondary, m_wrapped_text, caret_pos, &line_width);

        if (m_align & alignment::H_LEFT)        h_offset = rect.left;
        else if (m_align & alignment::H_CENTER) h_offset = ((rect.right - rect.left) / 2) - (caret.left / 2);
        else if (m_align & alignment::H_RIGHT)  h_offset = (rect.right - rect.left) - line_width - (caret.right - caret.left);

        v_offset += rect.top;

        // convert relative caret coords to absolute
        caret.top += v_offset;
        caret.bottom += v_offset;
        caret.left += h_offset;
        caret.right += h_offset;

        FrameRect(ctx->secondary, &caret, (HBRUSH)GetStockObject(WHITE_BRUSH));
    }
}

void c_console::c_output::draw(std::string text, const render_context* ctx, RECT* bounds, int caret_pos)
{
    c_output tmp_out(nullptr);
    tmp_out.m_text = text;
    tmp_out.draw(ctx, bounds, caret_pos);
}

int c_console::c_output::overflow(int c)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);

    m_text.push_back(c);
    m_dirty = true;

    if (c == '\n' && m_console != nullptr)
        m_console->update();

    return c;
}

int c_console::c_output::sync()
{
    if (m_console == nullptr) return -1;

    m_console->update();
    return 0;
}
