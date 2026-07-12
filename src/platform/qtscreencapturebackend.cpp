// SPDX-License-Identifier: GPL-3.0-or-later

#include "qtscreencapturebackend.h"

#include <QGuiApplication>
#include <QPoint>
#include <QPixmap>
#include <QScreen>

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#elif defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#include <unistd.h>
#elif defined(CLIPIT_HAS_X11)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#endif

namespace {

#if defined(Q_OS_WIN) || defined(CLIPIT_HAS_X11)
struct NativeWindowTarget
{
    WId id = 0;
    QPoint center;
    bool hasCenter = false;
};
#endif

#if defined(Q_OS_WIN)
NativeWindowTarget activeNativeWindow()
{
    const HWND window = GetForegroundWindow();
    if (!window || !IsWindow(window) || !IsWindowVisible(window))
        return {};

    NativeWindowTarget target;
    target.id = reinterpret_cast<WId>(window);
    RECT rect{};
    if (GetWindowRect(window, &rect)) {
        target.center = QPoint((rect.left + rect.right) / 2,
                               (rect.top + rect.bottom) / 2);
        target.hasCenter = true;
    }
    return target;
}
#elif defined(CLIPIT_HAS_X11)
Window topLevelX11Window(Display *display, Window window)
{
    Window current = window;
    while (current != None) {
        Window root = None;
        Window parent = None;
        Window *children = nullptr;
        unsigned int childCount = 0;
        if (!XQueryTree(display, current, &root, &parent, &children, &childCount))
            break;
        if (children)
            XFree(children);
        if (parent == None || parent == root)
            break;
        current = parent;
    }
    return current;
}

NativeWindowTarget activeNativeWindow()
{
    Display *display = XOpenDisplay(nullptr);
    if (!display)
        return {};

    const Window root = DefaultRootWindow(display);
    Window activeWindow = None;
    const Atom activeWindowAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
    if (activeWindowAtom != None) {
        Atom actualType = None;
        int actualFormat = 0;
        unsigned long itemCount = 0;
        unsigned long bytesAfter = 0;
        unsigned char *data = nullptr;
        const int status = XGetWindowProperty(
            display, root, activeWindowAtom, 0, 1, False, XA_WINDOW, &actualType,
            &actualFormat, &itemCount, &bytesAfter, &data);
        if (status == Success && actualType == XA_WINDOW && actualFormat == 32
            && itemCount == 1 && data) {
            activeWindow = *reinterpret_cast<Window *>(data);
        }
        if (data)
            XFree(data);
    }

    if (activeWindow == None || activeWindow == PointerRoot) {
        int revertTo = 0;
        XGetInputFocus(display, &activeWindow, &revertTo);
    }

    NativeWindowTarget target;
    if (activeWindow != None && activeWindow != PointerRoot) {
        const Window topLevel = topLevelX11Window(display, activeWindow);
        target.id = static_cast<WId>(topLevel);

        XWindowAttributes attributes{};
        Window child = None;
        int rootX = 0;
        int rootY = 0;
        if (XGetWindowAttributes(display, topLevel, &attributes)
            && XTranslateCoordinates(display, topLevel, root, 0, 0, &rootX, &rootY,
                                     &child)) {
            target.center = QPoint(rootX + attributes.width / 2,
                                   rootY + attributes.height / 2);
            target.hasCenter = true;
        }
    }

    XCloseDisplay(display);
    return target;
}
#elif defined(Q_OS_MACOS)
bool dictionaryNumber(CFDictionaryRef dictionary, CFStringRef key, qint64 *value)
{
    const auto number = static_cast<CFNumberRef>(CFDictionaryGetValue(dictionary, key));
    return number && CFGetTypeID(number) == CFNumberGetTypeID()
           && CFNumberGetValue(number, kCFNumberSInt64Type, value);
}

QImage activeMacWindowImage()
{
    const CGWindowListOption listOptions = static_cast<CGWindowListOption>(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements);
    const CFArrayRef windows = CGWindowListCopyWindowInfo(listOptions, kCGNullWindowID);
    if (!windows)
        return {};

    CGWindowID activeWindow = kCGNullWindowID;
    const qint64 ownPid = static_cast<qint64>(getpid());
    const CFIndex count = CFArrayGetCount(windows);
    for (CFIndex index = 0; index < count; ++index) {
        const auto info = static_cast<CFDictionaryRef>(
            CFArrayGetValueAtIndex(windows, index));
        qint64 layer = -1;
        qint64 ownerPid = -1;
        qint64 windowNumber = 0;
        if (!dictionaryNumber(info, kCGWindowLayer, &layer) || layer != 0
            || !dictionaryNumber(info, kCGWindowOwnerPID, &ownerPid)
            || ownerPid == ownPid
            || !dictionaryNumber(info, kCGWindowNumber, &windowNumber)) {
            continue;
        }

        const auto boundsDictionary = static_cast<CFDictionaryRef>(
            CFDictionaryGetValue(info, kCGWindowBounds));
        CGRect bounds{};
        if (!boundsDictionary
            || !CGRectMakeWithDictionaryRepresentation(boundsDictionary, &bounds)
            || bounds.size.width < 2 || bounds.size.height < 2) {
            continue;
        }
        activeWindow = static_cast<CGWindowID>(windowNumber);
        break;
    }
    CFRelease(windows);

    if (activeWindow == kCGNullWindowID)
        return {};

    const CGWindowImageOption imageOptions = kCGWindowImageBestResolution;
    const CGImageRef source = CGWindowListCreateImage(
        CGRectNull, kCGWindowListOptionIncludingWindow, activeWindow, imageOptions);
    if (!source)
        return {};

    const size_t width = CGImageGetWidth(source);
    const size_t height = CGImageGetHeight(source);
    QImage image(static_cast<int>(width), static_cast<int>(height),
                 QImage::Format_ARGB32_Premultiplied);
    const CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    const CGBitmapInfo bitmapInfo = static_cast<CGBitmapInfo>(
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
    const CGContextRef context = image.isNull() || !colorSpace
                                     ? nullptr
                                     : CGBitmapContextCreate(
                                           image.bits(), width, height, 8,
                                           static_cast<size_t>(image.bytesPerLine()),
                                           colorSpace, bitmapInfo);
    if (context) {
        CGContextTranslateCTM(context, 0, static_cast<CGFloat>(height));
        CGContextScaleCTM(context, 1, -1);
        CGContextDrawImage(context,
                           CGRectMake(0, 0, static_cast<CGFloat>(width),
                                      static_cast<CGFloat>(height)),
                           source);
        CGContextRelease(context);
    } else {
        image = {};
    }
    if (colorSpace)
        CGColorSpaceRelease(colorSpace);
    CGImageRelease(source);
    return image;
}
#endif

} // namespace

namespace Clipit {

QtScreenCaptureBackend::QtScreenCaptureBackend(QObject *parent)
    : ScreenCaptureBackend(parent)
{
}

bool QtScreenCaptureBackend::available() const
{
    return QGuiApplication::primaryScreen() != nullptr;
}

void QtScreenCaptureBackend::capture(CaptureTarget target)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        emit failed(tr("没有可用的屏幕"));
        return;
    }

    QImage image;
    if (target == CaptureTarget::Screen) {
        image = screen->grabWindow(0).toImage();
    } else {
#if defined(Q_OS_MACOS)
        image = activeMacWindowImage();
#elif defined(Q_OS_WIN) || defined(CLIPIT_HAS_X11)
        const NativeWindowTarget window = activeNativeWindow();
        if (!window.id) {
            emit failed(tr("无法确定当前活动窗口"));
            return;
        }
        if (window.hasCenter) {
            if (QScreen *windowScreen = QGuiApplication::screenAt(window.center))
                screen = windowScreen;
        }
        image = screen->grabWindow(window.id).toImage();
#else
        emit failed(tr("当前构建不支持活动窗口截图"));
        return;
#endif
    }
    if (image.isNull()) {
        emit failed(target == CaptureTarget::Screen ? tr("无法读取屏幕画面")
                                                    : tr("无法读取当前活动窗口画面"));
        return;
    }
    emit captured(image);
}

void QtScreenCaptureBackend::cancel()
{
}

} // namespace Clipit
