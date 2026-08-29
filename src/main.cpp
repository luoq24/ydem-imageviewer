#include "viewer.h"
#include <cstring>

// define dark mode for older Windows
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

void ViewerApp::CenterImage(bool resetZoom) {
    if (resetZoom) {
        m_ctx.zoomFactor = 1.0f;
    }
    m_ctx.offsetX = 0.0f;
    m_ctx.offsetY = 0.0f;
    InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
}

void ViewerApp::SetActualSize() {
    m_ctx.zoomFactor = 1.0f;
    m_ctx.offsetX = 0.0f;
    m_ctx.offsetY = 0.0f;
    InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
}

void ViewerApp::SetZoomLevel(float zoom) {
    m_ctx.zoomFactor = zoom;
    m_ctx.offsetX = 0.0f;
    m_ctx.offsetY = 0.0f;
    InvalidateRect(m_ctx.hWnd, nullptr, FALSE);
}

void ViewerApp::UpdateTitleBarTheme(HWND hWnd, BackgroundColor bgColor) {
    // black/grey background triggers dark mode
    BOOL useDarkMode = (bgColor == BackgroundColor::Black || bgColor == BackgroundColor::Grey) ? TRUE : FALSE;
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
}

HICON ViewerApp::CreateAppIconFromPng() {
    // The PNG is embedded as an RCDATA resource (rc.exe cannot compile PNG as an ICON resource)
    HRSRC hRes = FindResourceW(m_ctx.hInst, MAKEINTRESOURCE(IDI_APPICON), RT_RCDATA);
    if (!hRes) return nullptr;
    HGLOBAL hGlobal = LoadResource(m_ctx.hInst, hRes);
    if (!hGlobal) return nullptr;
    const BYTE* pData = static_cast<const BYTE*>(LockResource(hGlobal));
    DWORD dataSize = SizeofResource(m_ctx.hInst, hRes);
    if (!pData || dataSize == 0) return nullptr;

    ComPtr<IWICStream> stream;
    if (FAILED(m_ctx.wicFactory->CreateStream(&stream))) return nullptr;
    if (FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(pData), dataSize))) return nullptr;

    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(m_ctx.wicFactory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder))) return nullptr;

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) return nullptr;

    ComPtr<IWICFormatConverter> converter;
    if (FAILED(m_ctx.wicFactory->CreateFormatConverter(&converter))) return nullptr;
    if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom))) return nullptr;

    UINT width = 0, height = 0;
    if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) return nullptr;

    std::vector<BYTE> pixels(static_cast<size_t>(width) * height * 4);
    WICRect rc = { 0, 0, static_cast<INT>(width), static_cast<INT>(height) };
    if (FAILED(converter->CopyPixels(&rc, width * 4, static_cast<UINT>(pixels.size()), pixels.data()))) return nullptr;

    // Build a 32bpp top-down DIB section and copy the BGRA pixels in
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = static_cast<LONG>(width);
    bmi.bmiHeader.biHeight = -static_cast<LONG>(height); // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(nullptr);
    void* pBits = nullptr;
    HBITMAP hbmColor = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    ReleaseDC(nullptr, hdc);
    if (!hbmColor || !pBits) {
        if (hbmColor) DeleteObject(hbmColor);
        return nullptr;
    }
    memcpy(pBits, pixels.data(), pixels.size());

    // All-zero monochrome mask: transparency is driven by the bitmap's alpha channel
    DWORD maskRowBytes = ((width + 31) / 32) * 4;
    std::vector<BYTE> maskBits(maskRowBytes * height, 0);
    HBITMAP hbmMask = CreateBitmap(width, height, 1, 1, maskBits.data());
    if (!hbmMask) {
        DeleteObject(hbmColor);
        return nullptr;
    }

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmColor = hbmColor;
    ii.hbmMask = hbmMask;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hbmColor);
    DeleteObject(hbmMask);
    return hIcon;
}

int ViewerApp::Run(HINSTANCE hInstance, int nCmdShow, LPWSTR lpCmdLine) {
    m_ctx.hInst = hInstance;

    wchar_t exePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);

    std::wstring portableSettingsPath = std::wstring(exePath) + L"\\MIV-settings.ini";
    bool canUsePortable = false;
    // Write-Test: Attempt to open or create the file with write access 
    HANDLE hTest = CreateFileW(portableSettingsPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hTest != INVALID_HANDLE_VALUE) {
        // Success if writable
        CloseHandle(hTest);
        canUsePortable = true;
    }

    // Route based on permissions
    if (canUsePortable) {
        m_ctx.settingsPath = portableSettingsPath;
    }
    else {
        // Fallback if read-only (MSIX App).
        PWSTR localAppDataPath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppDataPath))) {
            std::wstring appDataFolder = std::wstring(localAppDataPath) + L"\\deminimis\\MinimalImageViewer";
            CoTaskMemFree(localAppDataPath);

            // Ensure directory structure exists before writing settings
            SHCreateDirectoryExW(nullptr, appDataFolder.c_str(), nullptr);

            m_ctx.settingsPath = appDataFolder + L"\\MIV-settings.ini";
        }
        else {
            // Absolute failsafe 
            m_ctx.settingsPath = portableSettingsPath;
        }
    }

    ReadSettings(m_ctx.settingsPath, m_ctx.windowPlacement, m_ctx.startFullScreen, m_ctx.enforceSingleInstance, m_ctx.alwaysOnTop);
    float sysDpiScale = GetDpiForSystem() / 96.0f;

    if (m_ctx.enforceSingleInstance) {
        HWND existingWnd = FindWindowW(L"MinimalImageViewer", nullptr);
        if (existingWnd) {
            SetForegroundWindow(existingWnd);
            if (IsIconic(existingWnd)) {
                ShowWindow(existingWnd, SW_RESTORE);
            }
            if (lpCmdLine && *lpCmdLine) {
                COPYDATASTRUCT cds{};
                cds.dwData = 1;
                cds.cbData = (static_cast<DWORD>(wcslen(lpCmdLine)) + 1) * sizeof(wchar_t);
                cds.lpData = lpCmdLine;
                SendMessage(existingWnd, WM_COPYDATA, reinterpret_cast<WPARAM>(hInstance), reinterpret_cast<LPARAM>(&cds));
            }
            return 0;
        }
    }

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        return 1;
    }
    wil::unique_couninitialize_call cleanupCOM;

    timeBeginPeriod(1);

    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_ctx.wicFactory)))) {
        MessageBoxW(nullptr, Tr(StrId::ErrWicFactory), Tr(StrId::ErrCaption), MB_OK | MB_ICONERROR);
        return 1;
    }

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), (void**)&m_ctx.d2dFactory))) {
        MessageBoxW(nullptr, Tr(StrId::ErrD2dFactory), Tr(StrId::ErrCaption), MB_OK | MB_ICONERROR);
        return 1;
    }

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_ctx.writeFactory.GetAddressOf())))) {
        MessageBoxW(nullptr, Tr(StrId::ErrDWriteFactory), Tr(StrId::ErrCaption), MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = ViewerApp::StaticWndProc;
    wcex.hInstance = hInstance;
    m_ctx.appIcon.reset(CreateAppIconFromPng());
    wcex.hIcon = m_ctx.appIcon.get();
    wcex.hIconSm = m_ctx.appIcon.get();
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszClassName = L"MinimalImageViewer";
    RegisterClassExW(&wcex);

    DWORD exStyle = (m_ctx.alwaysOnTop) ? WS_EX_TOPMOST : 0;
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    int w = static_cast<int>(800 * sysDpiScale);
    int h = static_cast<int>(600 * sysDpiScale);

    if (m_ctx.windowPlacement.rcNormalPosition.left != CW_USEDEFAULT) {
        x = m_ctx.windowPlacement.rcNormalPosition.left;
        y = m_ctx.windowPlacement.rcNormalPosition.top;
        w = m_ctx.windowPlacement.rcNormalPosition.right - m_ctx.windowPlacement.rcNormalPosition.left;
        h = m_ctx.windowPlacement.rcNormalPosition.bottom - m_ctx.windowPlacement.rcNormalPosition.top;
    }

    m_ctx.hWnd = CreateWindowExW(
        exStyle,
        wcex.lpszClassName,
        AppNameAndVersion(),
        WS_OVERLAPPEDWINDOW,
        x, y, w, h,
        nullptr, nullptr, hInstance, this
    );
    if (!m_ctx.hWnd) {
        MessageBoxW(nullptr, Tr(StrId::ErrCreateWindow), Tr(StrId::ErrCaption), MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    DragAcceptFiles(m_ctx.hWnd, TRUE);
    UpdateTitleBarTheme(m_ctx.hWnd, m_ctx.bgColor);

    m_ctx.isInitialized = true;

    if (m_ctx.windowPlacement.rcNormalPosition.left != CW_USEDEFAULT) {
        SetWindowPlacement(m_ctx.hWnd, &m_ctx.windowPlacement);
    }

    if (m_ctx.startFullScreen) {
        ToggleFullScreen();
    }
    else if (m_ctx.windowPlacement.rcNormalPosition.left == CW_USEDEFAULT) {
        ShowWindow(m_ctx.hWnd, nCmdShow);
    }

    Render();
    UpdateWindow(m_ctx.hWnd);

    if (lpCmdLine && *lpCmdLine) {
        std::wstring filePath(lpCmdLine);
        if (filePath.length() >= 2 && filePath.front() == L'"' && filePath.back() == L'"') {
            filePath = filePath.substr(1, filePath.length() - 2);
        }
        LoadImageFromFile(filePath);
    }

    InvalidateRect(m_ctx.hWnd, nullptr, FALSE);

    // Flush  working set to drop idle RAM 
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

    UpdateAcceleratorTable();

    StartPathQueryServer();

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!m_ctx.hAccelTable || !TranslateAcceleratorW(m_ctx.hWnd, m_ctx.hAccelTable.get(), &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    timeEndPeriod(1);
    m_ctx.isShuttingDown = true;
    CleanupLoadingThread();
    StopPathQueryServer();

    // Allow up to 1.5 seconds for background threads to cleanly exit
    int timeoutMs = 1500;
    while (m_ctx.activeBackgroundThreads > 0 && timeoutMs > 0) {
        Sleep(10);
        timeoutMs -= 10;
    }

    m_ctx.wicConverter = nullptr;
    m_ctx.wicConverterOriginal = nullptr;
    m_ctx.undoStack.clear();
    m_ctx.d2dBitmap = nullptr;
    m_ctx.animationFrameMetadata.clear();
    m_ctx.animationFrameDelays.clear();
    m_ctx.textBrush = nullptr;
    m_ctx.textFormat = nullptr;
    m_ctx.renderTarget = nullptr;
    m_ctx.writeFactory = nullptr;
    m_ctx.d2dFactory = nullptr;
    m_ctx.wicFactory = nullptr;

    return static_cast<int>(msg.wParam);
    }

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    ViewerApp app;
    return app.Run(hInstance, nCmdShow, lpCmdLine);
}