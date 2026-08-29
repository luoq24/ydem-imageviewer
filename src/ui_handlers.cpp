#include "viewer.h"
#include "exif_utils.h"
#include <string>
#include <stdio.h>
#include <commctrl.h>
#include <format>
#include <d2d1helper.h>

#pragma comment(lib, "comctl32.lib")





void ViewerApp::OnPaint(HWND hWnd) {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hWnd, &ps);

    if (m_ctx.isInitialized) {
        Render();
    }

    EndPaint(hWnd, &ps);
}

void ViewerApp::HandleCommand(WORD cmd) {
    switch (cmd) {
    case IDM_OPEN:          OpenFileAction(); break;
    case IDM_REFRESH:
        if (!m_ctx.imageFiles.empty() && m_ctx.currentImageIndex != -1) {
            std::wstring currentFile = m_ctx.imageFiles[m_ctx.currentImageIndex];
            m_ctx.imageFiles.clear(); // force rescan 
            LoadImageFromFile(currentFile);
        }
        break;
    case IDM_COPY_PATH:     HandleCopyPath(); break;
    case IDM_SEND_ZYU_EDIT: SendToZiyuEdit(); break;
    case IDM_NEXT_IMG:
        if (!m_ctx.imageFiles.empty() && m_ctx.currentImageIndex != -1) {
            size_t size = m_ctx.imageFiles.size();
            m_ctx.currentImageIndex = (m_ctx.currentImageIndex + 1) % static_cast<int>(size);
            LPCWSTR fileName = PathFindFileNameW(m_ctx.imageFiles[m_ctx.currentImageIndex].c_str());
            LPCWSTR appTitle = AppNameAndVersion();
            std::wstring title = std::vformat(Tr(StrId::TitleLoadingFormat), std::make_wformat_args(fileName, appTitle));
            SetWindowTextW(m_ctx.hWnd, title.c_str());
            LoadImageFromFile(m_ctx.imageFiles[m_ctx.currentImageIndex], false);
        }
        break;
    case IDM_PREV_IMG:
        if (!m_ctx.imageFiles.empty() && m_ctx.currentImageIndex != -1) {
            size_t size = m_ctx.imageFiles.size();
            m_ctx.currentImageIndex = (m_ctx.currentImageIndex - 1 + static_cast<int>(size)) % static_cast<int>(size);
            LPCWSTR fileName = PathFindFileNameW(m_ctx.imageFiles[m_ctx.currentImageIndex].c_str());
            LPCWSTR appTitle = AppNameAndVersion();
            std::wstring title = std::vformat(Tr(StrId::TitleLoadingFormat), std::make_wformat_args(fileName, appTitle));
            SetWindowTextW(m_ctx.hWnd, title.c_str());
            LoadImageFromFile(m_ctx.imageFiles[m_ctx.currentImageIndex], true);
        }
        break;
    case IDM_FIRST_IMAGE:
        if (!m_ctx.imageFiles.empty() && m_ctx.currentImageIndex != -1) {
            m_ctx.currentImageIndex = 0;
            LPCWSTR fileName = PathFindFileNameW(m_ctx.imageFiles[m_ctx.currentImageIndex].c_str());
            LPCWSTR appTitle = AppNameAndVersion();
            std::wstring title = std::vformat(Tr(StrId::TitleLoadingFormat), std::make_wformat_args(fileName, appTitle));
            SetWindowTextW(m_ctx.hWnd, title.c_str());
            LoadImageFromFile(m_ctx.imageFiles[m_ctx.currentImageIndex], false);
        }
        break;
    case IDM_LAST_IMAGE:
        if (!m_ctx.imageFiles.empty() && m_ctx.currentImageIndex != -1) {
            m_ctx.currentImageIndex = static_cast<int>(m_ctx.imageFiles.size()) - 1;
            LPCWSTR fileName = PathFindFileNameW(m_ctx.imageFiles[m_ctx.currentImageIndex].c_str());
            LPCWSTR appTitle = AppNameAndVersion();
            std::wstring title = std::vformat(Tr(StrId::TitleLoadingFormat), std::make_wformat_args(fileName, appTitle));
            SetWindowTextW(m_ctx.hWnd, title.c_str());
            LoadImageFromFile(m_ctx.imageFiles[m_ctx.currentImageIndex], true);
        }
        break;
    case IDM_ZOOM_IN: {
        RECT cr; GetClientRect(m_ctx.hWnd, &cr);
        POINT centerPt = { (cr.right - cr.left) / 2, (cr.bottom - cr.top) / 2 };
        ZoomImage(1.25f, centerPt);
        break;
    }
    case IDM_ZOOM_OUT: {
        RECT cr; GetClientRect(m_ctx.hWnd, &cr);
        POINT centerPt = { (cr.right - cr.left) / 2, (cr.bottom - cr.top) / 2 };
        ZoomImage(0.8f, centerPt);
        break;
    }
    case IDM_ACTUAL_SIZE:   SetActualSize(); break;
    case IDM_ZOOM_200:      SetZoomLevel(2.0f); break;
    case IDM_ZOOM_300:      SetZoomLevel(3.0f); break;
    case IDM_FIT_TO_WINDOW: FitImageToWindow(); break;
    case IDM_FULLSCREEN:    ToggleFullScreen(); break;
    case IDM_EXIT:
        if (m_ctx.isCropMode || m_ctx.isSelectingCropRect || m_ctx.isCropPending) {
            bool wasCropActive = m_ctx.isCropActive;
            m_ctx.isCropMode = false;
            m_ctx.isSelectingCropRect = false; m_ctx.isCropPending = false;
            if (wasCropActive) {
                m_ctx.isCropActive = false;
                ApplyEffectsToView(); FitImageToWindow();
            }
            else {
                InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
            }
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            if (GetCapture() == m_ctx.hWnd) ReleaseCapture();
            InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
        }
        else {
            SendMessage(m_ctx.hWnd, WM_CLOSE, 0, 0);
        }
        break;
    case IDM_QUIT: // “关闭应用”：真正退出进程
        DestroyWindow(m_ctx.hWnd);
        break;
    case IDM_OPEN_LOCATION: OpenFileLocationAction(); break;
    case IDM_PROPERTIES:    ShowImageProperties(); break;
    case IDM_SLIDESHOW:
        m_ctx.isSlideshowActive = !m_ctx.isSlideshowActive;
        if (m_ctx.isSlideshowActive) {
            SetTimer(m_ctx.hWnd, SLIDESHOW_TIMER_ID, std::max(1, m_ctx.slideshowIntervalSeconds) * 1000, nullptr);
        }
        else {
            KillTimer(m_ctx.hWnd, SLIDESHOW_TIMER_ID);
        }
        break;
    case IDM_PREFERENCES:   OpenPreferencesDialog(); break;
    case IDM_KEYBINDINGS:   OpenKeybindingsDialog(); break;
    case IDM_CUSTOM_ZOOM:   OpenZoomDialog(); break;
    case IDM_CONTEXT_MENU: {
        RECT rc;
        GetClientRect(m_ctx.hWnd, &rc);
        POINT pt = { (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };
        ClientToScreen(m_ctx.hWnd, &pt);
        OnContextMenu(m_ctx.hWnd, pt);
        break;
    }

    // Hardcoded fallbacks 
    case IDM_UNDO:
        if (!m_ctx.undoStack.empty()) {
            std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
            m_ctx.wicConverterOriginal = m_ctx.undoStack.back();
            m_ctx.undoStack.pop_back(); m_ctx.isCropActive = false; m_ctx.cropRectLocal = { 0 };
            m_ctx.isOsdCacheValid = false;
            ApplyEffectsToView(); FitImageToWindow(); InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
        }
        break;
    case IDM_CENTER_IMAGE:  CenterImage(true); break;
    case IDM_COMMIT_CROP:
        if (m_ctx.isCropPending) {
            m_ctx.isCropActive = true;
            m_ctx.isCropPending = false; m_ctx.isCropMode = false;
            CommitCrop(); ApplyEffectsToView(); FitImageToWindow(); InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
        }
        break;
    case IDM_TOGGLE_OSD:
        m_ctx.isOsdVisible = !m_ctx.isOsdVisible;
        InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
        break;
    case IDM_PLAY_PAUSE:
        if (m_ctx.animationFrameDelays.size() > 1) {
            if (!m_ctx.isAnimationPaused) {
                m_ctx.isAnimationPaused = true;
                KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
            }
            else {
                m_ctx.currentAnimationFrame = (m_ctx.currentAnimationFrame + 1) % m_ctx.animationFrameDelays.size();
                UpdateViewToCurrentFrame();
            }
        }
        break;
    case IDM_RESUME_ANIM:
        if (m_ctx.animationFrameDelays.size() > 1 && m_ctx.isAnimationPaused) {
            m_ctx.isAnimationPaused = false;
            UINT delay = m_ctx.animationFrameDelays[m_ctx.currentAnimationFrame];
            SetTimer(m_ctx.hWnd, ANIMATION_TIMER_ID, delay > 0 ? delay : 100, nullptr);
        }
        break;
    case IDM_ANIM_NEXT_FRAME:
        if (m_ctx.isAnimated && !m_ctx.animationFrameDelays.empty()) {
            const UINT frameCount = static_cast<UINT>(m_ctx.animationFrameDelays.size());

            m_ctx.isAnimationPaused = true;
            KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
            m_ctx.currentAnimationFrame = (m_ctx.currentAnimationFrame + 1) % frameCount;
            UpdateViewToCurrentFrame();
        }
        break;
    case IDM_ANIM_PREV_FRAME:
        if (m_ctx.isAnimated && !m_ctx.animationFrameDelays.empty()) {
            const UINT frameCount = static_cast<UINT>(m_ctx.animationFrameDelays.size());

            m_ctx.isAnimationPaused = true;
            KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
            m_ctx.currentAnimationFrame = (m_ctx.currentAnimationFrame + frameCount - 1) % frameCount;
            UpdateViewToCurrentFrame();
        }
        break;
    case IDM_ANIM_FIRST_FRAME:
        if (m_ctx.isAnimated && !m_ctx.animationFrameDelays.empty()) {
            m_ctx.isAnimationPaused = true;
            KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
            m_ctx.currentAnimationFrame = 0;
            UpdateViewToCurrentFrame();
        }
        break;
    case IDM_SORT_BY_NAME_ASC:
    case IDM_SORT_BY_NAME_DESC:
    case IDM_SORT_BY_DATE_ASC:
    case IDM_SORT_BY_DATE_DESC:
    case IDM_SORT_BY_SIZE_ASC:
    case IDM_SORT_BY_SIZE_DESC:
    {
        std::wstring currentFile;
        if (m_ctx.currentImageIndex >= 0 && m_ctx.currentImageIndex < static_cast<int>(m_ctx.imageFiles.size())) {
            currentFile = m_ctx.imageFiles[m_ctx.currentImageIndex];
        }

        m_ctx.isSortAscending = (cmd == IDM_SORT_BY_NAME_ASC || cmd == IDM_SORT_BY_DATE_ASC || cmd == IDM_SORT_BY_SIZE_ASC);
        if (cmd == IDM_SORT_BY_NAME_ASC || cmd == IDM_SORT_BY_NAME_DESC) m_ctx.currentSortCriteria = SortCriteria::ByName;
        else if (cmd == IDM_SORT_BY_DATE_ASC || cmd == IDM_SORT_BY_DATE_DESC) m_ctx.currentSortCriteria = SortCriteria::ByDateModified;
        else m_ctx.currentSortCriteria = SortCriteria::ByFileSize;

        if (!m_ctx.currentDirectory.empty()) {
            m_ctx.imageFiles.clear();
            LoadImageFromFile(currentFile);
        }
        break;
    }
    }
}



void ViewerApp::OnContextMenu(HWND hWnd, POINT pt) {
    HMENU hMenu = CreatePopupMenu();

    auto addAction = [&](HMENU menu, UINT id, ActionID act, const wchar_t* text) {
        std::wstring hk = GetHotkeyString(m_ctx.hotkeys[act]);
        std::wstring label = text;
        if (!hk.empty()) label += L"\t" + hk;
        AppendMenuW(menu, MF_STRING, id, label.c_str());
        };

    UINT copyPathFlags = (m_ctx.currentImageIndex != -1) ? MF_STRING : MF_STRING | MF_GRAYED;
    AppendMenuW(hMenu, copyPathFlags, IDM_COPY_PATH, Tr(StrId::MenuCopyPath));
    UINT ziyuFlags = (m_ctx.currentImageIndex != -1) ? MF_STRING : MF_STRING | MF_GRAYED;
    AppendMenuW(hMenu, ziyuFlags, IDM_SEND_ZYU_EDIT, Tr(StrId::MenuZiyuEdit));
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    HMENU hNativeMenu = CreatePopupMenu();

    AppendMenuW(hNativeMenu, MF_STRING, IDM_OPEN, (std::wstring(Tr(StrId::MenuOpenImage)) + L"\tCtrl+O").c_str());
    AppendMenuW(hNativeMenu, MF_STRING, IDM_REFRESH, (std::wstring(Tr(StrId::MenuRefresh)) + L"\tF5").c_str());
    AppendMenuW(hNativeMenu, MF_SEPARATOR, 0, nullptr);
    addAction(hNativeMenu, IDM_NEXT_IMG, Act_Next, Tr(StrId::MenuNextImage));
    addAction(hNativeMenu, IDM_PREV_IMG, Act_Prev, Tr(StrId::MenuPrevImage));
    AppendMenuW(hNativeMenu, MF_SEPARATOR, 0, nullptr);

    HMENU hSortMenu = CreatePopupMenu();
    auto addSortItem = [&](UINT id, SortCriteria crit, bool asc, LPCWSTR text) {
        UINT flags = MF_STRING | ((m_ctx.currentSortCriteria == crit && m_ctx.isSortAscending == asc) ? MF_CHECKED : MF_UNCHECKED);
        AppendMenuW(hSortMenu, flags, id, text);
        };
    addSortItem(IDM_SORT_BY_NAME_ASC, SortCriteria::ByName, true, Tr(StrId::MenuSortNameAsc));
    addSortItem(IDM_SORT_BY_NAME_DESC, SortCriteria::ByName, false, Tr(StrId::MenuSortNameDesc));
    addSortItem(IDM_SORT_BY_DATE_ASC, SortCriteria::ByDateModified, true, Tr(StrId::MenuSortDateAsc));
    addSortItem(IDM_SORT_BY_DATE_DESC, SortCriteria::ByDateModified, false, Tr(StrId::MenuSortDateDesc));
    addSortItem(IDM_SORT_BY_SIZE_ASC, SortCriteria::ByFileSize, true, Tr(StrId::MenuSortSizeAsc));
    addSortItem(IDM_SORT_BY_SIZE_DESC, SortCriteria::ByFileSize, false, Tr(StrId::MenuSortSizeDesc));
    AppendMenuW(hNativeMenu, MF_POPUP, (UINT_PTR)hSortMenu, Tr(StrId::MenuSortBy));
    AppendMenuW(hNativeMenu, MF_SEPARATOR, 0, nullptr);

    HMENU hViewMenu = CreatePopupMenu();
    addAction(hViewMenu, IDM_ZOOM_IN, Act_ZoomIn, Tr(StrId::MenuZoomIn));
    addAction(hViewMenu, IDM_ZOOM_OUT, Act_ZoomOut, Tr(StrId::MenuZoomOut));
    addAction(hViewMenu, IDM_ACTUAL_SIZE, Act_Actual, Tr(StrId::MenuActualSize100));
    AppendMenuW(hViewMenu, MF_STRING, IDM_ZOOM_200, Tr(StrId::MenuZoom200));
    AppendMenuW(hViewMenu, MF_STRING, IDM_ZOOM_300, Tr(StrId::MenuZoom300));
    addAction(hViewMenu, IDM_FIT_TO_WINDOW, Act_Fit, Tr(StrId::MenuFitToWindow));
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
    addAction(hViewMenu, IDM_FULLSCREEN, Act_Fullscreen, Tr(StrId::MenuFullScreen));
    addAction(hViewMenu, IDM_SLIDESHOW, Act_Slideshow, Tr(StrId::MenuToggleSlideshow));
    AppendMenuW(hNativeMenu, MF_POPUP, (UINT_PTR)hViewMenu, Tr(StrId::MenuView));

    UINT locationFlags = (m_ctx.currentImageIndex != -1) ? MF_STRING : MF_STRING | MF_GRAYED;
    AppendMenuW(hNativeMenu, locationFlags, IDM_OPEN_LOCATION, Tr(StrId::MenuOpenLocation));
    AppendMenuW(hNativeMenu, locationFlags, IDM_PROPERTIES, Tr(StrId::MenuProperties));

    AppendMenuW(hNativeMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hNativeMenu, MF_STRING, IDM_PREFERENCES, Tr(StrId::MenuPreferences));
    AppendMenuW(hNativeMenu, MF_STRING, IDM_KEYBINDINGS, Tr(StrId::MenuKeybindings));
    AppendMenuW(hNativeMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hNativeMenu, MF_STRING, IDM_EXIT, (std::wstring(Tr(StrId::MenuHideToBackground)) + L"\tEsc").c_str());
    AppendMenuW(hNativeMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hNativeMenu, MF_STRING, IDM_QUIT, Tr(StrId::MenuCloseApp));

    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hNativeMenu, Tr(StrId::MenuNative));

    if (m_ctx.isSlideshowActive) {
        CheckMenuItem(hMenu, IDM_SLIDESHOW, MF_BYCOMMAND | MF_CHECKED);
    }

    int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd > 0) {
        HandleCommand(static_cast<WORD>(cmd));
    }
}




LRESULT CALLBACK ViewerApp::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    ViewerApp* pApp = nullptr;

    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pApp = reinterpret_cast<ViewerApp*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
    }
    else {
        pApp = reinterpret_cast<ViewerApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pApp) {
        return pApp->WndProc(hWnd, message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT ViewerApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static POINT dragStart = {};
    switch (message) {
    case WM_APP_IMAGE_READY:
        OnImageReady(wParam != 0, (int)lParam);
        break;
    case WM_APP_DIR_READY:
        OnDirReady((int)lParam);
        break;
    case WM_APP_IMAGE_LOADED:
        FinalizeImageLoad(true, static_cast<int>(wParam));
        break;
    case WM_APP_IMAGE_LOAD_FAILED:
        KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
        if (lParam != 0) {
            if (m_ctx.loadSequenceId == (int)lParam) {
                FinalizeImageLoad(false, -1);
            }
        }
        else {
            FinalizeImageLoad(false, -1);
        }
        break;

    case WM_TIMER:
        if (wParam == ANIMATION_TIMER_ID) {
            std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
            if (m_ctx.isAnimated && !m_ctx.animationFrameDelays.empty()) {
                const UINT frameCount = static_cast<UINT>(m_ctx.animationFrameDelays.size());

                UINT currentDelay = m_ctx.animationFrameDelays[m_ctx.currentAnimationFrame];
                m_ctx.currentAnimationFrame = (m_ctx.currentAnimationFrame + 1) % frameCount;
                UINT nextDelay = m_ctx.animationFrameDelays[m_ctx.currentAnimationFrame];

                m_ctx.currentAnimatedConverter = GetCompositedAnimationFrame(m_ctx.currentAnimationFrame);
                m_ctx.wicConverterOriginal = m_ctx.currentAnimatedConverter;
                m_ctx.wicConverter = m_ctx.currentAnimatedConverter;
                m_ctx.d2dBitmap = nullptr; // Force D2D recreation

                UpdateWindowTitle();
                InvalidateRect(hWnd, nullptr, FALSE);
                if (currentDelay != nextDelay) {
                    KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
                    SetTimer(m_ctx.hWnd, ANIMATION_TIMER_ID, nextDelay, nullptr);
                }
            }
        }
        else if (wParam == LOADING_TIMER_ID) {
            KillTimer(m_ctx.hWnd, LOADING_TIMER_ID);
            if (m_ctx.isLoading) {
                InvalidateRect(hWnd, nullptr, FALSE);
            }
        }
        else if (wParam == NAV_DEBOUNCE_TIMER_ID) {
            KillTimer(m_ctx.hWnd, NAV_DEBOUNCE_TIMER_ID);
            if (m_ctx.pendingNavIndex != -1 && m_ctx.pendingNavIndex < m_ctx.imageFiles.size()) {
                LoadImageFromFile(m_ctx.imageFiles[m_ctx.pendingNavIndex].c_str(), m_ctx.startAtEnd);
                m_ctx.pendingNavIndex = -1;
            }
        }
        else if (wParam == SLIDESHOW_TIMER_ID) {
            if (m_ctx.isSlideshowActive) {
                HandleCommand(IDM_NEXT_IMG);
            }
        }
        
        else if (wParam == AUTO_REFRESH_TIMER_ID) {
            if (m_ctx.isAutoRefresh && !m_ctx.isLoading && !m_ctx.imageFiles.empty() && m_ctx.currentImageIndex >= 0) {
                const std::wstring& currentFile = m_ctx.imageFiles[m_ctx.currentImageIndex];
                WIN32_FILE_ATTRIBUTE_DATA fad;
                if (GetFileAttributesExW(currentFile.c_str(), GetFileExInfoStandard, &fad)) {
                    if (CompareFileTime(&fad.ftLastWriteTime, &m_ctx.lastWriteTime) > 0) {
                        // verify file is not locked
                        HANDLE hFile = CreateFileW(currentFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                        if (hFile != INVALID_HANDLE_VALUE) {
                            CloseHandle(hFile);
                            m_ctx.preserveView = true;
                            m_ctx.imageFiles.clear(); // force directory rescan
                            LoadImageFromFile(currentFile);
                        }
                    }
                }
            }
        }
        break;
    case WM_DPICHANGED: {
        RECT* prcNewWindow = reinterpret_cast<RECT*>(lParam);
        // 核心功能1：自动切屏期间由 ApplyMonitorPlacement 统一设置目标尺寸，
        // 不让系统按 DPI 比例缩放覆盖（两个显示器 DPI 不同会破坏百分比尺寸）
        if (!m_ctx.suppressDpiResize) {
            SetWindowPos(hWnd, nullptr,
                prcNewWindow->left, prcNewWindow->top,
                prcNewWindow->right - prcNewWindow->left,
                prcNewWindow->bottom - prcNewWindow->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }

        // Update the DPI-dependent text format instead of dropping all device resources
        if (m_ctx.writeFactory) {
            float dpiScale = HIWORD(wParam) / 96.0f;
            m_ctx.textFormat = nullptr;

            if (SUCCEEDED(m_ctx.writeFactory->CreateTextFormat(
                L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, 14.0f * dpiScale, I18n::DWriteLocale(), &m_ctx.textFormat)))
            {
                m_ctx.textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                m_ctx.textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            }
        }

        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; 
    case WM_PAINT:
        OnPaint(hWnd);
        break;
    case WM_COMMAND:
        HandleCommand(LOWORD(wParam));
        break;
    case WM_XBUTTONDOWN: {
        WORD xButton = GET_XBUTTON_WPARAM(wParam);
        if (xButton == XBUTTON1) { // Standard Back button
            HandleCommand(IDM_PREV_IMG);
            return TRUE;
        }
        else if (xButton == XBUTTON2) { // Standard Forward button
            HandleCommand(IDM_NEXT_IMG);
            return TRUE;
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        // 上滚动 = 上一张，下滚动 = 下一张
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) {
            HandleCommand(IDM_PREV_IMG); // 上滚动 = 上一张
        }
        else {
            HandleCommand(IDM_NEXT_IMG); // 下滚动 = 下一张
        }
        break;
    }
    case WM_MBUTTONUP: {
        // 中键点击 = 全屏/退出全屏
        HandleCommand(IDM_FULLSCREEN);
        break;
    }
    case WM_LBUTTONDBLCLK:
        FitImageToWindow();
        break;
    case WM_RBUTTONUP: {
        if (m_ctx.isCropMode || m_ctx.isSelectingCropRect || m_ctx.isCropPending) {
            bool wasCropActive = m_ctx.isCropActive;
            m_ctx.isCropMode = false;
            m_ctx.isSelectingCropRect = false;
            m_ctx.isCropPending = false;
            if (wasCropActive) {
                m_ctx.isCropActive = false;
                ApplyEffectsToView();
                FitImageToWindow();
            }
            else {
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            if (GetCapture() == hWnd) {
                ReleaseCapture();
            }
            InvalidateRect(hWnd, nullptr, FALSE);
            break;
        }
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ClientToScreen(hWnd, &pt);
        OnContextMenu(hWnd, pt);
        break;
    }
    case WM_DROPFILES:
        HandleDropFiles(reinterpret_cast<HDROP>(wParam));
        break;
    case WM_LBUTTONDOWN: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        if (m_ctx.isCropMode) {
            m_ctx.isCropPending = false;
            m_ctx.isSelectingCropRect = true;
            m_ctx.cropStartPoint = pt;
            m_ctx.cropRectWindow = D2D1::RectF(
                static_cast<float>(pt.x), static_cast<float>(pt.y),
                static_cast<float>(pt.x), static_cast<float>(pt.y)
            );
            SetCapture(hWnd);
        }
        else {
            RECT rc; GetClientRect(hWnd, &rc);
            int width = rc.right - rc.left;

            // Check for left/right 8% navigation clicks
            if (!m_ctx.imageFiles.empty() && width > 0 && pt.x < width * 0.08) {
                HandleCommand(IDM_PREV_IMG);
            }
            else if (!m_ctx.imageFiles.empty() && width > 0 && pt.x > width * 0.92) {
                HandleCommand(IDM_NEXT_IMG);
            }
            else {
                UINT w = 0, h = 0;
                if (GetCurrentImageSize(&w, &h)) {
                    m_ctx.isDraggingImage = true;
                    dragStart = pt;
                    SetCapture(hWnd);
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                }
            }
        }
        break;
    }
    case WM_LBUTTONUP:
        if (m_ctx.isSelectingCropRect) {
            m_ctx.isSelectingCropRect = false;
            m_ctx.isCropMode = false;
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            ReleaseCapture();

            float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
            ConvertWindowToImagePoint(m_ctx.cropStartPoint, x1, y1);
            POINT endPoint = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ConvertWindowToImagePoint(endPoint, x2, y2);

            m_ctx.cropRectLocal = D2D1::RectF(std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2));

            UINT imgWidth = 0, imgHeight = 0;
            GetCurrentImageSize(&imgWidth, &imgHeight);

            m_ctx.cropRectLocal.left = std::max(0.0f, m_ctx.cropRectLocal.left);
            m_ctx.cropRectLocal.top = std::max(0.0f, m_ctx.cropRectLocal.top);
            m_ctx.cropRectLocal.right = std::min(static_cast<float>(imgWidth), m_ctx.cropRectLocal.right);
            m_ctx.cropRectLocal.bottom = std::min(static_cast<float>(imgHeight), m_ctx.cropRectLocal.bottom);

            if (m_ctx.cropRectLocal.left < m_ctx.cropRectLocal.right && m_ctx.cropRectLocal.top < m_ctx.cropRectLocal.bottom) {
                m_ctx.isCropPending = true;
            }
            else {
                m_ctx.isCropPending = false;
            }
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        else if (m_ctx.isDraggingImage) {
            m_ctx.isDraggingImage = false;
            ReleaseCapture();
        }
        break;
    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (m_ctx.isSelectingCropRect) {
            m_ctx.cropRectWindow.right = static_cast<float>(pt.x);
            m_ctx.cropRectWindow.bottom = static_cast<float>(pt.y);
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        else if (m_ctx.isDraggingImage) {
            m_ctx.offsetX += (pt.x - dragStart.x);
            m_ctx.offsetY += (pt.y - dragStart.y);
            dragStart = pt;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;
    }
    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            if (m_ctx.isCropMode || m_ctx.isSelectingCropRect || m_ctx.isCropPending) {
                SetCursor(LoadCursor(nullptr, IDC_CROSS));
                return TRUE;
            }
            if (m_ctx.isDraggingImage) {
                SetCursor(LoadCursor(nullptr, IDC_HAND));
                return TRUE;
            }

            // Check if over 8% navigation zones
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            RECT rc;
            GetClientRect(hWnd, &rc);
            int width = rc.right - rc.left;

            if (!m_ctx.imageFiles.empty() && width > 0 && (pt.x < width * 0.08 || pt.x > width * 0.92)) {
                SetCursor(LoadCursor(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    case WM_COPYDATA: {
        PCOPYDATASTRUCT pcds = reinterpret_cast<PCOPYDATASTRUCT>(lParam);
        if (pcds && pcds->dwData == 1) {
            std::wstring filePath(static_cast<wchar_t*>(pcds->lpData));
            if (filePath.length() >= 2 && filePath.front() == L'"' && filePath.back() == L'"') {
                filePath = filePath.substr(1, filePath.length() - 2);
            }
            // 外部打开新图：先清掉当前显示的旧图并刷为背景色，避免“先闪旧图再出新图”
            ClearCurrentImageView();
            LoadImageFromFile(filePath);
        }
        return TRUE;
    }
    case WM_SIZE:
        if (m_ctx.renderTarget) {
            if (wParam == SIZE_MINIMIZED) {
                KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
            }
            else {
                if (m_ctx.swapChain) {
                    m_ctx.renderTarget->SetTarget(nullptr);
                    m_ctx.swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                    ComPtr<IDXGISurface> backBuffer;
                    m_ctx.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
                    D2D1_BITMAP_PROPERTIES1 bmpProps = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
                    ComPtr<ID2D1Bitmap1> targetBmp;
                    m_ctx.renderTarget->CreateBitmapFromDxgiSurface(backBuffer.Get(), &bmpProps, &targetBmp);
                    m_ctx.renderTarget->SetTarget(targetBmp.Get());
                }
                if (m_ctx.isAnimated && !m_ctx.animationFrameDelays.empty()) {
                    KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
                    SetTimer(m_ctx.hWnd, ANIMATION_TIMER_ID, m_ctx.animationFrameDelays[m_ctx.currentAnimationFrame], nullptr);
                }
            }
        }
        if (wParam != SIZE_MINIMIZED) {
            if (!m_ctx.isLoading) {
                if (!m_ctx.preserveZoomOnResize) {
                    FitImageToWindow();
                }
                InvalidateRect(hWnd, nullptr, FALSE);
            }
        }
        break;
    case WM_CLOSE:
        // Esc / 关闭按钮 / Alt+F4 统一为“隐藏到后台”，进程常驻以便下次看图更快
        HideToBackground();
        return 0;
    case WM_DESTROY:
        KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
        KillTimer(m_ctx.hWnd, AUTO_REFRESH_TIMER_ID);
        if (m_ctx.hPropsWnd) {
            DestroyWindow(m_ctx.hPropsWnd);
        }
        if (!m_ctx.isFullScreen) {
            m_ctx.windowPlacement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(m_ctx.hWnd, &m_ctx.windowPlacement);
        }
        WriteSettings(m_ctx.settingsPath, m_ctx.windowPlacement, m_ctx.startFullScreen, m_ctx.enforceSingleInstance, m_ctx.alwaysOnTop);
        CleanupLoadingThread();
        DiscardDeviceResources();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void ViewerApp::HideToBackground() {
    // 1. 使所有在途的加载/预加载/目录扫描任务失效（loadSequenceId 机制会拦截过期结果）
    m_ctx.cancelPreloading = true;
    ++m_ctx.loadSequenceId;
    m_ctx.isLoading = false;
    CleanupPreloadingThreads();
    KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
    KillTimer(m_ctx.hWnd, AUTO_REFRESH_TIMER_ID);
    KillTimer(m_ctx.hWnd, LOADING_TIMER_ID);
    KillTimer(m_ctx.hWnd, NAV_DEBOUNCE_TIMER_ID);
    KillTimer(m_ctx.hWnd, SLIDESHOW_TIMER_ID);

    // 保持全屏状态：全屏时按 Esc 直接隐藏到后台，下次唤起仍以全屏显示
    //（不调用 ToggleFullScreen，保留 isFullScreen / WS_POPUP 样式与窗口位置）

    // 2. 短暂等待后台线程收尾，避免与下面的清理产生竞态
    int waitMs = 300;
    while (m_ctx.activeBackgroundThreads > 0 && waitMs > 0) {
        Sleep(5);
        waitMs -= 5;
    }

    // 3. 释放大块图片缓存（像素/解码器/预加载/动画），并把已呈现帧刷为背景色，
    //    保留 WIC/D2D 工厂、渲染目标、交换链等设备资源，使下次看图无需重建
    ClearCurrentImageView();

    m_ctx.currentImageIndex = -1;
    m_ctx.currentDirectory.clear();
    m_ctx.currentFilePathOverride.clear();
    m_ctx.pendingNavIndex = -1;
    m_ctx.stagedFoundIndex = -1;
    m_ctx.imageFiles.clear();
    m_ctx.stagedImageFiles.clear();
    m_ctx.isAnimated = false;
    m_ctx.isSvg = false;
    m_ctx.isCropMode = false;
    m_ctx.isSelectingCropRect = false;
    m_ctx.isCropPending = false;
    m_ctx.isCropActive = false;
    m_ctx.isFading = false;
    m_ctx.isSlideshowActive = false;
    m_ctx.startAtEnd = false;
    m_ctx.lastCompositedFrame = -1;

    // 4. 隐藏窗口，进程常驻后台
    ShowWindow(m_ctx.hWnd, SW_HIDE);

    // 5. 将工作集交还系统，压低后台常驻内存
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
}

void ViewerApp::ClearCurrentImageView() {
    // 释放大块图片缓存（保留 WIC/D2D 工厂、渲染目标、交换链等设备资源）
    {
        std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
        m_ctx.d2dBitmap = nullptr;
        m_ctx.wicConverter = nullptr;
        m_ctx.wicConverterOriginal = nullptr;
        m_ctx.undoStack.clear();
        m_ctx.rawFileData.clear();
        m_ctx.stagedRawFileData.clear();
        m_ctx.wicStream = nullptr;
        m_ctx.stagedWicStream = nullptr;
        m_ctx.stagedFrames.clear();
        m_ctx.stagedDelays.clear();
        m_ctx.stagedFrameMetadata.clear();
        m_ctx.stagedStaticConverter = nullptr;
        m_ctx.animationDecoder = nullptr;
        m_ctx.currentAnimatedConverter = nullptr;
        m_ctx.animationD2DBitmaps.clear();
        m_ctx.animationFrameMetadata.clear();
        m_ctx.animationFrameDelays.clear();
        m_ctx.animationCanvas.clear();
        m_ctx.animationCanvasPrev.clear();
        m_ctx.highResImageSource = nullptr;
        m_ctx.svgDocument = nullptr;
        m_ctx.svgData.clear();
        m_ctx.stagedSvgData.clear();
    }
    m_ctx.isOsdCacheValid = false;

    // 立即把已呈现帧刷为背景色，避免窗口重新显示时残留旧图造成闪屏
    if (m_ctx.renderTarget) {
        m_ctx.renderTarget->BeginDraw();
        D2D1_COLOR_F color;
        switch (m_ctx.bgColor) {
        case BackgroundColor::Black:      color = D2D1::ColorF(0.0f, 0.0f, 0.0f); break;
        case BackgroundColor::White:      color = D2D1::ColorF(1.0f, 1.0f, 1.0f); break;
        case BackgroundColor::Transparent: color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f); break;
        default:
        case BackgroundColor::Grey:       color = D2D1::ColorF(0.117f, 0.117f, 0.117f); break;
        }
        m_ctx.renderTarget->Clear(color);
        m_ctx.renderTarget->EndDraw();
        if (m_ctx.swapChain) {
            m_ctx.swapChain->Present(1, 0);
        }
    }
    InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
}

