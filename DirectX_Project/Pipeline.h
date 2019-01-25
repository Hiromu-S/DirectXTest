#pragma once

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <vector>
#include <memory>

using namespace Microsoft::WRL;

#pragma comment(lib, "d3dcompiler.lib")

class Pipeline {
public:
	Pipeline();
	~Pipeline();
	
protected:
	struct MatrixBufferType {
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};	

	ComPtr<ID3D12PipelineState>	mPipelineState;
	ComPtr<ID3D12RootSignature>	mRootSignature;
	ComPtr<ID3D12Resource> mConstantBufferLight;
	
	void RenderShader(ID3D12GraphicsCommandList* dev, int drawIndexCount);

	virtual HRESULT CreateRootSignature(ID3D12Device* dev) = 0;
	virtual HRESULT CreatePipelineStateObject(ID3D12Device* dev) = 0;
};
