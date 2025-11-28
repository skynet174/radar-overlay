#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <cmath>
#include "Radar.h"
#include "Interface.h"
#include "AudioCapture.h"
#include "Settings.h"

// Global settings for saving on exit
static AppSettings g_Settings;
static std::wstring g_SettingsPath;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Set high priority for the process
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    
    // Load settings
    g_SettingsPath = Settings::GetSettingsPath();
    Settings::Load(g_SettingsPath, g_Settings);
    
    // Create components
    Radar radar;
    Interface ui;
    AudioCapture audio;
    
    // Initialize radar
    if (!radar.Initialize(hInstance))
    {
        MessageBoxW(nullptr, L"Failed to initialize Radar", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Initialize interface
    if (!ui.Initialize(hInstance))
    {
        MessageBoxW(nullptr, L"Failed to initialize Interface", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Initialize audio capture
    if (!audio.Initialize())
    {
        MessageBoxW(nullptr, L"Failed to initialize Audio Capture.\nMake sure you have an audio device.", L"Warning", MB_OK | MB_ICONWARNING);
    }
    else
    {
        // Update interface with actual capture device info
        ui.SetCaptureDeviceInfo(audio.GetDeviceName(), audio.GetChannelCount());
    }
    
    // Apply loaded settings to radar
    radar.SetPosition(g_Settings.position);
    radar.SetSize(g_Settings.size);
    radar.SetOpacity(g_Settings.opacity);
    radar.SetShowDegrees(g_Settings.showDegrees);
    radar.SetShowSweep(g_Settings.showSweep);
    radar.SetEchoType(static_cast<EchoType>(g_Settings.echoType));
    radar.SetMultiSource(g_Settings.multiSource);
    
    // Apply loaded settings to interface
    ui.ApplySettings(g_Settings);
    
    // Set callback for position changes
    ui.SetPositionCallback([&radar](RadarPosition pos)
    {
        radar.SetPosition(pos);
        g_Settings.position = pos;
        Settings::Save(g_SettingsPath, g_Settings);
    });
    
    // Set callback for visibility toggle
    ui.SetToggleCallback([&radar](bool visible)
    {
        if (visible)
            radar.Show();
        else
            radar.Hide();
        g_Settings.radarVisible = visible;
        Settings::Save(g_SettingsPath, g_Settings);
    });
    
    // Set callback for degrees toggle
    ui.SetDegreesCallback([&radar](bool visible)
    {
        radar.SetShowDegrees(visible);
        g_Settings.showDegrees = visible;
        Settings::Save(g_SettingsPath, g_Settings);
    });
    
    // Set callback for sweep toggle
    ui.SetSweepCallback([&radar](bool visible)
    {
        radar.SetShowSweep(visible);
        g_Settings.showSweep = visible;
        Settings::Save(g_SettingsPath, g_Settings);
    });
    
    // Set callback for multi-source toggle
    ui.SetMultiSourceCallback([&radar](bool multi)
    {
        radar.SetMultiSource(multi);
        g_Settings.multiSource = multi;
        Settings::Save(g_SettingsPath, g_Settings);
    });
    
    // Set callback for size change
    ui.SetSizeCallback([&radar](int size)
    {
        radar.SetSize(size);
        g_Settings.size = size;
        Settings::Save(g_SettingsPath, g_Settings);
    });
    
    // Set callback for random signature
    ui.SetSignatureCallback([&radar]()
    {
        radar.AddRandomSignature();
    });
    
    // Set callback for echo type
    ui.SetEchoCallback([&radar](int type)
    {
        radar.SetEchoType(static_cast<EchoType>(type));
        g_Settings.echoType = type;
        Settings::Save(g_SettingsPath, g_Settings);
    });
    
    // Set callback for opacity
    ui.SetOpacityCallback([&radar](float opacity)
    {
        radar.SetOpacity(opacity);
        g_Settings.opacity = opacity;
        Settings::Save(g_SettingsPath, g_Settings);
    });
    
    // Set callback for audio capture toggle
    ui.SetAudioCaptureCallback([&audio](bool enabled)
    {
        if (enabled)
            audio.Start();
        else
            audio.Stop();
        g_Settings.audioCaptureEnabled = enabled;
        Settings::Save(g_SettingsPath, g_Settings);
    });
    
    // Set multi-source audio callback
    audio.SetMultiSourceCallback([&radar](const SoundSources& sources)
    {
        float angles[8], distances[8], intensities[8];
        
        for (int i = 0; i < sources.count && i < 8; i++)
        {
            angles[i] = sources.sources[i].angle;
            
            // Invert distance: loud = close to center, quiet = far from center
            float mag = sources.sources[i].magnitude;
            distances[i] = 0.9f - (mag * 0.75f);
            distances[i] = fmaxf(0.1f, fminf(0.9f, distances[i]));
            
            // intensity based on magnitude (loud = more visible)
            intensities[i] = fminf(1.0f, mag * 1.5f);
        }
        
        radar.UpdateAudioSources(sources.count, angles, distances, intensities);
    });
    
    // Show windows based on settings
    if (g_Settings.radarVisible)
        radar.Show();
    ui.Show();
    
    // Start audio if was enabled
    if (g_Settings.audioCaptureEnabled)
        audio.Start();
    
    // Main loop - only handle messages, rendering is in separate thread
    while (radar.IsRunning() && ui.IsRunning())
    {
        radar.Update();
        ui.Update();
        Sleep(1);  // Minimal sleep to not hog CPU
    }
    
    // Cleanup
    audio.Shutdown();
    radar.Shutdown();
    ui.Shutdown();
    
    return 0;
}
