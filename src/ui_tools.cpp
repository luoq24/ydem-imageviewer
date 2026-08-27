#include "viewer.h"
#include <stdio.h>
#include <format>

void ViewerApp::UpdateWindowTitle() {
    if (m_ctx.loadingFilePath.empty()) {
        SetWindowTextW(m_ctx.hWnd, AppNameAndVersion());
        return;
    }

    // 剪贴板图像在内部以固定英文标记存储，显示时翻译
    std::wstring displayPath = m_ctx.loadingFilePath;
    if (displayPath == L"Clipboard Image") {
        displayPath = Tr(StrId::ClipboardImage);
    }

    std::wstring title;
    LPCWSTR appTitle = AppNameAndVersion();
    if (m_ctx.animationFrameDelays.size() > 1) {
        UINT frameNumber = m_ctx.currentAnimationFrame + 1;
        size_t frameCount = m_ctx.animationFrameDelays.size();
        title = std::vformat(Tr(StrId::TitleFrameFormat),
            std::make_wformat_args(displayPath, frameNumber, frameCount, appTitle));
    }
    else {
        title = std::vformat(Tr(StrId::TitleSimpleFormat),
            std::make_wformat_args(displayPath, appTitle));
    }
    SetWindowTextW(m_ctx.hWnd, title.c_str());
}

void ViewerApp::UpdateViewToCurrentFrame() {
    {
        std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
        if (!m_ctx.animationFrameDelays.empty()) {
            m_ctx.currentAnimatedConverter = GetCompositedAnimationFrame(m_ctx.currentAnimationFrame);
            m_ctx.wicConverterOriginal = m_ctx.currentAnimatedConverter;
            m_ctx.wicConverter = m_ctx.currentAnimatedConverter;
            m_ctx.d2dBitmap = nullptr;
        }
    }
    ApplyEffectsToView();
    UpdateWindowTitle();
    InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
}

void ViewerApp::ToggleFullScreen() {
    if (!m_ctx.isFullScreen) {
        m_ctx.savedStyle = GetWindowLong(m_ctx.hWnd, GWL_STYLE);

        m_ctx.windowPlacement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(m_ctx.hWnd, &m_ctx.windowPlacement);

        HMONITOR hMonitor = MonitorFromWindow(m_ctx.hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(hMonitor, &mi);

        SetWindowLong(m_ctx.hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

        // Do not make fullscreen topmost
        SetWindowPos(
            m_ctx.hWnd,
            HWND_TOP,
            mi.rcMonitor.left,
            mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW
        );

        m_ctx.isFullScreen = true;
    }
    else {
        SetWindowLong(m_ctx.hWnd, GWL_STYLE, m_ctx.savedStyle | WS_VISIBLE);
        SetWindowPlacement(m_ctx.hWnd, &m_ctx.windowPlacement);

        SetWindowPos(
            m_ctx.hWnd,
            m_ctx.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW
        );

        m_ctx.isFullScreen = false;
    }

    if (!m_ctx.preserveZoomOnResize) {
        FitImageToWindow();
    }
    else {
        InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
    }
}