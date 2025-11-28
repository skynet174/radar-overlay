#include "Settings.h"
#include <fstream>
#include <sstream>
#include <ShlObj.h>

std::wstring Settings::GetSettingsPath()
{
    wchar_t exePath[MAX_PATH];
    
    // Get path to exe
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    
    // Remove exe filename, keep directory
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
    {
        path = path.substr(0, lastSlash + 1);
    }
    
    return path + L"settings.ini";
}

bool Settings::Load(const std::wstring& path, AppSettings& settings)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;
    
    std::string line;
    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == '[')
            continue;
        
        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        
        // Trim whitespace
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(0, 1);
        
        if (key == "position")
            settings.position = static_cast<RadarPosition>(std::stoi(value));
        else if (key == "size")
            settings.size = std::stoi(value);
        else if (key == "opacity")
            settings.opacity = std::stof(value);
        else if (key == "showDegrees")
            settings.showDegrees = (value == "1" || value == "true");
        else if (key == "showSweep")
            settings.showSweep = (value == "1" || value == "true");
        else if (key == "radarVisible")
            settings.radarVisible = (value == "1" || value == "true");
        else if (key == "echoType")
            settings.echoType = std::stoi(value);
        else if (key == "multiSource")
            settings.multiSource = (value == "1" || value == "true");
        else if (key == "audioCaptureEnabled")
            settings.audioCaptureEnabled = (value == "1" || value == "true");
    }
    
    file.close();
    return true;
}

bool Settings::Save(const std::wstring& path, const AppSettings& settings)
{
    std::ofstream file(path);
    if (!file.is_open())
        return false;
    
    file << "# Radar Overlay Settings\n";
    file << "[Radar]\n";
    file << "position=" << static_cast<int>(settings.position) << "\n";
    file << "size=" << settings.size << "\n";
    file << "opacity=" << settings.opacity << "\n";
    file << "showDegrees=" << (settings.showDegrees ? "1" : "0") << "\n";
    file << "showSweep=" << (settings.showSweep ? "1" : "0") << "\n";
    file << "radarVisible=" << (settings.radarVisible ? "1" : "0") << "\n";
    file << "echoType=" << settings.echoType << "\n";
    file << "multiSource=" << (settings.multiSource ? "1" : "0") << "\n";
    file << "\n[Audio]\n";
    file << "audioCaptureEnabled=" << (settings.audioCaptureEnabled ? "1" : "0") << "\n";
    
    file.close();
    return true;
}

