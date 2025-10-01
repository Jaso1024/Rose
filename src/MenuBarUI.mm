#include "MenuBarUI.h"
#include "Settings.h"
#include "Constants.h"
#include "AudioRecorder.h"
#include <Cocoa/Cocoa.h>

static NSMenu* BuildModelMenu(id target) {
    NSMenu* modelMenu = [[NSMenu alloc] init];
    Settings::Model currentModel = Settings::getInstance().getModel();

    NSMenuItem* tinyItem = [[NSMenuItem alloc] initWithTitle:@"Tiny (Fast)" action:@selector(selectTinyModel:) keyEquivalent:@""];
    [tinyItem setTarget:target];
    [tinyItem setState:(currentModel == Settings::MODEL_TINY ? NSControlStateValueOn : NSControlStateValueOff)];
    [modelMenu addItem:tinyItem];

    NSMenuItem* baseItem = [[NSMenuItem alloc] initWithTitle:@"Base (Balanced)" action:@selector(selectBaseModel:) keyEquivalent:@""];
    [baseItem setTarget:target];
    [baseItem setState:(currentModel == Settings::MODEL_BASE ? NSControlStateValueOn : NSControlStateValueOff)];
    [modelMenu addItem:baseItem];

    NSMenuItem* smallItem = [[NSMenuItem alloc] initWithTitle:@"Small (Quality)" action:@selector(selectSmallModel:) keyEquivalent:@""];
    [smallItem setTarget:target];
    [smallItem setState:(currentModel == Settings::MODEL_SMALL ? NSControlStateValueOn : NSControlStateValueOff)];
    [modelMenu addItem:smallItem];

    NSMenuItem* mediumItem = [[NSMenuItem alloc] initWithTitle:@"Medium (Quality+)" action:@selector(selectMediumModel:) keyEquivalent:@""];
    [mediumItem setTarget:target];
    [mediumItem setState:(currentModel == Settings::MODEL_MEDIUM ? NSControlStateValueOn : NSControlStateValueOff)];
    [modelMenu addItem:mediumItem];

    NSMenuItem* largeItem = [[NSMenuItem alloc] initWithTitle:@"Large (Max)" action:@selector(selectLargeModel:) keyEquivalent:@""];
    [largeItem setTarget:target];
    [largeItem setState:(currentModel == Settings::MODEL_LARGE ? NSControlStateValueOn : NSControlStateValueOff)];
    [modelMenu addItem:largeItem];

    return modelMenu;
}

static NSMenu* BuildBestOfMenu(id target) {
    NSMenu* bestOfMenu = [[NSMenu alloc] init];
    int currentBestOfN = Settings::getInstance().getBestOfN();
    for (int i = constants::kBestOfNMin; i <= constants::kBestOfNMax; i++) {
        NSString* title = [NSString stringWithFormat:@"%d", i];
        if (i == constants::kBestOfNMin) title = @"1 (Fastest)";
        if (i == constants::kBestOfNDefault) title = @"5 (Balanced)";
        if (i == constants::kBestOfNMax) title = @"10 (Best Quality)";
        NSMenuItem* nItem = [[NSMenuItem alloc] initWithTitle:title action:@selector(setBestOfN:) keyEquivalent:@""];
        [nItem setTarget:target];
        [nItem setTag:i];
        [nItem setState:(i == currentBestOfN ? NSControlStateValueOn : NSControlStateValueOff)];
        [bestOfMenu addItem:nItem];
    }
    return bestOfMenu;
}

static NSMenu* BuildDeviceMenu(id target) {
    NSMenu* deviceMenu = [[NSMenu alloc] init];
    int currentDeviceId = Settings::getInstance().getDeviceId();
    std::vector<AudioDevice> devices = AudioRecorder::getInputDevices();

    NSMenuItem* defaultItem = [[NSMenuItem alloc] initWithTitle:@"System Default" action:@selector(selectDevice:) keyEquivalent:@""];
    [defaultItem setTarget:target];
    [defaultItem setTag:-1];
    [defaultItem setState:(currentDeviceId == -1 ? NSControlStateValueOn : NSControlStateValueOff)];
    [deviceMenu addItem:defaultItem];

    if (!devices.empty()) {
        [deviceMenu addItem:[NSMenuItem separatorItem]];
        for (const auto& device : devices) {
            NSString* name = [NSString stringWithUTF8String:device.name.c_str()];
            NSMenuItem* devItem = [[NSMenuItem alloc] initWithTitle:name action:@selector(selectDevice:) keyEquivalent:@""];
            [devItem setTarget:target];
            [devItem setTag:device.id];
            [devItem setState:(device.id == currentDeviceId ? NSControlStateValueOn : NSControlStateValueOff)];
            [deviceMenu addItem:devItem];
        }
    }
    return deviceMenu;
}

static NSMenu* BuildHotkeyMenu(id target) {
    NSMenu* hotkeyMenu = [[NSMenu alloc] init];
    std::string currentHotkey = Settings::getInstance().getHotkey();
    const auto& opts = constants::HotkeyOptions();
    for (size_t i = 0; i < opts.size(); ++i) {
        NSString* title = [NSString stringWithUTF8String:opts[i].second.c_str()];
        NSString* value = [NSString stringWithUTF8String:opts[i].first.c_str()];
        NSMenuItem* hkItem = [[NSMenuItem alloc] initWithTitle:title action:@selector(setHotkey:) keyEquivalent:@""];
        [hkItem setTarget:target];
        [hkItem setRepresentedObject:value];
        [hkItem setState:([value isEqualToString:[NSString stringWithUTF8String:currentHotkey.c_str()]]
                        ? NSControlStateValueOn : NSControlStateValueOff)];
        [hotkeyMenu addItem:hkItem];
    }
    return hotkeyMenu;
}

static NSMenu* BuildRetainMenu(id target) {
    NSMenu* retainMenu = [[NSMenu alloc] init];
    int current = Settings::getInstance().getModelRetainSeconds();
    int options[] = {0, 5, 10, 30, 60};
    int count = sizeof(options)/sizeof(options[0]);
    for (int i = 0; i < count; ++i) {
        int sec = options[i];
        NSString* title = [NSString stringWithFormat:@"%ds", sec];
        if (sec == constants::kRetainSecondsDefault) {
            title = [NSString stringWithFormat:@"%ds (Default)", sec];
        }
        NSMenuItem* it = [[NSMenuItem alloc] initWithTitle:title action:@selector(setRetainSeconds:) keyEquivalent:@""];
        [it setTarget:target];
        [it setTag:sec];
        [it setState:(sec == current ? NSControlStateValueOn : NSControlStateValueOff)];
        [retainMenu addItem:it];
    }
    return retainMenu;
}

static NSMenu* BuildLanguageMenu(id target) {
    NSMenu* languageMenu = [[NSMenu alloc] init];
    std::string current = Settings::getInstance().getLanguage();
    const auto& langs = constants::LanguageOptions();
    for (size_t i = 0; i < langs.size(); ++i) {
        NSString* title = [NSString stringWithUTF8String:langs[i].second.c_str()];
        NSString* value = [NSString stringWithUTF8String:langs[i].first.c_str()];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:@selector(setLanguage:) keyEquivalent:@""];
        [item setTarget:target];
        [item setRepresentedObject:value];
        BOOL on = ([value isEqualToString:[NSString stringWithUTF8String:current.c_str()]]);
        [item setState:(on ? NSControlStateValueOn : NSControlStateValueOff)];
        [languageMenu addItem:item];
    }
    return languageMenu;
}

@interface StatusBarDelegate : NSObject {
    std::function<void()> quitCallback;
    std::function<void()> settingsChangeCallback;
    std::function<void()> toggleRecordCallback;
}
- (void)setQuitCallback:(std::function<void()>)callback;
- (void)setSettingsChangeCallback:(std::function<void()>)callback;
- (void)setToggleRecordCallback:(std::function<void()>)callback;
- (void)quit:(id)sender;
- (void)toggleRecording:(id)sender;
- (void)selectTinyModel:(id)sender;
- (void)selectBaseModel:(id)sender;
- (void)selectSmallModel:(id)sender;
- (void)selectMediumModel:(id)sender;
- (void)selectLargeModel:(id)sender;
- (void)setBestOfN:(id)sender;
- (void)selectDevice:(id)sender;
  - (void)setHotkey:(id)sender;
  - (void)setLanguage:(id)sender;
  - (void)setRetainSeconds:(id)sender;
@end

@implementation StatusBarDelegate
- (void)setQuitCallback:(std::function<void()>)callback {
    quitCallback = callback;
}

- (void)setSettingsChangeCallback:(std::function<void()>)callback {
    settingsChangeCallback = callback;
}

- (void)setToggleRecordCallback:(std::function<void()>)callback {
    toggleRecordCallback = callback;
}

- (void)quit:(id)sender {
    (void)sender;
    if (quitCallback) {
        quitCallback();
    }
}

- (void)toggleRecording:(id)sender {
    (void)sender;
    if (toggleRecordCallback) {
        toggleRecordCallback();
    }
}

- (void)selectTinyModel:(id)sender {
    (void)sender;
    Settings::getInstance().setModel(Settings::MODEL_TINY);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
}

- (void)selectBaseModel:(id)sender {
    (void)sender;
    Settings::getInstance().setModel(Settings::MODEL_BASE);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
}

- (void)selectSmallModel:(id)sender {
    (void)sender;
    Settings::getInstance().setModel(Settings::MODEL_SMALL);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
}

- (void)selectMediumModel:(id)sender {
    (void)sender;
    Settings::getInstance().setModel(Settings::MODEL_MEDIUM);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
}

- (void)selectLargeModel:(id)sender {
    (void)sender;
    Settings::getInstance().setModel(Settings::MODEL_LARGE);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
}

- (void)setBestOfN:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    int value = (int)[item tag];
    Settings::getInstance().setBestOfN(value);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
}

- (void)selectDevice:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    int deviceId = (int)[item tag];
    Settings::getInstance().setDeviceId(deviceId);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
}

  - (void)setHotkey:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    NSString* hotkey = [item representedObject];
    Settings::getInstance().setHotkey([hotkey UTF8String]);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
  }

  - (void)setLanguage:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    NSString* lang = [item representedObject];
    Settings::getInstance().setLanguage([lang UTF8String]);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
  }

- (void)setRetainSeconds:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    int sec = (int)[item tag];
    Settings::getInstance().setModelRetainSeconds(sec);
    if (settingsChangeCallback) {
        settingsChangeCallback();
    }
}
@end

MenuBarUI::MenuBarUI() : statusItem(nullptr), delegate(nullptr) {}

MenuBarUI::~MenuBarUI() {
    @autoreleasepool {
        if (statusItem) {
            NSStatusItem* item = (__bridge_transfer NSStatusItem*)statusItem;
            [[NSStatusBar systemStatusBar] removeStatusItem:item];
            statusItem = nullptr;
        }
        if (delegate) {
            id del = (__bridge_transfer id)delegate;
            (void)del;
            delegate = nullptr;
        }
    }
}

bool MenuBarUI::initialize(std::function<void()> quitCallback,
                          std::function<void()> settingsChangeCallback,
                          std::function<void()> toggleRecordCallback) {
    @autoreleasepool {
        onQuit = quitCallback;
        onSettingsChange = settingsChangeCallback;
        onToggleRecord = toggleRecordCallback;

        NSStatusBar* statusBar = [NSStatusBar systemStatusBar];
        NSStatusItem* item = [statusBar statusItemWithLength:NSVariableStatusItemLength];

        StatusBarDelegate* del = [[StatusBarDelegate alloc] init];
        [del setQuitCallback:quitCallback];
        [del setSettingsChangeCallback:settingsChangeCallback];
        [del setToggleRecordCallback:toggleRecordCallback];
        delegate = (__bridge_retained void*)del;

        NSStatusBarButton* button = [item button];
        [button setTitle:@"IDLE"];

        statusItem = (__bridge_retained void*)item;

        updateMenu();

        return true;
    }
}

void MenuBarUI::updateMenu() {
    @autoreleasepool {
        if (!statusItem) return;

        NSStatusItem* item = (__bridge NSStatusItem*)statusItem;
        StatusBarDelegate* del = (__bridge StatusBarDelegate*)delegate;

        NSMenu* menu = [[NSMenu alloc] init];
        [menu addItemWithTitle:@"Rose" action:nil keyEquivalent:@""];
        [menu addItem:[NSMenuItem separatorItem]];

        NSMenuItem* statusMenuItem = [[NSMenuItem alloc] initWithTitle:@"Idle"
                                                                 action:nil
                                                          keyEquivalent:@""];
        [statusMenuItem setTag:1];
        [menu addItem:statusMenuItem];

        [menu addItem:[NSMenuItem separatorItem]];

        NSMenuItem* toggleItem = [[NSMenuItem alloc] initWithTitle:@"Start Recording"
                                                            action:@selector(toggleRecording:)
                                                     keyEquivalent:@""];
        [toggleItem setTarget:del];
        [toggleItem setTag:2];
        [menu addItem:toggleItem];

        [menu addItem:[NSMenuItem separatorItem]];

        NSMenuItem* modelItem = [[NSMenuItem alloc] initWithTitle:@"Model" action:nil keyEquivalent:@""];
        NSMenu* modelMenu = BuildModelMenu(del);
        [modelItem setSubmenu:modelMenu];
        [menu addItem:modelItem];

        NSMenuItem* bestOfItem = [[NSMenuItem alloc] initWithTitle:@"Best of N" action:nil keyEquivalent:@""];
        NSMenu* bestOfMenu = BuildBestOfMenu(del);
        [bestOfItem setSubmenu:bestOfMenu];
        [menu addItem:bestOfItem];

        NSMenuItem* deviceItem = [[NSMenuItem alloc] initWithTitle:@"Audio Device" action:nil keyEquivalent:@""];
        NSMenu* deviceMenu = BuildDeviceMenu(del);
        [deviceItem setSubmenu:deviceMenu];
        [menu addItem:deviceItem];

        NSMenuItem* hotkeyItem = [[NSMenuItem alloc] initWithTitle:@"Hotkey" action:nil keyEquivalent:@""];
        NSMenu* hotkeyMenu = BuildHotkeyMenu(del);
        [hotkeyItem setSubmenu:hotkeyMenu];
        [menu addItem:hotkeyItem];

        NSMenuItem* languageItem = [[NSMenuItem alloc] initWithTitle:@"Language" action:nil keyEquivalent:@""];
        NSMenu* languageMenu = BuildLanguageMenu(del);
        [languageItem setSubmenu:languageMenu];
        [menu addItem:languageItem];

        NSMenuItem* retainItem = [[NSMenuItem alloc] initWithTitle:@"Model Retention" action:nil keyEquivalent:@""];
        NSMenu* retainMenu = BuildRetainMenu(del);
        [retainItem setSubmenu:retainMenu];
        [menu addItem:retainItem];

        [menu addItem:[NSMenuItem separatorItem]];

        NSMenuItem* quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                           action:@selector(quit:)
                                                    keyEquivalent:@"q"];
        [quitItem setTarget:del];
        [menu addItem:quitItem];

        [item setMenu:menu];
    }
}

void MenuBarUI::setRecordingState(bool recording) {
    @autoreleasepool {
        if (!statusItem) return;

        NSStatusItem* item = (__bridge NSStatusItem*)statusItem;
        NSStatusBarButton* button = [item button];

        if (recording) {
            [button setTitle:@"REC"];
            NSMenuItem* statusMenuItem = [[item menu] itemWithTag:1];
            [statusMenuItem setTitle:@"Recording"];
            NSMenuItem* toggleItem = [[item menu] itemWithTag:2];
            [toggleItem setTitle:@"Stop Recording"];
        } else {
            [button setTitle:@"IDLE"];
            NSMenuItem* statusMenuItem = [[item menu] itemWithTag:1];
            [statusMenuItem setTitle:@"Idle"];
            NSMenuItem* toggleItem = [[item menu] itemWithTag:2];
            [toggleItem setTitle:@"Start Recording"];
        }
    }
}

void MenuBarUI::run() {
    @autoreleasepool {
        [[NSApplication sharedApplication] run];
    }
}
