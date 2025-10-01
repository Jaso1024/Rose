#pragma once

#include <string>

class ClipboardManager {
public:
    static void copyToClipboard(const std::string& text);
};