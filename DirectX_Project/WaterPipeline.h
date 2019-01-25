#pragma once
#include "Pipeline.h"
class WaterPipeline :
	public Pipeline
{
public:
	WaterPipeline();
	~WaterPipeline();

	bool Initialize(ID3D12Device* dev);

	bool Render(ID3D12GraphicsCommandList* cmdList, int drawIndexCount, 
		DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj, DirectX::XMMATRIX reflectionMatrix,
		ID3D12DescriptorHeap* reflectionTexture, ID3D12DescriptorHeap* refractionTexture, ID3D12DescriptorHeap* normalTexture,
		float waterTranslation, float reflectionRefractionScale);

private:

	ComPtr<ID3D12Resource> mWaterBuffer;
	ComPtr<ID3D12Resource> mReflectionBuffer;
	ComPtr<ID3D12Resource> mConstantBufferMatrix;
	
	struct MatrixBufferType
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
	};
	struct ReflectionBufferType
	{
		DirectX::XMMATRIX reflection;
	};
	struct WaterBufferType
	{
		float waterTranslation;
		float reflectRefractScale;
		DirectX::XMFLOAT2 padding;
	};

	bool SetShaderParameters(ID3D12GraphicsCommandList* cmdList,
		DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj, DirectX::XMMATRIX reflectionMatrix,
		ID3D12DescriptorHeap* reflectionTexture, ID3D12DescriptorHeap* refractionTexture, ID3D12DescriptorHeap* normalTexture,
		float waterTranslation, float reflectionRefractionScale);

	HRESULT CreateRootSignature(ID3D12Device* dev) override;
	HRESULT CreatePipelineStateObject(ID3D12Device* dev) override;
};

