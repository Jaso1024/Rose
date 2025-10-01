#pragma once

#include <functional>
#include <Carbon/Carbon.h>

class HotkeyMonitor {
public:
    HotkeyMonitor();
    ~HotkeyMonitor();

    bool initialize(std::function<void()> callback);
    void start();
    void stop();
    bool update();

private:
    static OSStatus hotKeyHandler(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* userData);
    bool registerCurrentHotkey();

    std::function<void()> hotkeyCallback;
    EventHotKeyRef hotKeyRef;
    EventHandlerRef handlerRef;
    bool running;
};
