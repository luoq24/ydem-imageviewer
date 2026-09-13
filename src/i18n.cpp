#include "i18n.h"
#include <iterator>

namespace {
    // 中文为默认语言
    AppLanguage g_language = AppLanguage::Chinese;

    struct StrPair {
        const wchar_t* zh;
        const wchar_t* en;
    };

    // 顺序必须与 StrId 完全一致
    constexpr StrPair kStrings[] = {
        // 窗口标题 / 状态
        { L"{0} - {1}", L"{0} - {1}" },                                              // TitleSimpleFormat
        { L"{0}（第 {1}/{2} 帧）- {3}", L"{0} (Frame {1}/{2}) - {3}" },               // TitleFrameFormat
        { L"{0}  [加载中...] - {1}", L"{0}  [Loading...] - {1}" },                    // TitleLoadingFormat
        { L"加载失败 - {0}", L"Load Failed - {0}" },                                  // TitleLoadFailedFormat
        { L"剪贴板图像 - {0}", L"Clipboard Image - {0}" },                            // TitleClipboardFormat
        { L"剪贴板图像", L"Clipboard Image" },                                        // ClipboardImage
        // 右键菜单
        { L"打开图片", L"Open Image" },                                               // MenuOpenImage
        { L"刷新", L"Refresh" },                                                     // MenuRefresh
        { L"复制", L"Copy" },                                                        // MenuCopy
        { L"粘贴", L"Paste" },                                                       // MenuPaste
        { L"下一张", L"Next Image" },                                                 // MenuNextImage
        { L"上一张", L"Previous Image" },                                             // MenuPrevImage
        { L"排序方式", L"Sort By" },                                                  // MenuSortBy
        { L"名称（升序）", L"Name (Ascending)" },                                      // MenuSortNameAsc
        { L"名称（降序）", L"Name (Descending)" },                                     // MenuSortNameDesc
        { L"修改日期（升序）", L"Date Modified (Ascending)" },                          // MenuSortDateAsc
        { L"修改日期（降序）", L"Date Modified (Descending)" },                         // MenuSortDateDesc
        { L"文件大小（升序）", L"File Size (Ascending)" },                              // MenuSortSizeAsc
        { L"文件大小（降序）", L"File Size (Descending)" },                             // MenuSortSizeDesc
        { L"编辑", L"Edit" },                                                        // MenuEdit
        { L"顺时针旋转", L"Rotate Clockwise" },                                        // MenuRotateCW
        { L"逆时针旋转", L"Rotate Counter-Clockwise" },                                // MenuRotateCCW
        { L"水平翻转", L"Flip" },                                                     // MenuFlip
        { L"裁剪", L"Crop" },                                                        // MenuCrop
        { L"调整大小...", L"Resize Image..." },                                       // MenuResizeImage
        { L"查看", L"View" },                                                        // MenuView
        { L"放大", L"Zoom In" },                                                     // MenuZoomIn
        { L"缩小", L"Zoom Out" },                                                    // MenuZoomOut
        { L"实际大小 (100%)", L"Actual Size (100%)" },                                // MenuActualSize100
        { L"缩放 200%", L"Zoom 200%" },                                              // MenuZoom200
        { L"缩放 300%", L"Zoom 300%" },                                              // MenuZoom300
        { L"适应窗口", L"Fit to Window" },                                            // MenuFitToWindow
        { L"全屏", L"Full Screen" },                                                 // MenuFullScreen
        { L"幻灯片放映", L"Toggle Slideshow" },                                       // MenuToggleSlideshow
        { L"保存", L"Save" },                                                        // MenuSave
        { L"另存为", L"Save As" },                                                   // MenuSaveAs
        { L"打开文件所在位置", L"Open File Location" },                                // MenuOpenLocation
        { L"属性...", L"Properties..." },                                            // MenuProperties
        { L"设置...", L"Preferences..." },                                           // MenuPreferences
        { L"快捷键...", L"Keybindings..." },                                         // MenuKeybindings
        { L"删除图片", L"Delete Image" },                                             // MenuDeleteImage
        { L"隐藏到后台", L"Hide to Background" },                                     // MenuHideToBackground
        { L"关闭应用", L"Close App" },                                                // MenuCloseApp
        { L"复制路径", L"Copy Path" },                                               // MenuCopyPath
        { L"原生", L"Native" },                                                      // MenuNative
        { L"【自娱】编辑", L"[Ziyu] Edit" },                                         // MenuZiyuEdit
        { L"lineart", L"lineart" },                                                  // MenuZiyuLineart
        { L"【自娱】H3", L"[Ziyu] H3" },                                               // MenuZiyuH3
        { L"【自娱】一键高清", L"[Ziyu] Quick HD" },                                    // MenuZiyuQuickHD
        { L"【player】播放", L"[player] Play" },                                       // MenuPlayerPlay
        { L"PS打开", L"Open in PS" },                                                // MenuPsOpen
        { L"发PS图层", L"Send as PS Layer" },                                          // MenuPsLayer
        // 快捷键对话框中的动作名
        { L"打开文件", L"Open File" },                                                // ActNameOpenFile
        { L"实际大小", L"Actual Size" },                                              // ActNameActualSize
        { L"全屏", L"Fullscreen" },                                                  // ActNameFullscreen
        { L"自定义缩放", L"Custom Zoom" },                                            // ActNameCustomZoom
        { L"撤销", L"Undo" },                                                        // ActNameUndo
        { L"居中显示图像", L"Center Image" },                                         // ActNameCenterImage
        { L"确认裁剪", L"Commit Crop" },                                              // ActNameCommitCrop
        { L"切换信息叠加层（OSD）", L"Toggle OSD" },                                    // ActNameToggleOSD
        { L"播放/暂停动画", L"Play/Pause Animation" },                                 // ActNamePlayPause
        { L"恢复播放动画", L"Resume Animation" },                                      // ActNameResumeAnim
        { L"下一帧", L"Next Frame" },                                                 // ActNameNextFrame
        { L"上一帧", L"Previous Frame" },                                             // ActNamePrevFrame
        { L"第一帧", L"First Frame" },                                                // ActNameFirstFrame
        { L"打开右键菜单", L"Open Context Menu" },                                     // ActNameContextMenu
        { L"第一张图片", L"First Image" },                                             // ActNameFirstImage
        { L"最后一张图片", L"Last Image" },                                             // ActNameLastImage
        // 快捷键名称
        { L"左方向键", L"Left Arrow" },                                               // HkLeft
        { L"右方向键", L"Right Arrow" },                                              // HkRight
        { L"上方向键", L"Up Arrow" },                                                 // HkUp
        { L"下方向键", L"Down Arrow" },                                               // HkDown
        { L"空格", L"Spacebar" },                                                    // HkSpacebar
        { L"小键盘 ", L"Numpad " },                                                   // HkNumpadPrefix
        // 消息框
        { L"错误", L"Error" },                                                       // ErrCaption
        { L"无法创建 WIC 图像工厂。", L"Failed to create WIC Imaging Factory." },        // ErrWicFactory
        { L"无法创建 Direct2D 工厂。", L"Failed to create Direct2D Factory." },         // ErrD2dFactory
        { L"无法创建 DirectWrite 工厂。", L"Failed to create DirectWrite Factory." },   // ErrDWriteFactory
        { L"无法创建窗口。", L"Failed to create window." },                             // ErrCreateWindow
        { L"未检测到自娱工具在运行，请先启动“自娱工具”再重试。",
          L"The Ziyu tool is not running. Please start it first and try again." },      // ErrZiyuNotRunning
        { L"发送到自娱工具失败。", L"Failed to send to the Ziyu tool." },                // ErrZiyuSendFailed
        { L"找不到 ydem_player 配置文件（config.yaml）。",
          L"ydem_player config file (config.yaml) not found." },                       // ErrPlayerConfigMissing
        { L"未能在 ydem_player 中找到与当前图片对应的视频。",
          L"No matching video found in ydem_player for the current image." },          // ErrPlayerVideoNotFound
        { L"启动 PotPlayer 失败。", L"Failed to launch PotPlayer." },                   // ErrPlayerLaunchFailed
        { L"确定要删除这张图片吗？", L"Are you sure you want to delete?" },              // DeleteConfirmMsg
        { L"确认删除", L"Confirm Delete" },                                           // DeleteConfirmTitle
        { L"此 HDR 图像过大，超出了 stb_image HDR 加载器的安全上限。",
          L"This HDR image is too large for the stb_image HDR loader safety limit." }, // HdrTooLargeMsg
        { L"HDR 图像过大", L"HDR Image Too Large" },                                  // HdrTooLargeTitle
        { L"若要原生查看 HEIC 和 AVIF 图像，需要从微软商店安装免费的“HEIF 图像扩展”。\n\n是否要打开商店页面？",
          L"To view HEIC and AVIF images natively, you need the free 'HEIF Image Extensions' from the Microsoft Store.\n\nWould you like to open the Store page?" }, // CodecMissingMsg
        { L"缺少图像编解码器", L"Missing Image Codec" },                                // CodecMissingTitle
        { L"保存", L"Save" },                                                        // SaveCaption
        { L"保存错误", L"Save Error" },                                               // SaveErrCaption
        { L"另存为错误", L"Save As Error" },                                          // SaveAsErrCaption
        { L"保存编辑", L"Save Edits" },                                               // SaveEditsTitle
        { L"无法获取要保存的图像源。", L"Could not get image source to save." },          // SaveErrSource
        { L"图像保存失败。", L"Failed to save image." },                                // SaveFailed
        { L"没有需要保存的更改。", L"No changes to save." },                             // SaveNoChanges
        { L"HEIC 和 AVIF 文件无法被直接覆盖。是否要将编辑内容另存为 PNG 文件？",
          L"HEIC and AVIF files cannot be natively overwritten. Would you like to save your edits as a PNG instead?" }, // SaveEditsMsg
        { L"另存为 PNG 失败。", L"Failed to save as PNG." },                           // SavePngFailed
        { L"无法确定原始文件格式，请使用“另存为”。", L"Could not determine original file format. Use 'Save As'." }, // SaveUnknownFormat
        { L"替换原始文件失败。", L"Failed to replace the original file." },              // SaveReplaceFailed
        { L"保存图像到临时文件失败。", L"Failed to save image to temporary file." },       // SaveTempFailed
        { L"调整大小错误", L"Resize Error" },                                         // ResizeErrCaption
        { L"无法获取要调整大小的图像源。", L"Could not get image source to resize." },     // ResizeSource
        { L"初始化图像缩放器失败。", L"Failed to initialize image scaler." },            // ResizeInitFailed
        { L"创建图像缩放器失败。", L"Failed to create image scaler." },                  // ResizeCreateFailed
        { L"保存调整大小后的图像失败。", L"Failed to save resized image." },               // ResizeSaveFailed
        { L"没有已加载的图像可供调整大小。", L"No image loaded to resize." },              // ResizeNoImage
        { L"输入无效", L"Invalid Input" },                                            // InvalidInputCaption
        { L"请输入有效的宽度和高度（非零正数）。", L"Please enter valid (non-zero) positive numbers for width and height." }, // ResizeInvalidMsg
        { L"请输入有效的正百分比数值。", L"Please enter a valid positive percentage." },  // ZoomInvalidMsg
        // 首选项对话框
        { L"设置", L"Preferences" },                                                  // DlgPreferencesTitle
        { L"默认背景颜色", L"Default Background Color" },                              // PrefBgGroup
        { L"灰色（默认）", L"Grey (Default)" },                                        // PrefBgGrey
        { L"黑色", L"Black" },                                                       // PrefBgBlack
        { L"白色", L"White" },                                                       // PrefBgWhite
        { L"透明", L"Transparent" },                                                  // PrefBgTransparent
        { L"应用设置", L"Application Settings" },                                     // PrefAppGroup
        { L"总在最前", L"Always on Top" },                                            // PrefAlwaysOnTop
        { L"启动时全屏", L"Start in full screen" },                                    // PrefStartFullscreen
        { L"强制单实例运行", L"Enforce single instance" },                              // PrefSingleInstance
        { L"自动刷新（文件更改时重新加载）", L"Auto Refresh (Reload if file changes)" },   // PrefAutoRefresh
        { L"平滑缩放（插值）", L"Smooth Scaling (Interpolation)" },                     // PrefSmoothScaling
        { L"启用图像淡入动画", L"Enable Image Fade Animation" },                         // PrefFadeAnimation
        { L"显示图像信息叠加层（OSD）", L"Show Image Info Overlay (OSD)" },               // PrefShowOsd
        { L"删除前询问", L"Ask before deleting" },                                     // PrefAskDelete
        { L"调整大小/全屏时保持缩放", L"Preserve zoom on resize/fullscreen" },            // PrefPreserveZoom
        { L"根据图片横竖方向自动切换显示器", L"Auto-move window to matching display orientation" }, // PrefAutoMonitorPlacement
        { L"列表循环（浏览到最后一张后回到第一张）", L"List Loop (wrap to first after last image)" },  // PrefListLoop
        { L"默认缩放模式", L"Default Zoom Mode" },                                     // PrefZoomGroup
        { L"适应窗口", L"Fit to Window" },                                            // PrefZoomFit
        { L"实际大小 (100%)", L"Actual Size (100%)" },                                // PrefZoomActual
        { L"界面语言", L"Language" },                                                  // PrefLanguageGroup
        { L"中文（简体）", L"Chinese (Simplified)" },                                  // PrefLangZh
        { L"English", L"English" },                                                  // PrefLangEn
        // 快捷键 / 自定义缩放 / 调整大小对话框
        { L"快捷键", L"Keybindings" },                                                // DlgKeybindingsTitle
        { L"操作:", L"Action:" },                                                    // KbAction
        { L"快捷键:", L"Shortcut:" },                                                 // KbShortcut
        { L"确定", L"OK" },                                                          // BtnOk
        { L"取消", L"Cancel" },                                                      // BtnCancel
        { L"应用", L"Apply" },                                                       // BtnApply
        { L"已应用！", L"Applied!" },                                                 // BtnApplied
        { L"关闭", L"Close" },                                                       // BtnClose
        { L"恢复默认", L"Restore Defaults" },                                         // BtnRestoreDefaults
        { L"确定要将所有快捷键恢复为默认设置吗？", L"Are you sure you want to restore all keybindings to their default values?" }, // KbRestoreConfirmMsg
        { L"恢复默认快捷键", L"Restore Default Keybindings" },                         // KbRestoreConfirmTitle
        { L"自定义缩放", L"Custom Zoom" },                                            // DlgZoomTitle
        { L"缩放百分比 (%):", L"Zoom Percentage (%):" },                              // ZoomLabel
        { L"调整图像大小", L"Resize Image" },                                          // DlgResizeTitle
        { L"宽度:", L"Width:" },                                                     // ResizeWidth
        { L"高度:", L"Height:" },                                                    // ResizeHeight
        { L"保持纵横比", L"Maintain aspect ratio" },                                   // ResizeAspect
        // OSD 信息叠加层
        { L"图像格式: ", L"Image Format: " },                                         // OsdImageFormat
        { L"尺寸: ", L"Dimensions: " },                                              // OsdDimensions
        { L"方向: ", L"Orientation: " },                                             // OsdOrientation
        { L"位深度: ", L"Bit Depth: " },                                             // OsdBitDepth
        { L"DPI: ", L"DPI: " },                                                      // OsdDpi
        { L"文件大小: ", L"File Size: " },                                            // OsdFileSize
        { L"属性: ", L"Attributes: " },                                              // OsdAttributes
        { L"光圈: ", L"F-stop: " },                                                  // OsdFStop
        { L"曝光: ", L"Exposure: " },                                                // OsdExposure
        { L"ISO: ", L"ISO: " },                                                      // OsdIso
        { L"作者: ", L"Author: " },                                                  // OsdAuthor
        { L"软件: ", L"Software: " },                                                // OsdSoftware
        // 画面覆盖文字
        { L"加载中...", L"Loading..." },                                              // DrawLoading
        { L"右键单击显示选项，或将图片拖拽到此处", L"Right-click for options or drag an image here" }, // DrawEmpty
        { L"按 Enter 应用裁剪，按 Esc 取消", L"Press Enter to apply crop, Esc to cancel" }, // DrawCropHint
        // 属性 / 元数据
        { L"无", L"N/A" },                                                           // PropNa
        { L"未知", L"Unknown" },                                                     // PropUnknown
        { L"{0} x {1} 像素", L"{0} x {1} pixels" },                                  // PropPixelsFormat
        { L"{0} 位", L"{0}-bit" },                                                   // PropBitDepthFormat
        { L"{0}（{1} 字节）", L"{0} ({1} Bytes)" },                                   // PropFileSizeFormat
        { L"只读; ", L"Read-only; " },                                               // PropAttrReadOnly
        { L"隐藏; ", L"Hidden; " },                                                  // PropAttrHidden
        { L"系统; ", L"System; " },                                                  // PropAttrSystem
        { L"常规", L"Normal" },                                                      // PropAttrNormal
        { L"详细信息", L"Details" },                                                   // PropDetailsTab
        // 文件对话框
        { L"所有图片文件\0*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tiff;*.tif;*.ico;*.webp;*.heic;*.heif;*.avif;*.cr2;*.cr3;*.nef;*.dng;*.arw;*.orf;*.rw2;*.svg;*.qoi;*.hdr\0HDR 文件 (*.hdr)\0*.hdr\0SVG 文件 (*.svg)\0*.svg\0QOI 文件 (*.qoi)\0*.qoi\0PNG 文件 (*.png)\0*.png\0JPEG 文件 (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0所有文件 (*.*)\0*.*\0",
          L"All Image Files\0*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tiff;*.tif;*.ico;*.webp;*.heic;*.heif;*.avif;*.cr2;*.cr3;*.nef;*.dng;*.arw;*.orf;*.rw2;*.svg;*.qoi;*.hdr\0HDR Files (*.hdr)\0*.hdr\0SVG Files (*.svg)\0*.svg\0QOI Files (*.qoi)\0*.qoi\0PNG Files (*.png)\0*.png\0JPEG Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0All Files (*.*)\0*.*\0" }, // FilterOpen
        { L"PNG 文件 (*.png)\0*.png\0JPEG 文件 (*.jpg)\0*.jpg\0BMP 文件 (*.bmp)\0*.bmp\0所有文件 (*.*)\0*.*\0",
          L"PNG File (*.png)\0*.png\0JPEG File (*.jpg)\0*.jpg\0BMP File (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0" }, // FilterSave
        { L"未命名", L"Untitled" },                                                   // DefaultFileName
    };

    static_assert(static_cast<size_t>(StrId::Count) == std::size(kStrings), "i18n 字符串表与 StrId 不同步");
}

namespace I18n {
    AppLanguage GetLanguage() {
        return g_language;
    }

    void SetLanguage(AppLanguage lang) {
        g_language = lang;
    }

    const wchar_t* DWriteLocale() {
        return (g_language == AppLanguage::Chinese) ? L"zh-cn" : L"en-us";
    }
}

const wchar_t* Tr(StrId id) {
    const size_t index = static_cast<size_t>(id);
    if (index >= static_cast<size_t>(StrId::Count)) return L"";
    return (I18n::GetLanguage() == AppLanguage::Chinese) ? kStrings[index].zh : kStrings[index].en;
}
