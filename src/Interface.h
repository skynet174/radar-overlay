#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <functional>
#include <string>
#include "Radar.h"

struct AppSettings;

class Interface
{
public:
    using PositionCallback = std::function<void(RadarPosition)>;
    using ToggleCallback = std::function<void(bool)>;
    using SizeCallback = std::function<void(int)>;

    Interface();
    ~Interface();

    bool Initialize(HINSTANCE hInstance);
    void Shutdown();
    
    void Show();
    void Hide();
    bool IsVisible() const { return m_Visible; }
    
    void SetPositionCallback(PositionCallback callback) { m_PositionCallback = callback; }
    void SetToggleCallback(ToggleCallback callback) { m_ToggleCallback = callback; }
    void SetDegreesCallback(ToggleCallback callback) { m_DegreesCallback = callback; }
    void SetSweepCallback(ToggleCallback callback) { m_SweepCallback = callback; }
    void SetMultiSourceCallback(ToggleCallback callback) { m_MultiSourceCallback = callback; }
    void SetSizeCallback(SizeCallback callback) { m_SizeCallback = callback; }
    void SetOpacityCallback(std::function<void(float)> callback) { m_OpacityCallback = callback; }
    void SetSignatureCallback(std::function<void()> callback) { m_SignatureCallback = callback; }
    void SetEchoCallback(std::function<void(int)> callback) { m_EchoCallback = callback; }
    void SetAudioCaptureCallback(ToggleCallback callback) { m_AudioCaptureCallback = callback; }
    
    void Update();
    bool IsRunning() const { return m_Running; }
    
    void SetCaptureDeviceInfo(const char* name, int channels);
    void ApplySettings(const AppSettings& settings);

private:
    bool CreateMenuWindow(HINSTANCE hInstance);
    void OnButtonClick(int buttonId);
    void GetAudioDeviceInfo();
    void CheckDeviceUpdate();
    void UpdateDeviceLabels();
    std::wstring GetChannelLayoutName(int channels);
    
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static constexpr int BTN_TOP_LEFT = 1001;
    static constexpr int BTN_TOP_RIGHT = 1002;
    static constexpr int BTN_BOTTOM_LEFT = 1003;
    static constexpr int BTN_BOTTOM_RIGHT = 1004;
    static constexpr int BTN_TOGGLE = 1005;
    static constexpr int BTN_DEGREES = 1006;
    static constexpr int BTN_SIZE_RESET = 1007;
    static constexpr int BTN_RANDOM_SIG = 1008;
    static constexpr int BTN_ECHO_PING = 1009;
    static constexpr int BTN_ECHO_TRAIL = 1010;
    static constexpr int BTN_ECHO_RIPPLE = 1011;
    static constexpr int BTN_ECHO_LINE = 1012;
    static constexpr int BTN_ECHO_HEX = 1013;
    static constexpr int BTN_ECHO_ARC = 1014;
    static constexpr int BTN_ECHO_CONE = 1015;
    static constexpr int BTN_ECHO_PULSE = 1016;
    static constexpr int BTN_AUDIO_CAPTURE = 1017;
    static constexpr int BTN_RESTART_AUDIO = 1018;
    static constexpr int BTN_SWEEP = 1019;
    static constexpr int BTN_MULTI_SOURCE = 1020;

    HWND m_Hwnd = nullptr;
    HWND m_GroupAudio = nullptr;
    HWND m_LabelDevice = nullptr;
    HWND m_LabelChannels = nullptr;
    HWND m_GroupPosition = nullptr;
    HWND m_BtnTopLeft = nullptr;
    HWND m_BtnTopRight = nullptr;
    HWND m_BtnBottomLeft = nullptr;
    HWND m_BtnBottomRight = nullptr;
    HWND m_BtnToggle = nullptr;
    HWND m_BtnDegrees = nullptr;
    HWND m_BtnSweep = nullptr;
    HWND m_BtnMultiSource = nullptr;
    HWND m_BtnRandomSig = nullptr;
    HWND m_GroupEcho = nullptr;
    HWND m_BtnEchoPing = nullptr;
    HWND m_BtnEchoTrail = nullptr;
    HWND m_BtnEchoRipple = nullptr;
    HWND m_BtnEchoLine = nullptr;
    HWND m_BtnEchoHex = nullptr;
    HWND m_BtnEchoArc = nullptr;
    HWND m_BtnEchoCone = nullptr;
    HWND m_BtnEchoPulse = nullptr;
    HWND m_BtnAudioCapture = nullptr;
    HWND m_BtnRestartAudio = nullptr;
    HWND m_GroupOpacity = nullptr;
    HWND m_SliderOpacity = nullptr;
    HWND m_LabelOpacity = nullptr;
    HWND m_GroupSize = nullptr;
    HWND m_SliderSize = nullptr;
    HWND m_BtnSizeReset = nullptr;
    
    static constexpr int DEFAULT_RADAR_SIZE = 400;
    
    int m_Width = 340;
    int m_Height = 720;
    int m_RadarSize = 400;
    
    bool m_Visible = false;
    bool m_Running = true;
    bool m_RadarVisible = true;
    bool m_DegreesVisible = true;
    bool m_SweepVisible = true;
    bool m_MultiSourceEnabled = true;
    bool m_AudioCaptureEnabled = false;
    
    std::wstring m_DeviceName;
    int m_Channels = 0;
    
    DWORD m_LastDeviceCheck = 0;
    static constexpr DWORD DEVICE_CHECK_INTERVAL = 3000; // 3 seconds
    
    PositionCallback m_PositionCallback;
    ToggleCallback m_ToggleCallback;
    ToggleCallback m_DegreesCallback;
    ToggleCallback m_SweepCallback;
    ToggleCallback m_MultiSourceCallback;
    SizeCallback m_SizeCallback;
    std::function<void(float)> m_OpacityCallback;
    std::function<void()> m_SignatureCallback;
    std::function<void(int)> m_EchoCallback;
    ToggleCallback m_AudioCaptureCallback;
};

