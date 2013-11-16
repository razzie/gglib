#ifndef WIN32_AERO_HPP_INCLUDED
#define WIN32_AERO_HPP_INCLUDED

#define _WIN32_WINNT	0x0501
#define _WIN32_IE       0x0501
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

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

extern "C"
{
    HRESULT WINAPI DwmExtendFrameIntoClientArea(HWND hWnd, const MARGINS *pMarInset);
    HRESULT WINAPI DwmIsCompositionEnabled(BOOL *pfEnabled);
    HRESULT WINAPI DwmEnableBlurBehindWindow(HWND hWnd, const DWM_BLURBEHIND *pBlurBehind);

    HRESULT WINAPI DrawThemeTextEx(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText,
                                    int iCharCount, DWORD dwFlags, LPRECT pRect, const DTTOPTS *pOptions);
}

namespace gg
{
    inline bool enable_glass(HWND hWnd)
    {
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
        return false;
    }
};

#endif // WIN32_AERO_HPP_INCLUDED
