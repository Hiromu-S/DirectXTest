#include <Windows.h>
#include <tchar.h>
#include <wrl/client.h>
#include <stdexcept>
#include <dxgi1_3.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <fstream>
#include <vector>
#include "dxcommon.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace std;
using Microsoft::WRL::ComPtr;

namespace {
	const int WINDOW_WIDTH = 400;
	const int WINDOW_HEIGHT = 400;
	const int BUFFER_COUNT = 2;
	HWND g_mainWindowHandle = 0;
};

void CHK(HRESULT hr) {
	if FAILED(hr) {
		throw runtime_error("HRESULT is failed value.");
		//_exit(1);
	}
}

class D3D {
	ComPtr<IDXGIFactory2> mDxgiFactory;
	ComPtr<IDXGISwapChain1> mSwapChain;
	ComPtr<ID3D12Resource> mD3DBuffer[BUFFER_COUNT];
	int mBufferWidth, mBufferHeight;
	UINT64 mFrameCount = 0;

	BOOL mIsFullscreen = false;

	ID3D12Device* mDevice;
	ComPtr<ID3D12CommandAllocator> mCmdAlloc;
	ComPtr<ID3D12CommandQueue> mCmdQueue;

	ComPtr<ID3D12GraphicsCommandList> mCmdList;
	ComPtr<ID3D12Fence> mFence;
	HANDLE mFenceEventHandle = 0;

	ComPtr<ID3D12DescriptorHeap> mDescHeapRtv;
	ComPtr<ID3D12DescriptorHeap> mDescHeapCbvSrvUav;
	ComPtr<ID3D12DescriptorHeap> mDescHeapSampler;

	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12PipelineState> mPso;
	ComPtr<ID3D12Resource> mTex;
	ComPtr<ID3D12Resource> mCB;
	void* mCBUploadPtr = nullptr;
	ComPtr<ID3D12Resource> mVB;
	D3D12_VERTEX_BUFFER_VIEW mVBView = {};
	D3D12_INDEX_BUFFER_VIEW mIBView = {};

public:
	D3D(int width, int height, HWND hWnd)
		:mBufferWidth(width), mBufferHeight(height), mDevice(nullptr) {
			{
#if _DEBUG

				CHK(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(mDxgiFactory.ReleaseAndGetAddressOf())));

#else

				CHK(CreateDXGIFactory2(0, IID_PPV_ARGS(mDxgiFactory.ReleaseAndGetAddressOf())));

#endif /* _DEBUG */

			}

#if _DEBUG

		ID3D12Debug* debug = nullptr;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
		if (debug) {
			debug->EnableDebugLayer();
			debug->Release();
			debug = nullptr;
		}

#endif /* _DEBUG */

		//Create device
		ID3D12Device* dev;
		CHK(D3D12CreateDevice(
			nullptr,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&dev)));
		mDevice = dev;
		CHK(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCmdAlloc.ReleaseAndGetAddressOf())));
		
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		CHK(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mCmdQueue.ReleaseAndGetAddressOf())));
		
		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Width = width;
		scDesc.Height = height;
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.SampleDesc.Count = 1;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = BUFFER_COUNT;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		CHK(mDxgiFactory->CreateSwapChainForHwnd(mCmdQueue.Get(), hWnd, &scDesc, nullptr, nullptr, mSwapChain.ReleaseAndGetAddressOf()));
		CHK(mDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			mCmdAlloc.Get(),
			nullptr,
			IID_PPV_ARGS(mCmdList.ReleaseAndGetAddressOf())));
		CHK(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.ReleaseAndGetAddressOf())));

		mFenceEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		for (int i = 0; i < BUFFER_COUNT; i++) {
			CHK(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mD3DBuffer[i].ReleaseAndGetAddressOf())));
			mD3DBuffer[i]->SetName(L"SwapChain_Buffer");
		}

		{
			//Create Descriptor heap for 10 Descriptors.
			//Descriptor heap allocates memorys storing all descriptors.
			//Example...CBV, UAV, SRV, specially RTV, DSV.
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = 10;
			//desc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
			desc.NodeMask = 0;
			CHK(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mDescHeapRtv.ReleaseAndGetAddressOf())));

			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = 100;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			desc.NodeMask = 0;
			CHK(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mDescHeapCbvSrvUav.ReleaseAndGetAddressOf())));

			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			desc.NumDescriptors = 10;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			desc.NodeMask = 0;
			CHK(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mDescHeapSampler.ReleaseAndGetAddressOf())));
		}

		auto rtvStep = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		for (auto i = 0u; i < BUFFER_COUNT; i++) {
			auto d = mDescHeapRtv->GetCPUDescriptorHandleForHeapStart();
			d.ptr += i * rtvStep;
			mDevice->CreateRenderTargetView(mD3DBuffer[i].Get(), nullptr, d);
		}

		{
			CD3DX12_DESCRIPTOR_RANGE descRange1[2], descRange2[1];
			descRange1[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
			descRange1[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
			descRange2[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

			CD3DX12_ROOT_PARAMETER rootParam[2];
			rootParam[0].InitAsDescriptorTable(ARRAYSIZE(descRange1), descRange1);
			rootParam[1].InitAsDescriptorTable(ARRAYSIZE(descRange2), descRange2);

			ID3D10Blob *sig, *info;
			auto rootSigDesc = D3D12_ROOT_SIGNATURE_DESC();
			rootSigDesc.NumParameters = 2;
			rootSigDesc.NumStaticSamplers = 0;
			rootSigDesc.pParameters = rootParam;
			rootSigDesc.pStaticSamplers = nullptr;
			CHK(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &info));
			mDevice->CreateRootSignature(0,
				sig->GetBufferPointer(),
				sig->GetBufferSize(),
				IID_PPV_ARGS(mRootSignature.ReleaseAndGetAddressOf()));
			sig->Release();
		}

		ID3D10Blob *vs, *ps;
		{
			ID3D10Blob *info;
			UINT flag = 0;
#if _DEBUG
			flag |= D3DCOMPILE_DEBUG;
#endif /*_DEBUG*/
			CHK(D3DCompileFromFile(L"Fullscreen.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", flag, 0, &vs, &info));
			CHK(D3DCompileFromFile(L"Fullscreen.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", flag, 0, &ps, &info));
		}
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.InputLayout.NumElements = 0;
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.VS.pShaderBytecode = vs->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vs->GetBufferSize();
		psoDesc.PS.pShaderBytecode = ps->GetBufferPointer();
		psoDesc.PS.BytecodeLength = ps->GetBufferSize();
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(CCCD3DX12_DEFAULT());
		psoDesc.BlendState = CD3DX12_BLEND_DESC(CCCD3DX12_DEFAULT());
		//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		//psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		//psoDesc.RasterizerState.DepthClipEnable = true;
		psoDesc.DepthStencilState.DepthEnable = false;
		psoDesc.DepthStencilState.StencilEnable = false;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		CHK(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(mPso.ReleaseAndGetAddressOf())));
		vs->Release();
		ps->Release();
		/*
		struct float3 {
			float f[3];
		};
		float3 vbData[4] = {
			{-0.7f, 0.7f, 0.0f },
			{0.7f, 0.7f, 0.0f},
			{-0.7f, -0.7f, 0.0f},
			{0.7f, -0.7f, 0.0f}
		};
		unsigned short ibData[6] = { 0, 1, 2, 2, 1, 3 };
		CHK(mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vbData) + sizeof(ibData)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(mVB.ReleaseAndGetAddressOf())));
		mVB->SetName(L"VertexBuffer");
		char* vbUploadPtr = nullptr;
		CHK(mVB->Map(0, nullptr, reinterpret_cast<void**>(&vbUploadPtr)));
		memcpy_s(vbUploadPtr, sizeof(vbData), vbData, sizeof(vbData));
		memcpy_s(vbUploadPtr + sizeof(vbData), sizeof(ibData), ibData, sizeof(ibData));
		mVB->Unmap(0, nullptr);

		mVBView.BufferLocation = mVB->GetGPUVirtualAddress();
		mVBView.StrideInBytes = sizeof(float3);
		mVBView.SizeInBytes = sizeof(vbData);
		mIBView.BufferLocation = mVB->GetGPUVirtualAddress() + sizeof(vbData);
		mIBView.Format = DXGI_FORMAT_R16_UINT;
		mIBView.SizeInBytes = sizeof(ibData);
		*/

		{ //Read DDS file
			vector<char> texData(4 * 256 * 256);
			ifstream ifs("d3d12.dds", ios::binary);
			if (!ifs) {
				throw runtime_error("Texture not found");
			}
			ifs.seekg(128, ios::beg);
			ifs.read(texData.data(), texData.size());
			auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_B8G8R8A8_UNORM, 256, 256, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE,
				D3D12_TEXTURE_LAYOUT_UNKNOWN, 0);
			
			CHK(mDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				//&CD3DX12_RESOURCE_DESC::Buffer(texData.size()),
				//&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_B8G8R8A8_UNORM, 256, 256, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE,
				//							  D3D12_TEXTURE_LAYOUT_UNKNOWN, 0),
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(mTex.ReleaseAndGetAddressOf())));
			
			mTex->SetName(L"Texture");
			D3D12_BOX box = {};
			box.right = 256;
			box.bottom = 256;
			box.back = 1;
			CHK(mTex->WriteToSubresource(0, &box, texData.data(), 4 * 256, 4 * 256 * 256));
			
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0; // No MIP
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		mDevice->CreateShaderResourceView(mTex.Get(), &srvDesc, mDescHeapCbvSrvUav->GetCPUDescriptorHandleForHeapStart());

		D3D12_SAMPLER_DESC samplerDesc;
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = FLT_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		mDevice->CreateSampler(&samplerDesc, mDescHeapSampler->GetCPUDescriptorHandleForHeapStart());

		CHK(mDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(mCB.ReleaseAndGetAddressOf())));
		mCB->SetName(L"ConstantBuffer");
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mCB->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // must be a multiple of 256
		auto cbvSrvUavDescHandle = mDescHeapCbvSrvUav->GetCPUDescriptorHandleForHeapStart();
		auto cbvSrvUavStep = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cbvSrvUavDescHandle.ptr += cbvSrvUavStep;
		mDevice->CreateConstantBufferView(
			&cbvDesc,
			cbvSrvUavDescHandle);
		CHK(mCB->Map(0, nullptr, reinterpret_cast<void**>(&mCBUploadPtr)));
	}

	~D3D() {
		CloseHandle(mFenceEventHandle);

		BOOL isFullscreen;
		CHK(mSwapChain->GetFullscreenState(&isFullscreen, nullptr));
		if (isFullscreen) {
			CHK(mSwapChain->SetFullscreenState(FALSE, nullptr));
		}
	}

	ID3D12Device* GetDevice() const { return mDevice; }

	void Draw() {
		mFrameCount++;
		/*
		if (mFrameCount == 9)
		{
			CHK(mSwapChain->SetFullscreenState(TRUE, nullptr));
		}
		if (mFrameCount == 119)
		{
			CHK(mSwapChain->SetFullscreenState(FALSE, nullptr));
		}

		// Check window state
		BOOL isFullscreen;
		CHK(mSwapChain->GetFullscreenState(&isFullscreen, nullptr));

		// If state was changed, regenerate resources
		if (mIsFullscreen != isFullscreen) {
			mIsFullscreen = isFullscreen;
			for (auto& d : mD3DBuffer) {
				d.Reset();
			}

			RECT rect;
			GetClientRect(GetActiveWindow(), &rect);
			DXGI_MODE_DESC mode = {};
			mode.Format = DXGI_FORMAT_UNKNOWN;
			mode.Width = rect.right;
			mode.Height = rect.bottom;
			mode.RefreshRate.Denominator = 1;
			mode.RefreshRate.Numerator = 60; // FIXME: Frame rate is variant
			CHK(mSwapChain->ResizeTarget(&mode));
			CHK(mSwapChain->ResizeBuffers(BUFFER_COUNT, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
			auto rtvStep = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			for (auto i = 0u; i < BUFFER_COUNT; i++) {
				CHK(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mD3DBuffer[i].ReleaseAndGetAddressOf())));
				mD3DBuffer[i]->SetName(L"SwapChain_Buffer");

				auto desc = mD3DBuffer[i]->GetDesc();
				mBufferWidth = (int)desc.Width;
				mBufferHeight = (int)desc.Height;
				stringstream ss;
				ss << "#SwapChain size changed (" << mBufferWidth << "," << mBufferHeight << ")" << endl;
				OutputDebugStringA(ss.str().c_str());

				auto d = mDescHeapRtv->GetCPUDescriptorHandleForHeapStart();
				d.ptr += i * rtvStep;
				mDevice->CreateRenderTargetView(mD3DBuffer[i].Get(), nullptr, d);
			}
		}*/

		auto descHandleRtvStep = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE descHandleRtv = mDescHeapRtv->GetCPUDescriptorHandleForHeapStart();
		descHandleRtv.ptr += ((mFrameCount - 1) % BUFFER_COUNT) * descHandleRtvStep;
		ID3D12Resource* d3dBuffer = mD3DBuffer[(mFrameCount - 1) % BUFFER_COUNT].Get();

		SetResourceBarrier(mCmdList.Get(), d3dBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		D3D12_VIEWPORT viewport = {};
		viewport.Width = (float)mBufferWidth;
		viewport.Height = (float)mBufferHeight;
		mCmdList->RSSetViewports(1, &viewport);
		D3D12_RECT scissor = {};
		scissor.right = (LONG)mBufferWidth;
		scissor.bottom = (LONG)mBufferHeight;
		mCmdList->RSSetScissorRects(1, &scissor);

		//Clear
		{
			auto saturate = [](float a) { return a < 0 ? 0 : a > 1 ? 1 : a; };
			float clearColor[4];
			static float h = 0.0f;
			h += 0.01f;
			if (h >= 1) h = 0.0f;
			clearColor[0] = saturate(std::abs(h * 6.0f - 3.0f) - 1.0f);
			clearColor[1] = saturate(2.0f - std::abs(h * 6.0f - 2.0f));
			clearColor[2] = saturate(2.0f - std::abs(h * 6.0f - 4.0f));
			mCmdList->ClearRenderTargetView(descHandleRtv, clearColor, 0, nullptr);
		}

		mCmdList->OMSetRenderTargets(1, &descHandleRtv, true, nullptr);

		//Draw
		{
			mCmdList->SetGraphicsRootSignature(mRootSignature.Get());
			ID3D12DescriptorHeap* descHeaps[] = { mDescHeapCbvSrvUav.Get(), mDescHeapSampler.Get() };
			mCmdList->SetDescriptorHeaps(ARRAYSIZE(descHeaps), descHeaps);
			mCmdList->SetGraphicsRootDescriptorTable(0, mDescHeapCbvSrvUav->GetGPUDescriptorHandleForHeapStart());
			mCmdList->SetGraphicsRootDescriptorTable(1, mDescHeapSampler->GetGPUDescriptorHandleForHeapStart());
			mCmdList->SetPipelineState(mPso.Get());
			mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			//mCmdList->IASetVertexBuffers(0, 1, &mVBView);
			//mCmdList->IASetIndexBuffer(&mIBView);
			mCmdList->DrawInstanced(4, 1, 0, 0);
		}

		SetResourceBarrier(mCmdList.Get(), d3dBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		CHK(mCmdList->Close());
		ID3D12CommandList* const cmdList = mCmdList.Get();
		mCmdQueue->ExecuteCommandLists(1, &cmdList);

		CHK(mSwapChain->Present(1, 0));

		CHK(mFence->SetEventOnCompletion(mFrameCount, mFenceEventHandle));

#if 1
		CHK(mCmdQueue->Signal(mFence.Get(), mFrameCount));
		DWORD wait = WaitForSingleObject(mFenceEventHandle, 10000);
		if (wait != WAIT_OBJECT_0)
			throw runtime_error("Failed WaitForSingleObject().");
#else 

#endif
		CHK(mCmdAlloc->Reset());
		CHK(mCmdList->Reset(mCmdAlloc.Get(), nullptr));


	}

private:
	void SetResourceBarrier(ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* res,
		D3D12_RESOURCE_STATES before,
		D3D12_RESOURCE_STATES after) {
		D3D12_RESOURCE_BARRIER desc = {};
		desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		desc.Transition.pResource = res;
		desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		desc.Transition.StateBefore = before;
		desc.Transition.StateAfter = after;
		desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		commandList->ResourceBarrier(1, &desc);
	}
};
LRESULT CALLBACK mWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) {
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) {
				PostMessage(hWnd, WM_DESTROY, 0, 0);
				return 0;
			}
			break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

static HWND setupWindow(int width, int height) {
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = mWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = (HMODULE)GetModuleHandle(0);
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = _T("WindowClass");
	wcex.hIconSm = nullptr;
	if (!RegisterClassEx(&wcex)) {
		throw runtime_error("RegisterClassEx()");
	}

	RECT rect = { 0, 0, width, height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	const int windowWidth = (rect.right - rect.left);
	const int windowHeight = (rect.bottom - rect.top);

	HWND hWnd = CreateWindow(_T("WindowClass"), _T("Window"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, windowWidth, windowHeight, nullptr, nullptr, nullptr, nullptr);
	if (!hWnd) {
		throw runtime_error("CreateWindow()");
	}

	return hWnd;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	MSG  msg;
	ZeroMemory(&msg, sizeof msg);
	ID3D12Device* dev = nullptr;

#ifdef NDEBUG

	try

#endif

	{
		g_mainWindowHandle = setupWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
		ShowWindow(g_mainWindowHandle, SW_SHOW);
		UpdateWindow(g_mainWindowHandle);
		D3D d3d(WINDOW_WIDTH, WINDOW_HEIGHT, g_mainWindowHandle);
		dev = d3d.GetDevice();

		while (msg.message != WM_QUIT) {
			BOOL r = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
			if (r == 0) {
				d3d.Draw();
			}
			else {
				DispatchMessage(&msg);
			}
		}
	}

#ifdef NDEBUG

	catch (std::exception &e) {
		MessageBoxA(g_mainWindowHandle, e.what(), "Exception occuured.", MB_ICONSTOP);
	}

#endif

	if (dev)
		dev->Release();

	return static_cast<int>(msg.wParam);

}