#include "ClipboardManager.h"
#include <Cocoa/Cocoa.h>

void ClipboardManager::copyToClipboard(const std::string& text) {
    if (text.empty()) return;
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
        [pasteboard setString:nsText forType:NSPasteboardTypeString];
    }
}
