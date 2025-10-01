#pragma once

#include <string>
#include <functional>

class Settings {
public:
    enum Model {
        MODEL_TINY = 0,
        MODEL_BASE = 1,
        MODEL_SMALL = 2,
        MODEL_MEDIUM = 3,
        MODEL_LARGE = 4
    };

    static Settings& getInstance();

    void load();
    void save();

    Model getModel() const { return model; }
    void setModel(Model m);

    int getBestOfN() const { return bestOfN; }
    void setBestOfN(int n);

    std::string getHotkey() const { return hotkey; }
    void setHotkey(const std::string& key);

    int getDeviceId() const { return deviceId; }
    void setDeviceId(int id);

    std::string getModelPath() const;
    std::string getModelName() const;

    std::string getLanguage() const { return language; }
    void setLanguage(const std::string& lang);

    int getModelRetainSeconds() const { return retainSeconds; }
    void setModelRetainSeconds(int seconds);

    void setOnChangeCallback(std::function<void()> callback) {
        onChangeCallback = callback;
    }

private:
    Settings();
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    Model model;
    int bestOfN;
    std::string hotkey;
    int deviceId;
    std::string configPath;
    std::function<void()> onChangeCallback;

    void notifyChange();

    std::string language;
    int retainSeconds;
};
