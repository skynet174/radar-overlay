#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio-0.11.23/miniaudio.h"
#include "AudioCapture.h"
#include <cmath>
#include <cstring>

// Global pointer for callback
static AudioCapture* g_AudioCapture = nullptr;

// miniaudio callback
void AudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    (void)pOutput;
    
    if (!g_AudioCapture || !pInput || frameCount == 0)
        return;
    
    const float* samples = (const float*)pInput;
    ma_uint32 channels = pDevice->capture.channels;
    
    // Calculate peak levels for each channel
    float peaks[8] = {0};
    
    for (ma_uint32 i = 0; i < frameCount; i++)
    {
        for (ma_uint32 ch = 0; ch < channels && ch < 8; ch++)
        {
            float sample = fabsf(samples[i * channels + ch]);
            if (sample > peaks[ch])
                peaks[ch] = sample;
        }
    }
    
    // Update AudioCapture with peak values
    AudioChannels& chans = const_cast<AudioChannels&>(g_AudioCapture->GetChannels());
    chans.channelCount = (int)channels;
    
    if (channels >= 2)
    {
        chans.frontLeft = peaks[0];
        chans.frontRight = peaks[1];
    }
    if (channels >= 6)  // 5.1
    {
        chans.center = peaks[2];
        chans.lfe = peaks[3];
        chans.rearLeft = peaks[4];
        chans.rearRight = peaks[5];
    }
    if (channels >= 8)  // 7.1
    {
        chans.sideLeft = peaks[6];
        chans.sideRight = peaks[7];
    }
}

AudioCapture::AudioCapture()
{
    g_AudioCapture = this;
}

AudioCapture::~AudioCapture()
{
    Shutdown();
    g_AudioCapture = nullptr;
}

bool AudioCapture::Initialize()
{
    ma_context context;
    ma_result result = ma_context_init(NULL, 0, NULL, &context);
    if (result != MA_SUCCESS)
        return false;
    
    // Enumerate capture devices to find CABLE Output
    ma_device_info* pCaptureDevices;
    ma_uint32 captureDeviceCount;
    
    result = ma_context_get_devices(&context, NULL, NULL, &pCaptureDevices, &captureDeviceCount);
    if (result != MA_SUCCESS)
    {
        ma_context_uninit(&context);
        return false;
    }
    
    // Find CABLE Output device
    ma_device_id* targetDeviceId = nullptr;
    ma_device_id cableDeviceId;
    bool foundCable = false;
    
    for (ma_uint32 i = 0; i < captureDeviceCount; i++)
    {
        // Check if device name contains "CABLE" or "VB-Audio"
        if (strstr(pCaptureDevices[i].name, "CABLE") != nullptr ||
            strstr(pCaptureDevices[i].name, "VB-Audio") != nullptr)
        {
            cableDeviceId = pCaptureDevices[i].id;
            targetDeviceId = &cableDeviceId;
            foundCable = true;
            strncpy(m_DeviceName, pCaptureDevices[i].name, sizeof(m_DeviceName) - 1);
            m_DeviceName[sizeof(m_DeviceName) - 1] = '\0';
            break;
        }
    }
    
    // If CABLE not found, use default and save its name
    if (!foundCable && captureDeviceCount > 0)
    {
        strncpy(m_DeviceName, pCaptureDevices[0].name, sizeof(m_DeviceName) - 1);
        m_DeviceName[sizeof(m_DeviceName) - 1] = '\0';
    }
    
    ma_context_uninit(&context);
    
    // Initialize device
    m_Device = new ma_device;
    
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.pDeviceID = targetDeviceId;  // NULL = default, or specific CABLE device
    config.capture.format = ma_format_f32;
    config.capture.channels = 0;  // Use device native channel count
    config.sampleRate = 48000;
    config.dataCallback = AudioDataCallback;
    config.pUserData = this;
    
    result = ma_device_init(NULL, &config, (ma_device*)m_Device);
    if (result != MA_SUCCESS)
    {
        delete (ma_device*)m_Device;
        m_Device = nullptr;
        return false;
    }
    
    // Get actual channel count
    m_Channels.channelCount = ((ma_device*)m_Device)->capture.channels;
    
    return true;
}

bool AudioCapture::Reinitialize()
{
    bool wasRunning = m_Running;
    Shutdown();
    
    bool success = Initialize();
    
    if (success && wasRunning)
    {
        Start();
    }
    
    return success;
}

void AudioCapture::Shutdown()
{
    Stop();
    
    if (m_Device)
    {
        ma_device_uninit((ma_device*)m_Device);
        delete (ma_device*)m_Device;
        m_Device = nullptr;
    }
}

bool AudioCapture::Start()
{
    if (!m_Device || m_Running)
        return false;
    
    ma_result result = ma_device_start((ma_device*)m_Device);
    if (result != MA_SUCCESS)
        return false;
    
    m_Running = true;
    m_CaptureThread = std::thread(&AudioCapture::CaptureThread, this);
    
    return true;
}

void AudioCapture::Stop()
{
    m_Running = false;
    
    if (m_Device)
    {
        ma_device_stop((ma_device*)m_Device);
    }
    
    if (m_CaptureThread.joinable())
    {
        m_CaptureThread.join();
    }
}

void AudioCapture::CaptureThread()
{
    while (m_Running)
    {
        CalculateDirection();
        
        if (m_Callback && m_Direction.magnitude > m_Threshold)
        {
            m_Callback(m_Direction);
        }
        
        Sleep(20);  // ~50 updates per second
    }
}

void AudioCapture::CalculateDirection()
{
    float x = 0.0f;
    float y = 0.0f;
    
    // Get channel values
    float fl = m_Channels.frontLeft * m_Multiplier;
    float fr = m_Channels.frontRight * m_Multiplier;
    float rl = m_Channels.rearLeft * m_Multiplier;
    float rr = m_Channels.rearRight * m_Multiplier;
    float sl = m_Channels.sideLeft * m_Multiplier;
    float sr = m_Channels.sideRight * m_Multiplier;
    
    if (m_Channels.channelCount >= 2)
    {
        // Stereo: left-right balance
        x = fr - fl;
    }
    
    if (m_Channels.channelCount >= 6)
    {
        // 5.1: add rear channels for front-back
        x += (rr - rl) * 0.7f;
        y = (fl + fr) - (rl + rr);
    }
    
    if (m_Channels.channelCount >= 8)
    {
        // 7.1: add side channels
        x += (sr - sl);
    }
    
    // Normalize to -1 to 1 range
    float maxVal = fmaxf(fabsf(x), fabsf(y));
    if (maxVal > 1.0f)
    {
        x /= maxVal;
        y /= maxVal;
    }
    
    // Clamp
    x = fmaxf(-1.0f, fminf(1.0f, x));
    y = fmaxf(-1.0f, fminf(1.0f, y));
    
    // Add to history for smoothing
    m_HistoryX[m_HistoryIndex] = x;
    m_HistoryY[m_HistoryIndex] = y;
    m_HistoryIndex = (m_HistoryIndex + 1) % HISTORY_SIZE;
    
    // Average history
    float avgX = 0.0f, avgY = 0.0f;
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        avgX += m_HistoryX[i];
        avgY += m_HistoryY[i];
    }
    avgX /= HISTORY_SIZE;
    avgY /= HISTORY_SIZE;
    
    m_Direction.x = avgX;
    m_Direction.y = avgY;
    m_Direction.magnitude = sqrtf(avgX * avgX + avgY * avgY);
    
    // Calculate angle (0 = top/front, clockwise)
    float angle = atan2f(avgX, avgY) * 180.0f / 3.14159f;
    if (angle < 0) angle += 360.0f;
    m_Direction.angle = angle;
}
