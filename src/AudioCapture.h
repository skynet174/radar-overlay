#pragma once

#include <windows.h>
#include <functional>
#include <thread>
#include <atomic>
#include <array>

// Audio channel levels for 7.1 surround
struct AudioChannels
{
    float frontLeft = 0.0f;
    float frontRight = 0.0f;
    float center = 0.0f;
    float lfe = 0.0f;           // Subwoofer
    float rearLeft = 0.0f;
    float rearRight = 0.0f;
    float sideLeft = 0.0f;
    float sideRight = 0.0f;
    
    int channelCount = 0;
};

// Single sound source
struct SoundSource
{
    float angle = 0.0f;      // 0-360 degrees, 0 = front
    float magnitude = 0.0f;  // 0 to 1, sound intensity
    bool active = false;
};

// Multiple sound sources (up to 8 directions)
struct SoundSources
{
    static constexpr int MAX_SOURCES = 8;
    SoundSource sources[MAX_SOURCES];
    int count = 0;
};

// Calculated sound direction (legacy, for single source)
struct SoundDirection
{
    float x = 0.0f;     // -1 (left) to 1 (right)
    float y = 0.0f;     // -1 (back) to 1 (front)
    float magnitude = 0.0f;  // 0 to 1, sound intensity
    float angle = 0.0f;      // 0-360 degrees, 0 = front
};

class AudioCapture
{
public:
    using DirectionCallback = std::function<void(const SoundDirection&)>;
    using MultiSourceCallback = std::function<void(const SoundSources&)>;

    AudioCapture();
    ~AudioCapture();

    bool Initialize();
    void Shutdown();
    bool Reinitialize();
    
    bool Start();
    void Stop();
    bool IsRunning() const { return m_Running; }
    
    void SetCallback(DirectionCallback callback) { m_Callback = callback; }
    void SetMultiSourceCallback(MultiSourceCallback callback) { m_MultiCallback = callback; }
    void SetMultiplier(float mult) { m_Multiplier = mult; }
    void SetThreshold(float thresh) { m_Threshold = thresh; }
    
    const AudioChannels& GetChannels() const { return m_Channels; }
    const SoundSources& GetSources() const { return m_Sources; }
    int GetChannelCount() const { return m_Channels.channelCount; }
    const char* GetDeviceName() const { return m_DeviceName; }

private:
    void CaptureThread();
    void CalculateDirection();
    void CalculateMultipleSources();

private:
    std::atomic<bool> m_Running{false};
    std::thread m_CaptureThread;
    
    AudioChannels m_Channels;
    SoundDirection m_Direction;
    SoundSources m_Sources;
    DirectionCallback m_Callback;
    MultiSourceCallback m_MultiCallback;
    
    float m_Multiplier = 2.0f;
    float m_Threshold = 0.02f;
    
    // History for smoothing
    static constexpr int HISTORY_SIZE = 50;
    std::array<float, HISTORY_SIZE> m_HistoryX{};
    std::array<float, HISTORY_SIZE> m_HistoryY{};
    int m_HistoryIndex = 0;
    
    void* m_Device = nullptr;  // miniaudio device
    char m_DeviceName[256] = "Unknown";
};

