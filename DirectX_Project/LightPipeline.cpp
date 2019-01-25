#include "LightPipeline.h"
#include <tchar.h>

LightPipeline::LightPipeline()
{
}

LightPipeline::~LightPipeline()
{
}

bool LightPipeline::Initialize(ID3D12Device* dev, int drawIndexCount) {
	HRESULT hr{};
	mConstantBufferMatrix.resize(drawIndexCount);
	mDescHeapConstantBufferMatrix.resize(drawIndexCount);

	if FAILED(CreateRootSignature(dev)) {
		return false;
	}
	if FAILED(CreatePipelineStateObject(dev)) {
		return false;
	}

	D3D12_HEAP_PROPERTIES heapProperties{};
	D3D12_RESOURCE_DESC   resourceDesc{};

	//Matrix ConstantBuffer
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
	resourceDesc.SampleDesc.Quality = 0;//アンチェリ
	{
		for (auto& c : mConstantBufferMatrix) {
			if FAILED(dev->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(c.GetAddressOf()))) {
				return false;
			}
		}
	}

	//Create Light ConstantBuffer

	//ByteWidth sizeof(LightBufferType)
	{
		if FAILED(dev->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mConstantBufferLight))) {
			return false;
		}
	}

	//Create matrix DiscriptorHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc;
		descHeapDesc.NumDescriptors = 1;	
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//可視化
		descHeapDesc.NodeMask = 0;

		for (auto& dh : mDescHeapConstantBufferMatrix) {
			if FAILED(dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&dh))) {
				return false;
			}
		}
		//Create constant buffer view
		D3D12_CPU_DESCRIPTOR_HANDLE handleMatCBV{};
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		
		unsigned int U32Size = static_cast<unsigned int>(sizeof(MatrixBufferType) + (256 - sizeof(MatrixBufferType) % 256) % 256);
		for (int i = 0; i < mConstantBufferMatrix.size(); ++i) {
			cbvDesc.BufferLocation = mConstantBufferMatrix[i].Get()->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = U32Size;
			//unsigned int dhSize = dev->GetDescriptorHandleIncrementSize(descHeapDesc.Type);//デスクリプタのサイズ
			dev->CreateConstantBufferView(&cbvDesc, mDescHeapConstantBufferMatrix[i].Get()->GetCPUDescriptorHandleForHeapStart());
		}
	}
	return true;
}

bool LightPipeline::Render(ID3D12GraphicsCommandList* cmdList,
	int matNum, int drawIndexCount,
	DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj,
	ID3D12DescriptorHeap* tex,
	DirectX::XMFLOAT3 lightDir, DirectX::XMFLOAT4 ambColor, DirectX::XMFLOAT4 diffColor) {

	if FAILED(SetShaderParameters(cmdList, matNum, world, view, proj, tex, lightDir, ambColor, diffColor)) {
		return false;
	}

	RenderShader(cmdList, drawIndexCount);

	return true;
}

bool LightPipeline::SetShaderParameters(ID3D12GraphicsCommandList* cmdList,
	int matNum,
	DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj,
	ID3D12DescriptorHeap* tex,
	DirectX::XMFLOAT3 lightDir, DirectX::XMFLOAT4 ambColor, DirectX::XMFLOAT4 diffColor) {
	
	//転置行列
	DirectX::XMMATRIX transWorldMat = XMMatrixTranspose(world);
	DirectX::XMMATRIX transViewMat = XMMatrixTranspose(view);
	DirectX::XMMATRIX transProjMat = XMMatrixTranspose(proj);

	MatrixBufferType* matBuff{};
	LightBufferType* lightBuff{};

	if FAILED(mConstantBufferMatrix[matNum]->Map(0, nullptr, (void**)&matBuff)) {
		return false;
	}

	matBuff->world = transWorldMat;
	matBuff->view = transViewMat;
	matBuff->projection = transProjMat;

	mConstantBufferMatrix[matNum]->Unmap(0, nullptr);
	matBuff = nullptr;

	if FAILED(mConstantBufferLight->Map(0, nullptr, (void**)&lightBuff)) {
		return false;
	}

	lightBuff->ambientColor = ambColor;
	lightBuff->diffuseColor = diffColor;
	lightBuff->lightDirection = lightDir;

	mConstantBufferLight->Unmap(0, nullptr);
	lightBuff = nullptr;

	//PSOsetup
	cmdList->SetPipelineState(mPipelineState.Get());

	//rootsignature setup
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	//定数バッファをシェーダのレジスタにセット
	ID3D12DescriptorHeap* matrixHeaps[] = { mDescHeapConstantBufferMatrix[matNum].Get() };
	cmdList->SetDescriptorHeaps(_countof(matrixHeaps), matrixHeaps);

	//ルートパラメータに合わせる
	cmdList->SetGraphicsRootDescriptorTable(0, mDescHeapConstantBufferMatrix[matNum].Get()->GetGPUDescriptorHandleForHeapStart());// コマンドリストに設定
	cmdList->SetGraphicsRootConstantBufferView(1, mConstantBufferLight->GetGPUVirtualAddress());

	////テクスチャをシェーダのレジスタにセット
	ID3D12DescriptorHeap* texHeaps[] = { tex };
	cmdList->SetDescriptorHeaps(_countof(texHeaps), texHeaps);
	cmdList->SetGraphicsRootDescriptorTable(2, tex->GetGPUDescriptorHandleForHeapStart());

	return true;
}

HRESULT LightPipeline::CreateRootSignature(ID3D12Device* dev) {
	HRESULT hr{};

	D3D12_DESCRIPTOR_RANGE range[2]{};
	D3D12_ROOT_PARAMETER rootParams[3]{};
	D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	ComPtr<ID3DBlob> blob{};

	range[1].NumDescriptors = 1;
	//レジスタ番号
	range[1].BaseShaderRegister = 0;
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[1].OffsetInDescriptorsFromTableStart = 0;

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[0].DescriptorTable.pDescriptorRanges = &range[1];

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[1].Descriptor.ShaderRegister = 1;
	rootParams[1].Descriptor.RegisterSpace = 0;

	range[0].NumDescriptors = 1;
	range[0].BaseShaderRegister = 0;// レジスタ番号
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].OffsetInDescriptorsFromTableStart = 0;

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &range[0];

	//静的サンプラの設定
	//サンプラ
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
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

	//RootSignatureを作成するのに必要なバッファを確保しTableの情報をシリアライズ
	hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
	if (FAILED(hr)) {
		return hr;
	}
	//RootSignatureの作成
	hr = dev->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));

	return hr;
}



HRESULT LightPipeline::CreatePipelineStateObject(ID3D12Device* dev) {

	HRESULT hr;
	ComPtr<ID3DBlob> vertex_shader{};
	ComPtr<ID3DBlob> pixel_shader{};
#if defined(_DEBUG)

	UINT compile_flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

#else

	UINT compile_flag = 0;

#endif
	hr = D3DCompileFromFile(L"../HLSL/light.hlsl", nullptr, nullptr, "LightVertexShader", "vs_5_0", compile_flag, 0, vertex_shader.GetAddressOf(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	hr = D3DCompileFromFile(L"../HLSL/light.hlsl", nullptr, nullptr, "LightPixelShader", "ps_5_0", compile_flag, 0, pixel_shader.GetAddressOf(), nullptr);
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

	return hr;
}