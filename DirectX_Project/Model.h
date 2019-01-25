#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

using namespace Microsoft::WRL;
using namespace DirectX;

class Model
{

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT2 uv;
		XMFLOAT3 normal;
	};

	struct ModelType
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

public:
	Model();
	~Model();

	bool Initialize(ID3D12Device* dev, const WCHAR* modelFileName, const char* textureFileName);
	bool LoadModelAndCreateBuffers(ID3D12Device* dev, const WCHAR* modelFileName);
	bool LoadTexture(ID3D12Device* dev, const char* textureFileName);

	bool LoadModel(const WCHAR* filename);
	bool InitializeBuffers(ID3D12Device* dev);

	void Render(ID3D12GraphicsCommandList* cmdList);

	const UINT GetIndexCount() const { return mIndexCount; }
	ID3D12Resource* GetTexture() { return mTexture.Get(); }
	ID3D12DescriptorHeap* GetDescHeapTexture() { return mDescHeapTexture.Get(); }

protected:
	UINT mIndexCount = 0;
	UINT mVertexCount = 0;
	UINT mVBIndexOffset = 0;

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView = {};

	ModelType* mModel = nullptr;

	ComPtr<ID3D12Resource> mIndexBuffer;
	ComPtr<ID3D12Resource> mVertexBuffer;
	ComPtr<ID3D12Resource> mTexture;
	ComPtr<ID3D12DescriptorHeap> mDescHeapTexture;
};

