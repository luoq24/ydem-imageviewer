
#include "viewer.h"
#include <commctrl.h>
#include <stdio.h>

template<typename T>
T* GetAppFromDialog(HWND hDlg, UINT message, LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
        return reinterpret_cast<T*>(lParam);
    }
    return reinterpret_cast<T*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
}

// 对话框控件文本在显示前统一替换为当前语言的文本
static void LocalizePreferencesDialog(HWND hDlg) {
    SetWindowTextW(hDlg, Tr(StrId::DlgPreferencesTitle));
    SetDlgItemTextW(hDlg, IDC_STATIC_BG_GROUP, Tr(StrId::PrefBgGroup));
    SetDlgItemTextW(hDlg, IDC_RADIO_BG_GREY, Tr(StrId::PrefBgGrey));
    SetDlgItemTextW(hDlg, IDC_RADIO_BG_BLACK, Tr(StrId::PrefBgBlack));
    SetDlgItemTextW(hDlg, IDC_RADIO_BG_WHITE, Tr(StrId::PrefBgWhite));
    SetDlgItemTextW(hDlg, IDC_RADIO_BG_TRANSPARENT, Tr(StrId::PrefBgTransparent));
    SetDlgItemTextW(hDlg, IDC_STATIC_APP_GROUP, Tr(StrId::PrefAppGroup));
    SetDlgItemTextW(hDlg, IDC_CHECK_ALWAYS_ON_TOP, Tr(StrId::PrefAlwaysOnTop));
    SetDlgItemTextW(hDlg, IDC_CHECK_START_FULLSCREEN, Tr(StrId::PrefStartFullscreen));
    SetDlgItemTextW(hDlg, IDC_CHECK_SINGLE_INSTANCE, Tr(StrId::PrefSingleInstance));
    SetDlgItemTextW(hDlg, IDC_CHECK_AUTO_REFRESH, Tr(StrId::PrefAutoRefresh));
    SetDlgItemTextW(hDlg, IDC_CHECK_SMOOTH_SCALING, Tr(StrId::PrefSmoothScaling));
    SetDlgItemTextW(hDlg, IDC_CHECK_FADE_ANIMATION, Tr(StrId::PrefFadeAnimation));
    SetDlgItemTextW(hDlg, IDC_CHECK_SHOW_OSD, Tr(StrId::PrefShowOsd));
    SetDlgItemTextW(hDlg, IDC_CHECK_ASK_DELETE, Tr(StrId::PrefAskDelete));
    SetDlgItemTextW(hDlg, IDC_CHECK_PRESERVE_ZOOM, Tr(StrId::PrefPreserveZoom));
    SetDlgItemTextW(hDlg, IDC_CHECK_AUTO_MONITOR, Tr(StrId::PrefAutoMonitorPlacement));
    SetDlgItemTextW(hDlg, IDC_CHECK_LIST_LOOP, Tr(StrId::PrefListLoop));
    SetDlgItemTextW(hDlg, IDC_STATIC_ZOOM_GROUP, Tr(StrId::PrefZoomGroup));
    SetDlgItemTextW(hDlg, IDC_RADIO_ZOOM_FIT, Tr(StrId::PrefZoomFit));
    SetDlgItemTextW(hDlg, IDC_RADIO_ZOOM_ACTUAL, Tr(StrId::PrefZoomActual));
    SetDlgItemTextW(hDlg, IDC_STATIC_LANG_GROUP, Tr(StrId::PrefLanguageGroup));
    SetDlgItemTextW(hDlg, IDC_RADIO_LANG_ZH, Tr(StrId::PrefLangZh));
    SetDlgItemTextW(hDlg, IDC_RADIO_LANG_EN, Tr(StrId::PrefLangEn));
    SetDlgItemTextW(hDlg, IDOK, Tr(StrId::BtnOk));
    SetDlgItemTextW(hDlg, IDCANCEL, Tr(StrId::BtnCancel));
}

INT_PTR CALLBACK ViewerApp::PreferencesDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    ViewerApp* pApp = nullptr;
    if (message == WM_INITDIALOG) {
        pApp = reinterpret_cast<ViewerApp*>(lParam);
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pApp);
    }
    else {
        pApp = reinterpret_cast<ViewerApp*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
    }

    if (!pApp) return (INT_PTR)FALSE;

    auto& ctx = pApp->GetContext();
    switch (message) {
    case WM_INITDIALOG: {
        pApp->UpdateTitleBarTheme(hDlg, ctx.bgColor);
        if (ctx.bgColor == BackgroundColor::Black || ctx.bgColor == BackgroundColor::Grey) {
            if (!ctx.darkBrush) ctx.darkBrush.reset(CreateSolidBrush(RGB(32, 32, 32)));
        }
        else {
            ctx.darkBrush.reset();
        }

        LocalizePreferencesDialog(hDlg);

        int bgRadio = IDC_RADIO_BG_GREY + static_cast<int>(ctx.bgColor);
        CheckRadioButton(hDlg, IDC_RADIO_BG_GREY, IDC_RADIO_BG_TRANSPARENT, bgRadio);
        CheckDlgButton(hDlg, IDC_CHECK_ALWAYS_ON_TOP, ctx.alwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_START_FULLSCREEN, ctx.startFullScreen ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_SINGLE_INSTANCE, ctx.enforceSingleInstance ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_AUTO_REFRESH, ctx.isAutoRefresh ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_SMOOTH_SCALING, ctx.smoothScaling ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_FADE_ANIMATION, ctx.enableFadeAnimation ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_SHOW_OSD, ctx.isOsdVisible ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_ASK_DELETE, ctx.askToDelete ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_PRESERVE_ZOOM, ctx.preserveZoomOnResize ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_AUTO_MONITOR, ctx.autoMonitorPlacement ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_LIST_LOOP, ctx.listLoopEnabled ? BST_CHECKED : BST_UNCHECKED);

        CheckRadioButton(hDlg, IDC_RADIO_ZOOM_FIT, IDC_RADIO_ZOOM_ACTUAL,
            ctx.defaultZoomMode == DefaultZoomMode::Fit ? IDC_RADIO_ZOOM_FIT : IDC_RADIO_ZOOM_ACTUAL);

        CheckRadioButton(hDlg, IDC_RADIO_LANG_ZH, IDC_RADIO_LANG_EN,
            I18n::GetLanguage() == AppLanguage::Chinese ? IDC_RADIO_LANG_ZH : IDC_RADIO_LANG_EN);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            for (int id = IDC_RADIO_BG_GREY; id <= IDC_RADIO_BG_TRANSPARENT; ++id) {
                if (IsDlgButtonChecked(hDlg, id)) {
                    ctx.bgColor = static_cast<BackgroundColor>(id - IDC_RADIO_BG_GREY);
                }
            }

            ctx.alwaysOnTop = (IsDlgButtonChecked(hDlg, IDC_CHECK_ALWAYS_ON_TOP) == BST_CHECKED);
            const bool newStartFullScreen = (IsDlgButtonChecked(hDlg, IDC_CHECK_START_FULLSCREEN) == BST_CHECKED);
            ctx.startFullScreen = newStartFullScreen;
            ctx.enforceSingleInstance = (IsDlgButtonChecked(hDlg, IDC_CHECK_SINGLE_INSTANCE) == BST_CHECKED);

            bool newAutoRefresh = (IsDlgButtonChecked(hDlg, IDC_CHECK_AUTO_REFRESH) == BST_CHECKED);
            if (newAutoRefresh != ctx.isAutoRefresh) {
                ctx.isAutoRefresh = newAutoRefresh;
                if (ctx.isAutoRefresh) {
                    SetTimer(ctx.hWnd, AUTO_REFRESH_TIMER_ID, 1000, nullptr);
                }
                else {
                    KillTimer(ctx.hWnd, AUTO_REFRESH_TIMER_ID);
                }
            }

            bool newSmoothScaling = (IsDlgButtonChecked(hDlg, IDC_CHECK_SMOOTH_SCALING) == BST_CHECKED);
            if (newSmoothScaling != ctx.smoothScaling) {
                ctx.smoothScaling = newSmoothScaling;
            }

            ctx.enableFadeAnimation = (IsDlgButtonChecked(hDlg, IDC_CHECK_FADE_ANIMATION) == BST_CHECKED);
            ctx.isOsdVisible = (IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_OSD) == BST_CHECKED);
            ctx.askToDelete = (IsDlgButtonChecked(hDlg, IDC_CHECK_ASK_DELETE) == BST_CHECKED);
            ctx.preserveZoomOnResize = (IsDlgButtonChecked(hDlg, IDC_CHECK_PRESERVE_ZOOM) == BST_CHECKED);
            ctx.autoMonitorPlacement = (IsDlgButtonChecked(hDlg, IDC_CHECK_AUTO_MONITOR) == BST_CHECKED);
            ctx.listLoopEnabled = (IsDlgButtonChecked(hDlg, IDC_CHECK_LIST_LOOP) == BST_CHECKED);

            if (IsDlgButtonChecked(hDlg, IDC_RADIO_ZOOM_FIT)) {
                ctx.defaultZoomMode = DefaultZoomMode::Fit;
            }
            else if (IsDlgButtonChecked(hDlg, IDC_RADIO_ZOOM_ACTUAL)) {
                ctx.defaultZoomMode = DefaultZoomMode::Actual;
            }

            // 语言切换：立即生效（菜单在每次打开时重建，OSD 缓存需要失效）
            const AppLanguage newLanguage = IsDlgButtonChecked(hDlg, IDC_RADIO_LANG_EN) == BST_CHECKED
                ? AppLanguage::English : AppLanguage::Chinese;
            if (newLanguage != I18n::GetLanguage()) {
                I18n::SetLanguage(newLanguage);
                ctx.isOsdCacheValid = false;
                pApp->UpdateWindowTitle();
            }

            // Immediate update from fullscreen setting
            if (!ctx.startFullScreen && ctx.isFullScreen) {
                pApp->ToggleFullScreen();
            }

            // Fullscreen not topmost
            if (!ctx.isFullScreen) {
                SetWindowPos(
                    ctx.hWnd,
                    ctx.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                    0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE
                );
            }

            pApp->UpdateTitleBarTheme(ctx.hWnd, ctx.bgColor);
            InvalidateRect(ctx.hWnd, NULL, FALSE);

            // 核心功能1：若开启，立即把当前窗口移到匹配图片方向的显示器
            pApp->ApplyMonitorPlacement();

            // Auto-save 
            if (!ctx.isFullScreen) {
                ctx.windowPlacement.length = sizeof(WINDOWPLACEMENT);
                GetWindowPlacement(ctx.hWnd, &ctx.windowPlacement);
            }

            pApp->WriteSettings(
                ctx.settingsPath,
                ctx.windowPlacement,
                ctx.startFullScreen,
                ctx.enforceSingleInstance,
                ctx.alwaysOnTop
            );

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    case WM_DESTROY:
        break;
    }
    return (INT_PTR)FALSE;
}

void ViewerApp::OpenPreferencesDialog() {
    DialogBoxParam(m_ctx.hInst, MAKEINTRESOURCE(IDD_PREFERENCES_DIALOG), m_ctx.hWnd, PreferencesDialogProc, (LPARAM)this);
}

std::wstring ViewerApp::GetHotkeyString(WORD hk) {
    if (!hk) return L"";
    std::wstring str;
    BYTE mods = HIBYTE(hk);
    BYTE vk = LOBYTE(hk);

    if (mods & HOTKEYF_CONTROL) str += L"Ctrl+";
    if (mods & HOTKEYF_SHIFT) str += L"Shift+";
    if (mods & HOTKEYF_ALT) str += L"Alt+";

    wchar_t keyName[64] = { 0 };
    switch (vk) {
    case VK_LEFT: wcscpy_s(keyName, Tr(StrId::HkLeft)); break;
    case VK_RIGHT: wcscpy_s(keyName, Tr(StrId::HkRight)); break;
    case VK_UP: wcscpy_s(keyName, Tr(StrId::HkUp)); break;
    case VK_DOWN: wcscpy_s(keyName, Tr(StrId::HkDown)); break;
    case VK_ESCAPE: wcscpy_s(keyName, L"Esc"); break;
    case VK_RETURN: wcscpy_s(keyName, L"Enter"); break;
    case VK_SPACE: wcscpy_s(keyName, Tr(StrId::HkSpacebar)); break;
    case VK_DELETE: wcscpy_s(keyName, L"Delete"); break;
    case VK_INSERT: wcscpy_s(keyName, L"Insert"); break;
    case VK_HOME: wcscpy_s(keyName, L"Home"); break;
    case VK_END: wcscpy_s(keyName, L"End"); break;
    case VK_PRIOR: wcscpy_s(keyName, L"Page Up"); break;
    case VK_NEXT: wcscpy_s(keyName, L"Page Down"); break;
    case VK_ADD:
    case VK_OEM_PLUS: wcscpy_s(keyName, L"+"); break;
    case VK_SUBTRACT:
    case VK_OEM_MINUS: wcscpy_s(keyName, L"-"); break;
    case VK_MULTIPLY: wcscpy_s(keyName, L"*"); break;
    case VK_DIVIDE: wcscpy_s(keyName, L"/"); break;
    case VK_TAB: wcscpy_s(keyName, L"Tab"); break;
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9: {
        std::wstring numPad = std::wstring(Tr(StrId::HkNumpadPrefix)) + static_cast<wchar_t>(L'0' + (vk - VK_NUMPAD0));
        wcscpy_s(keyName, numPad.c_str());
        break;
    }
    default: {
        UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        LONG lParam = (scanCode << 16);
        // Ensure GetKeyNameTextW doesn't fall back to numpad equivalents
        if (mods & HOTKEYF_EXT) lParam |= (1 << 24);
        GetKeyNameTextW(lParam, keyName, 64);
        break;
    }
    }
    str += keyName;
    return str;
}

// 快捷键对话框中显示的动作名称（按 ActionID 顺序），随当前语言返回
static const wchar_t* GetActionName(int actionIndex) {
    switch (static_cast<ActionID>(actionIndex)) {
    case Act_Next:         return Tr(StrId::MenuNextImage);
    case Act_Prev:         return Tr(StrId::MenuPrevImage);
    case Act_FirstImage:   return Tr(StrId::ActNameFirstImage);
    case Act_LastImage:    return Tr(StrId::ActNameLastImage);
    case Act_ZoomIn:       return Tr(StrId::MenuZoomIn);
    case Act_ZoomOut:      return Tr(StrId::MenuZoomOut);
    case Act_Fit:          return Tr(StrId::MenuFitToWindow);
    case Act_Actual:       return Tr(StrId::ActNameActualSize);
    case Act_Fullscreen:   return Tr(StrId::ActNameFullscreen);
    case Act_RotateCW:     return Tr(StrId::MenuRotateCW);
    case Act_RotateCCW:    return Tr(StrId::MenuRotateCCW);
    case Act_Flip:         return Tr(StrId::MenuFlip);
    case Act_Crop:         return Tr(StrId::MenuCrop);
    case Act_CustomZoom:   return Tr(StrId::ActNameCustomZoom);
    case Act_Exit:         return Tr(StrId::MenuHideToBackground);
    case Act_Open:         return Tr(StrId::ActNameOpenFile);
    case Act_Refresh:      return Tr(StrId::MenuRefresh);
    case Act_Copy:         return Tr(StrId::MenuCopy);
    case Act_Paste:        return Tr(StrId::MenuPaste);
    case Act_Save:         return Tr(StrId::MenuSave);
    case Act_SaveAs:       return Tr(StrId::MenuSaveAs);
    case Act_Delete:       return Tr(StrId::MenuDeleteImage);
    case Act_Undo:         return Tr(StrId::ActNameUndo);
    case Act_CenterImage:  return Tr(StrId::ActNameCenterImage);
    case Act_CommitCrop:   return Tr(StrId::ActNameCommitCrop);
    case Act_ToggleOSD:    return Tr(StrId::ActNameToggleOSD);
    case Act_PlayPause:    return Tr(StrId::ActNamePlayPause);
    case Act_ResumeAnim:   return Tr(StrId::ActNameResumeAnim);
    case Act_AnimNext:     return Tr(StrId::ActNameNextFrame);
    case Act_AnimPrev:     return Tr(StrId::ActNamePrevFrame);
    case Act_AnimFirst:    return Tr(StrId::ActNameFirstFrame);
    case Act_ContextMenu:  return Tr(StrId::ActNameContextMenu);
    case Act_Slideshow:    return Tr(StrId::MenuToggleSlideshow);
    default:               return L"";
    }
}

static void LocalizeKeybindingsDialog(HWND hDlg) {
    SetWindowTextW(hDlg, Tr(StrId::DlgKeybindingsTitle));
    SetDlgItemTextW(hDlg, IDC_STATIC_KB_ACTION, Tr(StrId::KbAction));
    SetDlgItemTextW(hDlg, IDC_STATIC_KB_SHORTCUT, Tr(StrId::KbShortcut));
    SetDlgItemTextW(hDlg, IDOK, Tr(StrId::BtnApply));
    SetDlgItemTextW(hDlg, IDCANCEL, Tr(StrId::BtnClose));
    SetDlgItemTextW(hDlg, IDC_BTN_RESTORE_DEFAULTS, Tr(StrId::BtnRestoreDefaults));
}

INT_PTR CALLBACK ViewerApp::KeybindingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    ViewerApp* pApp = GetAppFromDialog<ViewerApp>(hDlg, message, lParam);
    if (!pApp) return (INT_PTR)FALSE;

    auto& ctx = pApp->GetContext();

    switch (message) {
    case WM_INITDIALOG: {
        LocalizeKeybindingsDialog(hDlg);
        HWND hCombo = GetDlgItem(hDlg, IDC_COMBO_ACTION);
        for (int i = 0; i < Act_Count; ++i) {
            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)GetActionName(i));
        }
        SendMessageW(hCombo, CB_SETCURSEL, 0, 0);
        SendMessageW(GetDlgItem(hDlg, IDC_HOTKEY_CTRL), HKM_SETHOTKEY, ctx.hotkeys[0], 0);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_COMBO_ACTION && HIWORD(wParam) == CBN_SELCHANGE) {
            int idx = static_cast<int>(SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0));
            if (idx != CB_ERR) {
                SendMessageW(GetDlgItem(hDlg, IDC_HOTKEY_CTRL), HKM_SETHOTKEY, ctx.hotkeys[idx], 0);
            }
            return (INT_PTR)TRUE;
        }
        switch (LOWORD(wParam)) {
        case IDOK: {
            int idx = static_cast<int>(SendMessageW(GetDlgItem(hDlg, IDC_COMBO_ACTION), CB_GETCURSEL, 0, 0));
            if (idx != CB_ERR) {
                ctx.hotkeys[idx] = static_cast<WORD>(SendMessageW(GetDlgItem(hDlg, IDC_HOTKEY_CTRL), HKM_GETHOTKEY, 0, 0));
                pApp->UpdateAcceleratorTable(); 

                // Auto-save keybinding 
                if (!ctx.isFullScreen) {
                    ctx.windowPlacement.length = sizeof(WINDOWPLACEMENT);
                    GetWindowPlacement(ctx.hWnd, &ctx.windowPlacement);
                }
                pApp->WriteSettings(ctx.settingsPath, ctx.windowPlacement, ctx.startFullScreen, ctx.enforceSingleInstance, ctx.alwaysOnTop);

                SetDlgItemTextW(hDlg, IDOK, Tr(StrId::BtnApplied));
                SetTimer(hDlg, KEYBINDING_TIMER_ID, 1500, nullptr);
            }
            return (INT_PTR)TRUE;
        }
        case IDC_BTN_RESTORE_DEFAULTS: {
            // 二次确认：询问是否恢复所有快捷键为默认
            int ret = MessageBoxW(hDlg, Tr(StrId::KbRestoreConfirmMsg), Tr(StrId::KbRestoreConfirmTitle), MB_YESNO | MB_ICONQUESTION);
            if (ret == IDYES) {
                pApp->ResetHotkeysToDefault();

                // Auto-save keybinding
                if (!ctx.isFullScreen) {
                    ctx.windowPlacement.length = sizeof(WINDOWPLACEMENT);
                    GetWindowPlacement(ctx.hWnd, &ctx.windowPlacement);
                }
                pApp->WriteSettings(ctx.settingsPath, ctx.windowPlacement, ctx.startFullScreen, ctx.enforceSingleInstance, ctx.alwaysOnTop);

                // 刷新当前选中动作的热键显示
                int idx = static_cast<int>(SendMessageW(GetDlgItem(hDlg, IDC_COMBO_ACTION), CB_GETCURSEL, 0, 0));
                if (idx != CB_ERR) {
                    SendMessageW(GetDlgItem(hDlg, IDC_HOTKEY_CTRL), HKM_SETHOTKEY, ctx.hotkeys[idx], 0);
                }
            }
            return (INT_PTR)TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    case WM_TIMER:
        if (wParam == KEYBINDING_TIMER_ID) {
            KillTimer(hDlg, KEYBINDING_TIMER_ID);
            SetDlgItemTextW(hDlg, IDOK, Tr(StrId::BtnApply));
        }
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

void ViewerApp::OpenKeybindingsDialog() {
    DialogBoxParam(m_ctx.hInst, MAKEINTRESOURCE(IDD_KEYBINDINGS_DIALOG), m_ctx.hWnd, KeybindingsDialogProc, (LPARAM)this);
}

INT_PTR CALLBACK ViewerApp::ZoomDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    ViewerApp* pApp = GetAppFromDialog<ViewerApp>(hDlg, message, lParam);
    if (!pApp) return (INT_PTR)FALSE;

    if (message == WM_INITDIALOG) {
        SetWindowTextW(hDlg, Tr(StrId::DlgZoomTitle));
        SetDlgItemTextW(hDlg, IDC_STATIC_ZOOM_LABEL, Tr(StrId::ZoomLabel));
        SetDlgItemTextW(hDlg, IDOK, Tr(StrId::BtnOk));
        SetDlgItemTextW(hDlg, IDCANCEL, Tr(StrId::BtnCancel));
        // Default the text box to the current zoom level
        SetDlgItemInt(hDlg, IDC_EDIT_ZOOM, static_cast<UINT>(pApp->GetContext().zoomFactor * 100.0f + 0.5f), FALSE);
        return (INT_PTR)TRUE;
    }

    switch (message) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            BOOL success = FALSE;
            UINT val = GetDlgItemInt(hDlg, IDC_EDIT_ZOOM, &success, FALSE);
            if (success && val > 0) {
                pApp->SetZoomLevel(val / 100.0f);
                EndDialog(hDlg, IDOK);
            }
            else {
                MessageBoxW(hDlg, Tr(StrId::ZoomInvalidMsg), Tr(StrId::InvalidInputCaption), MB_ICONERROR);
            }
            return (INT_PTR)TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void ViewerApp::OpenZoomDialog() {
    DialogBoxParam(m_ctx.hInst, MAKEINTRESOURCE(IDD_ZOOM_DIALOG), m_ctx.hWnd, ZoomDialogProc, (LPARAM)this);
}