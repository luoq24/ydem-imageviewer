#include "exif_utils.h"
#include "viewer.h"
#include <propvarutil.h>
#include <propsys.h>
#include <propkey.h>
#include <format>

std::wstring GetPropertyString(IPropertyStore* pStore, REFPROPERTYKEY key) {
    if (!pStore) return Tr(StrId::PropNa);

    wil::unique_prop_variant propValue;
    std::wstring val = Tr(StrId::PropNa);

    if (SUCCEEDED(pStore->GetValue(key, &propValue))) {
        wil::unique_cotaskmem_string pszDisplayValue;
        if (SUCCEEDED(PSFormatForDisplayAlloc(key, propValue, PDFF_DEFAULT, &pszDisplayValue))) {
            if (pszDisplayValue && wcslen(pszDisplayValue.get()) > 0) {
                val = pszDisplayValue.get();
            }
        }
    }
    return val;
}

std::wstring GetContainerFormatName(const GUID& guid, IWICImagingFactory* wicFactory) {
    if (!wicFactory) return Tr(StrId::PropUnknown);

    ComPtr<IWICComponentInfo> componentInfo;
    if (FAILED(wicFactory->CreateComponentInfo(guid, &componentInfo))) {
        return Tr(StrId::PropUnknown);
    }

    UINT cchActual = 0;
    // Get buffer length
    if (FAILED(componentInfo->GetFriendlyName(0, nullptr, &cchActual)) || cchActual == 0) {
        return Tr(StrId::PropUnknown);
    }

    // Fetch name
    std::wstring name(cchActual, L'\0');
    if (SUCCEEDED(componentInfo->GetFriendlyName(cchActual, name.data(), &cchActual))) {
        name.resize(cchActual > 0 ? cchActual - 1 : 0); 
        return name;
    }

    return Tr(StrId::PropUnknown);
}

std::wstring GetBitDepth(IWICBitmapFrameDecode* pFrame, IWICImagingFactory* wicFactory) {
    WICPixelFormatGUID pixelFormatGuid;
    if (FAILED(pFrame->GetPixelFormat(&pixelFormatGuid))) {
        return Tr(StrId::PropNa);
    }

    ComPtr<IWICComponentInfo> componentInfo;
    if (FAILED(wicFactory->CreateComponentInfo(pixelFormatGuid, &componentInfo))) {
        return Tr(StrId::PropNa);
    }

    ComPtr<IWICPixelFormatInfo> pixelFormatInfo;
    if (FAILED(componentInfo->QueryInterface(IID_PPV_ARGS(&pixelFormatInfo)))) {
        return Tr(StrId::PropNa);
    }

    UINT bpp = 0;
    if (SUCCEEDED(pixelFormatInfo->GetBitsPerPixel(&bpp))) {
        return std::vformat(Tr(StrId::PropBitDepthFormat), std::make_wformat_args(bpp));
    }
    return Tr(StrId::PropNa);
}