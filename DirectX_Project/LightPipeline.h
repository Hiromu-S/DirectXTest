#pragma once
#include "Pipeline.h"

class LightPipeline : public Pipeline {
public:
	LightPipeline();
	~LightPipeline();

	bool Initialize(ID3D12Device* dev, int drawIndexCount);
	bool Render(ID3D12GraphicsCommandList* cmdList, 
		int, int, 
		DirectX::XMMATRIX, DirectX::XMMATRIX, DirectX::XMMATRIX, 
		ID3D12DescriptorHeap* descHeap, 
		DirectX::XMFLOAT3, DirectX::XMFLOAT4, DirectX::XMFLOAT4);

protected:

	struct LightBufferType {
		DirectX::XMFLOAT4 ambientColor;
		DirectX::XMFLOAT4 diffuseColor;
		DirectX::XMFLOAT3 lightDirection;
		float padding;
	};

	std::vector<ComPtr<ID3D12Resource>> mConstantBufferMatrix;
	std::vector<ComPtr<ID3D12DescriptorHeap>> mDescHeapConstantBufferMatrix;	

	bool SetShaderParameters(ID3D12GraphicsCommandList*, 
		int, 
		DirectX::XMMATRIX, DirectX::XMMATRIX, DirectX::XMMATRIX, 
		ID3D12DescriptorHeap*, 
		DirectX::XMFLOAT3, DirectX::XMFLOAT4, DirectX::XMFLOAT4);

	HRESULT CreateRootSignature(ID3D12Device* dev) override;
	HRESULT CreatePipelineStateObject(ID3D12Device* dev) override;
};

