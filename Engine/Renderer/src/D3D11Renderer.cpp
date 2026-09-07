#include <kimia/D3D11Renderer.h>

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32) && defined(KIMIA_HAS_D3D11)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace kimia {

namespace {

using Microsoft::WRL::ComPtr;

struct Vertex {
  f32 position[3]{};
  f32 normal[3]{};
  f32 uv[2]{};
};

// Mat4 is column-major and the HLSL matrices below are explicitly
// column-major. Keeping the same convention avoids a transpose at every draw.
struct FrameConstants {
  f32 viewProjection[16]{};
  f32 model[16]{};
  f32 color[4]{};
  f32 lightDirection[4]{};
  f32 ambient[4]{};
  f32 textureFlags[4]{};
};

struct MeshGpu {
  ComPtr<ID3D11Buffer> vertexBuffer;
  ComPtr<ID3D11Buffer> indexBuffer;
  u32 indexCount = 0U;
};

struct TextureGpu {
  ComPtr<ID3D11ShaderResourceView> view;
  ComPtr<ID3D11SamplerState> sampler;
  u64 signature = 0U;
};

constexpr char kVertexShader[] = R"HLSL(
cbuffer Frame : register(b0) {
  column_major float4x4 viewProjection;
  column_major float4x4 model;
  float4 color;
  float4 lightDirection;
  float4 ambient;
  float4 textureFlags;
};
struct VSIn {
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD0;
};
struct VSOut {
  float4 position : SV_POSITION;
  float3 normal : NORMAL0;
  float2 uv : TEXCOORD0;
};
VSOut main(VSIn input) {
  VSOut output;
  float4 world = mul(model, float4(input.position, 1.0));
  output.position = mul(viewProjection, world);
  output.normal = normalize(mul((float3x3)model, input.normal));
  output.uv = input.uv;
  return output;
}
)HLSL";

constexpr char kPixelShader[] = R"HLSL(
cbuffer Frame : register(b0) {
  column_major float4x4 viewProjection;
  column_major float4x4 model;
  float4 color;
  float4 lightDirection;
  float4 ambient;
  float4 textureFlags;
};
Texture2D baseTexture : register(t0);
SamplerState baseSampler : register(s0);
struct PSIn {
  float4 position : SV_POSITION;
  float3 normal : NORMAL0;
  float2 uv : TEXCOORD0;
};
float4 main(PSIn input) : SV_TARGET {
  float3 n = normalize(input.normal);
  float3 l = normalize(-lightDirection.xyz);
  float diffuse = max(dot(n, l), 0.0);
  float4 surface = color;
  if (textureFlags.x > 0.5) surface *= baseTexture.Sample(baseSampler, input.uv);
  float3 lit = surface.rgb * (ambient.rgb + diffuse * (1.0 - ambient.rgb));
  return float4(saturate(lit), surface.a);
}
)HLSL";

bool compileShader(const char* source, const char* entry, const char* target, ComPtr<ID3DBlob>& bytecode,
                   std::string& error) {
  ComPtr<ID3DBlob> diagnostics;
  const HRESULT result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry, target,
                                    D3DCOMPILE_ENABLE_STRICTNESS, 0, &bytecode, &diagnostics);
  if (SUCCEEDED(result)) return true;
  if (diagnostics != nullptr) {
    error.assign(static_cast<const char*>(diagnostics->GetBufferPointer()), diagnostics->GetBufferSize());
  } else {
    error = "D3DCompile failed";
  }
  return false;
}

void copyMatrix(f32 (&destination)[16], const Mat4& source) {
  for (i32 column = 0; column < 4; ++column) {
    for (i32 row = 0; row < 4; ++row) {
      destination[column * 4 + row] = static_cast<f32>(source.at(column, row));
    }
  }
}

u64 textureSignature(const Image& image) {
  // Images are frame-owned but their pixels may be edited in place by the
  // Workbench. A compact FNV-1a signature keeps the pointer cache correct
  // without forcing callers to replace the Image object.
  u64 hash = 1469598103934665603ULL;
  const auto mix = [&hash](u8 value) {
    hash ^= static_cast<u64>(value);
    hash *= 1099511628211ULL;
  };
  for (const u8 value : {static_cast<u8>(image.width & 0xff), static_cast<u8>((image.width >> 8) & 0xff),
                         static_cast<u8>(image.height & 0xff), static_cast<u8>((image.height >> 8) & 0xff),
                         static_cast<u8>(image.channels)}) {
    mix(value);
  }
  for (const u8 value : image.pixels) mix(value);
  return hash;
}

const char* featureLevelString(D3D_FEATURE_LEVEL level) {
  switch (level) {
    case D3D_FEATURE_LEVEL_11_1:
      return "11_1";
    case D3D_FEATURE_LEVEL_11_0:
      return "11_0";
    default:
      return "unknown";
  }
}

std::string narrowAdapterName(const DXGI_ADAPTER_DESC& description) {
  if (description.Description[0] == L'\0') return "D3D11 hardware adapter";
  const int length = WideCharToMultiByte(CP_UTF8, 0, description.Description, -1, nullptr, 0, nullptr, nullptr);
  if (length <= 1) return "D3D11 hardware adapter";
  std::string result(static_cast<usize>(length), '\0');
  WideCharToMultiByte(CP_UTF8, 0, description.Description, -1, result.data(), length, nullptr, nullptr);
  if (!result.empty() && result.back() == '\0') result.pop_back();
  return result;
}

}  // namespace

struct D3D11Renderer::Impl {
  D3D11Options options;
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;
  ComPtr<IDXGISwapChain> swapChain;
  ComPtr<ID3D11RenderTargetView> renderTarget;
  ComPtr<ID3D11Texture2D> depthTexture;
  ComPtr<ID3D11DepthStencilView> depthView;
  ComPtr<ID3D11VertexShader> vertexShader;
  ComPtr<ID3D11PixelShader> pixelShader;
  ComPtr<ID3D11InputLayout> inputLayout;
  ComPtr<ID3D11Buffer> constantBuffer;
  std::unordered_map<const MeshData*, MeshGpu> meshes;
  std::unordered_map<const Image*, TextureGpu> textures;
  std::string adapter;
  std::string featureLevel = "D3D_FEATURE_LEVEL_11_0";
  u64 dedicatedVideoMemory = 0U;
  bool initialized = false;

  bool createTargets(i32 width, i32 height, std::string& error) {
    if (width <= 0 || height <= 0) {
      error = "D3D11 target size must be positive";
      return false;
    }
    renderTarget.Reset();
    depthTexture.Reset();
    depthView.Reset();

    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT result = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(result)) {
      error = "IDXGISwapChain::GetBuffer failed";
      return false;
    }
    result = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTarget);
    if (FAILED(result)) {
      error = "CreateRenderTargetView failed";
      return false;
    }

    D3D11_TEXTURE2D_DESC depthDescription{};
    depthDescription.Width = static_cast<UINT>(width);
    depthDescription.Height = static_cast<UINT>(height);
    depthDescription.MipLevels = 1;
    depthDescription.ArraySize = 1;
    depthDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDescription.SampleDesc.Count = 1;
    depthDescription.Usage = D3D11_USAGE_DEFAULT;
    depthDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    result = device->CreateTexture2D(&depthDescription, nullptr, &depthTexture);
    if (FAILED(result)) {
      error = "CreateTexture2D(depth) failed";
      return false;
    }
    result = device->CreateDepthStencilView(depthTexture.Get(), nullptr, &depthView);
    if (FAILED(result)) {
      error = "CreateDepthStencilView failed";
      return false;
    }
    return true;
  }

  bool uploadMesh(const MeshData& mesh, MeshGpu& destination, std::string& error) {
    if (!mesh.isValid()) {
      error = "cannot upload invalid mesh to D3D11";
      return false;
    }
    for (const u32 index : mesh.indices) {
      if (static_cast<usize>(index) >= mesh.positions.size()) {
        error = "mesh index is outside the D3D11 vertex buffer";
        return false;
      }
    }
    std::vector<Vertex> vertices;
    vertices.reserve(mesh.positions.size());
    for (usize i = 0; i < mesh.positions.size(); ++i) {
      Vertex vertex{};
      vertex.position[0] = static_cast<f32>(mesh.positions[i].x);
      vertex.position[1] = static_cast<f32>(mesh.positions[i].y);
      vertex.position[2] = static_cast<f32>(mesh.positions[i].z);
      vertex.normal[0] = static_cast<f32>(mesh.normals[i].x);
      vertex.normal[1] = static_cast<f32>(mesh.normals[i].y);
      vertex.normal[2] = static_cast<f32>(mesh.normals[i].z);
      if (!mesh.uvs.empty()) {
        vertex.uv[0] = static_cast<f32>(mesh.uvs[i].x);
        vertex.uv[1] = static_cast<f32>(mesh.uvs[i].y);
      }
      vertices.push_back(vertex);
    }

    D3D11_BUFFER_DESC vertexDescription{};
    vertexDescription.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    vertexDescription.Usage = D3D11_USAGE_DEFAULT;
    vertexDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = vertices.data();
    HRESULT result = device->CreateBuffer(&vertexDescription, &vertexData, &destination.vertexBuffer);
    if (FAILED(result)) {
      error = "CreateBuffer(vertex) failed";
      return false;
    }

    D3D11_BUFFER_DESC indexDescription{};
    indexDescription.ByteWidth = static_cast<UINT>(mesh.indices.size() * sizeof(u32));
    indexDescription.Usage = D3D11_USAGE_DEFAULT;
    indexDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexData{};
    indexData.pSysMem = mesh.indices.data();
    result = device->CreateBuffer(&indexDescription, &indexData, &destination.indexBuffer);
    if (FAILED(result)) {
      error = "CreateBuffer(index) failed";
      destination.vertexBuffer.Reset();
      return false;
    }
    destination.indexCount = static_cast<u32>(mesh.indices.size());
    return true;
  }

  bool uploadTexture(const Image& image, TextureGpu& destination, std::string& error) {
    if (image.isEmpty() || image.width <= 0 || image.height <= 0 ||
        (image.channels != 3 && image.channels != 4)) {
      error = "cannot upload invalid D3D11 texture";
      return false;
    }
    const usize pixelCount = static_cast<usize>(image.width) * static_cast<usize>(image.height);
    const usize requiredBytes = pixelCount * static_cast<usize>(image.channels);
    if (image.pixels.size() < requiredBytes) {
      error = "D3D11 texture pixel buffer is truncated";
      return false;
    }
    std::vector<u8> rgba(pixelCount * 4U);
    for (usize i = 0; i < pixelCount; ++i) {
      const usize source = i * static_cast<usize>(image.channels);
      const usize target = i * 4U;
      rgba[target] = image.pixels[source];
      rgba[target + 1U] = image.pixels[source + 1U];
      rgba[target + 2U] = image.pixels[source + 2U];
      rgba[target + 3U] = image.channels == 4 ? image.pixels[source + 3U] : 255U;
    }

    D3D11_TEXTURE2D_DESC description{};
    description.Width = static_cast<UINT>(image.width);
    description.Height = static_cast<UINT>(image.height);
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = rgba.data();
    data.SysMemPitch = static_cast<UINT>(image.width * 4);
    ComPtr<ID3D11Texture2D> texture;
    HRESULT result = device->CreateTexture2D(&description, &data, &texture);
    if (FAILED(result)) {
      error = "CreateTexture2D(texture) failed";
      return false;
    }
    result = device->CreateShaderResourceView(texture.Get(), nullptr, &destination.view);
    if (FAILED(result)) {
      error = "CreateShaderResourceView failed";
      return false;
    }
    D3D11_SAMPLER_DESC samplerDescription{};
    samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;
    result = device->CreateSamplerState(&samplerDescription, &destination.sampler);
    if (FAILED(result)) {
      error = "CreateSamplerState failed";
      destination.view.Reset();
      return false;
    }
    destination.signature = textureSignature(image);
    return true;
  }
};

D3D11Renderer::D3D11Renderer() : impl_(new Impl()) {}
D3D11Renderer::~D3D11Renderer() { shutdown(); }

bool D3D11Renderer::initialize(const D3D11Options& options, std::string& error) {
  shutdown();
  impl_->options = options;
  if (options.nativeWindow == nullptr) {
    error = "D3D11 requires a native HWND";
    return false;
  }
  if (options.width <= 0 || options.height <= 0) {
    error = "D3D11 target size must be positive";
    return false;
  }

  UINT flags = 0U;
  if (options.debugLayer) flags |= D3D11_CREATE_DEVICE_DEBUG;
  const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
  DXGI_SWAP_CHAIN_DESC swapDescription{};
  swapDescription.BufferCount = 2;
  swapDescription.BufferDesc.Width = static_cast<UINT>(options.width);
  swapDescription.BufferDesc.Height = static_cast<UINT>(options.height);
  swapDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapDescription.OutputWindow = static_cast<HWND>(options.nativeWindow);
  swapDescription.SampleDesc.Count = 1;
  swapDescription.Windowed = TRUE;
  swapDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  D3D_FEATURE_LEVEL selected{};
  HRESULT result = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, static_cast<UINT>(sizeof(levels) / sizeof(levels[0])),
      D3D11_SDK_VERSION, &swapDescription, &impl_->swapChain, &impl_->device, &selected, &impl_->context);
  if (FAILED(result)) {
    // Some older Windows installations reject flip-discard even though the
    // GPU supports D3D11. Retry with the conservative discard swap effect.
    swapDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, static_cast<UINT>(sizeof(levels) / sizeof(levels[0])),
        D3D11_SDK_VERSION, &swapDescription, &impl_->swapChain, &impl_->device, &selected, &impl_->context);
  }
  if (FAILED(result)) {
    // A stripped-down Windows image may expose the 11.0 runtime but not the
    // 11.1 feature-level entry point. Keep the hardware path usable there.
    const D3D_FEATURE_LEVEL fallbackLevels[] = {D3D_FEATURE_LEVEL_11_0};
    result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, fallbackLevels,
        static_cast<UINT>(sizeof(fallbackLevels) / sizeof(fallbackLevels[0])), D3D11_SDK_VERSION,
        &swapDescription, &impl_->swapChain, &impl_->device, &selected, &impl_->context);
  }
  if (FAILED(result)) {
    error = "D3D11CreateDeviceAndSwapChain failed";
    shutdown();
    return false;
  }
  impl_->featureLevel = std::string("D3D_FEATURE_LEVEL_") + featureLevelString(selected);

  ComPtr<IDXGIDevice> dxgiDevice;
  ComPtr<IDXGIAdapter> adapter;
  DXGI_ADAPTER_DESC adapterDescription{};
  if (SUCCEEDED(impl_->device.As(&dxgiDevice)) && SUCCEEDED(dxgiDevice->GetAdapter(&adapter)) &&
      SUCCEEDED(adapter->GetDesc(&adapterDescription))) {
    impl_->adapter = narrowAdapterName(adapterDescription);
    impl_->dedicatedVideoMemory = static_cast<u64>(adapterDescription.DedicatedVideoMemory);
  } else {
    impl_->adapter = "D3D11 hardware adapter";
  }

  ComPtr<ID3DBlob> vertexBytecode;
  ComPtr<ID3DBlob> pixelBytecode;
  if (!compileShader(kVertexShader, "main", "vs_5_0", vertexBytecode, error) ||
      !compileShader(kPixelShader, "main", "ps_5_0", pixelBytecode, error)) {
    shutdown();
    return false;
  }
  result = impl_->device->CreateVertexShader(vertexBytecode->GetBufferPointer(), vertexBytecode->GetBufferSize(),
                                             nullptr, &impl_->vertexShader);
  if (FAILED(result)) {
    error = "CreateVertexShader failed";
    shutdown();
    return false;
  }
  result = impl_->device->CreatePixelShader(pixelBytecode->GetBufferPointer(), pixelBytecode->GetBufferSize(), nullptr,
                                            &impl_->pixelShader);
  if (FAILED(result)) {
    error = "CreatePixelShader failed";
    shutdown();
    return false;
  }

  const D3D11_INPUT_ELEMENT_DESC inputDescription[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };
  result = impl_->device->CreateInputLayout(inputDescription, static_cast<UINT>(sizeof(inputDescription) / sizeof(inputDescription[0])),
                                            vertexBytecode->GetBufferPointer(), vertexBytecode->GetBufferSize(),
                                            &impl_->inputLayout);
  if (FAILED(result)) {
    error = "CreateInputLayout failed";
    shutdown();
    return false;
  }

  D3D11_BUFFER_DESC constantDescription{};
  constantDescription.ByteWidth = sizeof(FrameConstants);
  constantDescription.Usage = D3D11_USAGE_DYNAMIC;
  constantDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  constantDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  result = impl_->device->CreateBuffer(&constantDescription, nullptr, &impl_->constantBuffer);
  if (FAILED(result)) {
    error = "CreateBuffer(constants) failed";
    shutdown();
    return false;
  }
  if (!impl_->createTargets(options.width, options.height, error)) {
    shutdown();
    return false;
  }
  impl_->initialized = true;
  return true;
}

void D3D11Renderer::shutdown() {
  if (impl_ == nullptr) return;
  impl_->initialized = false;
  impl_->meshes.clear();
  impl_->textures.clear();
  impl_->renderTarget.Reset();
  impl_->depthView.Reset();
  impl_->depthTexture.Reset();
  impl_->constantBuffer.Reset();
  impl_->inputLayout.Reset();
  impl_->vertexShader.Reset();
  impl_->pixelShader.Reset();
  impl_->swapChain.Reset();
  impl_->context.Reset();
  impl_->device.Reset();
  impl_->adapter.clear();
  impl_->featureLevel = "D3D_FEATURE_LEVEL_11_0";
  impl_->dedicatedVideoMemory = 0U;
}

bool D3D11Renderer::ready() const { return impl_ != nullptr && impl_->initialized; }

bool D3D11Renderer::resize(i32 width, i32 height, std::string& error) {
  if (!ready()) {
    error = "D3D11 renderer is not initialized";
    return false;
  }
  if (width <= 0 || height <= 0) {
    error = "D3D11 resize dimensions must be positive";
    return false;
  }
  impl_->context->OMSetRenderTargets(0, nullptr, nullptr);
  impl_->renderTarget.Reset();
  impl_->depthView.Reset();
  impl_->depthTexture.Reset();
  HRESULT result = impl_->swapChain->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height),
                                                    DXGI_FORMAT_UNKNOWN, 0);
  if (FAILED(result)) {
    error = "ResizeBuffers failed";
    return false;
  }
  impl_->options.width = width;
  impl_->options.height = height;
  return impl_->createTargets(width, height, error);
}

bool D3D11Renderer::render(const RenderScene& scene, i32 width, i32 height, std::string& error) {
  if (!ready()) {
    error = "D3D11 renderer is not initialized";
    return false;
  }
  if (width <= 0 || height <= 0) {
    error = "D3D11 render dimensions must be positive";
    return false;
  }
  if (width != impl_->options.width || height != impl_->options.height) {
    if (!resize(width, height, error)) return false;
  }

  const float clear[] = {0.035F, 0.04F, 0.055F, 1.0F};
  ID3D11RenderTargetView* renderTargets[] = {impl_->renderTarget.Get()};
  impl_->context->OMSetRenderTargets(1, renderTargets, impl_->depthView.Get());
  impl_->context->ClearRenderTargetView(impl_->renderTarget.Get(), clear);
  impl_->context->ClearDepthStencilView(impl_->depthView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0F, 0);

  const D3D11_VIEWPORT viewport{0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height), 0.0F, 1.0F};
  impl_->context->RSSetViewports(1, &viewport);
  impl_->context->IASetInputLayout(impl_->inputLayout.Get());
  impl_->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  impl_->context->VSSetShader(impl_->vertexShader.Get(), nullptr, 0);
  impl_->context->PSSetShader(impl_->pixelShader.Get(), nullptr, 0);
  impl_->context->VSSetConstantBuffers(0, 1, impl_->constantBuffer.GetAddressOf());
  impl_->context->PSSetConstantBuffers(0, 1, impl_->constantBuffer.GetAddressOf());

  // RenderScene uses the legacy OpenGL-style depth range [-1, 1]. D3D11
  // expects [0, 1], so convert clip-space z once per frame rather than
  // forcing every game camera to know which backend is active.
  Mat4 depthConversion{};
  depthConversion.at(2, 2) = 0.5;
  depthConversion.at(3, 2) = 0.5;
  const Mat4 viewProjection = depthConversion * scene.projection * scene.view;
  std::unordered_map<const Image*, u64> frameTextureSignatures;
  for (const RenderObject& object : scene.objects) {
    if (object.mesh == nullptr || !object.mesh->isValid()) continue;
    auto found = impl_->meshes.find(object.mesh);
    if (found == impl_->meshes.end()) {
      MeshGpu gpu;
      if (!impl_->uploadMesh(*object.mesh, gpu, error)) return false;
      found = impl_->meshes.emplace(object.mesh, std::move(gpu)).first;
    }

    TextureGpu* texture = nullptr;
    if (object.texture != nullptr) {
      auto textureFound = impl_->textures.find(object.texture);
      auto signatureFound = frameTextureSignatures.find(object.texture);
      if (signatureFound == frameTextureSignatures.end()) {
        signatureFound = frameTextureSignatures.emplace(object.texture, textureSignature(*object.texture)).first;
      }
      const u64 signature = signatureFound->second;
      if (textureFound == impl_->textures.end() || textureFound->second.signature != signature) {
        TextureGpu gpu;
        if (!impl_->uploadTexture(*object.texture, gpu, error)) return false;
        if (textureFound == impl_->textures.end()) {
          textureFound = impl_->textures.emplace(object.texture, std::move(gpu)).first;
        } else {
          textureFound->second = std::move(gpu);
        }
      }
      texture = &textureFound->second;
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    const HRESULT mapResult = impl_->context->Map(impl_->constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(mapResult)) {
      error = "Map(frame constants) failed";
      return false;
    }
    FrameConstants constants{};
    copyMatrix(constants.viewProjection, viewProjection);
    copyMatrix(constants.model, object.model);
    constants.color[0] = static_cast<f32>(object.color.x);
    constants.color[1] = static_cast<f32>(object.color.y);
    constants.color[2] = static_cast<f32>(object.color.z);
    constants.color[3] = 1.0F;
    const Vec3 light = scene.lightDirection.normalized();
    constants.lightDirection[0] = static_cast<f32>(light.x);
    constants.lightDirection[1] = static_cast<f32>(light.y);
    constants.lightDirection[2] = static_cast<f32>(light.z);
    constants.ambient[0] = static_cast<f32>(scene.ambient);
    constants.ambient[1] = static_cast<f32>(scene.ambient);
    constants.ambient[2] = static_cast<f32>(scene.ambient);
    constants.ambient[3] = 1.0F;
    constants.textureFlags[0] = texture != nullptr ? 1.0F : 0.0F;
    std::memcpy(mapped.pData, &constants, sizeof(constants));
    impl_->context->Unmap(impl_->constantBuffer.Get(), 0);

    ID3D11ShaderResourceView* shaderResource = texture != nullptr ? texture->view.Get() : nullptr;
    ID3D11SamplerState* sampler = texture != nullptr ? texture->sampler.Get() : nullptr;
    impl_->context->PSSetShaderResources(0, 1, &shaderResource);
    impl_->context->PSSetSamplers(0, 1, &sampler);

    const UINT stride = sizeof(Vertex);
    const UINT offset = 0U;
    ID3D11Buffer* vertexBuffer = found->second.vertexBuffer.Get();
    impl_->context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    impl_->context->IASetIndexBuffer(found->second.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    impl_->context->DrawIndexed(found->second.indexCount, 0, 0);
  }

  const HRESULT presentResult = impl_->swapChain->Present(impl_->options.vsync ? 1U : 0U, 0U);
  if (FAILED(presentResult)) {
    if (presentResult == DXGI_ERROR_DEVICE_REMOVED || presentResult == DXGI_ERROR_DEVICE_RESET) {
      error = "D3D11 device was removed or reset";
      shutdown();
    } else {
      error = "IDXGISwapChain::Present failed";
    }
    return false;
  }
  return true;
}

void D3D11Renderer::setVsync(bool enabled) {
  if (impl_ != nullptr) impl_->options.vsync = enabled;
}

bool D3D11Renderer::vsync() const { return impl_ != nullptr && impl_->options.vsync; }

const std::string& D3D11Renderer::adapterName() const {
  static const std::string unavailable = "D3D11 unavailable";
  return impl_ != nullptr ? impl_->adapter : unavailable;
}

const std::string& D3D11Renderer::featureLevelName() const {
  static const std::string unavailable = "D3D11 unavailable";
  return impl_ != nullptr ? impl_->featureLevel : unavailable;
}

u64 D3D11Renderer::dedicatedVideoMemoryBytes() const {
  return impl_ != nullptr ? impl_->dedicatedVideoMemory : 0U;
}

}  // namespace kimia

#else

namespace kimia {

struct D3D11Renderer::Impl {};

D3D11Renderer::D3D11Renderer() : impl_(new Impl()) {}
D3D11Renderer::~D3D11Renderer() = default;

bool D3D11Renderer::initialize(const D3D11Options&, std::string& error) {
  error = "D3D11 is only available on Windows";
  return false;
}

void D3D11Renderer::shutdown() {}
bool D3D11Renderer::ready() const { return false; }
bool D3D11Renderer::resize(i32, i32, std::string& error) {
  error = "D3D11 is only available on Windows";
  return false;
}
bool D3D11Renderer::render(const RenderScene&, i32, i32, std::string& error) {
  error = "D3D11 is only available on Windows";
  return false;
}
void D3D11Renderer::setVsync(bool) {}
bool D3D11Renderer::vsync() const { return false; }
const std::string& D3D11Renderer::adapterName() const {
  static const std::string unavailable = "D3D11 unavailable";
  return unavailable;
}
const std::string& D3D11Renderer::featureLevelName() const {
  static const std::string unavailable = "D3D11 unavailable";
  return unavailable;
}
u64 D3D11Renderer::dedicatedVideoMemoryBytes() const { return 0U; }

}  // namespace kimia

#endif
