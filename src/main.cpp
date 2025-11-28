#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include "Radar.h"
#include "Interface.h"
#include "AudioCapture.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Set high priority for the process
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    
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
    
    // Set callback for position changes
    ui.SetPositionCallback([&radar](RadarPosition pos)
    {
        radar.SetPosition(pos);
    });
    
    // Set callback for visibility toggle
    ui.SetToggleCallback([&radar](bool visible)
    {
        if (visible)
            radar.Show();
        else
            radar.Hide();
    });
    
    // Set callback for degrees toggle
    ui.SetDegreesCallback([&radar](bool visible)
    {
        radar.SetShowDegrees(visible);
    });
    
    // Set callback for size change
    ui.SetSizeCallback([&radar](int size)
    {
        radar.SetSize(size);
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
    });
    
    // Set callback for opacity
    ui.SetOpacityCallback([&radar](float opacity)
    {
        radar.SetOpacity(opacity);
    });
    
    // Set callback for audio capture toggle
    ui.SetAudioCaptureCallback([&audio](bool enabled)
    {
        if (enabled)
            audio.Start();
        else
            audio.Stop();
    });
    
    // Set audio direction callback
    audio.SetCallback([&radar](const SoundDirection& dir)
    {
        // Add signature at detected sound direction
        radar.AddSignature(dir.angle, dir.magnitude, dir.magnitude);
    });
    
    // Show both windows
    radar.Show();
    ui.Show();
    
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
