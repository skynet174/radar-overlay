#include "Radar.h"
#include <cstring>
#include <cmath>
#include <cstdio>

#pragma comment(lib, "dxgi.lib")

// Vertex structure
struct Vertex
{
    float x, y;
    float u, v;
};

// Constant buffer
struct alignas(16) ConstantBuffer
{
    float time;
    float padding[3];
};

// Shader source with antialiasing
const char* g_RadarShader = R"(
cbuffer Constants : register(b0)
{
    float time;
    float3 padding;
};

struct VS_INPUT
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.uv = input.uv;
    return output;
}

// Antialiased line function
float aaline(float d, float width, float aa)
{
    return 1.0f - smoothstep(width - aa, width + aa, abs(d));
}

// Antialiased circle ring
float aaring(float dist, float radius, float width, float aa)
{
    return 1.0f - smoothstep(width - aa, width + aa, abs(dist - radius));
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.uv * 2.0f - 1.0f;
    float dist = length(uv);
    float angle = atan2(uv.y, uv.x);
    
    // Antialiasing factor (reduced for sharper look)
    float aa = 0.004f;
    
    // Soft circle mask with antialiased edge
    float circleMask = 1.0f - smoothstep(1.0f - aa, 1.0f + aa, dist);
    if (circleMask < 0.001f)
        discard;
    
    float3 baseColor = float3(0.0f, 0.05f, 0.02f);
    
    // Convert angle: 0 at top, clockwise direction
    // atan2 gives: 0 = right, PI/2 = top, PI = left, -PI/2 = bottom
    // We want: 0 = top, 90 = right, 180 = bottom, 270 = left
    float normAngle = 1.5708f - angle;  // Rotate 90 degrees (PI/2) and flip direction
    if (normAngle < 0.0f) normAngle += 6.28318f;
    if (normAngle > 6.28318f) normAngle -= 6.28318f;
    
    float sweepSpeed = 1.5f;
    float sweepAngle = 6.28318f - fmod(time * sweepSpeed, 6.28318f);  // Clockwise rotation
    
    float angleDiff = normAngle - sweepAngle;
    if (angleDiff < 0.0f) angleDiff += 6.28318f;
    
    // Calculate opposite angle difference (180 degrees apart)
    float angleDiff2 = angleDiff + 3.14159f;
    if (angleDiff2 > 6.28318f) angleDiff2 -= 6.28318f;
    
    // Smooth sweep trail for both lines
    float trailLength = 2.0f;
    float sweep = 0.0f;
    if (angleDiff < trailLength)
    {
        sweep = 1.0f - (angleDiff / trailLength);
        sweep = pow(sweep, 2.0f);
    }
    if (angleDiff2 < trailLength)
    {
        float sweep2 = 1.0f - (angleDiff2 / trailLength);
        sweep = max(sweep, pow(sweep2, 2.0f));
    }
    
    // Antialiased sweep lines (two opposite lines)
    float sweepLine1 = smoothstep(0.08f, 0.0f, angleDiff) + smoothstep(6.20f, 6.28318f, angleDiff);
    float sweepLine2 = smoothstep(0.08f, 0.0f, angleDiff2) + smoothstep(6.20f, 6.28318f, angleDiff2);
    float sweepLine = saturate(sweepLine1 + sweepLine2);
    
    // Antialiased concentric rings
    float rings = 0.0f;
    for (int i = 1; i <= 4; i++)
    {
        float ringDist = (float)i * 0.25f;
        rings += aaring(dist, ringDist, 0.006f, aa) * 0.4f;
    }
    
    // Antialiased cross lines (main axes)
    float crossWidth = 0.003f;
    float crossX = aaline(uv.x, crossWidth, aa);
    float crossY = aaline(uv.y, crossWidth, aa);
    float cross = max(crossX, crossY) * 0.3f;
    
    // Antialiased outer ring
    float outerRing = aaring(dist, 0.985f, 0.015f, aa) * 0.8f;
    
    float3 greenGlow = float3(0.0f, 1.0f, 0.4f);
    float3 color = baseColor;
    
    // Add grid with smooth blending
    color += greenGlow * rings;
    color += greenGlow * cross;
    color += greenGlow * outerRing;
    color += greenGlow * sweep * 0.6f;
    color += greenGlow * sweepLine * 0.8f;
    
    // Antialiased center dot
    float centerDot = 1.0f - smoothstep(0.015f - aa, 0.015f + aa, dist);
    color = lerp(color, greenGlow, centerDot);
    
    // Subtle scanlines
    float scanlines = sin(uv.y * 150.0f) * 0.02f + 1.0f;
    color *= scanlines;
    
    // Smooth vignette
    float vignette = 1.0f - smoothstep(0.3f, 1.0f, dist) * 0.4f;
    color *= vignette;
    
    // Alpha with smooth edge
    float alpha = lerp(0.9f, 1.0f, outerRing) * circleMask;
    
    return float4(color, alpha);
}
)";

Radar::Radar()
{
    QueryPerformanceFrequency(&m_Frequency);
    QueryPerformanceCounter(&m_StartTime);
    
    // Initialize random number generator
    std::random_device rd;
    m_Rng.seed(rd());
}

Radar::~Radar()
{
    Shutdown();
}

bool Radar::Initialize(HINSTANCE hInstance)
{
    if (!CreateOverlayWindow(hInstance))
        return false;
    
    if (!InitD3D())
        return false;
    
    if (!InitD2D())
        return false;
    
    return true;
}

void Radar::Shutdown()
{
    // Stop render thread
    m_RenderThreadRunning = false;
    if (m_RenderThread.joinable())
    {
        m_RenderThread.join();
    }
    
    CleanupD2D();
    CleanupD3D();
    
    if (m_Hwnd)
    {
        DestroyWindow(m_Hwnd);
        m_Hwnd = nullptr;
    }
}

bool Radar::CreateOverlayWindow(HINSTANCE hInstance)
{
    const wchar_t* className = L"RadarOverlayClass";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);
    
    m_Hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        className,
        L"Radar",
        WS_POPUP,
        0, 0, m_Size, m_Size,
        nullptr, nullptr, hInstance, nullptr);
    
    if (!m_Hwnd)
        return false;
    
    SetWindowLongPtr(m_Hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetLayeredWindowAttributes(m_Hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    
    MARGINS margins = {-1};
    DwmExtendFrameIntoClientArea(m_Hwnd, &margins);
    
    UpdateWindowPosition();
    
    return true;
}

bool Radar::InitD3D()
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = m_Size;
    scd.BufferDesc.Height = m_Size;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = m_Hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
        &scd, &m_SwapChain, &m_Device, nullptr, &m_Context);

    if (FAILED(hr)) return false;

    ID3D11Texture2D* backBuffer;
    m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    m_Device->CreateRenderTargetView(backBuffer, nullptr, &m_RenderTarget);
    backBuffer->Release();

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    hr = D3DCompile(g_RadarShader, strlen(g_RadarShader), nullptr, nullptr, nullptr,
        "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) return false;

    hr = D3DCompile(g_RadarShader, strlen(g_RadarShader), nullptr, nullptr, nullptr,
        "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) return false;

    m_Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_VertexShader);
    m_Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_PixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    m_Device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_InputLayout);

    vsBlob->Release();
    psBlob->Release();

    Vertex vertices[] = {
        {-1.0f, -1.0f, 0.0f, 1.0f},
        {-1.0f,  1.0f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 1.0f, 0.0f},
    };

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(vertices);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vsd = {};
    vsd.pSysMem = vertices;
    m_Device->CreateBuffer(&vbd, &vsd, &m_VertexBuffer);

    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(ConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_Device->CreateBuffer(&cbd, nullptr, &m_ConstantBuffer);

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_Device->CreateBlendState(&blendDesc, &m_BlendState);

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)m_Size;
    vp.Height = (float)m_Size;
    vp.MaxDepth = 1.0f;
    m_Context->RSSetViewports(1, &vp);

    return true;
}

bool Radar::InitD2D()
{
    HRESULT hr;
    
    // Create D2D Factory
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_D2DFactory);
    if (FAILED(hr)) return false;
    
    // Create DirectWrite Factory
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, 
        __uuidof(IDWriteFactory), 
        reinterpret_cast<IUnknown**>(&m_DWriteFactory));
    if (FAILED(hr)) return false;
    
    // Create text format
    hr = m_DWriteFactory->CreateTextFormat(
        L"Consolas",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.0f,
        L"en-us",
        &m_TextFormat);
    if (FAILED(hr)) return false;
    
    m_TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    
    // Get DXGI surface from swap chain
    IDXGISurface* dxgiSurface = nullptr;
    hr = m_SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&dxgiSurface);
    if (FAILED(hr)) return false;
    
    // Create D2D render target
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
    
    hr = m_D2DFactory->CreateDxgiSurfaceRenderTarget(dxgiSurface, &props, &m_D2DRenderTarget);
    dxgiSurface->Release();
    if (FAILED(hr)) return false;
    
    // Create brush for text
    hr = m_D2DRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 1.0f, 0.0f, 1.0f), // Yellow for debug
        &m_TextBrush);
    if (FAILED(hr)) return false;
    
    // Create brush for signatures
    hr = m_D2DRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 0.3f, 0.3f, 1.0f), // Red for signatures
        &m_SignatureBrush);
    if (FAILED(hr)) return false;
    
    return true;
}

void Radar::CleanupD3D()
{
    if (m_BlendState) { m_BlendState->Release(); m_BlendState = nullptr; }
    if (m_ConstantBuffer) { m_ConstantBuffer->Release(); m_ConstantBuffer = nullptr; }
    if (m_VertexBuffer) { m_VertexBuffer->Release(); m_VertexBuffer = nullptr; }
    if (m_InputLayout) { m_InputLayout->Release(); m_InputLayout = nullptr; }
    if (m_PixelShader) { m_PixelShader->Release(); m_PixelShader = nullptr; }
    if (m_VertexShader) { m_VertexShader->Release(); m_VertexShader = nullptr; }
    if (m_RenderTarget) { m_RenderTarget->Release(); m_RenderTarget = nullptr; }
    if (m_SwapChain) { m_SwapChain->Release(); m_SwapChain = nullptr; }
    if (m_Context) { m_Context->Release(); m_Context = nullptr; }
    if (m_Device) { m_Device->Release(); m_Device = nullptr; }
}

void Radar::CleanupD2D()
{
    if (m_SignatureBrush) { m_SignatureBrush->Release(); m_SignatureBrush = nullptr; }
    if (m_TextBrush) { m_TextBrush->Release(); m_TextBrush = nullptr; }
    if (m_TextFormat) { m_TextFormat->Release(); m_TextFormat = nullptr; }
    if (m_D2DRenderTarget) { m_D2DRenderTarget->Release(); m_D2DRenderTarget = nullptr; }
    if (m_DWriteFactory) { m_DWriteFactory->Release(); m_DWriteFactory = nullptr; }
    if (m_D2DFactory) { m_D2DFactory->Release(); m_D2DFactory = nullptr; }
}

void Radar::AddRandomSignature()
{
    std::uniform_real_distribution<float> angleDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> distDist(0.2f, 0.9f);
    std::uniform_real_distribution<float> intensityDist(0.5f, 1.0f);
    std::uniform_int_distribution<int> countDist(1, 5);
    
    int count = countDist(m_Rng);
    float baseAngle = angleDist(m_Rng);
    float baseDist = distDist(m_Rng);
    
    for (int i = 0; i < count; i++)
    {
        SignaturePoint sig;
        sig.angle = baseAngle + (i - count / 2.0f) * 8.0f;
        sig.distance = baseDist + (i * 0.05f);
        sig.intensity = intensityDist(m_Rng);
        sig.spawnTime = m_Time;
        sig.lifetime = 8.0f + intensityDist(m_Rng) * 4.0f;  // 8-12 seconds for longer demo
        sig.lastPingTime = -10.0f;
        sig.pingIntensity = 0.0f;
        
        while (sig.angle < 0.0f) sig.angle += 360.0f;
        while (sig.angle >= 360.0f) sig.angle -= 360.0f;
        
        if (sig.distance > 0.95f) sig.distance = 0.95f;
        
        m_Signatures.push_back(sig);
    }
}

void Radar::ClearSignatures()
{
    m_Signatures.clear();
}

void Radar::AddSignature(float angle, float distance, float intensity)
{
    SignaturePoint sig;
    sig.angle = angle;
    sig.distance = fminf(0.95f, fmaxf(0.1f, distance));
    sig.intensity = intensity;
    sig.spawnTime = m_Time;
    sig.lifetime = 2.0f;  // Shorter lifetime for audio signatures
    sig.lastPingTime = -10.0f;
    sig.pingIntensity = 0.0f;
    
    // Limit signatures to prevent overflow
    if (m_Signatures.size() > 50)
    {
        m_Signatures.erase(m_Signatures.begin());
    }
    
    m_Signatures.push_back(sig);
}

void Radar::RenderSignatures()
{
    if (!m_D2DRenderTarget || !m_SignatureBrush || m_Signatures.empty()) return;
    
    float center = m_Size / 2.0f;
    float maxRadius = m_Size / 2.0f - 10.0f;
    float scale = m_Size / 400.0f;
    
    // Calculate current sweep angle (0-360, 0 at top, clockwise)
    float sweepAngle = fmodf(m_Time * 1.5f * 180.0f / 3.14159f, 360.0f);
    m_CurrentSweepAngle = sweepAngle;
    
    // First pass: update ping states and calculate positions
    struct PointData {
        float x, y;
        float alpha;
        float pointSize;
        float pingEffect;
        float rippleRadius;
        bool valid;
    };
    std::vector<PointData> pointsData;
    
    auto it = m_Signatures.begin();
    while (it != m_Signatures.end())
    {
        float age = m_Time - it->spawnTime;
        if (age > it->lifetime)
        {
            it = m_Signatures.erase(it);
            continue;
        }
        
        // Check if sweep line is passing this point
        float angleDiff = fabsf(it->angle - sweepAngle);
        if (angleDiff > 180.0f) angleDiff = 360.0f - angleDiff;
        
        // Also check opposite sweep line (180 degrees apart)
        float sweepAngle2 = fmodf(sweepAngle + 180.0f, 360.0f);
        float angleDiff2 = fabsf(it->angle - sweepAngle2);
        if (angleDiff2 > 180.0f) angleDiff2 = 360.0f - angleDiff2;
        
        float minAngleDiff = fminf(angleDiff, angleDiff2);
        
        if (minAngleDiff < 5.0f)  // Within 5 degrees of sweep
        {
            it->lastPingTime = m_Time;
            it->pingIntensity = 1.0f;
        }
        
        // Decay ping intensity
        float timeSincePing = m_Time - it->lastPingTime;
        it->pingIntensity = fmaxf(0.0f, 1.0f - timeSincePing / 2.0f);
        
        // Calculate fade
        float fadeStart = it->lifetime * 0.6f;
        float alpha = 1.0f;
        if (age > fadeStart)
        {
            alpha = 1.0f - (age - fadeStart) / (it->lifetime - fadeStart);
        }
        alpha *= it->intensity;
        
        // Convert angle to screen position
        float radAngle = (it->angle - 90.0f) * 3.14159f / 180.0f;
        float x = center + cosf(radAngle) * (it->distance * maxRadius);
        float y = center + sinf(radAngle) * (it->distance * maxRadius);
        
        PointData pd;
        pd.x = x;
        pd.y = y;
        pd.alpha = alpha;
        pd.pointSize = (4.0f + it->intensity * 4.0f) * scale;
        pd.pingEffect = it->pingIntensity;
        pd.rippleRadius = timeSincePing * 30.0f * scale;
        pd.valid = true;
        pointsData.push_back(pd);
        
        ++it;
    }
    
    // Render Metaballs effect (merge close points)
    for (size_t i = 0; i < pointsData.size(); i++)
    {
        for (size_t j = i + 1; j < pointsData.size(); j++)
        {
            float dx = pointsData[j].x - pointsData[i].x;
            float dy = pointsData[j].y - pointsData[i].y;
            float dist = sqrtf(dx * dx + dy * dy);
            float mergeThreshold = (pointsData[i].pointSize + pointsData[j].pointSize) * 3.0f;
            
            if (dist < mergeThreshold && dist > 0.1f)
            {
                // Draw connecting blob
                float midX = (pointsData[i].x + pointsData[j].x) / 2.0f;
                float midY = (pointsData[i].y + pointsData[j].y) / 2.0f;
                float avgAlpha = (pointsData[i].alpha + pointsData[j].alpha) / 2.0f;
                float blobSize = (mergeThreshold - dist) / 2.0f;
                
                m_SignatureBrush->SetColor(D2D1::ColorF(1.0f, 0.2f, 0.2f, avgAlpha * 0.4f));
                D2D1_ELLIPSE blob = D2D1::Ellipse(D2D1::Point2F(midX, midY), blobSize, blobSize);
                m_D2DRenderTarget->FillEllipse(blob, m_SignatureBrush);
            }
        }
    }
    
    // Render each point with echo effect
    for (size_t i = 0; i < pointsData.size(); i++)
    {
        const auto& pd = pointsData[i];
        
        // Echo effects based on type
        switch (m_EchoType)
        {
        case EchoType::Ping:
            if (pd.pingEffect > 0.0f)
            {
                // Bright flash
                float flashSize = pd.pointSize * (1.0f + pd.pingEffect * 3.0f);
                m_SignatureBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 0.8f, pd.pingEffect * pd.alpha));
                D2D1_ELLIPSE flash = D2D1::Ellipse(D2D1::Point2F(pd.x, pd.y), flashSize, flashSize);
                m_D2DRenderTarget->FillEllipse(flash, m_SignatureBrush);
            }
            break;
            
        case EchoType::Trail:
            if (pd.pingEffect > 0.0f)
            {
                // Fading trail segments
                for (int t = 0; t < 5; t++)
                {
                    float trailAlpha = pd.pingEffect * (1.0f - t * 0.2f) * pd.alpha;
                    float trailAngle = (m_Signatures[i].angle - 90.0f - t * 3.0f) * 3.14159f / 180.0f;
                    float trailX = center + cosf(trailAngle) * (m_Signatures[i].distance * maxRadius);
                    float trailY = center + sinf(trailAngle) * (m_Signatures[i].distance * maxRadius);
                    
                    m_SignatureBrush->SetColor(D2D1::ColorF(0.3f, 1.0f, 0.3f, trailAlpha * 0.5f));
                    D2D1_ELLIPSE trail = D2D1::Ellipse(D2D1::Point2F(trailX, trailY), pd.pointSize * 0.8f, pd.pointSize * 0.8f);
                    m_D2DRenderTarget->FillEllipse(trail, m_SignatureBrush);
                }
            }
            break;
            
        case EchoType::Ripple:
            if (pd.pingEffect > 0.0f && pd.rippleRadius < 50.0f * scale)
            {
                // Expanding ring
                float rippleAlpha = pd.pingEffect * pd.alpha * 0.8f;
                m_SignatureBrush->SetColor(D2D1::ColorF(0.3f, 0.8f, 1.0f, rippleAlpha));
                D2D1_ELLIPSE ripple = D2D1::Ellipse(D2D1::Point2F(pd.x, pd.y), pd.rippleRadius, pd.rippleRadius);
                m_D2DRenderTarget->DrawEllipse(ripple, m_SignatureBrush, 2.0f * scale);
                
                // Second smaller ripple
                if (pd.rippleRadius > 15.0f * scale)
                {
                    float innerRadius = pd.rippleRadius - 15.0f * scale;
                    m_SignatureBrush->SetColor(D2D1::ColorF(0.3f, 0.8f, 1.0f, rippleAlpha * 0.5f));
                    D2D1_ELLIPSE ripple2 = D2D1::Ellipse(D2D1::Point2F(pd.x, pd.y), innerRadius, innerRadius);
                    m_D2DRenderTarget->DrawEllipse(ripple2, m_SignatureBrush, 1.5f * scale);
                }
            }
            break;
        }
        
        // Base point glow
        D2D1_ELLIPSE glow = D2D1::Ellipse(D2D1::Point2F(pd.x, pd.y), pd.pointSize * 2.5f, pd.pointSize * 2.5f);
        m_SignatureBrush->SetColor(D2D1::ColorF(1.0f, 0.3f, 0.3f, pd.alpha * 0.3f));
        m_D2DRenderTarget->FillEllipse(glow, m_SignatureBrush);
        
        // Inner bright point
        D2D1_ELLIPSE point = D2D1::Ellipse(D2D1::Point2F(pd.x, pd.y), pd.pointSize, pd.pointSize);
        m_SignatureBrush->SetColor(D2D1::ColorF(1.0f, 0.5f, 0.5f, pd.alpha));
        m_D2DRenderTarget->FillEllipse(point, m_SignatureBrush);
    }
}

void Radar::SetPosition(RadarPosition position)
{
    m_Position = position;
    UpdateWindowPosition();
}

void Radar::SetSize(int size)
{
    if (size < 100) size = 100;
    if (size > 600) size = 600;
    if (size == m_Size) return;
    
    m_Size = size;
    
    // Resize window
    UpdateWindowPosition();
    
    // Recreate swap chain and render targets with new size
    CleanupD2D();
    
    if (m_RenderTarget) { m_RenderTarget->Release(); m_RenderTarget = nullptr; }
    
    // Resize swap chain buffers
    if (m_SwapChain)
    {
        m_Context->OMSetRenderTargets(0, nullptr, nullptr);
        
        HRESULT hr = m_SwapChain->ResizeBuffers(0, m_Size, m_Size, DXGI_FORMAT_UNKNOWN, 0);
        if (SUCCEEDED(hr))
        {
            ID3D11Texture2D* backBuffer;
            m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
            m_Device->CreateRenderTargetView(backBuffer, nullptr, &m_RenderTarget);
            backBuffer->Release();
            
            // Update viewport
            D3D11_VIEWPORT vp = {};
            vp.Width = (float)m_Size;
            vp.Height = (float)m_Size;
            vp.MaxDepth = 1.0f;
            m_Context->RSSetViewports(1, &vp);
        }
    }
    
    // Recreate D2D render target
    InitD2D();
}

void Radar::UpdateWindowPosition()
{
    if (!m_Hwnd) return;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = 0, posY = 0;
    
    switch (m_Position)
    {
    case RadarPosition::TopLeft:
        posX = m_Margin;
        posY = m_Margin;
        break;
    case RadarPosition::TopRight:
        posX = screenWidth - m_Size - m_Margin;
        posY = m_Margin;
        break;
    case RadarPosition::BottomLeft:
        posX = m_Margin;
        posY = screenHeight - m_Size - m_Margin - 40;
        break;
    case RadarPosition::BottomRight:
        posX = screenWidth - m_Size - m_Margin;
        posY = screenHeight - m_Size - m_Margin - 40;
        break;
    }
    
    SetWindowPos(m_Hwnd, HWND_TOPMOST, posX, posY, m_Size, m_Size, SWP_NOACTIVATE);
}

void Radar::Show()
{
    if (m_Hwnd)
    {
        ShowWindow(m_Hwnd, SW_SHOWNOACTIVATE);
        m_Visible = true;
        
        // Start render thread if not running
        if (!m_RenderThreadRunning)
        {
            m_RenderThreadRunning = true;
            m_RenderThread = std::thread(&Radar::RenderThread, this);
        }
    }
}

void Radar::Hide()
{
    if (m_Hwnd)
    {
        ShowWindow(m_Hwnd, SW_HIDE);
        m_Visible = false;
    }
}

void Radar::RenderThread()
{
    // Set high priority for render thread
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    
    while (m_RenderThreadRunning && m_Running)
    {
        if (m_Visible && m_Context)
        {
            Render();
        }
        Sleep(16);  // ~60 FPS
    }
}

void Radar::SetOpacity(float opacity)
{
    m_Opacity = fmaxf(0.1f, fminf(1.0f, opacity));
    
    if (m_Hwnd)
    {
        BYTE alpha = (BYTE)(m_Opacity * 255);
        SetLayeredWindowAttributes(m_Hwnd, RGB(0, 0, 0), alpha, LWA_ALPHA);
    }
}

void Radar::Update()
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
}

void Radar::RenderText()
{
    if (!m_D2DRenderTarget || !m_ShowDegrees) return;
    
    // Recreate text format with proportional font size
    if (m_TextFormat)
    {
        m_TextFormat->Release();
        m_TextFormat = nullptr;
    }
    
    float fontSize = m_Size * 0.035f;  // Proportional to radar size
    if (fontSize < 8.0f) fontSize = 8.0f;
    
    m_DWriteFactory->CreateTextFormat(
        L"Consolas",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"en-us",
        &m_TextFormat);
    
    m_TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    
    float center = m_Size / 2.0f;
    float radius = m_Size / 2.0f - (m_Size * 0.06f);
    
    // Degree labels around the circle
    // 0 at top, clockwise: 0, 45, 90, 135, 180, 225, 270, 315
    struct DegreeLabel { int deg; float screenAngle; };
    DegreeLabel labels[] = {
        {0,   90.0f},   // Top
        {45,  45.0f},   // Top-Right
        {90,  0.0f},    // Right
        {135, -45.0f},  // Bottom-Right
        {180, -90.0f},  // Bottom
        {225, -135.0f}, // Bottom-Left
        {270, 180.0f},  // Left
        {315, 135.0f}   // Top-Left
    };
    
    for (const auto& label : labels)
    {
        float rad = label.screenAngle * 3.14159f / 180.0f;
        float x = center + cosf(rad) * radius;
        float y = center - sinf(rad) * radius;
        
        wchar_t text[16];
        swprintf_s(text, L"%d\u00B0", label.deg);
        
        D2D1_RECT_F rect = D2D1::RectF(x - 25, y - 10, x + 25, y + 10);
        m_D2DRenderTarget->DrawText(text, (UINT32)wcslen(text), m_TextFormat, rect, m_TextBrush);
    }
}

void Radar::Render()
{
    if (!m_Visible || !m_Context) return;
    
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    m_Time = (float)(currentTime.QuadPart - m_StartTime.QuadPart) / m_Frequency.QuadPart;

    D3D11_MAPPED_SUBRESOURCE mapped;
    m_Context->Map(m_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    ConstantBuffer* cb = (ConstantBuffer*)mapped.pData;
    cb->time = m_Time;
    m_Context->Unmap(m_ConstantBuffer, 0);

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    m_Context->ClearRenderTargetView(m_RenderTarget, clearColor);

    m_Context->OMSetRenderTargets(1, &m_RenderTarget, nullptr);
    m_Context->OMSetBlendState(m_BlendState, nullptr, 0xFFFFFFFF);
    m_Context->IASetInputLayout(m_InputLayout);
    m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_Context->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);

    m_Context->VSSetShader(m_VertexShader, nullptr, 0);
    m_Context->PSSetShader(m_PixelShader, nullptr, 0);
    m_Context->PSSetConstantBuffers(0, 1, &m_ConstantBuffer);

    m_Context->Draw(4, 0);
    
    // Render D2D content (text and signatures)
    if (m_D2DRenderTarget)
    {
        m_D2DRenderTarget->BeginDraw();
        RenderSignatures();
        RenderText();
        m_D2DRenderTarget->EndDraw();
    }
    
    m_SwapChain->Present(1, 0);
}

LRESULT CALLBACK Radar::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Radar* radar = reinterpret_cast<Radar*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
