#pragma once

#include "Object.h"
#include <DirectXMath.h>

using namespace DirectX;
using namespace Microsoft::WRL;

class Teapot : Object
{
public:
	Teapot();
	~Teapot();

	HRESULT Initialize(ID3D12Device* dev, ID3D12Resource* res) override;
	HRESULT Update() override;
	HRESULT Draw(ID3D12GraphicsCommandList* cmdList) override;

private:
	UINT mIndexCount = 0;
	UINT mVBIndexOffset = 0;
	void* mConstantBufferUploadPtr = nullptr;

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView = {};

	ComPtr<ID3D12Resource> mIndexBuffer;
	ComPtr<ID3D12Resource> mVertexBuffer;
	ComPtr<ID3D12Resource> mConstantBuffer;
	ComPtr<ID3D12Resource> mDepthBuffer;
	ComPtr<ID3D12Resource> mTexture;
	ComPtr<ID3D12DescriptorHeap> mDescHeap;
};

