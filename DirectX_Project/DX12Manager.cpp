#include "DX12Manager.h"

#include <iostream>


const int NUM_FRAMES_IN_FLIGHT = 3;

DX12Manager::DX12Manager(int width, int height, HWND hWnd)
	: mBufferWidth(width), mBufferHeight(height), mWindowHandle(hWnd), mDev(nullptr)
{

	CreateFactory();
	CreateDevice();
	CreateCommandList();
	CreateRenderTargetView();
	CreateDepthStencilBuffer();
	//CreateRootSignature();
	//CreatePipelineState();
	//CreateLightBuffer();
	//CreateShadowBuffer();
	//CreateShadowMapPipelineState();
	if (InitializeScene() == false) {
		MessageBox(hWnd, _T("Could not initialize the reflection render to texture object."), _T("Error"), MB_OK);
	}
}

bool DX12Manager::InitializeScene() {
	mCatBoard = new Model();
	if (mCatBoard->Initialize(mDev, L"../Resource/wall.txt", "../Resource/cat.dds") == false) {
		return false;
	}

	mGround = new Model();
	if (mGround->Initialize(mDev, L"../Resource/ground.txt", "../Resource/ground.dds") == false) {
		return false;
	}

	mLight = new Light;
	if (!mLight) {
		return false;
	}
	mLight->SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);
	mLight->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
	mLight->SetDirection(0.0f, -1.0f, 0.5f);
	
	mLightPipeline = new LightPipeline();
	//描画するオブジェクト数
	mLightPipeline->Initialize(mDev, 2);

	mRefractionTexture = new Texture(WINDOW_WIDTH, WINDOW_HEIGHT);
	mRefractionTexture->Initialize(mDev, WINDOW_WIDTH, WINDOW_HEIGHT);

	mReflectionTexture = new Texture(WINDOW_WIDTH, WINDOW_HEIGHT);
	mReflectionTexture->Initialize(mDev, WINDOW_WIDTH, WINDOW_HEIGHT);

	mCamera = new Camera();

	return true;
}

DX12Manager::~DX12Manager()
{
	//イベントハンドルのクローズ
	CloseHandle(mFenceEveneHandle);
}

HRESULT DX12Manager::CreateFactory() {
	HRESULT hr;

	/*Create a factory*/
#if _DEBUG
	CHK(hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(mDxgiFactory.ReleaseAndGetAddressOf())));
#else
	CHK(hr = CreateDXGIFactory2(0, IID_PPV_ARGS(mDxgiFactory.ReleaseAndGetAddressOf())));
#endif /* _DEBUG */
	
#if _DEBUG
	ID3D12Debug* debug = nullptr;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (debug) {
		debug->EnableDebugLayer();
		debug->Release();
		debug = nullptr;
	}
#endif /* _DEBUG */

	return hr;
}

HRESULT DX12Manager::CreateDevice() {
	HRESULT hr;

	CHK(hr = mDxgiFactory->EnumAdapters(0, (IDXGIAdapter**)mAdapter.GetAddressOf()));
	ID3D12Device* dev;
	CHK(hr = D3D12CreateDevice(
		mAdapter.Get(),
		D3D_FEATURE_LEVEL_11_1,
		IID_PPV_ARGS(&dev)));
	mDev = dev;

	return hr;
}

HRESULT DX12Manager::CreateCommandList() {
	HRESULT hr;

	CHK(mDev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCmdAlloc.ReleaseAndGetAddressOf())));

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	CHK(mDev->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mCmdQueue.ReleaseAndGetAddressOf())));

	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = WINDOW_WIDTH;
	scDesc.Height = WINDOW_HEIGHT;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = BUFFER_COUNT;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	CHK(mDxgiFactory->CreateSwapChainForHwnd(mCmdQueue.Get(), mWindowHandle, &scDesc, nullptr, nullptr, mSwapChain.ReleaseAndGetAddressOf()));

	CHK(hr = mDev->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mCmdAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(mCmdList.ReleaseAndGetAddressOf())));

	//CHK(hr = mSwapChain->QueryInterface(mSwapChain.GetAddressOf()));

	CHK(mDev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.ReleaseAndGetAddressOf())));

	mFenceEveneHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	return hr;
}

HRESULT DX12Manager::CreateRenderTargetView() {
	HRESULT hr;

	{//ディスクリプタヒープ作成　種類ごとに忘れず
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = BUFFER_COUNT;
		desc.NodeMask = 0;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHK(mDev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mDescHeapRtv.ReleaseAndGetAddressOf())));

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.NumDescriptors = 1;
		CHK(mDev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mDescHeapDsv.ReleaseAndGetAddressOf())));

		CHK(mDev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mDescHeapSB.ReleaseAndGetAddressOf())));

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		CHK(mDev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mDescHeapST.ReleaseAndGetAddressOf())));

		CHK(mDev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mImguiSrvDescHeap.ReleaseAndGetAddressOf())));

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		desc.NumDescriptors = 2;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		CHK(mDev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mDescHeapSampler.ReleaseAndGetAddressOf())));
	}

	auto rtvStep = mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (auto i = 0u; i < BUFFER_COUNT; i++)
	{
		CHK(hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(mD3DBuffer[i].ReleaseAndGetAddressOf())));
		mD3DBuffer[i]->SetName(L"SwapChain_Buffer");

		auto d = mDescHeapRtv->GetCPUDescriptorHandleForHeapStart();
		d.ptr += i * rtvStep;
		mDev->CreateRenderTargetView(mD3DBuffer[i].Get(), nullptr, d);
	}

	return hr;
}

/*
HRESULT DX12Manager::CreateRootSignature() {
	HRESULT hr{};

	D3D12_DESCRIPTOR_RANGE range[2]{};
	D3D12_ROOT_PARAMETER rootParams[4]{};
	D3D12_STATIC_SAMPLER_DESC samplerDesc[2]{};

	//変換行列用の定数バッファ	
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].Descriptor.RegisterSpace = 0;

	//ライト用の定数バッファ
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[1].Descriptor.ShaderRegister = 1;
	rootParams[1].Descriptor.RegisterSpace = 0;

	//テクスチャ
	range[0].NumDescriptors = 2;
	range[0].BaseShaderRegister = 0;
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &range[0];

	//サンプラ
	samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].MipLODBias = 0.0f;
	samplerDesc[0].MaxAnisotropy = 16;
	samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc[0].MinLOD = 0.0f;
	samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc[0].ShaderRegister = 0;
	samplerDesc[0].RegisterSpace = 0;
	samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	samplerDesc[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDesc[1].MipLODBias = 0.0f;
	samplerDesc[1].MaxAnisotropy = 16;
	samplerDesc[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	samplerDesc[1].MinLOD = 0.0f;
	samplerDesc[1].MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc[1].ShaderRegister = 1;
	samplerDesc[1].RegisterSpace = 0;
	samplerDesc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	ID3D10Blob *sig, *info;
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;

	rootSigDesc.Init(ARRAYSIZE(rootParams), rootParams, ARRAYSIZE(samplerDesc), samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CHK(hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &info));

	CHK(hr = mDev->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.ReleaseAndGetAddressOf())));
	sig->Release();
	info->Release();

	return hr;
}*/


HRESULT DX12Manager::CreateRootSignature() {

	HRESULT hr;

	//シェーダー内のレジスタへの入力フォーマットの作成
	CD3DX12_DESCRIPTOR_RANGE descRange[4];
	descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);//world, light
	descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // shadow map
	//descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	//影用と通常用
	descRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);

	/*
	CD3DX12_DESCRIPTOR_RANGE descRange2[1];
	//影用と通常用
	descRange2[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);
	*/

	CD3DX12_ROOT_PARAMETER rootParam[4];
	//rootParam[0].InitAsDescriptorTable(ARRAYSIZE(descRange1), descRange1);
	//rootParam[1].InitAsDescriptorTable(ARRAYSIZE(descRange2), descRange2, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParam[0].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParam[1].InitAsDescriptorTable(1, &descRange[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParam[2].InitAsDescriptorTable(1, &descRange[2], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParam[3].InitAsDescriptorTable(1, &descRange[3], D3D12_SHADER_VISIBILITY_PIXEL);

	ID3D10Blob *sig, *info;
	auto rootSigDesc = D3D12_ROOT_SIGNATURE_DESC();
	rootSigDesc.NumParameters = ARRAYSIZE(rootParam);
	rootSigDesc.NumStaticSamplers = 0;
	rootSigDesc.pParameters = rootParam;
	rootSigDesc.pStaticSamplers = nullptr;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	CHK(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &info));
	CHK(hr = mDev->CreateRootSignature(
		0,
		sig->GetBufferPointer(),
		sig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.ReleaseAndGetAddressOf())));
	sig->Release();
	//info->Release();

	return hr;
}

HRESULT DX12Manager::CreatePipelineState() {
	HRESULT hr;

	ID3D10Blob *vs, *ps;
	{
		ID3D10Blob *info;
		UINT flag = 0;
#if _DEBUG
		flag |= D3DCOMPILE_DEBUG;
#endif /* _DEBUG */
		CHK(D3DCompileFromFile(L"../HLSL/MeshTex.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", flag, 0, &vs, &info));
		CHK(D3DCompileFromFile(L"../HLSL/MeshTex.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", flag, 0, &ps, &info));
	}

	//シェーダーへのインプット
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	//PSOの作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.InputLayout.NumElements = 3;
	psoDesc.InputLayout.pInputElementDescs = inputLayout;
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS.pShaderBytecode = vs->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vs->GetBufferSize();
	psoDesc.PS.pShaderBytecode = ps->GetBufferPointer();
	psoDesc.PS.BytecodeLength = ps->GetBufferSize();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(CCCD3DX12_DEFAULT());
	psoDesc.BlendState = CD3DX12_BLEND_DESC(CCCD3DX12_DEFAULT());
	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	CHK(hr = mDev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(mPso.ReleaseAndGetAddressOf())));
	vs->Release();
	ps->Release();

	return hr;
}

//深度バッファとディスクリプタの作成
HRESULT DX12Manager::CreateDepthStencilBuffer() {
	HRESULT hr;

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32_TYPELESS, mBufferWidth, mBufferHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		D3D12_TEXTURE_LAYOUT_UNKNOWN, 0);
	D3D12_CLEAR_VALUE dsvClearValue;
	dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	dsvClearValue.DepthStencil.Depth = 1.0f;
	dsvClearValue.DepthStencil.Stencil = 0;
	CHK(hr = mDev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // No need to read/write by CPU
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&dsvClearValue,
		IID_PPV_ARGS(mDB.ReleaseAndGetAddressOf())));
	mDB->SetName(L"DepthTexture");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	mDev->CreateDepthStencilView(mDB.Get(), &dsvDesc, mDescHeapDsv->GetCPUDescriptorHandleForHeapStart());

	return hr;
}

HRESULT DX12Manager::CreateLightBuffer() {
	HRESULT hr;
	D3D12_HEAP_PROPERTIES heapProps{};
	D3D12_RESOURCE_DESC desc{};

	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 0;
	heapProps.VisibleNodeMask = 0;

	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = 256;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	CHK(hr = mDev->CreateCommittedResource(
		&heapProps,
		//&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		//&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, 
		&desc, 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&mLightBuffer)));

	//Viewport と Scissor
	mShadowViewport.Width = (float)mBufferWidth;
	mShadowViewport.Height = (float)mBufferHeight;
	mShadowViewport.MinDepth = 0.0f;
	mShadowViewport.MaxDepth = 1.0f;

	mShadowScissor.right = (LONG)mBufferWidth;
	mShadowScissor.bottom = (LONG)mBufferHeight;

	struct Light {
		XMFLOAT4X4 lightVP;
		XMFLOAT4 lightColor;
		XMFLOAT3 lightDir;
	};

	Light* p{};
	CHK(hr = mLightBuffer->Map(0, nullptr, (void**)&p));
	/*mLightPos = { 2.0f, 6.5f, -1.0f };
	mLightDest = { 0.0f, 0.0f, 0.0f };
	mLightDir = {
		mLightPos.x - mLightDest.x,
		mLightPos.y - mLightDest.y,
		mLightPos.z - mLightDest.z
	};
	mLightColor = { 1.0f,1.0f ,1.0f ,1.0f };
	XMMATRIX view = XMMatrixLookAtLH({ mLightPos.x, mLightPos.y, mLightPos.z }, { mLightDest.x, mLightDest.y, mLightDest.z }, { 0.0f, 1.0f, 0.0f });
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), 1.0f, 1.0f, 15.0f);
	XMFLOAT4X4 mat;
	XMStoreFloat4x4(&mat, XMMatrixTranspose(view * proj));
	
	p->lightVP = mat;
	p->lightDir = mLightDir;
	p->lightColor = mLightColor;
	mLightBuffer->Unmap(0, nullptr);
	p = nullptr;
	
	return hr;
	*/
	return S_OK;
}

HRESULT DX12Manager::CreateShadowBuffer() {
	HRESULT hr;

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32_TYPELESS, 
		mBufferWidth, 
		mBufferHeight, 
		1,
		1,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		D3D12_TEXTURE_LAYOUT_UNKNOWN, 
		0);
	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
	D3D12_CLEAR_VALUE dsvClearValue;
	dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	dsvClearValue.DepthStencil.Depth = 1.0f;
	dsvClearValue.DepthStencil.Stencil = 0;
	CHK(hr = mDev->CreateCommittedResource(
		//&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // No need to read/write by CPU
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&dsvClearValue,
		IID_PPV_ARGS(mShadowBuffer.ReleaseAndGetAddressOf())));
	mShadowBuffer->SetName(L"ShadowTexture");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	mDev->CreateDepthStencilView(mShadowBuffer.Get(), &dsvDesc, mDescHeapSB->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC resView{};
	resView.Format = DXGI_FORMAT_R32_FLOAT;
	resView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	resView.Texture2D.MipLevels = 1;
	resView.Texture2D.MostDetailedMip = 0;
	resView.Texture2D.PlaneSlice = 0;
	resView.Texture2D.ResourceMinLODClamp = 0.0F;
	resView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	mDev->CreateShaderResourceView(mShadowBuffer.Get(), &resView, mDescHeapST->GetCPUDescriptorHandleForHeapStart());

	return hr;
}

HRESULT DX12Manager::CreateShadowMapPipelineState() {
	HRESULT hr;

	ID3D10Blob *vs;
	{
		ID3D10Blob *info;
		UINT flag = 0;
#if _DEBUG
		flag |= D3DCOMPILE_DEBUG;
#endif /* _DEBUG */
		CHK(hr = D3DCompileFromFile(L"../HLSL/MeshTex.hlsl", nullptr, nullptr, "VSShadowMap", "vs_5_0", flag, 0, &vs, &info));
	}

	//シェーダーへのインプット
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	//PSOの作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout.NumElements = ARRAYSIZE(inputLayout);
	psoDesc.InputLayout.pInputElementDescs = inputLayout;
	
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	psoDesc.pRootSignature = mRootSignature.Get();
	
	psoDesc.VS.pShaderBytecode = vs->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vs->GetBufferSize();
	
	//sampler
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	//rasterize
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(CCCD3DX12_DEFAULT());
	psoDesc.BlendState = CD3DX12_BLEND_DESC(CCCD3DX12_DEFAULT());
	psoDesc.BlendState.AlphaToCoverageEnable = false;
	psoDesc.BlendState.IndependentBlendEnable = false;

	//depthstencilstate
	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	
	CHK(hr = mDev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(mShadowPso.ReleaseAndGetAddressOf())));
	vs->Release();

	return hr;
}

bool DX12Manager::RenderReflection() {
	XMMATRIX world, refView, proj;
	proj = XMMatrixPerspectiveFovLH((float)XM_PI / 4.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 1000.0f);
	world = XMMatrixIdentity();

	setResourceBarrier(mCmdList.Get(), mReflectionTexture->GetTexBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	mReflectionTexture->ClearRenderTarget(mCmdList.Get(), mDescHeapDsv.Get(), 0.0f, 0.0f, 0.0f, 1.0f);

	mCamera->Render2Reflection(1.0f);

	refView = mCamera->GetReflectionViewMatrix();

	world = XMMatrixTranslation(0.0f, 6.0f, 8.0f);
	
	mCatBoard->Render(mCmdList.Get());

	XMMATRIX view;
	mCamera->GetViewMatrix(view);

	mLightPipeline->Render(mCmdList.Get(), 0, mCatBoard->GetIndexCount(),
		world, view, proj, mCatBoard->GetDescHeapTexture(),
		mLight->GetDirection(), mLight->GetAmbientColor(), mLight->GetDiffuseColor());

	setResourceBarrier(mCmdList.Get(), mReflectionTexture->GetTexBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);

	CHK(mCmdList->Close());
	ExecuteCommandList();

	return true;
}

//CommandListの作成
HRESULT DX12Manager::UpdateCommandList() {
	HRESULT hr;

	XMMATRIX world, view, proj;

	proj = XMMatrixPerspectiveFovLH((float)XM_PI / 4.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 1000.0f);
	world = XMMatrixIdentity();

	mFrameCount++;

	mCamera->SetPosition(-13.0f, 10.0f, -12.0f);
	mCamera->SetRotation(240.0f, 240.0f, 0.0f);

	//現在のrender target viewの取得
	auto descHandleRtvStep = mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE descHandleRtv = mDescHeapRtv->GetCPUDescriptorHandleForHeapStart();
	descHandleRtv.ptr += ((mFrameCount - 1) % BUFFER_COUNT) * descHandleRtvStep;

	//現在のswap chainの取得
	ID3D12Resource* d3dBuffer = mD3DBuffer[(mFrameCount - 1) % BUFFER_COUNT].Get();

	//Viewport と Scissor
	D3D12_VIEWPORT viewport = {};
	viewport.Width = (float)mBufferWidth;
	viewport.Height = (float)mBufferHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	mCmdList->RSSetViewports(1, &viewport);
	D3D12_RECT scissor = {};
	scissor.right = (LONG)mBufferWidth;
	scissor.bottom = (LONG)mBufferHeight;
	mCmdList->RSSetScissorRects(1, &scissor);

	//コマンドリストに描画コマンドを積む
	
	/*----------for ShadowMap rendering----------*/
	/*setResourceBarrier(mCmdList.Get(), mShadowBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	mCmdList->ClearDepthStencilView(mDescHeapSB->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//set signature to shadow pso
	mCmdList->SetGraphicsRootSignature(mRootSignature.Get());
	mCmdList->SetPipelineState(mShadowPso.Get());
	
	//Viewport to Scissor
	mCmdList->RSSetViewports(1, &mShadowViewport);
	mCmdList->RSSetScissorRects(1, &mShadowScissor);

	//Set shadow depth buffer
	mCmdList->OMSetRenderTargets(1, nullptr, false, &mDescHeapSB->GetCPUDescriptorHandleForHeapStart());

	mCmdList->SetGraphicsRootConstantBufferView(1, mLightBuffer->GetGPUVirtualAddress());

	mTeapot->Draw(mCmdList.Get());

	setResourceBarrier(mCmdList.Get(), mShadowBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);

	CHK(hr = mCmdList->Close());
	ExecuteCommandList();
	*/
	/*----------for Normal rendering command----------*/
	setResourceBarrier(mCmdList.Get(), d3dBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto descHandleDsv = mDescHeapDsv->GetCPUDescriptorHandleForHeapStart();

	//depthtextureのクリア
	mCmdList->ClearDepthStencilView(descHandleDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	mLight->SetAmbientColor(mAmbientColor[0], mAmbientColor[1], mAmbientColor[2], mAmbientColor[3]);
	mLight->SetDiffuseColor(mDiffColor[0], mDiffColor[1], mDiffColor[2], mDiffColor[3]);
	mLight->SetDirection(mLightDir[0], -mLightDir[1], mLightDir[2]);

	//画面クリア
	mCmdList->ClearRenderTargetView(descHandleRtv, mClearColor, 0, nullptr);
	mCmdList->SetGraphicsRootSignature(mRootSignature.Get());
	//mCmdList->SetPipelineState(mPso.Get());

	mCmdList->RSSetViewports(1, &viewport);
	mCmdList->RSSetScissorRects(1, &scissor);

	mCmdList->OMSetRenderTargets(1, &descHandleRtv, true, &descHandleDsv);


	mCmdList->SetDescriptorHeaps(1, mImguiSrvDescHeap.GetAddressOf());

	mCamera->Render();

	mCamera->GetViewMatrix(view);
	world = XMMatrixTranslation(0.0f, 6.0f, 8.0f);

	mCatBoard->Render(mCmdList.Get());

	mLightPipeline->Render(mCmdList.Get(), 0, mCatBoard->GetIndexCount(), world, view, proj, mCatBoard->GetDescHeapTexture(), 
		mLight->GetDirection(), mLight->GetAmbientColor(), mLight->GetDiffuseColor());

	world = XMMatrixTranslation(0.0f, 0.0f, 15.0f);

	mGround->Render(mCmdList.Get());

	mLightPipeline->Render(mCmdList.Get(), 1, mGround->GetIndexCount(), world, view, proj, mGround->GetDescHeapTexture(),
		mLight->GetDirection(), mLight->GetAmbientColor(), mLight->GetDiffuseColor());

	setResourceBarrier(mCmdList.Get(), d3dBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	CHK(hr = mCmdList->Close());
	ExecuteCommandList();

	return hr;
}

HRESULT DX12Manager::ExecuteCommandList() {
	HRESULT hr;

	ID3D12CommandList* const cmdList = mCmdList.Get();
	mCmdQueue->ExecuteCommandLists(1, &cmdList);

	//キューのポップイベント
	CHK(mFence->SetEventOnCompletion(mFrameCount, mFenceEveneHandle));

	//描画終わるまで待つ
	//ストールさせてる
	CHK(hr = mCmdQueue->Signal(mFence.Get(), mFrameCount));
	DWORD wait = WaitForSingleObject(mFenceEveneHandle, 10000);
	if (wait != WAIT_OBJECT_0)
		throw runtime_error("Failed WaitForSingleObject().");

	//リセット
	CHK(mCmdAlloc->Reset());
	CHK(mCmdList->Reset(mCmdAlloc.Get(), nullptr));

	return hr;
}

//描画
HRESULT DX12Manager::Render() {
	HRESULT hr{};

	//RenderReflection();
	UpdateCommandList();
	RenderScene();

	return S_OK;
}

HRESULT DX12Manager::RenderScene() {
	HRESULT hr{};
	CHK(hr = mSwapChain->Present(1, 0));
	return hr;
}

//リソースの状態の変更
void DX12Manager::setResourceBarrier(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Resource* res,
	D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER desc = {};
	desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	desc.Transition.pResource = res;
	desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	desc.Transition.StateBefore = before;
	desc.Transition.StateAfter = after;
	desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	commandList->ResourceBarrier(1, &desc);
}