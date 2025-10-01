#pragma once

#include <functional>

class MenuBarUI {
public:
    MenuBarUI();
    ~MenuBarUI();

    bool initialize(std::function<void()> quitCallback,
                   std::function<void()> settingsChangeCallback,
                   std::function<void()> toggleRecordCallback);
    void setRecordingState(bool recording);
    void updateMenu();
    void run();

private:
    void* statusItem;
    void* delegate;
    std::function<void()> onQuit;
    std::function<void()> onSettingsChange;
    std::function<void()> onToggleRecord;
};
