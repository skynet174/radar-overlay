#pragma once

#include <string>
#include "Radar.h"

struct AppSettings
{
    // Radar
    RadarPosition position = RadarPosition::TopLeft;
    int size = 400;
    float opacity = 1.0f;
    bool showDegrees = true;
    bool showSweep = true;
    bool radarVisible = true;
    int echoType = 0;
    bool multiSource = true;
    
    // Audio
    bool audioCaptureEnabled = false;
};

class Settings
{
public:
    static bool Load(const std::wstring& path, AppSettings& settings);
    static bool Save(const std::wstring& path, const AppSettings& settings);
    static std::wstring GetSettingsPath();
};

