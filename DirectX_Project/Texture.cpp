#include "Texture.h"



Texture::Texture(float windowWidth, float windowHeight) {
	mViewport.Width = windowWidth;
	mViewport.Height = windowHeight;
	mViewport.MaxDepth = 1.0f;

	mScissor.right = windowWidth;
	mScissor.bottom = windowHeight;
}


Texture::~Texture()
{
}

bool Texture::Initialize(ID3D12Device* dev, float texWidth, float texHeight) {
	D3D12_HEAP_PROPERTIES heapProps{};
	D3D12_RESOURCE_DESC resDesc{};

	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 0;
	heapProps.VisibleNodeMask = 0;

	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Alignment = 0;
	resDesc.Width = 640U;
	resDesc.Height = 480U;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearVal{};
	FLOAT clearCol[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearVal.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	clearVal.Color[0] = clearCol[0];
	clearVal.Color[1] = clearCol[1];
	clearVal.Color[2] = clearCol[2];
	clearVal.Color[3] = clearCol[3];
	clearVal.DepthStencil.Depth = 0.0f;
	clearVal.DepthStencil.Stencil = 0;

	if FAILED(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &clearVal, IID_PPV_ARGS(&mTexBuffer))) {
		return false;
	}

	// Heap作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	if FAILED(dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDescHeapSRV))) {
		return false;
	}

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if FAILED(dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDescHeapRTV))) {
		return false;
	}

	D3D12_RENDER_TARGET_VIEW_DESC render_desc{};
	render_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	render_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;  //レンダーターゲットリソースへのアクセス方法を指定します。
	render_desc.Texture2D.MipSlice = 0;							//使用できるみっぷマップレベルのインデックス
	render_desc.Texture2D.PlaneSlice = 0;						//テクスチャで使用する平面のインデックス
	render_desc.Buffer.FirstElement = 0;						// アクセスするバッファの設定
	render_desc.Buffer.NumElements = 1;

	mRTVHandle = mDescHeapRTV.Get()->GetCPUDescriptorHandleForHeapStart();
	dev->CreateRenderTargetView(mTexBuffer.Get(), &render_desc, mRTVHandle);

	D3D12_SHADER_RESOURCE_VIEW_DESC resViewDesc{};
	resViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	resViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	resViewDesc.Texture2D.MipLevels = 1;
	resViewDesc.Texture2D.MostDetailedMip = 0;
	resViewDesc.Texture2D.PlaneSlice = 0;
	resViewDesc.Texture2D.ResourceMinLODClamp = 0.0F;
	resViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	dev->CreateShaderResourceView(mTexBuffer.Get(), &resViewDesc, mDescHeapSRV->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void Texture::SetRenderTarget(ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* dsv) {
	cmdList->OMSetRenderTargets(1, &mRTVHandle, TRUE, &dsv->GetCPUDescriptorHandleForHeapStart());
}

void Texture::ClearRenderTarget(ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* dsv, float r, float g, float b, float a) {
	float color[4] = {
		r, g, b, a
	};

	cmdList->ClearRenderTargetView(mRTVHandle, color, 0, nullptr);

	cmdList->ClearDepthStencilView(dsv->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissor);
}