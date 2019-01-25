#pragma once

#include <Windows.h>
#include <tchar.h>
#include <wrl/client.h>
#include <stdexcept>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include "dxcommon.h"
#include "Teapot.h"
#include "Model.h"
#include "LightPipeline.h"
#include "Light.h"
#include "Camera.h"
#include "Texture.h"

#include <DirectXMath.h>
using DirectX::XMFLOAT3; // for WaveFrontReader
#include "WaveFrontReader.h"

using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

static void CHK(HRESULT hr)
{
	if (FAILED(hr))
		throw runtime_error("HRESULT is failed value.");
}

namespace
{
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 480;
	const int BUFFER_COUNT = 2;
	HWND g_mainWindowHandle = 0;
};

class DX12Manager
{
public:
	DX12Manager(int width, int height, HWND hWnd);
	~DX12Manager();

	ID3D12Device* GetDevice() const { return mDev; }

	HRESULT CreateFactory();
	HRESULT CreateDevice();
	HRESULT CreateCommandList();
	HRESULT CreateRenderTargetView();
	HRESULT CreateDepthStencilBuffer();
	HRESULT CreateRootSignature();
	HRESULT CreatePipelineState();
	HRESULT CreateLightBuffer();
	HRESULT CreateShadowBuffer();
	HRESULT CreateShadowMapPipelineState();

	HRESULT RenderScene();

	bool InitializeScene();

	bool RenderReflection();

	HRESULT UpdateCommandList();
	HRESULT ExecuteCommandList();
	HRESULT Render();

private:

	HWND mWindowHandle;

	ComPtr<IDXGIFactory4> mDxgiFactory;
	ComPtr<IDXGIAdapter3> mAdapter;
	ComPtr<IDXGISwapChain1> mSwapChain;
	ComPtr<ID3D12Resource> mD3DBuffer[BUFFER_COUNT];
	int mBufferWidth, mBufferHeight;
	UINT64 mFrameCount = 0;

	ID3D12Device* mDev;
	ComPtr<ID3D12CommandAllocator> mCmdAlloc;
	ComPtr<ID3D12CommandQueue> mCmdQueue;

	ComPtr<ID3D12GraphicsCommandList> mCmdList;
	ComPtr<ID3D12Fence> mFence;
	HANDLE mFenceEveneHandle = 0;

	ComPtr<ID3D12DescriptorHeap> mDescHeapRtv;
	ComPtr<ID3D12DescriptorHeap> mDescHeapDsv;
	ComPtr<ID3D12DescriptorHeap> mDescHeapSampler;
	
	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12PipelineState> mPso;
	ComPtr<ID3D12Resource> mDB;
	
	//Shadow mapping
	ComPtr<ID3D12Resource> mLightBuffer; //Constant
	ComPtr<ID3D12Resource> mShadowBuffer; //Depth
	//ComPtr<ID3D12DescriptorHeap> mDescHeapLight;
	ComPtr<ID3D12DescriptorHeap> mDescHeapSB;
	ComPtr<ID3D12DescriptorHeap> mDescHeapST;//texture
	ComPtr<ID3D12PipelineState> mShadowPso;

	ComPtr<ID3D12DescriptorHeap> mImguiSrvDescHeap; //Imgui
	
	XMFLOAT3 mLightPos;
	XMFLOAT3 mLightDest;
	//XMFLOAT3 mLightDir;
	XMFLOAT4 mLightColor;

	D3D12_VIEWPORT mShadowViewport{};
	D3D12_RECT mShadowScissor{};

	Teapot* mTeapot = nullptr;
	Model* mCatBoard = nullptr;
	Model* mGround = nullptr;
	Light* mLight = nullptr;
	Camera* mCamera = nullptr;
	LightPipeline* mLightPipeline = nullptr;
	Texture* mRefractionTexture = nullptr;
	Texture* mReflectionTexture = nullptr;

	float mClearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
	float mAmbientColor[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
	float mDiffColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float mLightDir[3] = { 0.0f, -1.0f, 0.5f };

	void setResourceBarrier(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* res,
		D3D12_RESOURCE_STATES before,
		D3D12_RESOURCE_STATES after);

};

