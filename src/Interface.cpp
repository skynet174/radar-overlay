#include "Interface.h"
#include <dwmapi.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <audioclient.h>
#include <commctrl.h>
#include <shellapi.h>
#include <cstdio>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comctl32.lib")

Interface::Interface()
{
}

Interface::~Interface()
{
    Shutdown();
}

bool Interface::Initialize(HINSTANCE hInstance)
{
    GetAudioDeviceInfo();
    return CreateMenuWindow(hInstance);
}

void Interface::Shutdown()
{
    if (m_Hwnd)
    {
        DestroyWindow(m_Hwnd);
        m_Hwnd = nullptr;
    }
}

void Interface::GetAudioDeviceInfo()
{
    // Device name will come from AudioCapture via SetCaptureDeviceName()
    // Just set defaults here
    if (m_DeviceName.empty())
        m_DeviceName = L"Waiting for audio...";
    if (m_Channels == 0)
        m_Channels = 2;
}

std::wstring Interface::GetChannelLayoutName(int channels)
{
    std::wstring name;
    switch (channels)
    {
    case 1: name = L"Mono"; break;
    case 2: name = L"Stereo"; break;
    case 4: name = L"Quadro"; break;
    case 6: name = L"5.1 Surround"; break;
    case 8: name = L"7.1 Surround"; break;
    default: name = L"Custom"; break;
    }
    return name + L" (" + std::to_wstring(channels) + L")";
}

bool Interface::CreateMenuWindow(HINSTANCE hInstance)
{
    const wchar_t* className = L"RadarInterfaceClass";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);
    
    // Center on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - m_Width) / 2;
    int posY = (screenHeight - m_Height) / 2;
    
    m_Hwnd = CreateWindowExW(
        0,
        className,
        L"Radar Settings",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        posX, posY, m_Width, m_Height,
        nullptr, nullptr, hInstance, nullptr);
    
    if (!m_Hwnd)
        return false;
    
    SetWindowLongPtr(m_Hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    // Enable dark mode
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(m_Hwnd, 20, &darkMode, sizeof(darkMode));
    
    // Get client area
    RECT rc;
    GetClientRect(m_Hwnd, &rc);
    int clientWidth = rc.right - rc.left;
    
    int margin = 10;
    int groupWidth = clientWidth - margin * 2;
    
    // ===== Audio Device Group =====
    m_GroupAudio = CreateWindowW(L"BUTTON", L"Capture Device",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        margin, 10, groupWidth, 80,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    // Truncate device name if too long
    std::wstring displayName = m_DeviceName;
    if (displayName.length() > 35)
        displayName = displayName.substr(0, 32) + L"...";
    
    m_LabelDevice = CreateWindowW(L"STATIC", displayName.c_str(),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        margin + 10, 30, groupWidth - 20, 20,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    std::wstring channelInfo = L"Channels: " + GetChannelLayoutName(m_Channels);
    m_LabelChannels = CreateWindowW(L"STATIC", channelInfo.c_str(),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        margin + 10, 55, groupWidth - 20, 20,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    // ===== Position Group =====
    m_GroupPosition = CreateWindowW(L"BUTTON", L"Radar Position",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        margin, 100, groupWidth, 110,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    int btnWidth = 130;
    int btnHeight = 30;
    int spacing = 10;
    int gridStartX = margin + (groupWidth - (btnWidth * 2 + spacing)) / 2;
    int gridStartY = 125;
    
    m_BtnTopLeft = CreateWindowW(L"BUTTON", L"Top Left",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        gridStartX, gridStartY, btnWidth, btnHeight,
        m_Hwnd, (HMENU)BTN_TOP_LEFT, hInstance, nullptr);
    
    m_BtnTopRight = CreateWindowW(L"BUTTON", L"Top Right",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        gridStartX + btnWidth + spacing, gridStartY, btnWidth, btnHeight,
        m_Hwnd, (HMENU)BTN_TOP_RIGHT, hInstance, nullptr);
    
    m_BtnBottomLeft = CreateWindowW(L"BUTTON", L"Bottom Left",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        gridStartX, gridStartY + btnHeight + spacing, btnWidth, btnHeight,
        m_Hwnd, (HMENU)BTN_BOTTOM_LEFT, hInstance, nullptr);
    
    m_BtnBottomRight = CreateWindowW(L"BUTTON", L"Bottom Right",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        gridStartX + btnWidth + spacing, gridStartY + btnHeight + spacing, btnWidth, btnHeight,
        m_Hwnd, (HMENU)BTN_BOTTOM_RIGHT, hInstance, nullptr);
    
    // ===== Size Group =====
    m_GroupSize = CreateWindowW(L"BUTTON", L"Radar Size",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        margin, 215, groupWidth, 60,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    // Size slider (100 - 600 pixels)
    m_SliderSize = CreateWindowW(TRACKBAR_CLASSW, nullptr,
        WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_NOTICKS,
        margin + 10, 240, groupWidth - 70, 25,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    SendMessageW(m_SliderSize, TBM_SETRANGE, TRUE, MAKELPARAM(100, 600));
    SendMessageW(m_SliderSize, TBM_SETPOS, TRUE, m_RadarSize);
    
    // Size reset button (shows current size, click to reset)
    wchar_t sizeText[32];
    swprintf_s(sizeText, L"%dpx", m_RadarSize);
    m_BtnSizeReset = CreateWindowW(L"BUTTON", sizeText,
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        margin + groupWidth - 60, 240, 55, 25,
        m_Hwnd, (HMENU)BTN_SIZE_RESET, hInstance, nullptr);
    
    // ===== Control Buttons =====
    int btnY = 285;
    int totalWidth = btnWidth * 2 + spacing;
    int btnStartX = (clientWidth - totalWidth) / 2;
    
    m_BtnToggle = CreateWindowW(L"BUTTON", L"Hide Radar",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnStartX, btnY, btnWidth, btnHeight,
        m_Hwnd, (HMENU)BTN_TOGGLE, hInstance, nullptr);
    
    m_BtnDegrees = CreateWindowW(L"BUTTON", L"Degrees\u00B0",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnStartX + btnWidth + spacing, btnY, btnWidth, btnHeight,
        m_Hwnd, (HMENU)BTN_DEGREES, hInstance, nullptr);
    
    // Random signature button
    m_BtnRandomSig = CreateWindowW(L"BUTTON", L"Random Signature",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        (clientWidth - 150) / 2, btnY + btnHeight + spacing, 150, btnHeight,
        m_Hwnd, (HMENU)BTN_RANDOM_SIG, hInstance, nullptr);
    
    // ===== Echo Type Group =====
    int echoY = btnY + btnHeight * 2 + spacing * 2;
    m_GroupEcho = CreateWindowW(L"BUTTON", L"Echo Type",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        margin, echoY, groupWidth, 50,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    int echoBtnWidth = 85;
    int echoStartX = margin + (groupWidth - echoBtnWidth * 3 - spacing * 2) / 2;
    int echoBtnY = echoY + 20;
    
    m_BtnEchoPing = CreateWindowW(L"BUTTON", L"[Ping]",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        echoStartX, echoBtnY, echoBtnWidth, 25,
        m_Hwnd, (HMENU)BTN_ECHO_PING, hInstance, nullptr);
    
    m_BtnEchoTrail = CreateWindowW(L"BUTTON", L"Trail",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        echoStartX + echoBtnWidth + spacing, echoBtnY, echoBtnWidth, 25,
        m_Hwnd, (HMENU)BTN_ECHO_TRAIL, hInstance, nullptr);
    
    m_BtnEchoRipple = CreateWindowW(L"BUTTON", L"Ripple",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        echoStartX + (echoBtnWidth + spacing) * 2, echoBtnY, echoBtnWidth, 25,
        m_Hwnd, (HMENU)BTN_ECHO_RIPPLE, hInstance, nullptr);
    
    // Audio capture button
    int audioBtnY = echoY + 60;
    m_BtnAudioCapture = CreateWindowW(L"BUTTON", L"Start Audio Capture",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        (clientWidth - 180) / 2, audioBtnY, 180, 30,
        m_Hwnd, (HMENU)BTN_AUDIO_CAPTURE, hInstance, nullptr);
    
    // Restart audio drivers button
    m_BtnRestartAudio = CreateWindowW(L"BUTTON", L"Restart Audio Service",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        (clientWidth - 180) / 2, audioBtnY + 35, 180, 25,
        m_Hwnd, (HMENU)BTN_RESTART_AUDIO, hInstance, nullptr);
    
    // ===== Opacity Group =====
    int opacityY = audioBtnY + 70;
    m_GroupOpacity = CreateWindowW(L"BUTTON", L"Opacity",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        margin, opacityY, groupWidth, 50,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    m_SliderOpacity = CreateWindowW(TRACKBAR_CLASSW, nullptr,
        WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_NOTICKS,
        margin + 10, opacityY + 20, groupWidth - 70, 25,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    SendMessageW(m_SliderOpacity, TBM_SETRANGE, TRUE, MAKELPARAM(10, 100));
    SendMessageW(m_SliderOpacity, TBM_SETPOS, TRUE, 100);
    
    m_LabelOpacity = CreateWindowW(L"STATIC", L"100%",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        margin + groupWidth - 55, opacityY + 23, 50, 20,
        m_Hwnd, nullptr, hInstance, nullptr);
    
    return true;
}

void Interface::Show()
{
    if (m_Hwnd)
    {
        ShowWindow(m_Hwnd, SW_SHOW);
        m_Visible = true;
    }
}

void Interface::Hide()
{
    if (m_Hwnd)
    {
        ShowWindow(m_Hwnd, SW_HIDE);
        m_Visible = false;
    }
}

void Interface::Update()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            m_Running = false;
            return;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Periodic device check
    CheckDeviceUpdate();
}

void Interface::CheckDeviceUpdate()
{
    DWORD now = GetTickCount();
    if (now - m_LastDeviceCheck < DEVICE_CHECK_INTERVAL)
        return;
    
    m_LastDeviceCheck = now;
    
    // Store old values
    std::wstring oldName = m_DeviceName;
    int oldChannels = m_Channels;
    
    // Refresh device info
    GetAudioDeviceInfo();
    
    // Update UI if changed
    if (oldName != m_DeviceName || oldChannels != m_Channels)
    {
        UpdateDeviceLabels();
    }
}

void Interface::UpdateDeviceLabels()
{
    if (!m_LabelDevice || !m_LabelChannels)
        return;
    
    // Truncate device name if too long
    std::wstring displayName = m_DeviceName;
    if (displayName.length() > 35)
        displayName = displayName.substr(0, 32) + L"...";
    
    SetWindowTextW(m_LabelDevice, displayName.c_str());
    
    std::wstring channelInfo = L"Channels: " + GetChannelLayoutName(m_Channels);
    SetWindowTextW(m_LabelChannels, channelInfo.c_str());
}

void Interface::SetCaptureDeviceInfo(const char* name, int channels)
{
    if (!name) return;
    
    // Convert from char to wchar_t
    int len = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    if (len > 0)
    {
        m_DeviceName.resize(len - 1);
        MultiByteToWideChar(CP_UTF8, 0, name, -1, &m_DeviceName[0], len);
    }
    
    m_Channels = channels;
    
    UpdateDeviceLabels();
}

void Interface::OnButtonClick(int buttonId)
{
    RadarPosition pos;
    
    switch (buttonId)
    {
    case BTN_TOP_LEFT:
        pos = RadarPosition::TopLeft;
        if (m_PositionCallback) m_PositionCallback(pos);
        break;
    case BTN_TOP_RIGHT:
        pos = RadarPosition::TopRight;
        if (m_PositionCallback) m_PositionCallback(pos);
        break;
    case BTN_BOTTOM_LEFT:
        pos = RadarPosition::BottomLeft;
        if (m_PositionCallback) m_PositionCallback(pos);
        break;
    case BTN_BOTTOM_RIGHT:
        pos = RadarPosition::BottomRight;
        if (m_PositionCallback) m_PositionCallback(pos);
        break;
    case BTN_TOGGLE:
        m_RadarVisible = !m_RadarVisible;
        SetWindowTextW(m_BtnToggle, m_RadarVisible ? L"Hide Radar" : L"Show Radar");
        if (m_ToggleCallback) m_ToggleCallback(m_RadarVisible);
        break;
    case BTN_DEGREES:
        m_DegreesVisible = !m_DegreesVisible;
        SetWindowTextW(m_BtnDegrees, m_DegreesVisible ? L"Degrees\u00B0" : L"Degrees");
        if (m_DegreesCallback) m_DegreesCallback(m_DegreesVisible);
        break;
    case BTN_SIZE_RESET:
        m_RadarSize = DEFAULT_RADAR_SIZE;
        SendMessageW(m_SliderSize, TBM_SETPOS, TRUE, m_RadarSize);
        SetWindowTextW(m_BtnSizeReset, L"400px");
        if (m_SizeCallback) m_SizeCallback(m_RadarSize);
        break;
    case BTN_RANDOM_SIG:
        if (m_SignatureCallback) m_SignatureCallback();
        break;
    case BTN_ECHO_PING:
        SetWindowTextW(m_BtnEchoPing, L"[Ping]");
        SetWindowTextW(m_BtnEchoTrail, L"Trail");
        SetWindowTextW(m_BtnEchoRipple, L"Ripple");
        if (m_EchoCallback) m_EchoCallback(0);
        break;
    case BTN_ECHO_TRAIL:
        SetWindowTextW(m_BtnEchoPing, L"Ping");
        SetWindowTextW(m_BtnEchoTrail, L"[Trail]");
        SetWindowTextW(m_BtnEchoRipple, L"Ripple");
        if (m_EchoCallback) m_EchoCallback(1);
        break;
    case BTN_ECHO_RIPPLE:
        SetWindowTextW(m_BtnEchoPing, L"Ping");
        SetWindowTextW(m_BtnEchoTrail, L"Trail");
        SetWindowTextW(m_BtnEchoRipple, L"[Ripple]");
        if (m_EchoCallback) m_EchoCallback(2);
        break;
    case BTN_AUDIO_CAPTURE:
        m_AudioCaptureEnabled = !m_AudioCaptureEnabled;
        SetWindowTextW(m_BtnAudioCapture, m_AudioCaptureEnabled ? L"Stop Audio Capture" : L"Start Audio Capture");
        if (m_AudioCaptureCallback) m_AudioCaptureCallback(m_AudioCaptureEnabled);
        break;
    case BTN_RESTART_AUDIO:
        // Restart Windows Audio Service with admin rights
        ShellExecuteW(nullptr, L"runas", L"cmd.exe", 
            L"/c net stop Audiosrv & net start Audiosrv & net stop AudioEndpointBuilder & net start AudioEndpointBuilder", 
            nullptr, SW_HIDE);
        break;
    }
}

LRESULT CALLBACK Interface::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Interface* ui = reinterpret_cast<Interface*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    
    switch (uMsg)
    {
    case WM_COMMAND:
        if (ui)
        {
            int buttonId = LOWORD(wParam);
            ui->OnButtonClick(buttonId);
        }
        return 0;
    
    case WM_HSCROLL:
        if (ui && (HWND)lParam == ui->m_SliderSize)
        {
            int size = (int)SendMessageW(ui->m_SliderSize, TBM_GETPOS, 0, 0);
            ui->m_RadarSize = size;
            
            wchar_t sizeText[32];
            swprintf_s(sizeText, L"%dpx", size);
            SetWindowTextW(ui->m_BtnSizeReset, sizeText);
            
            if (ui->m_SizeCallback)
                ui->m_SizeCallback(size);
        }
        else if (ui && (HWND)lParam == ui->m_SliderOpacity)
        {
            int opacity = (int)SendMessageW(ui->m_SliderOpacity, TBM_GETPOS, 0, 0);
            
            wchar_t opacityText[32];
            swprintf_s(opacityText, L"%d%%", opacity);
            SetWindowTextW(ui->m_LabelOpacity, opacityText);
            
            if (ui->m_OpacityCallback)
                ui->m_OpacityCallback(opacity / 100.0f);
        }
        return 0;
        
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
