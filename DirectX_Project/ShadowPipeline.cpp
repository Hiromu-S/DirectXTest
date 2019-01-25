#include "ShadowPipeline.h"



ShadowPipeline::ShadowPipeline()
{
}


ShadowPipeline::~ShadowPipeline()
{
}


bool ShadowPipeline::Initialize(ID3D12Device* dev) {

	if FAILED(CreateRootSignature(dev)) {
		return false;
	}
	if FAILED(CreatePipelineStateObject(dev)) {
		return false;
	}



	return true;
}

bool ShadowPipeline::Render(ID3D12GraphicsCommandList* cmdList,
	int matNum, int drawIndexCount,
	DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj,
	ID3D12DescriptorHeap* tex,
	DirectX::XMFLOAT3 lightDir, DirectX::XMFLOAT4 ambColor, DirectX::XMFLOAT4 diffColor) {

	RenderShader(cmdList, drawIndexCount);

	return true;
}

HRESULT ShadowPipeline::CreateRootSignature(ID3D12Device* dev) {



	return S_OK;
}

HRESULT ShadowPipeline::CreatePipelineStateObject(ID3D12Device* dev) {
	HRESULT hr;
	ComPtr<ID3DBlob> vertex_shader{};
	ComPtr<ID3DBlob> pixel_shader{};
#if defined(_DEBUG)

	UINT compile_flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

#else

	UINT compile_flag = 0;

#endif
	hr = D3DCompileFromFile(L"../HLSL/shadowMap.hlsl", nullptr, nullptr, "shadowVertexShader", "vs_5_0", compile_flag, 0, vertex_shader.GetAddressOf(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	hr = D3DCompileFromFile(L"../HLSL/shadowMap.hlsl", nullptr, nullptr, "shadowPixelShader", "ps_5_0", compile_flag, 0, pixel_shader.GetAddressOf(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	//頂点レイアウト.
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};

	//シェーダーの設定
	pipelineStateDesc.VS.pShaderBytecode = vertex_shader->GetBufferPointer();
	pipelineStateDesc.VS.BytecodeLength = vertex_shader->GetBufferSize();
	pipelineStateDesc.PS.pShaderBytecode = pixel_shader->GetBufferPointer();
	pipelineStateDesc.PS.BytecodeLength = pixel_shader->GetBufferSize();

	//インプットレイアウトの設定
	pipelineStateDesc.InputLayout.pInputElementDescs = InputElementDesc;
	pipelineStateDesc.InputLayout.NumElements = _countof(InputElementDesc);

	//サンプル系の設定
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.SampleDesc.Quality = 0;
	pipelineStateDesc.SampleMask = UINT_MAX;

	//レンダーターゲットの設定
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;

	//三角形に設定
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//ルートシグネチャ
	pipelineStateDesc.pRootSignature = mRootSignature.Get();

	//ラスタライザステートの設定
	pipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//D3D12_CULL_MODE_BACK;
	pipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipelineStateDesc.RasterizerState.FrontCounterClockwise = FALSE;
	pipelineStateDesc.RasterizerState.DepthBias = 0;
	pipelineStateDesc.RasterizerState.DepthBiasClamp = 0;
	pipelineStateDesc.RasterizerState.SlopeScaledDepthBias = 0;
	pipelineStateDesc.RasterizerState.DepthClipEnable = TRUE;
	pipelineStateDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	pipelineStateDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	pipelineStateDesc.RasterizerState.MultisampleEnable = FALSE;

	//ブレンドステートの設定
	for (int i = 0; i < _countof(pipelineStateDesc.BlendState.RenderTarget); ++i) {
		pipelineStateDesc.BlendState.RenderTarget[i].BlendEnable = FALSE;
		pipelineStateDesc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
		pipelineStateDesc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
		pipelineStateDesc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
		pipelineStateDesc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
		pipelineStateDesc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
		pipelineStateDesc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		pipelineStateDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		pipelineStateDesc.BlendState.RenderTarget[i].LogicOpEnable = FALSE;
		pipelineStateDesc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR;
	}

	pipelineStateDesc.BlendState.AlphaToCoverageEnable = FALSE;
	pipelineStateDesc.BlendState.IndependentBlendEnable = FALSE;

	//デプスステンシルステートの設定
	pipelineStateDesc.DepthStencilState.DepthEnable = TRUE;								//深度テストあり
	pipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipelineStateDesc.DepthStencilState.StencilEnable = FALSE;							//ステンシルテストなし
	pipelineStateDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	pipelineStateDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	pipelineStateDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	pipelineStateDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	hr = dev->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&mPipelineState));
	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}
