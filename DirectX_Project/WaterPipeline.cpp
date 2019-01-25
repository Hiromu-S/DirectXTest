#include "WaterPipeline.h"

#include <tchar.h>

WaterPipeline::WaterPipeline()
{
}

WaterPipeline::~WaterPipeline()
{
}

bool WaterPipeline::Initialize(ID3D12Device* dev) {
	if FAILED(CreateRootSignature(dev)) {
		return false;
	}
	if FAILED(CreatePipelineStateObject(dev)) {
		return false;
	}

	D3D12_HEAP_PROPERTIES heapProperties{};
	D3D12_RESOURCE_DESC   resourceDesc{};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = 256; //ByteWidth sizeof(MatrixBufferType)
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	{
		if FAILED(dev->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mConstantBufferMatrix)))
		{
			return false;
		}
	}
	//Reflecction matrixBuffer
	{
		if FAILED(dev->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mReflectionBuffer)))
		{
			return false;
		}
	}
	//Water Buffer
	{
		if FAILED(dev->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mWaterBuffer)))
		{
			return false;
		}
	}

	return true;
}

bool WaterPipeline::Render(ID3D12GraphicsCommandList* cmdList, int drawIndexCount,
	DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj, DirectX::XMMATRIX reflectionMatrix,
	ID3D12DescriptorHeap* reflectionTexture, ID3D12DescriptorHeap* refractionTexture, ID3D12DescriptorHeap* normalTexture,
	float waterTranslation, float reflectionRefractionScale) {

	if (!SetShaderParameters(cmdList, world, view, proj, reflectionMatrix,
		reflectionTexture, refractionTexture, normalTexture,
		waterTranslation, reflectionRefractionScale)) {
		return false;
	}

	RenderShader(cmdList, drawIndexCount);

	return true;
}

bool WaterPipeline::SetShaderParameters(ID3D12GraphicsCommandList* cmdList,
	DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj, DirectX::XMMATRIX reflectionMatrix,
	ID3D12DescriptorHeap* reflectionTexture, ID3D12DescriptorHeap* refractionTexture, ID3D12DescriptorHeap* normalTexture,
	float waterTranslation, float reflectionRefractionScale) {

	//転置
	DirectX::XMMATRIX transWorldmatrix = DirectX::XMMatrixTranspose(world);
	DirectX::XMMATRIX transViewMatrix = DirectX::XMMatrixTranspose(view);
	DirectX::XMMATRIX transProjectionmatrix = DirectX::XMMatrixTranspose(proj);
	DirectX::XMMATRIX transRreflectionMatrix = DirectX::XMMatrixTranspose(reflectionMatrix);

	MatrixBufferType* matBuff{};
	ReflectionBufferType* reflecBuffer{};
	WaterBufferType* waterBuff{};
	if FAILED(mConstantBufferMatrix->Map(0, nullptr, (void**)&matBuff)) {
		return false;
	}
	matBuff->proj = transProjectionmatrix;
	matBuff->view = transViewMatrix;
	matBuff->world = transWorldmatrix;
	mConstantBufferMatrix->Unmap(0, nullptr);
	matBuff = nullptr;

	if FAILED(mReflectionBuffer->Map(0, nullptr, (void**)&reflecBuffer)) {
		return false;
	}
	reflecBuffer->reflection = transRreflectionMatrix;
	mReflectionBuffer->Unmap(0, nullptr);
	reflecBuffer = nullptr;

	if FAILED(mWaterBuffer->Map(0, nullptr, (void**)&waterBuff)) {
		return false;
	}
	waterBuff->waterTranslation = waterTranslation;
	waterBuff->reflectRefractScale = reflectionRefractionScale;
	waterBuff->padding = DirectX::XMFLOAT2(0.0f, 0.0f);
	mWaterBuffer->Unmap(0, nullptr);
	waterBuff = nullptr;

	cmdList->SetPipelineState(mPipelineState.Get());
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	//定数バッファをシェーダのレジスタにセット
	cmdList->SetGraphicsRootConstantBufferView(0, mConstantBufferMatrix->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, mReflectionBuffer->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(3, mWaterBuffer->GetGPUVirtualAddress());

	////テクスチャをシェーダのレジスタにセット
	ID3D12DescriptorHeap* tex_heaps[] = { reflectionTexture };
	cmdList->SetDescriptorHeaps(_countof(tex_heaps), tex_heaps);
	cmdList->SetGraphicsRootDescriptorTable(2, reflectionTexture->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* tex_heaps2[] = { refractionTexture };
	cmdList->SetDescriptorHeaps(_countof(tex_heaps2), tex_heaps2);
	cmdList->SetGraphicsRootDescriptorTable(4, refractionTexture->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* tex_heaps3[] = { normalTexture };
	cmdList->SetDescriptorHeaps(_countof(tex_heaps3), tex_heaps3);
	cmdList->SetGraphicsRootDescriptorTable(5, normalTexture->GetGPUDescriptorHandleForHeapStart());

	return true;
}

HRESULT WaterPipeline::CreateRootSignature(ID3D12Device* dev) {
	HRESULT hr{};

	D3D12_DESCRIPTOR_RANGE		range[3]{};				
	D3D12_ROOT_PARAMETER		rootParams[6]{};	
	D3D12_ROOT_SIGNATURE_DESC	rootSigDesc{};  
	D3D12_STATIC_SAMPLER_DESC	samplerDesc{};
	ComPtr<ID3DBlob> blob{};

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].Descriptor.RegisterSpace = 0;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[1].Descriptor.ShaderRegister = 1;
	rootParams[1].Descriptor.RegisterSpace = 0;

	range[0].NumDescriptors = 1;
	//レジスタ番号
	range[0].BaseShaderRegister = 0;
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].OffsetInDescriptorsFromTableStart = 0;

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &range[0];

	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[3].Descriptor.ShaderRegister = 2;
	rootParams[3].Descriptor.RegisterSpace = 0;

	range[1].NumDescriptors = 1;
	range[1].BaseShaderRegister = 1;
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[1].OffsetInDescriptorsFromTableStart = 0;

	rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[4].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[4].DescriptorTable.pDescriptorRanges = &range[1];

	range[2].NumDescriptors = 1;
	range[2].BaseShaderRegister = 2;
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[2].OffsetInDescriptorsFromTableStart = 0;

	rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[5].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[5].DescriptorTable.pDescriptorRanges = &range[2];

	//静的サンプラの設定
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSigDesc.NumParameters = _countof(rootParams);
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = &samplerDesc;

	// RootSignatureを作成するのに必要なバッファを確保しTableの情報をシリアライズ
	if FAILED(hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr)) {
		return hr;
	}
	// RootSignatureの作成
	if FAILED(hr = dev->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature))) {
		return hr;
	}

	return S_OK;
}

HRESULT WaterPipeline::CreatePipelineStateObject(ID3D12Device* dev) {
	HRESULT hr{};
	ComPtr<ID3D10Blob> vertexShader;
	ComPtr<ID3D10Blob> pixelShader;

	UINT compileFlag = 0;

#if _DEBUG

	compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

#endif

	if FAILED(hr = D3DCompileFromFile(L"../HLSL/Water.hlsl", nullptr, nullptr, "WaterVertexShader", "vs_5_0", compileFlag, 0, vertexShader.GetAddressOf(), nullptr)) {
		return hr;
	}

	if FAILED(hr = D3DCompileFromFile(L"../HLSL/Water.hlsl", nullptr, nullptr, "WaterPixelShader", "ps_5_0", compileFlag, 0, pixelShader.GetAddressOf(), nullptr)) {
		return hr;
	}

	// 頂点レイアウト.
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};

	//シェーダーの設定
	pipelineStateDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer();
	pipelineStateDesc.VS.BytecodeLength = vertexShader->GetBufferSize();
	pipelineStateDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer();
	pipelineStateDesc.PS.BytecodeLength = pixelShader->GetBufferSize();


	//インプットレイアウトの設定
	pipelineStateDesc.InputLayout.pInputElementDescs = InputElementDesc;
	pipelineStateDesc.InputLayout.NumElements = _countof(InputElementDesc);


	//サンプラーの設定
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
		pipelineStateDesc.BlendState.RenderTarget[i].BlendEnable = true;
		pipelineStateDesc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		pipelineStateDesc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
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
	if FAILED(hr = dev->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&mPipelineState))) {
		return hr;
	}

	return S_OK;
}