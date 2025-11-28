#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <vector>
#include <random>
#include <thread>
#include <atomic>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

struct SignaturePoint
{
    float angle;      // 0-360 degrees
    float distance;   // 0-1 normalized
    float intensity;  // 0-1 
    float spawnTime;  // when it appeared
    float lifetime;   // how long it lives
    float lastPingTime; // when sweep last hit this point
    float pingIntensity; // current ping/echo intensity
};

enum class EchoType
{
    Ping,       // Bright flash when line passes
    Trail,      // Fading trail behind sweep on points
    Ripple,     // Circular wave from point
    Line,       // Line from center to sound source
    Hex,        // Honeycomb grid with highlighted cells
    Arc,        // Arc/sector in sound direction
    Cone,       // Cone beam from center
    Pulse       // Pulsating rings from center toward sound
};

enum class RadarPosition
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

class Radar
{
public:
    Radar();
    ~Radar();

    bool Initialize(HINSTANCE hInstance);
    void Shutdown();
    
    void SetPosition(RadarPosition position);
    RadarPosition GetPosition() const { return m_Position; }
    
    void Show();
    void Hide();
    bool IsVisible() const { return m_Visible; }
    
    void SetShowDegrees(bool show) { m_ShowDegrees = show; }
    bool GetShowDegrees() const { return m_ShowDegrees; }
    
    void SetShowSweep(bool show) { m_ShowSweep = show; }
    bool GetShowSweep() const { return m_ShowSweep; }
    
    void SetMultiSource(bool multi) { m_MultiSource = multi; }
    bool GetMultiSource() const { return m_MultiSource; }
    
    void SetSize(int size);
    int GetSize() const { return m_Size; }
    
    void SetOpacity(float opacity);
    float GetOpacity() const { return m_Opacity; }
    
    void AddRandomSignature();
    void AddSignature(float angle, float distance, float intensity);
    void UpdateAudioPoint(float angle, float distance, float intensity);
    void UpdateAudioSources(int count, const float* angles, const float* distances, const float* intensities);
    void ClearSignatures();
    
    void SetEchoType(EchoType type) { m_EchoType = type; }
    EchoType GetEchoType() const { return m_EchoType; }
    
    void Update();
    void Render();
    
    bool IsRunning() const { return m_Running; }
    void Stop() { m_Running = false; }

private:
    bool CreateOverlayWindow(HINSTANCE hInstance);
    bool InitD3D();
    bool InitD2D();
    void CleanupD3D();
    void CleanupD2D();
    void RenderText();
    void RenderSignatures();
    void UpdateWindowPosition();
    void RenderThread();
    
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    // Window
    HWND m_Hwnd = nullptr;
    int m_Size = 400;
    int m_Margin = 20;
    RadarPosition m_Position = RadarPosition::TopLeft;
    bool m_Visible = false;
    bool m_Running = true;
    bool m_ShowDegrees = true;
    bool m_ShowSweep = true;
    bool m_MultiSource = true;  // true = multiple sources, false = single interpolated
    float m_Opacity = 1.0f;

    // D3D11
    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_Context = nullptr;
    IDXGISwapChain* m_SwapChain = nullptr;
    ID3D11RenderTargetView* m_RenderTarget = nullptr;
    ID3D11VertexShader* m_VertexShader = nullptr;
    ID3D11PixelShader* m_PixelShader = nullptr;
    ID3D11InputLayout* m_InputLayout = nullptr;
    ID3D11Buffer* m_VertexBuffer = nullptr;
    ID3D11Buffer* m_ConstantBuffer = nullptr;
    ID3D11BlendState* m_BlendState = nullptr;

    // D2D / DirectWrite for text
    ID2D1Factory* m_D2DFactory = nullptr;
    ID2D1RenderTarget* m_D2DRenderTarget = nullptr;
    ID2D1SolidColorBrush* m_TextBrush = nullptr;
    ID2D1SolidColorBrush* m_SignatureBrush = nullptr;
    IDWriteFactory* m_DWriteFactory = nullptr;
    IDWriteTextFormat* m_TextFormat = nullptr;
    
    // Signatures
    std::vector<SignaturePoint> m_Signatures;
    std::mt19937 m_Rng;
    EchoType m_EchoType = EchoType::Ping;
    float m_CurrentSweepAngle = 0.0f;

    // Timing
    float m_Time = 0.0f;
    LARGE_INTEGER m_Frequency;
    LARGE_INTEGER m_StartTime;
    
    // Render thread
    std::thread m_RenderThread;
    std::atomic<bool> m_RenderThreadRunning{false};
    
    // Audio point with trail history
    struct AudioTrailPoint {
        float angle = 0.0f;
        float distance = 0.0f;
        float intensity = 0.0f;
        float time = -1.0f;  // Negative = not yet used
    };
    static const int TRAIL_HISTORY_SIZE = 20;
    AudioTrailPoint m_AudioTrail[TRAIL_HISTORY_SIZE] = {};
    int m_AudioTrailIndex = 0;
    bool m_HasAudioPoint = false;
    float m_AudioPointAngle = 0.0f;
    float m_AudioPointDistance = 0.5f;
    float m_AudioPointIntensity = 0.0f;
    
    // Multiple audio sources
    static const int MAX_AUDIO_SOURCES = 8;
    struct AudioSource {
        float angle = 0.0f;
        float distance = 0.0f;
        float intensity = 0.0f;
        bool active = false;
        AudioTrailPoint trail[TRAIL_HISTORY_SIZE] = {};
        int trailIndex = 0;
    };
    AudioSource m_AudioSources[MAX_AUDIO_SOURCES] = {};
    int m_AudioSourceCount = 0;
};
