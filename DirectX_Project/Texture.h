#pragma once

#include <d3d12.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

class Texture
{
public:
	Texture(float windowWidth, float windowHeight);
	~Texture();

	bool Initialize(ID3D12Device* dev, float texWidth, float texHeight);

	void SetRenderTarget(ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* dsv);
	void ClearRenderTarget(ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* dsv, float r, float g, float b, float a);

	ID3D12DescriptorHeap* GetSRV() { return mDescHeapSRV.Get(); }
	ID3D12Resource* GetTexBuffer() { return mTexBuffer.Get(); }

private:
	ComPtr<ID3D12DescriptorHeap> mDescHeapSRV;
	ComPtr<ID3D12DescriptorHeap> mDescHeapRTV;
	ComPtr<ID3D12Resource> mTexBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE mRTVHandle;

	D3D12_VIEWPORT mViewport{};
	D3D12_RECT mScissor{};

};

