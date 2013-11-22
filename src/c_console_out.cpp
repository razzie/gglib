#include "c_console.hpp"
#include "gg/util.hpp"
#include "gg/taskmgr.hpp"

using namespace gg;

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

c_console::c_output::c_output(c_console* con, gg::color c, int align, bool visible)
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

void c_console::c_output::flag_dirty()
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_dirty = true;
}

bool c_console::c_output::is_dirty() const
{
    //tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
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

void c_console::c_output::set_color(gg::color c)
{
    tthread::lock_guard<tthread::fast_mutex> guard(m_mutex);
    m_color = c;
}

gg::color c_console::c_output::get_color() const
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
