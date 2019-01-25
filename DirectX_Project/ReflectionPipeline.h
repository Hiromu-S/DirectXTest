#pragma once
#include "Pipeline.h"

class ReflectionPipeline : public Pipeline {
public:
	ReflectionPipeline();
	~ReflectionPipeline();

	bool Initialize(ID3D12Device* dev);

protected:
	struct reflection {
		DirectX::XMMATRIX lightDir;
		DirectX::XMMATRIX ambColor;
		DirectX::XMMATRIX diffColor;
	};

	HRESULT CreateRootSignature(ID3D12Device* dev) override;
	HRESULT CreatePipelineStateObject(ID3D12Device* dev) override;
};

