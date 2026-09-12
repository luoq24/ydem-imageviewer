#include "viewer.h"
#include <string>

// 默认快捷键参照 ACDSee 查看模式设置（见 https://help.acdsee.cn/acdsee-home-2023/.../Viewer_keyboard_shortcuts.htm）
namespace {
    const WORD g_defaultKeys[Act_Count] = {
        // 浏览/缩放/全屏/旋转（ACDSee：→← 翻页，Home/End 首/尾，+ - 缩放，* 适应窗口，/ 实际大小，F 全屏，Ctrl+Shift+→/← 旋转）
        MAKEWORD(VK_RIGHT, HOTKEYF_EXT), MAKEWORD(VK_LEFT, HOTKEYF_EXT), MAKEWORD(VK_HOME, HOTKEYF_EXT), MAKEWORD(VK_END, HOTKEYF_EXT),
        VK_ADD, VK_SUBTRACT, VK_MULTIPLY, VK_DIVIDE, 'F',
        MAKEWORD(VK_RIGHT, HOTKEYF_CONTROL | HOTKEYF_SHIFT | HOTKEYF_EXT), MAKEWORD(VK_LEFT, HOTKEYF_CONTROL | HOTKEYF_SHIFT | HOTKEYF_EXT),
        0, 'C', 'Z', VK_ESCAPE,
        // 文件/编辑（ACDSee：Ctrl+O 打开，F5 刷新，Ctrl+C/V 复制粘贴，Ctrl+S 保存，Delete 删除，Ctrl+Z 撤销）
        MAKEWORD('O', HOTKEYF_CONTROL), VK_F5, MAKEWORD('C', HOTKEYF_CONTROL), MAKEWORD('V', HOTKEYF_CONTROL),
        MAKEWORD('S', HOTKEYF_CONTROL), MAKEWORD('S', HOTKEYF_CONTROL | HOTKEYF_SHIFT), MAKEWORD(VK_DELETE, HOTKEYF_EXT),
        MAKEWORD('Z', HOTKEYF_CONTROL), 0, VK_RETURN, 'I', VK_SPACE, MAKEWORD(VK_SPACE, HOTKEYF_SHIFT),
        // 动画/多页导航（ACDSee：Shift+PageDown/PageUp/Home），幻灯片 Alt+S
        MAKEWORD(VK_NEXT, HOTKEYF_SHIFT | HOTKEYF_EXT), MAKEWORD(VK_PRIOR, HOTKEYF_SHIFT | HOTKEYF_EXT),
        MAKEWORD(VK_HOME, HOTKEYF_SHIFT | HOTKEYF_EXT), MAKEWORD(VK_F10, HOTKEYF_SHIFT), MAKEWORD('S', HOTKEYF_ALT)
    };
}

// 将内存中的快捷键全部恢复为默认值，并重建加速键表
void ViewerApp::ResetHotkeysToDefault() {
    for (int i = 0; i < Act_Count; ++i) {
        m_ctx.hotkeys[i] = g_defaultKeys[i];
    }
    UpdateAcceleratorTable();
}

void ViewerApp::ReadSettings(const std::wstring& path, WINDOWPLACEMENT& wp, bool& fullscreen, bool& singleInstance, bool& alwaysOnTop) {
    auto getInt = [&](LPCWSTR sec, LPCWSTR key, int def) { return GetPrivateProfileIntW(sec, key, def, path.c_str());
        };

    // 语言：0 = 中文（默认），1 = 英文
    int langChoice = getInt(L"Settings", L"Language", 0);
    I18n::SetLanguage(langChoice == 1 ? AppLanguage::English : AppLanguage::Chinese);

    fullscreen = getInt(L"Settings", L"StartFullScreen", 0) == 1;
    singleInstance = getInt(L"Settings", L"EnforceSingleInstance", 1) == 1;
    m_ctx.alwaysOnTop = getInt(L"Settings", L"AlwaysOnTop", 0) == 1;
    m_ctx.autoMonitorPlacement = getInt(L"Settings", L"AutoMonitorPlacement", 0) == 1;
    m_ctx.smoothScaling = getInt(L"Settings", L"SmoothScaling", 1) == 1;
    m_ctx.enableFadeAnimation = getInt(L"Settings", L"EnableFadeAnimation", 1) == 1;
    m_ctx.isOsdVisible = getInt(L"Settings", L"ShowOSD", 0) == 1;
    m_ctx.askToDelete = getInt(L"Settings", L"AskToDelete", 1) == 1;
    m_ctx.preserveZoomOnResize = getInt(L"Settings", L"PreserveZoomOnResize", 0) == 1;
    m_ctx.isAutoRefresh = getInt(L"Settings", L"AutoRefresh", 0) == 1;
    m_ctx.slideshowIntervalSeconds = getInt(L"Settings", L"SlideshowInterval", 3);

    int bgChoice = getInt(L"Settings", L"BackgroundColor", 0);
    m_ctx.bgColor = static_cast<BackgroundColor>((bgChoice < 0 || bgChoice > 3) ? 0 : bgChoice);

    int zoomChoice = getInt(L"Settings", L"DefaultZoomMode", 0);
    m_ctx.defaultZoomMode = static_cast<DefaultZoomMode>((zoomChoice < 0 || zoomChoice > 1) ? 0 : zoomChoice);

    int sortChoice = getInt(L"Settings", L"SortCriteria", 0);
    m_ctx.currentSortCriteria = static_cast<SortCriteria>((sortChoice < 0 || sortChoice > 2) ? 0 : sortChoice);

    m_ctx.isSortAscending = getInt(L"Settings", L"SortAscending", 1) == 1;
    m_ctx.listLoopEnabled = getInt(L"Settings", L"ListLoop", 0) == 1;
    wp.length = sizeof(WINDOWPLACEMENT);
    wp.rcNormalPosition.left = CW_USEDEFAULT;
    wp.showCmd = SW_SHOWNORMAL;

    // Setup default before attempting read
    GetPrivateProfileStructW(L"Window", L"Placement", &wp, sizeof(WINDOWPLACEMENT), path.c_str());
    const wchar_t* keyNames[Act_Count] = {
        L"Next", L"Prev", L"FirstImage", L"LastImage", L"ZoomIn", L"ZoomOut", L"Fit", L"Actual", L"Fullscreen", L"RotateCW", L"RotateCCW", L"Flip", L"Crop", L"CustomZoom", L"Exit",
        L"Open", L"Refresh", L"Copy", L"Paste", L"Save", L"SaveAs", L"Delete", L"Undo", L"CenterImage", L"CommitCrop", L"ToggleOSD", L"PlayPause", L"ResumeAnim",
        L"AnimNext", L"AnimPrev", L"AnimFirst", L"ContextMenu", L"Slideshow"
    };
    for (int i = 0; i < Act_Count; ++i) {
        m_ctx.hotkeys[i] = (WORD)getInt(L"Keys", keyNames[i], g_defaultKeys[i]);
        BYTE vk = LOBYTE(m_ctx.hotkeys[i]);
        if (vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN ||
            vk == VK_DELETE || vk == VK_INSERT || vk == VK_HOME || vk == VK_END ||
            vk == VK_PRIOR || vk == VK_NEXT || vk == VK_DIVIDE) {
            BYTE mods = HIBYTE(m_ctx.hotkeys[i]);
            if (!(mods & HOTKEYF_EXT)) {
                m_ctx.hotkeys[i] = MAKEWORD(vk, mods | HOTKEYF_EXT);
            }
        }
    }
}

void ViewerApp::WriteSettings(const std::wstring& path, const WINDOWPLACEMENT& wp, bool fullscreen, bool singleInstance, bool alwaysOnTop) {
    auto writeInt = [&](LPCWSTR section, LPCWSTR key, int val) {
        WritePrivateProfileStringW(section, key, std::to_wstring(val).c_str(), path.c_str());
        };

    writeInt(L"Settings", L"StartFullScreen", fullscreen ? 1 : 0);
    writeInt(L"Settings", L"Language", I18n::GetLanguage() == AppLanguage::English ? 1 : 0);
    writeInt(L"Settings", L"EnforceSingleInstance", singleInstance ? 1 : 0);
    writeInt(L"Settings", L"AlwaysOnTop", alwaysOnTop ? 1 : 0);
    writeInt(L"Settings", L"AutoMonitorPlacement", m_ctx.autoMonitorPlacement ? 1 : 0);
    writeInt(L"Settings", L"SmoothScaling", m_ctx.smoothScaling ? 1 : 0);
    writeInt(L"Settings", L"EnableFadeAnimation", m_ctx.enableFadeAnimation ? 1 : 0);
    writeInt(L"Settings", L"ShowOSD", m_ctx.isOsdVisible ? 1 : 0);
    writeInt(L"Settings", L"AskToDelete", m_ctx.askToDelete ? 1 : 0);
    writeInt(L"Settings", L"PreserveZoomOnResize", m_ctx.preserveZoomOnResize ? 1 : 0);
    writeInt(L"Settings", L"AutoRefresh", m_ctx.isAutoRefresh ? 1 : 0);
    writeInt(L"Settings", L"SlideshowInterval", m_ctx.slideshowIntervalSeconds);
    writeInt(L"Settings", L"BackgroundColor", static_cast<int>(m_ctx.bgColor));
    writeInt(L"Settings", L"DefaultZoomMode", static_cast<int>(m_ctx.defaultZoomMode));
    writeInt(L"Settings", L"SortCriteria", static_cast<int>(m_ctx.currentSortCriteria));
    writeInt(L"Settings", L"SortAscending", m_ctx.isSortAscending ? 1 : 0);
    writeInt(L"Settings", L"ListLoop", m_ctx.listLoopEnabled ? 1 : 0);

    const wchar_t* keyNames[Act_Count] = {
        L"Next", L"Prev", L"FirstImage", L"LastImage", L"ZoomIn", L"ZoomOut", L"Fit", L"Actual", L"Fullscreen", L"RotateCW", L"RotateCCW", L"Flip", L"Crop", L"CustomZoom", L"Exit",
        L"Open", L"Refresh", L"Copy", L"Paste", L"Save", L"SaveAs", L"Delete", L"Undo", L"CenterImage", L"CommitCrop", L"ToggleOSD", L"PlayPause", L"ResumeAnim",
        L"AnimNext", L"AnimPrev", L"AnimFirst", L"ContextMenu", L"Slideshow"
    };
    for (int i = 0; i < Act_Count; ++i) {
        writeInt(L"Keys", keyNames[i], m_ctx.hotkeys[i]);
    }

    if (!IsIconic(m_ctx.hWnd) && wp.rcNormalPosition.left != CW_USEDEFAULT) {
        WritePrivateProfileStructW(L"Window", L"Placement", const_cast<WINDOWPLACEMENT*>(&wp), sizeof(WINDOWPLACEMENT), path.c_str());
    }

    // Force flush INI cache 
    WritePrivateProfileStringW(NULL, NULL, NULL, path.c_str());
}

void ViewerApp::UpdateAcceleratorTable() {
    m_ctx.hAccelTable.reset();

    std::vector<ACCEL> accels;
    auto addAccel = [&](WORD virtKey, BYTE modifiers, WORD cmd) {
        ACCEL a = {};
        a.cmd = cmd;
        a.key = virtKey;
        a.fVirt = FVIRTKEY;
        if (modifiers & HOTKEYF_CONTROL) a.fVirt |= FCONTROL;
        if (modifiers & HOTKEYF_SHIFT) a.fVirt |= FSHIFT;
        if (modifiers & HOTKEYF_ALT) a.fVirt |= FALT;
        accels.push_back(a);
        };

    // Map all configurable actions dynamically
    // 已禁用动作映射为 0：复制/粘贴/编辑(旋转/翻转/裁剪)/保存/另存为/删除图片 不注册任何加速键
    WORD actionCmds[Act_Count] = {
        IDM_NEXT_IMG, IDM_PREV_IMG, IDM_FIRST_IMAGE, IDM_LAST_IMAGE, IDM_ZOOM_IN, IDM_ZOOM_OUT, IDM_FIT_TO_WINDOW,
        IDM_ACTUAL_SIZE, IDM_FULLSCREEN, 0, 0, 0,
        0, IDM_CUSTOM_ZOOM, IDM_EXIT,
        IDM_OPEN, IDM_REFRESH, 0, 0, 0, 0, 0, IDM_UNDO,
        IDM_CENTER_IMAGE, IDM_COMMIT_CROP, IDM_TOGGLE_OSD, IDM_PLAY_PAUSE, IDM_RESUME_ANIM,
        IDM_ANIM_NEXT_FRAME, IDM_ANIM_PREV_FRAME, IDM_ANIM_FIRST_FRAME, IDM_CONTEXT_MENU, IDM_SLIDESHOW
    };

    for (int i = 0; i < Act_Count; ++i) {
        if (m_ctx.hotkeys[i] && actionCmds[i] != 0) {
            addAccel(LOBYTE(m_ctx.hotkeys[i]), HIBYTE(m_ctx.hotkeys[i]), actionCmds[i]);
        }
    }

    if (!accels.empty()) {
        m_ctx.hAccelTable.reset(CreateAcceleratorTableW(accels.data(), static_cast<int>(accels.size())));
    }
}