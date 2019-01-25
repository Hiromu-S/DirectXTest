#pragma once
#include "Pipeline.h"
class ShadowPipeline :
	public Pipeline
{
public:
	ShadowPipeline();
	~ShadowPipeline();

	bool Initialize(ID3D12Device* dev);

	bool Render(ID3D12GraphicsCommandList* cmdList,
		int matNum, int drawIndexCount,
		DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj,
		ID3D12DescriptorHeap* tex,
		DirectX::XMFLOAT3 lightDir, DirectX::XMFLOAT4 ambColor, DirectX::XMFLOAT4 diffColor);

private:

	HRESULT CreateRootSignature(ID3D12Device* dev) override;
	HRESULT CreatePipelineStateObject(ID3D12Device* dev) override;
};

