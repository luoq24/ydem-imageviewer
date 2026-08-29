#include "viewer.h"
#include <format>

void ViewerApp::OpenFileAction() {
    wchar_t szFile[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = m_ctx.hWnd;
    ofn.lpstrFilter = Tr(StrId::FilterOpen);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (GetOpenFileNameW(&ofn)) {
        LoadImageFromFile(szFile);
    }
}

void ViewerApp::DeleteCurrentImage() {
    if (m_ctx.currentImageIndex < 0 || m_ctx.imageFiles.empty()) return;

    if (m_ctx.askToDelete) {
        if (MessageBoxW(m_ctx.hWnd, Tr(StrId::DeleteConfirmMsg), Tr(StrId::DeleteConfirmTitle), MB_YESNO | MB_ICONWARNING) != IDYES) {
            return;
        }
    }

    std::wstring filePath = m_ctx.imageFiles[m_ctx.currentImageIndex];

    ComPtr<IFileOperation> fileOp;
    HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fileOp));

    if (SUCCEEDED(hr)) {
        // Send to recycle bin with no prompt
        hr = fileOp->SetOperationFlags(FOF_ALLOWUNDO | FOF_NOCONFIRMATION);
        if (SUCCEEDED(hr)) {
            ComPtr<IShellItem> itemToDelete;
            hr = SHCreateItemFromParsingName(filePath.c_str(), nullptr, IID_PPV_ARGS(&itemToDelete));

            if (SUCCEEDED(hr)) {
                hr = fileOp->DeleteItem(itemToDelete.Get(), nullptr);

                if (SUCCEEDED(hr)) {
                    hr = fileOp->PerformOperations();

                    if (SUCCEEDED(hr)) {
                        BOOL aborted = FALSE;
                        fileOp->GetAnyOperationsAborted(&aborted);

                        if (!aborted) {
                            m_ctx.imageFiles.erase(m_ctx.imageFiles.begin() + m_ctx.currentImageIndex);

                            if (m_ctx.imageFiles.empty()) {
                                m_ctx.currentImageIndex = -1;
                                {
                                    std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
                                    m_ctx.wicConverter = nullptr;
                                    m_ctx.wicConverterOriginal = nullptr;
                                    m_ctx.undoStack.clear();
                                    m_ctx.d2dBitmap = nullptr;
                                    { std::scoped_lock plk(m_ctx.pathMutex); m_ctx.loadingFilePath = L""; }
                                }
                                InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
                                SetWindowTextW(m_ctx.hWnd, AppNameAndVersion());
                            }
                            else {
                                if (m_ctx.currentImageIndex >= static_cast<int>(m_ctx.imageFiles.size())) {
                                    m_ctx.currentImageIndex = 0;
                                }
                                LoadImageFromFile(m_ctx.imageFiles[m_ctx.currentImageIndex]);
                            }
                        }
                    }
                }
            }
        }
    }
}

void ViewerApp::HandleDropFiles(HDROP hDrop) {
    UINT charsRequired = DragQueryFileW(hDrop, 0, nullptr, 0);

    if (charsRequired > 0) {
        std::wstring filePath(charsRequired + 1, L'\0');
        if (DragQueryFileW(hDrop, 0, filePath.data(), charsRequired + 1)) {
            filePath.resize(charsRequired); 
            LoadImageFromFile(filePath);
        }
    }
    DragFinish(hDrop);
}

void ViewerApp::HandleCopy() {
    if (OpenClipboard(m_ctx.hWnd)) {
        EmptyClipboard();
        // Copy as CF_HDROP (File Path Only)
        if (!m_ctx.loadingFilePath.empty() && m_ctx.loadingFilePath != L"Clipboard Image") {
            size_t size = (m_ctx.loadingFilePath.length() + 1) * sizeof(wchar_t);
            HGLOBAL hMemDrop = GlobalAlloc(GMEM_MOVEABLE, sizeof(DROPFILES) + size + sizeof(wchar_t));
            if (hMemDrop) {
                BYTE* pData = static_cast<BYTE*>(GlobalLock(hMemDrop));
                if (pData) {
                    DROPFILES* pDrop = reinterpret_cast<DROPFILES*>(pData);
                    pDrop->pFiles = sizeof(DROPFILES);
                    pDrop->pt = { 0, 0 };
                    pDrop->fNC = FALSE;
                    pDrop->fWide = TRUE;
                    wchar_t* pPath = reinterpret_cast<wchar_t*>(pData + sizeof(DROPFILES));
                    wcscpy_s(pPath, m_ctx.loadingFilePath.length() + 1, m_ctx.loadingFilePath.c_str());

                    GlobalUnlock(hMemDrop);
                    SetClipboardData(CF_HDROP, hMemDrop);
                }
                else {
                    GlobalFree(hMemDrop);
                }
            }
        }
        CloseClipboard();
    }
}

void ViewerApp::HandleCopyPath() {
    std::wstring path = m_ctx.loadingFilePath;
    if (path.empty() || path == L"Clipboard Image") return;

    if (OpenClipboard(m_ctx.hWnd)) {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (path.length() + 1) * sizeof(wchar_t));
        if (hMem) {
            wchar_t* pData = static_cast<wchar_t*>(GlobalLock(hMem));
            if (pData) {
                wcscpy_s(pData, path.length() + 1, path.c_str());
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
            else {
                GlobalFree(hMem);
            }
        }
        CloseClipboard();
    }
}

bool ViewerApp::SendLocalTaskToZiyu(const wchar_t* taskKey) {
    std::wstring path = m_ctx.loadingFilePath;
    if (path.empty() || path == L"Clipboard Image") return false;

    // 组装 JSON：{"local_task_key":[taskKey,0],"source_path":"<路径>"}
    // 接收端 LocalPipeReceiver 以 task_key, _ = local_task_key 解包二元数组，
    // 首元素为注册的字符串键（WidgetZiyuImageEdit 监听），
    // 收到后直接引用该路径执行对应任务（不移动、不复制原文件）
    std::wstring escaped;
    escaped.reserve(path.size() + 8);
    for (wchar_t ch : path) {
        if (ch == L'\\' || ch == L'"') escaped += L'\\';
        escaped += ch;
    }
    std::wstring json = L"{\"local_task_key\":[\"" + std::wstring(taskKey) + L"\",0],\"source_path\":\"" + escaped + L"\"}";

    // 转 UTF-8（自娱工具按 UTF-8 解码）
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, json.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 0) return false;
    std::string utf8(utf8Len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, json.c_str(), -1, utf8.data(), utf8Len, nullptr, nullptr);
    utf8.resize(utf8Len - 1); // 去掉结尾的 '\0'

    // 连接“自娱工具”的本地管道服务器（QLocalServer 对应命名管道）
    const wchar_t* pipeName = L"\\\\.\\pipe\\rh_local_ziyu";
    if (!WaitNamedPipeW(pipeName, 1000)) {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::ErrZiyuNotRunning), Tr(StrId::ErrCaption), MB_ICONWARNING);
        return false;
    }

    HANDLE hPipe = CreateFileW(pipeName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr, OPEN_EXISTING, 0, nullptr);
    if (hPipe == INVALID_HANDLE_VALUE) {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::ErrZiyuSendFailed), Tr(StrId::ErrCaption), MB_ICONERROR);
        return false;
    }

    DWORD written = 0;
    WriteFile(hPipe, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    FlushFileBuffers(hPipe);
    CloseHandle(hPipe);
    return true;
}

void ViewerApp::SendToZiyuEdit() {
    SendLocalTaskToZiyu(L"ziyu_edit");
}

void ViewerApp::SendToZiyuEditLineart() {
    // 触发“自娱工具”中的 lineart 预处理（ComfyUI Standard Lineart → 发 PS 图层）
    SendLocalTaskToZiyu(L"lineart");
}

void ViewerApp::HandlePaste() {
    if (OpenClipboard(m_ctx.hWnd)) {
        if (IsClipboardFormatAvailable(CF_HDROP)) {
            HANDLE hData = GetClipboardData(CF_HDROP);
            if (hData) {
                HDROP hDrop = static_cast<HDROP>(hData);
                UINT charsRequired = DragQueryFileW(hDrop, 0, nullptr, 0);
                if (charsRequired > 0) {
                    std::wstring filePath(charsRequired + 1, L'\0');
                    if (DragQueryFileW(hDrop, 0, filePath.data(), charsRequired + 1)) {
                        filePath.resize(charsRequired);
                        LoadImageFromFile(filePath);
                    }
                }
            }
        }
        else if (IsClipboardFormatAvailable(CF_BITMAP) || IsClipboardFormatAvailable(CF_DIB)) {
            HBITMAP hBitmap = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
            if (hBitmap) {
               std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
                ComPtr<IWICBitmap> wicBitmap;
                HRESULT hr = m_ctx.wicFactory->CreateBitmapFromHBITMAP(hBitmap, NULL, WICBitmapUseAlpha, &wicBitmap);

                if (SUCCEEDED(hr)) {
                    if (ComPtr<IWICFormatConverter> converter = ConvertToFormat(m_ctx.wicFactory.Get(), wicBitmap.Get())) {
                        // reset state for new pasted image
                        m_ctx.wicConverter = converter;
                        m_ctx.wicConverterOriginal = converter;
                        m_ctx.d2dBitmap = nullptr;
                        m_ctx.animationFrameMetadata.clear();
                        m_ctx.animationFrameDelays.clear();
                        m_ctx.isAnimated = false;
                        // clear file context
                        m_ctx.imageFiles.clear();
                        m_ctx.currentImageIndex = -1;
                        m_ctx.currentDirectory = L"";
                        { std::scoped_lock plk(m_ctx.pathMutex); m_ctx.loadingFilePath = L"Clipboard Image"; }
                        m_ctx.originalContainerFormat = GUID_ContainerFormatPng;
                        m_ctx.isOsdCacheValid = false;

                        m_ctx.zoomFactor = 1.0f;
                        m_ctx.offsetX = 0;
                        m_ctx.offsetY = 0;

                        // stop animations
                        KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
                        LPCWSTR appTitle = AppNameAndVersion();
                        std::wstring clipTitle = std::vformat(Tr(StrId::TitleClipboardFormat), std::make_wformat_args(appTitle));
                        SetWindowTextW(m_ctx.hWnd, clipTitle.c_str());
                        InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
                    }
                }
            }
        } 

        CloseClipboard();
    }
}

void ViewerApp::OpenFileLocationAction() {
    if (m_ctx.loadingFilePath.empty()) return;
    PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(m_ctx.loadingFilePath.c_str());
    if (pidl) {
        SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
        ILFree(pidl);
    }
}

// 核心功能1：根据当前图片的长宽比，将窗口移动到合适的显示器上
// （典型场景：1 个横屏显示器 + 1 个竖屏显示器）
void ViewerApp::ApplyMonitorPlacement() {
    if (!m_ctx.autoMonitorPlacement) return;
    // 最小化时不移动（最大化/全屏会特殊处理，见下）
    if (IsIconic(m_ctx.hWnd)) return;

    UINT imgW = m_ctx.originalWidth;
    UINT imgH = m_ctx.originalHeight;
    if (imgW == 0 || imgH == 0) return;

    // 考虑 EXIF 自动旋转：90°/270° 时横竖互换
    if (m_ctx.rotationAngle == 90 || m_ctx.rotationAngle == 270) {
        std::swap(imgW, imgH);
    }

    enum class Orient { Landscape, Portrait, Square };
    auto orientOfSize = [](int w, int h) {
        return (w > h) ? Orient::Landscape : (h > w) ? Orient::Portrait : Orient::Square;
    };
    Orient imgOrient = orientOfSize(static_cast<int>(imgW), static_cast<int>(imgH));
    if (imgOrient == Orient::Square) return; // 正方形图片不切换

    // 收集所有显示器（虚拟屏幕坐标）
    struct MonRect { RECT rc; };
    std::vector<MonRect> monitors;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR, HDC, LPRECT rc, LPARAM lParam) -> BOOL {
        auto* vec = reinterpret_cast<std::vector<MonRect>*>(lParam);
        vec->push_back(MonRect{ *rc });
        return TRUE;
        }, reinterpret_cast<LPARAM>(&monitors));
    if (monitors.empty()) return;

    auto orientOfRect = [&](const RECT& rc) {
        return orientOfSize(rc.right - rc.left, rc.bottom - rc.top);
    };

    RECT winRect;
    GetWindowRect(m_ctx.hWnd, &winRect);
    int winW = winRect.right - winRect.left;
    int winH = winRect.bottom - winRect.top;

    // 若当前所在显示器方向已匹配，则不移动
    HMONITOR curMon = MonitorFromWindow(m_ctx.hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{ sizeof(mi) };
    if (!GetMonitorInfoW(curMon, &mi)) return; // 无法确定当前显示器，保持原位
    if (orientOfRect(mi.rcMonitor) == imgOrient) return;

    // 在方向匹配的显示器中，选择距当前窗口中心最近的一块
    POINT winCenter = { winRect.left + winW / 2, winRect.top + winH / 2 };
    const RECT* best = nullptr;
    int bestDist = INT_MAX;
    for (const auto& m : monitors) {
        if (orientOfRect(m.rc) != imgOrient) continue;
        int cx = m.rc.left + (m.rc.right - m.rc.left) / 2;
        int cy = m.rc.top + (m.rc.bottom - m.rc.top) / 2;
        int dx = cx - winCenter.x;
        int dy = cy - winCenter.y;
        int dist = dx * dx + dy * dy;
        if (dist < bestDist) {
            bestDist = dist;
            best = &m.rc;
        }
    }
    if (!best) return; // 没有匹配方向的显示器，保持原位

    if (m_ctx.isFullScreen) {
        // 全屏状态：保持全屏，直接铺满目标显示器
        m_ctx.suppressDpiResize = true;
        SetWindowPos(m_ctx.hWnd, nullptr,
            best->left, best->top,
            best->right - best->left, best->bottom - best->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        m_ctx.suppressDpiResize = false;
        return;
    }

    // 将矩形从当前显示器“等比映射”到目标显示器：
    // 中心点相对当前显示器的横向/纵向百分比（m%、n%）保持不变；
    // 宽/高相对当前显示器的百分比保持不变。
    auto remapTo = [&](const RECT& src, const RECT& dst) {
        int srcW = src.right - src.left;
        int srcH = src.bottom - src.top;
        int curMonW = mi.rcMonitor.right - mi.rcMonitor.left;
        int curMonH = mi.rcMonitor.bottom - mi.rcMonitor.top;
        int dstMonW = dst.right - dst.left;
        int dstMonH = dst.bottom - dst.top;

        double centerRatioX = curMonW > 0 ? (double)((src.left + src.right) / 2 - mi.rcMonitor.left) / curMonW : 0.0;
        double centerRatioY = curMonH > 0 ? (double)((src.top + src.bottom) / 2 - mi.rcMonitor.top) / curMonH : 0.0;
        double sizeRatioW = curMonW > 0 ? (double)srcW / curMonW : 1.0;
        double sizeRatioH = curMonH > 0 ? (double)srcH / curMonH : 1.0;

        int newW = std::max(1, (int)std::lround(sizeRatioW * dstMonW));
        int newH = std::max(1, (int)std::lround(sizeRatioH * dstMonH));
        int newCX = dst.left + (int)std::lround(centerRatioX * dstMonW);
        int newCY = dst.top + (int)std::lround(centerRatioY * dstMonH);

        RECT out;
        out.left = newCX - newW / 2;
        out.top = newCY - newH / 2;
        out.right = out.left + newW;
        out.bottom = out.top + newH;
        return out;
    };

    if (IsZoomed(m_ctx.hWnd)) {
        // 最大化状态：先把“还原位置”按比例映射到目标显示器，还原后再重新最大化。
        // 注意：对已最大化的窗口直接 SetWindowPlacement(SW_SHOWMAXIMIZED) 不会重新定位，
        // 必须先还原到新位置，再重新最大化。
        WINDOWPLACEMENT wp{ sizeof(WINDOWPLACEMENT) };
        GetWindowPlacement(m_ctx.hWnd, &wp);
        wp.rcNormalPosition = remapTo(wp.rcNormalPosition, *best);
        wp.showCmd = SW_RESTORE;
        m_ctx.suppressDpiResize = true;
        SetWindowPlacement(m_ctx.hWnd, &wp);
        ShowWindow(m_ctx.hWnd, SW_MAXIMIZE);
        m_ctx.suppressDpiResize = false;
        return;
    }

    // 窗口化状态：按比例映射位置与大小
    RECT mapped = remapTo(winRect, *best);
    m_ctx.suppressDpiResize = true;
    SetWindowPos(m_ctx.hWnd, nullptr,
        mapped.left, mapped.top,
        mapped.right - mapped.left, mapped.bottom - mapped.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
    m_ctx.suppressDpiResize = false;
}

// ===== 路径查询服务（供自用软件查询当前图片路径） =====

std::wstring ViewerApp::GetCurrentImagePath() {
    std::scoped_lock lock(m_ctx.pathMutex);
    return m_ctx.loadingFilePath;
}

void ViewerApp::StartPathQueryServer() {
    if (m_pathServerRunning.exchange(true)) return;
    m_pathServerThread = std::thread([this]() {
        const wchar_t* pipeName = L"\\\\.\\pipe\\rh_local_imageviewer";
        while (m_pathServerRunning.load()) {
            HANDLE hPipe = CreateNamedPipeW(pipeName,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1, 65536, 65536, 0, nullptr);
            if (hPipe == INVALID_HANDLE_VALUE) { Sleep(200); continue; }

            // 阻塞等待客户端连接（StopPathQueryServer 用哑连接唤醒）
            ConnectNamedPipe(hPipe, nullptr);
            if (!m_pathServerRunning.load()) { CloseHandle(hPipe); break; }

            // 读取请求（客户端发来的 JSON，内容不严格校验）
            char buf[8192] = {};
            DWORD got = 0;
            ReadFile(hPipe, buf, sizeof(buf), &got, nullptr);

            // 响应：当前完整路径（UTF-8）
            std::wstring path = GetCurrentImagePath();
            std::wstring esc; esc.reserve(path.size() + 8);
            for (wchar_t ch : path) {
                if (ch == L'\\' || ch == L'"') esc += L'\\';
                esc += ch;
            }
            std::wstring wj = L"{\"local_task_key\":[\"current_path\",0],\"current_path\":\"" + esc + L"\"}";

            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wj.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string json;
            if (utf8Len > 0) {
                json.resize(utf8Len - 1);
                WideCharToMultiByte(CP_UTF8, 0, wj.c_str(), -1, json.data(), utf8Len, nullptr, nullptr);
            }

            DWORD wr = 0;
            WriteFile(hPipe, json.data(), static_cast<DWORD>(json.size()), &wr, nullptr);
            FlushFileBuffers(hPipe);
            CloseHandle(hPipe);
        }
    });
}

void ViewerApp::StopPathQueryServer() {
    if (!m_pathServerRunning.exchange(false)) return;
    // 用哑连接唤醒可能阻塞在 ConnectNamedPipe 的服务线程
    HANDLE h = CreateFileW(L"\\\\.\\pipe\\rh_local_imageviewer",
                           GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    if (m_pathServerThread.joinable()) m_pathServerThread.join();
}