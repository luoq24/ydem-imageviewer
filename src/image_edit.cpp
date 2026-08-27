#include "viewer.h"
#include <objidl.h>
#include <format>



HRESULT ViewerApp::EncodeAndSaveImage(ComPtr<IWICBitmapSource> source, const std::wstring& filePath, const GUID& containerFormat) {
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> props;

    RETURN_IF_FAILED(m_ctx.wicFactory->CreateStream(&stream));
    RETURN_IF_FAILED(stream->InitializeFromFilename(filePath.c_str(), GENERIC_WRITE));
    RETURN_IF_FAILED(m_ctx.wicFactory->CreateEncoder(containerFormat, nullptr, &encoder));
    RETURN_IF_FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache));
    RETURN_IF_FAILED(encoder->CreateNewFrame(&frame, &props));
    RETURN_IF_FAILED(frame->Initialize(props.Get()));
    RETURN_IF_FAILED(frame->WriteSource(source.Get(), nullptr));
    RETURN_IF_FAILED(frame->Commit());
    return encoder->Commit();
}

void ViewerApp::CommitCrop() {
   std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
    if (!m_ctx.isCropActive || !m_ctx.wicConverterOriginal) {
        m_ctx.isCropActive = false;
        return;
    }

    ComPtr<IWICBitmapSource> source = m_ctx.wicConverterOriginal;

    ComPtr<IWICBitmapClipper> clipper;
    if (SUCCEEDED(m_ctx.wicFactory->CreateBitmapClipper(&clipper))) {
        WICRect rc;
        rc.X = static_cast<INT>(floor(m_ctx.cropRectLocal.left));
        rc.Y = static_cast<INT>(floor(m_ctx.cropRectLocal.top));
        rc.Width = static_cast<INT>(ceil(m_ctx.cropRectLocal.right)) - rc.X;
        rc.Height = static_cast<INT>(ceil(m_ctx.cropRectLocal.bottom)) - rc.Y;
        if (rc.Width > 0 && rc.Height > 0) {
            if (SUCCEEDED(clipper->Initialize(source.Get(), &rc))) {
                if (ComPtr<IWICFormatConverter> converter = ConvertToFormat(m_ctx.wicFactory.Get(), clipper.Get())) {

                    // Limit the undo stack to 10 states to prevent OOM exceptions
                    constexpr size_t MAX_UNDO_STEPS = 10;
                    if (m_ctx.undoStack.size() >= MAX_UNDO_STEPS) {
                        m_ctx.undoStack.erase(m_ctx.undoStack.begin());
                    }

                    m_ctx.undoStack.push_back(m_ctx.wicConverterOriginal);
                    m_ctx.wicConverterOriginal = converter;
                    m_ctx.isDownscaled = false; // Edits destroy high-res alignment

                        if (m_ctx.isAnimated) {
                            m_ctx.isAnimated = false;
                            m_ctx.animationFrameMetadata.clear();
                            m_ctx.animationFrameDelays.clear();
                            KillTimer(m_ctx.hWnd, ANIMATION_TIMER_ID);
                        }
                    }
                
            }
        }
    }
    m_ctx.isCropActive = false;
    m_ctx.cropRectLocal = { 0 };
    m_ctx.isOsdCacheValid = false;
}

void ViewerApp::ApplyEffectsToView() {
    ComPtr<IWICBitmapSource> source;
    {
       std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
        if (!m_ctx.wicConverterOriginal) {
            m_ctx.wicConverter = nullptr;
            return;
        }
        source = m_ctx.wicConverterOriginal;
    }

    // apply crop if active first.
    if (m_ctx.isCropActive) {
        ComPtr<IWICBitmapClipper> clipper;
        if (SUCCEEDED(m_ctx.wicFactory->CreateBitmapClipper(&clipper))) {
            WICRect rc;
            rc.X = static_cast<INT>(floor(m_ctx.cropRectLocal.left));
            rc.Y = static_cast<INT>(floor(m_ctx.cropRectLocal.top));
            rc.Width = static_cast<INT>(ceil(m_ctx.cropRectLocal.right)) - rc.X;
            rc.Height = static_cast<INT>(ceil(m_ctx.cropRectLocal.bottom)) - rc.Y;

            if (rc.Width > 0 && rc.Height > 0) {
                if (SUCCEEDED(clipper->Initialize(source.Get(), &rc))) {
                    source = clipper;
                }
            }
        }
    }
    m_ctx.renderScale = 1.0f;
    if (m_ctx.renderTarget) {
        UINT maxDim = m_ctx.renderTarget->GetMaximumBitmapSize();
        UINT w = 0, h = 0;
        if (SUCCEEDED(source->GetSize(&w, &h)) && (w > maxDim || h > maxDim)) {
            float ratio = std::min(static_cast<float>(maxDim) / w, static_cast<float>(maxDim) / h);
            m_ctx.renderScale = ratio;
            UINT newW = static_cast<UINT>(w * ratio);
            UINT newH = static_cast<UINT>(h * ratio);

            ComPtr<IWICBitmapScaler> scaler;
            if (SUCCEEDED(m_ctx.wicFactory->CreateBitmapScaler(&scaler))) {
                if (SUCCEEDED(scaler->Initialize(source.Get(), newW, newH, WICBitmapInterpolationModeFant))) {
                    source = scaler;
                }
            }
        }
    }

    if (ComPtr<IWICFormatConverter> converter = ConvertToFormat(m_ctx.wicFactory.Get(), source.Get())) {
        std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
        m_ctx.wicConverter = converter;
        m_ctx.d2dBitmap = nullptr;
    }
}

ComPtr<IWICBitmapSource> ViewerApp::ApplyCropAndTransform(ComPtr<IWICBitmapSource> source) {
    if (m_ctx.isCropActive) {
        ComPtr<IWICBitmapClipper> clipper;
        if (SUCCEEDED(m_ctx.wicFactory->CreateBitmapClipper(&clipper))) {
            WICRect rc = {
                static_cast<INT>(floor(m_ctx.cropRectLocal.left)),
                static_cast<INT>(floor(m_ctx.cropRectLocal.top)),
                static_cast<INT>(ceil(m_ctx.cropRectLocal.right)) - static_cast<INT>(floor(m_ctx.cropRectLocal.left)),
                static_cast<INT>(ceil(m_ctx.cropRectLocal.bottom)) - static_cast<INT>(floor(m_ctx.cropRectLocal.top))
            };
            if (rc.Width > 0 && rc.Height > 0 && SUCCEEDED(clipper->Initialize(source.Get(), &rc))) {
                source = clipper;
            }
        }
    }

    if (m_ctx.rotationAngle != 0 || m_ctx.isFlippedHorizontal) {
        ComPtr<IWICBitmapFlipRotator> rotator;
        if (SUCCEEDED(m_ctx.wicFactory->CreateBitmapFlipRotator(&rotator))) {
            WICBitmapTransformOptions options = WICBitmapTransformRotate0;
            switch (m_ctx.rotationAngle) {
            case 90:  options = WICBitmapTransformRotate90; break;
            case 180: options = WICBitmapTransformRotate180; break;
            case 270: options = WICBitmapTransformRotate270; break;
            }
            if (m_ctx.isFlippedHorizontal) {
                options = static_cast<WICBitmapTransformOptions>(options | WICBitmapTransformFlipHorizontal);
            }
            if (SUCCEEDED(rotator->Initialize(source.Get(), options))) {
                source = rotator;
            }
        }
    }
    return source;
}

ComPtr<IWICBitmapSource> ViewerApp::GetSaveSource(const GUID& targetFormat) {
   std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
    ComPtr<IWICBitmapSource> source;

    if (m_ctx.isAnimated && m_ctx.currentAnimationFrame < m_ctx.animationFrameDelays.size()) {
        source = m_ctx.currentAnimatedConverter;
    }
    else if (m_ctx.wicConverterOriginal) {
        source = m_ctx.wicConverterOriginal;
    }
    else {
        return nullptr;
    }

    source = ApplyCropAndTransform(source);

    WICPixelFormatGUID sourcePixelFormat{};
    if (FAILED(source->GetPixelFormat(&sourcePixelFormat))) return nullptr;

    if (targetFormat == GUID_ContainerFormatJpeg && sourcePixelFormat != GUID_WICPixelFormat24bppBGR) {
        if (ComPtr<IWICFormatConverter> converter = ConvertToFormat(m_ctx.wicFactory.Get(), source.Get(), GUID_WICPixelFormat24bppBGR, WICBitmapPaletteTypeCustom)) {
            source = converter;
        }
    }
    return source;
}

void ViewerApp::SaveImageAs() {
    UINT imgWidth, imgHeight;
    if (!GetCurrentImageSize(&imgWidth, &imgHeight)) return;

    wchar_t szFile[MAX_PATH] = { 0 };
    wcscpy_s(szFile, (std::wstring(Tr(StrId::DefaultFileName)) + L".png").c_str());
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = m_ctx.hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = Tr(StrId::FilterSave);
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"png";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileNameW(&ofn)) return;

    GUID containerFormat = GUID_ContainerFormatPng;
    const wchar_t* ext = PathFindExtensionW(ofn.lpstrFile);
    if (ext) {
        if (_wcsicmp(ext, L".jpg") == 0 || _wcsicmp(ext, L".jpeg") == 0) containerFormat = GUID_ContainerFormatJpeg;
        else if (_wcsicmp(ext, L".bmp") == 0) containerFormat = GUID_ContainerFormatBmp;
    }

    ComPtr<IWICBitmapSource> source = GetSaveSource(containerFormat);
    if (!source) {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::SaveErrSource), Tr(StrId::SaveErrCaption), MB_ICONERROR);
        return;
    }

    HRESULT hr = EncodeAndSaveImage(source, ofn.lpstrFile, containerFormat);

    if (SUCCEEDED(hr)) {
        LoadImageFromFile(ofn.lpstrFile);
    }
    else {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::SaveFailed), Tr(StrId::SaveAsErrCaption), MB_ICONERROR);
    }
}

void ViewerApp::SaveImage() {
    if (m_ctx.currentImageIndex < 0 || m_ctx.currentImageIndex >= static_cast<int>(m_ctx.imageFiles.size())) {
        UINT imgWidth, imgHeight;
        if (GetCurrentImageSize(&imgWidth, &imgHeight)) {
            SaveImageAs();
        }
        return;
    }

    const std::wstring& originalPath = m_ctx.imageFiles[m_ctx.currentImageIndex];
    if (m_ctx.rotationAngle == 0 && !m_ctx.isFlippedHorizontal && !m_ctx.isCropActive) {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::SaveNoChanges), Tr(StrId::SaveCaption), MB_OK | MB_ICONINFORMATION);
        return;
    }

    // AVIF/HEIC save prompt
    const wchar_t* ext = PathFindExtensionW(originalPath.c_str());
    if (ext && (_wcsicmp(ext, L".heic") == 0 || _wcsicmp(ext, L".heif") == 0 || _wcsicmp(ext, L".avif") == 0)) {
        if (MessageBoxW(m_ctx.hWnd, Tr(StrId::SaveEditsMsg), Tr(StrId::SaveEditsTitle), MB_YESNO | MB_ICONQUESTION) == IDYES) {

            wchar_t newPath[MAX_PATH];
            wcscpy_s(newPath, MAX_PATH, originalPath.c_str());
            PathRenameExtensionW(newPath, L".png");

            ComPtr<IWICBitmapSource> source = GetSaveSource(GUID_ContainerFormatPng);
            if (source && SUCCEEDED(EncodeAndSaveImage(source, newPath, GUID_ContainerFormatPng))) {
                LoadImageFromFile(newPath);
            }
            else {
                MessageBoxW(m_ctx.hWnd, Tr(StrId::SavePngFailed), Tr(StrId::SaveErrCaption), MB_ICONERROR);
            }
        }
        return;
    }


    GUID containerFormat{};
    {
       std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
        containerFormat = m_ctx.originalContainerFormat;
    }

    if (containerFormat == GUID_NULL) {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::SaveUnknownFormat), Tr(StrId::SaveErrCaption), MB_ICONERROR);
        return;
    }

    ComPtr<IWICBitmapSource> source = GetSaveSource(containerFormat);
    if (!source) {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::SaveErrSource), Tr(StrId::SaveErrCaption), MB_ICONERROR);
        return;
    }

    std::wstring tempPath = originalPath + L".tmp_save";
    HRESULT hr = EncodeAndSaveImage(source, tempPath, containerFormat);

    if (SUCCEEDED(hr)) {
        if (ReplaceFileW(originalPath.c_str(), tempPath.c_str(), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr)) {
            LoadImageFromFile(originalPath.c_str());
        }
        else {
            DeleteFileW(tempPath.c_str());
            MessageBoxW(m_ctx.hWnd, Tr(StrId::SaveReplaceFailed), Tr(StrId::SaveErrCaption), MB_ICONERROR);
        }
    }
    else {
        DeleteFileW(tempPath.c_str());
        MessageBoxW(m_ctx.hWnd, Tr(StrId::SaveTempFailed), Tr(StrId::SaveErrCaption), MB_ICONERROR);
    }
}

void ViewerApp::SaveImageWithResize(const std::wstring& filePath, const GUID& containerFormat, UINT newWidth, UINT newHeight) {
    ComPtr<IWICBitmapSource> source;
    {
       std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
        if (m_ctx.isAnimated && m_ctx.currentAnimationFrame < m_ctx.animationFrameDelays.size()) {
            source = m_ctx.currentAnimatedConverter;
        }
        else if (m_ctx.wicConverterOriginal) {
            source = m_ctx.wicConverterOriginal;
        }
        else {
            MessageBoxW(m_ctx.hWnd, Tr(StrId::ResizeSource), Tr(StrId::ResizeErrCaption), MB_ICONERROR);
            return;
        }
    }

    source = ApplyCropAndTransform(source);

    ComPtr<IWICBitmapScaler> scaler;
    if (SUCCEEDED(m_ctx.wicFactory->CreateBitmapScaler(&scaler))) {
        if (SUCCEEDED(scaler->Initialize(source.Get(), newWidth, newHeight, WICBitmapInterpolationModeFant))) {
            source = scaler;
        }
        else {
            MessageBoxW(m_ctx.hWnd, Tr(StrId::ResizeInitFailed), Tr(StrId::ResizeErrCaption), MB_ICONERROR);
            return;
        }
    }
    else {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::ResizeCreateFailed), Tr(StrId::ResizeErrCaption), MB_ICONERROR);
        return;
    }

    WICPixelFormatGUID sourcePixelFormat{};
    if (SUCCEEDED(source->GetPixelFormat(&sourcePixelFormat))) {
        if (containerFormat == GUID_ContainerFormatJpeg && sourcePixelFormat != GUID_WICPixelFormat24bppBGR) {
            if (ComPtr<IWICFormatConverter> converter = ConvertToFormat(m_ctx.wicFactory.Get(), source.Get(), GUID_WICPixelFormat24bppBGR, WICBitmapPaletteTypeCustom)) {
                source = converter;
            }
        }
    }

    HRESULT hr = EncodeAndSaveImage(source, filePath, containerFormat);

    if (SUCCEEDED(hr)) {
        LoadImageFromFile(filePath.c_str());
    }
    else {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::ResizeSaveFailed), Tr(StrId::ResizeErrCaption), MB_ICONERROR);
    }
}

struct ResizeDialogParams {
    UINT origWidth;
    UINT origHeight;
    UINT newWidth;
    UINT newHeight;
    bool isUpdating;
};

static INT_PTR CALLBACK ResizeDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        SetWindowTextW(hDlg, Tr(StrId::DlgResizeTitle));
        SetDlgItemTextW(hDlg, IDC_STATIC_RESIZE_WIDTH, Tr(StrId::ResizeWidth));
        SetDlgItemTextW(hDlg, IDC_STATIC_RESIZE_HEIGHT, Tr(StrId::ResizeHeight));
        SetDlgItemTextW(hDlg, IDC_CHECK_ASPECT, Tr(StrId::ResizeAspect));
        SetDlgItemTextW(hDlg, IDOK, Tr(StrId::BtnOk));
        SetDlgItemTextW(hDlg, IDCANCEL, Tr(StrId::BtnCancel));
        ResizeDialogParams* pParams = reinterpret_cast<ResizeDialogParams*>(lParam);
        pParams->isUpdating = false;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pParams);
        SetDlgItemInt(hDlg, IDC_EDIT_WIDTH, pParams->origWidth, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_HEIGHT, pParams->origHeight, FALSE);
        CheckDlgButton(hDlg, IDC_CHECK_ASPECT, BST_CHECKED);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
            ResizeDialogParams* pParams = reinterpret_cast<ResizeDialogParams*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
            if (!pParams || pParams->isUpdating) return (INT_PTR)FALSE;
            if (!IsDlgButtonChecked(hDlg, IDC_CHECK_ASPECT)) return (INT_PTR)FALSE;

            pParams->isUpdating = true;
            if (pParams->origWidth > 0 && pParams->origHeight > 0) {
                double aspect = static_cast<double>(pParams->origHeight) / pParams->origWidth;
                int id = LOWORD(wParam);
                BOOL success = FALSE;
                UINT val = GetDlgItemInt(hDlg, id, &success, FALSE);
                if (success) {
                    if (id == IDC_EDIT_WIDTH) {
                        UINT newHeight = static_cast<UINT>(val * aspect + 0.5);
                        SetDlgItemInt(hDlg, IDC_EDIT_HEIGHT, newHeight, FALSE);
                    }
                    else if (id == IDC_EDIT_HEIGHT) {
                        UINT newWidth = static_cast<UINT>(val / aspect + 0.5);
                        SetDlgItemInt(hDlg, IDC_EDIT_WIDTH, newWidth, FALSE);
                    }
                }
            }
            pParams->isUpdating = false;
            return (INT_PTR)TRUE;
        }

        switch (LOWORD(wParam)) {
        case IDOK: {
            ResizeDialogParams* pParams = reinterpret_cast<ResizeDialogParams*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
            if (pParams) {
                BOOL successW, successH;
                pParams->newWidth = GetDlgItemInt(hDlg, IDC_EDIT_WIDTH, &successW, FALSE);
                pParams->newHeight = GetDlgItemInt(hDlg, IDC_EDIT_HEIGHT, &successH, FALSE);
                if (successW && successH && pParams->newWidth > 0 && pParams->newHeight > 0) {
                    EndDialog(hDlg, IDOK);
                }
                else {
                    MessageBoxW(hDlg, Tr(StrId::ResizeInvalidMsg), Tr(StrId::InvalidInputCaption), MB_ICONERROR);
                }
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

void ViewerApp::ResizeImageAction() {
    ResizeDialogParams params = {};
    if (!GetCurrentImageSize(&params.origWidth, &params.origHeight)) {
        MessageBoxW(m_ctx.hWnd, Tr(StrId::ResizeNoImage), Tr(StrId::ResizeErrCaption), MB_ICONERROR);
        return;
    }
    params.newWidth = params.origWidth;
    params.newHeight = params.origHeight;

    if (DialogBoxParam(m_ctx.hInst, MAKEINTRESOURCE(IDD_RESIZE_DIALOG), m_ctx.hWnd, ResizeDialogProc, (LPARAM)&params) == IDOK) {

        wchar_t szFile[MAX_PATH] = { 0 };
        wcscpy_s(szFile, (std::wstring(Tr(StrId::DefaultFileName)) + L".png").c_str());
        const wchar_t* filter = Tr(StrId::FilterSave);
        UINT filterIndex = 1;
        const wchar_t* defaultExt = L"png";

        GUID originalFormat = GUID_NULL;
        {
           std::lock_guard<std::recursive_mutex> lock(m_ctx.wicMutex);
            originalFormat = m_ctx.originalContainerFormat;
        }

        if (originalFormat == GUID_ContainerFormatJpeg) {
            filterIndex = 2;
            defaultExt = L"jpg";
            wcscpy_s(szFile, (std::wstring(Tr(StrId::DefaultFileName)) + L".jpg").c_str());
        }
        else if (originalFormat == GUID_ContainerFormatBmp) {
            filterIndex = 3;
            defaultExt = L"bmp";
            wcscpy_s(szFile, (std::wstring(Tr(StrId::DefaultFileName)) + L".bmp").c_str());
        }

        if (m_ctx.currentImageIndex >= 0 && m_ctx.currentImageIndex < static_cast<int>(m_ctx.imageFiles.size())) {
            const std::wstring& originalPath = m_ctx.imageFiles[m_ctx.currentImageIndex];
            wchar_t originalFileName[MAX_PATH];
            wcscpy_s(originalFileName, MAX_PATH, originalPath.c_str());
            PathRemoveExtensionW(originalFileName);
            PathStripPathW(originalFileName);

            std::wstring formattedName = std::format(L"{}_resized.{}", originalFileName, defaultExt);
            wcscpy_s(szFile, MAX_PATH, formattedName.c_str());
        }

        OPENFILENAMEW ofn = { sizeof(ofn) };
        ofn.hwndOwner = m_ctx.hWnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = filterIndex;
        ofn.lpstrDefExt = defaultExt;
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

        if (GetSaveFileNameW(&ofn)) {
            GUID containerFormat = GUID_ContainerFormatPng;
            const wchar_t* ext = PathFindExtensionW(ofn.lpstrFile);
            if (ext) {
                if (_wcsicmp(ext, L".jpg") == 0 || _wcsicmp(ext, L".jpeg") == 0) containerFormat = GUID_ContainerFormatJpeg;
                else if (_wcsicmp(ext, L".bmp") == 0) containerFormat = GUID_ContainerFormatBmp;
            }
            SaveImageWithResize(ofn.lpstrFile, containerFormat, params.newWidth, params.newHeight);
        }
    }
}