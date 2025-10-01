#include "Settings.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <mach-o/dyld.h>
#include <limits.h>

#include "Constants.h"

Settings::Settings() : model(MODEL_TINY), bestOfN(constants::kBestOfNDefault), hotkey(constants::kDefaultHotkey), deviceId(-1), language("en"), retainSeconds(constants::kRetainSecondsDefault) {
    const char* home = std::getenv("HOME");
    if (home) {
        configPath = std::string(home) + "/.rose_config";
    } else {
        configPath = ".rose_config";
    }
}

Settings& Settings::getInstance() {
    static Settings instance;
    return instance;
}

void Settings::load() {
    std::ifstream file(configPath);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "model") {
            int m = std::stoi(value);
            if (m >= MODEL_TINY && m <= MODEL_LARGE) {
                model = static_cast<Model>(m);
            }
        } else if (key == "bestOfN") {
            int n = std::stoi(value);
            if (n >= constants::kBestOfNMin && n <= constants::kBestOfNMax) {
                bestOfN = n;
            }
        } else if (key == "hotkey") {
            hotkey = value;
        } else if (key == "deviceId") {
            deviceId = std::stoi(value);
        } else if (key == "language") {
            if (!value.empty()) language = value;
        } else if (key == "retainSeconds") {
            int s = std::stoi(value);
            if (s >= constants::kRetainSecondsMin && s <= constants::kRetainSecondsMax) {
                retainSeconds = s;
            }
        }
    }
}

void Settings::save() {
    std::ofstream file(configPath);
    if (!file.is_open()) return;

    file << "model=" << static_cast<int>(model) << "\n";
    file << "bestOfN=" << bestOfN << "\n";
    file << "hotkey=" << hotkey << "\n";
    file << "deviceId=" << deviceId << "\n";
    file << "language=" << language << "\n";
    file << "retainSeconds=" << retainSeconds << "\n";
}

void Settings::setModel(Model m) {
    if (model != m) {
        model = m;
        save();
        notifyChange();
    }
}

void Settings::setBestOfN(int n) {
    if (n >= constants::kBestOfNMin && n <= constants::kBestOfNMax && bestOfN != n) {
        bestOfN = n;
        save();
        notifyChange();
    }
}

void Settings::setHotkey(const std::string& key) {
    if (hotkey == key) return;
    bool allowed = false;
    for (const auto& kv : constants::HotkeyOptions()) {
        if (kv.first == key) { allowed = true; break; }
    }
    if (!allowed) return;
    hotkey = key;
    save();
    notifyChange();
}

void Settings::setDeviceId(int id) {
    if (deviceId != id) {
        deviceId = id;
        save();
        notifyChange();
    }
}

void Settings::setLanguage(const std::string& lang) {
    if (lang.empty() || language == lang) return;
    language = lang;
    save();
    notifyChange();
}


std::string Settings::getModelPath() const {
    namespace fs = std::filesystem;

    auto candidates_for_model = [this]() -> std::vector<std::string> {
        switch (model) {
            case MODEL_TINY:   return {"ggml-tiny.en.bin",   "ggml-tiny.bin"};
            case MODEL_BASE:   return {"ggml-base.en.bin",   "ggml-base.bin"};
            case MODEL_SMALL:  return {"ggml-small.en.bin",  "ggml-small.bin"};
            case MODEL_MEDIUM: return {"ggml-medium.en.bin", "ggml-medium.bin"};
            case MODEL_LARGE:  return {"ggml-large-v3.bin",  "ggml-large.bin", "ggml-large-v2.bin"};
            default:           return {"ggml-tiny.en.bin",   "ggml-tiny.bin"};
        }
    }();

    auto prefix_for_model = [this]() -> std::string {
        switch (model) {
            case MODEL_TINY:   return "ggml-tiny";
            case MODEL_BASE:   return "ggml-base";
            case MODEL_SMALL:  return "ggml-small";
            case MODEL_MEDIUM: return "ggml-medium";
            case MODEL_LARGE:  return "ggml-large";
            default:           return "ggml-tiny";
        }
    }();

    auto find_in_dir = [&](const fs::path& dir) -> std::string {
        
        for (const auto& name : candidates_for_model) {
            fs::path p = dir / name;
            if (fs::exists(p)) return fs::absolute(p).string();
        }
        
        if (fs::exists(dir) && fs::is_directory(dir)) {
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                auto fname = entry.path().filename().string();
                if (fname.rfind(prefix_for_model, 0) == 0 && entry.path().extension() == ".bin") {
                    return fs::absolute(entry.path()).string();
                }
            }
        }
        return {};
    };

    
    if (auto p = find_in_dir(fs::path("models")); !p.empty()) return p;

    
    char exePath[PATH_MAX];
    uint32_t sz = sizeof(exePath);
    if (_NSGetExecutablePath(exePath, &sz) == 0) {
        fs::path exec = fs::path(exePath);
        fs::path resModelsDir = exec.parent_path().parent_path() / "Resources" / "models";
        if (auto p = find_in_dir(resModelsDir); !p.empty()) return p;
    }

    
    return (fs::path("models") / candidates_for_model.front()).string();
}

std::string Settings::getModelName() const {
    switch (model) {
        case MODEL_TINY: return "Tiny (Fast)";
        case MODEL_BASE: return "Base (Balanced)";
        case MODEL_SMALL: return "Small (Quality)";
        case MODEL_MEDIUM: return "Medium (Quality+)";
        case MODEL_LARGE: return "Large (Max)";
        default: return "Tiny";
    }
}

void Settings::notifyChange() {
    if (onChangeCallback) {
        onChangeCallback();
    }
}

void Settings::setModelRetainSeconds(int seconds) {
    if (seconds < constants::kRetainSecondsMin || seconds > constants::kRetainSecondsMax) return;
    if (retainSeconds != seconds) {
        retainSeconds = seconds;
        save();
        notifyChange();
    }
}
