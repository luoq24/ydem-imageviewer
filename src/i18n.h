#pragma once

// UI 本地化模块：支持中文（默认）与英文

enum class AppLanguage {
    Chinese = 0,
    English = 1
};

enum class StrId {
    // 窗口标题 / 状态
    TitleSimpleFormat, TitleFrameFormat, TitleLoadingFormat, TitleLoadFailedFormat, TitleClipboardFormat,
    ClipboardImage,
    // 右键菜单
    MenuOpenImage, MenuRefresh, MenuCopy, MenuPaste, MenuNextImage, MenuPrevImage,
    MenuSortBy, MenuSortNameAsc, MenuSortNameDesc, MenuSortDateAsc, MenuSortDateDesc, MenuSortSizeAsc, MenuSortSizeDesc,
    MenuEdit, MenuRotateCW, MenuRotateCCW, MenuFlip, MenuCrop, MenuResizeImage,
    MenuView, MenuZoomIn, MenuZoomOut, MenuActualSize100, MenuZoom200, MenuZoom300, MenuFitToWindow, MenuFullScreen, MenuToggleSlideshow,
    MenuSave, MenuSaveAs, MenuOpenLocation, MenuProperties, MenuPreferences, MenuKeybindings, MenuDeleteImage, MenuExit,
    // 快捷键对话框中的动作名（与菜单不重复的部分）
    ActNameOpenFile, ActNameActualSize, ActNameFullscreen, ActNameCustomZoom, ActNameUndo, ActNameCenterImage,
    ActNameCommitCrop, ActNameToggleOSD, ActNamePlayPause, ActNameResumeAnim, ActNameNextFrame, ActNamePrevFrame,
    ActNameFirstFrame, ActNameContextMenu,
    // 快捷键名称
    HkLeft, HkRight, HkUp, HkDown, HkSpacebar, HkNumpadPrefix,
    // 消息框
    ErrCaption, ErrWicFactory, ErrD2dFactory, ErrDWriteFactory, ErrCreateWindow,
    DeleteConfirmMsg, DeleteConfirmTitle,
    HdrTooLargeMsg, HdrTooLargeTitle,
    CodecMissingMsg, CodecMissingTitle,
    SaveCaption, SaveErrCaption, SaveAsErrCaption, SaveEditsTitle,
    SaveErrSource, SaveFailed, SaveNoChanges, SaveEditsMsg, SavePngFailed, SaveUnknownFormat, SaveReplaceFailed, SaveTempFailed,
    ResizeErrCaption, ResizeSource, ResizeInitFailed, ResizeCreateFailed, ResizeSaveFailed, ResizeNoImage,
    InvalidInputCaption, ResizeInvalidMsg, ZoomInvalidMsg,
    // 首选项对话框
    DlgPreferencesTitle, PrefBgGroup, PrefBgGrey, PrefBgBlack, PrefBgWhite, PrefBgTransparent,
    PrefAppGroup, PrefAlwaysOnTop, PrefStartFullscreen, PrefSingleInstance, PrefAutoRefresh, PrefSmoothScaling,
    PrefFadeAnimation, PrefShowOsd, PrefAskDelete, PrefPreserveZoom,
    PrefZoomGroup, PrefZoomFit, PrefZoomActual, PrefLanguageGroup, PrefLangZh, PrefLangEn,
    // 快捷键 / 自定义缩放 / 调整大小对话框
    DlgKeybindingsTitle, KbAction, KbShortcut, BtnOk, BtnCancel, BtnApply, BtnApplied, BtnClose,
    DlgZoomTitle, ZoomLabel,
    DlgResizeTitle, ResizeWidth, ResizeHeight, ResizeAspect,
    // OSD 信息叠加层
    OsdImageFormat, OsdDimensions, OsdOrientation, OsdBitDepth, OsdDpi, OsdFileSize, OsdAttributes,
    OsdFStop, OsdExposure, OsdIso, OsdAuthor, OsdSoftware,
    // 画面覆盖文字
    DrawLoading, DrawEmpty, DrawCropHint,
    // 属性 / 元数据
    PropNa, PropUnknown, PropPixelsFormat, PropBitDepthFormat, PropFileSizeFormat,
    PropAttrReadOnly, PropAttrHidden, PropAttrSystem, PropAttrNormal, PropDetailsTab,
    // 文件对话框
    FilterOpen, FilterSave, DefaultFileName,
    Count
};

namespace I18n {
    AppLanguage GetLanguage();
    void SetLanguage(AppLanguage lang);
    // DirectWrite 文本区域设置（字体回退相关）
    const wchar_t* DWriteLocale();
}

// 返回当前语言的文本
const wchar_t* Tr(StrId id);

// 应用名称与版本号（不做翻译，集中管理）
inline const wchar_t* AppNameAndVersion() { return L"Minimal Image Viewer v2.0.3"; }
