#include "HotkeyMonitor.h"
#include "Settings.h"
#include "Constants.h"
#include <string>

HotkeyMonitor::HotkeyMonitor() : hotKeyRef(nullptr), handlerRef(nullptr), running(false) {}

HotkeyMonitor::~HotkeyMonitor() {
    stop();
}

static bool parseHotkeyStrict(const std::string& s, UInt32& outKeyCode, UInt32& outModifiers) {
    bool allowed = false;
    for (const auto& kv : constants::HotkeyOptions()) {
        if (kv.first == s) { allowed = true; break; }
    }
    if (!allowed) return false;

    outModifiers = 0;
    outKeyCode = 0;

    size_t start = 0;
    while (start <= s.size()) {
        size_t plus = s.find('+', start);
        std::string tok = s.substr(start, plus == std::string::npos ? std::string::npos : plus - start);
        if (tok == "cmd") outModifiers |= cmdKey;
        else if (tok == "shift") outModifiers |= shiftKey;
        else if (tok == "option") outModifiers |= optionKey;
        else if (tok == "space") outKeyCode = kVK_Space;
        else if (tok == "r") outKeyCode = kVK_ANSI_R;
        start = (plus == std::string::npos) ? s.size() + 1 : plus + 1;
    }

    return outKeyCode != 0;
}

bool HotkeyMonitor::registerCurrentHotkey() {
    if (hotKeyRef) {
        UnregisterEventHotKey(hotKeyRef);
        hotKeyRef = nullptr;
    }
    if (!handlerRef) {
        EventTypeSpec eventType;
        eventType.eventClass = kEventClassKeyboard;
        eventType.eventKind = kEventHotKeyPressed;
        InstallApplicationEventHandler(&HotkeyMonitor::hotKeyHandler, 1, &eventType, this, &handlerRef);
    }

    UInt32 keyCode = 0;
    UInt32 modifiers = 0;
    std::string hotkey = Settings::getInstance().getHotkey();
    if (!parseHotkeyStrict(hotkey, keyCode, modifiers)) {
        parseHotkeyStrict(constants::kDefaultHotkey, keyCode, modifiers);
    }

    EventHotKeyID hotKeyID;
    hotKeyID.signature = 'ROSE';
    hotKeyID.id = 1;

    OSStatus err = RegisterEventHotKey(keyCode, modifiers, hotKeyID, GetApplicationEventTarget(), 0, &hotKeyRef);
    return (err == noErr);
}

bool HotkeyMonitor::initialize(std::function<void()> callback) {
    hotkeyCallback = callback;
    return registerCurrentHotkey();
}
 
bool HotkeyMonitor::update() {
    return registerCurrentHotkey();
}

void HotkeyMonitor::start() {
    running = true;
}

void HotkeyMonitor::stop() {
    running = false;
    if (hotKeyRef) {
        UnregisterEventHotKey(hotKeyRef);
        hotKeyRef = nullptr;
    }
    if (handlerRef) {
        RemoveEventHandler(handlerRef);
        handlerRef = nullptr;
    }
}

OSStatus HotkeyMonitor::hotKeyHandler(EventHandlerCallRef, EventRef inEvent, void* userData) {
    (void)inEvent;
    HotkeyMonitor* self = static_cast<HotkeyMonitor*>(userData);
    if (self && self->running && self->hotkeyCallback) {
        self->hotkeyCallback();
    }
    return noErr;
}
