#include "Teapot.h"
#include "DX12Manager.h"
#include <vector>

Teapot::Teapot()
{
}


Teapot::~Teapot()
{
	mConstantBuffer->Unmap(0, nullptr);
}

HRESULT Teapot::Initialize(ID3D12Device* dev, ID3D12Resource* res) {
	HRESULT hr = 1;

	WaveFrontReader<uint16_t> mesh;
	CHK(mesh.Load(L"../Resource/teapot_tex2.obj"));

	mIndexCount = static_cast<UINT>(mesh.indices.size());
	mVBIndexOffset = static_cast<UINT>(sizeof(mesh.vertices[0]) * mesh.vertices.size());
	UINT IBSize = static_cast<UINT>(sizeof(mesh.indices[0]) * mIndexCount);

	void* vbData = mesh.vertices.data();
	void* ibData = mesh.indices.data();
	CHK(dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mVBIndexOffset + IBSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mVertexBuffer.ReleaseAndGetAddressOf())));
	mVertexBuffer->SetName(L"VertexBuffer");
	char* vbUploadPtr = nullptr;
	CHK(mVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vbUploadPtr)));
	memcpy_s(vbUploadPtr, mVBIndexOffset, vbData, mVBIndexOffset);
	memcpy_s(vbUploadPtr + mVBIndexOffset, IBSize, ibData, IBSize);
	mVertexBuffer->Unmap(0, nullptr);

	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = sizeof(mesh.vertices[0]);
	mVertexBufferView.SizeInBytes = mVBIndexOffset;
	mIndexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress() + mVBIndexOffset;
	mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	mIndexBufferView.SizeInBytes = IBSize;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 2;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CHK(dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(mDescHeap.ReleaseAndGetAddressOf())));
	/*
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	descHeapDesc.NumDescriptors = 100;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	CHK(dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(mDescHeapSampler.ReleaseAndGetAddressOf())));
	*/
	CHK(dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mConstantBuffer.ReleaseAndGetAddressOf())));
	mConstantBuffer->SetName(L"ConstantBuffer");
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = mConstantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // must be a multiple of 256
	dev->CreateConstantBufferView(
		&cbvDesc,
		mDescHeap->GetCPUDescriptorHandleForHeapStart());
	//CHK(mConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mConstantBufferUploadPtr)));

	{
		// Read DDS file

		vector<char> texData(4 * 256 * 256);
		ifstream ifs("../Resource/orange.dds", ios::binary);
		if (!ifs)
			throw runtime_error("Texture not found.");
		ifs.seekg(128, ios::beg); // Skip DDS header
		ifs.read(texData.data(), texData.size());
		ifs.close();

		//Read all file
		/*
		vector<byte> texData;
		fstream ifs("orange.png", ios_base::in | ios_base::binary);
		if (ifs.fail()) {
		throw runtime_error("Texture not found.");
		}
		ifs.seekg(0, ios::end);
		int size = ifs.tellg();
		int width, height;
		ifs.read((char *)&width, sizeof(width));
		ifs.read((char *)&height, sizeof(height));
		texData.resize(4 * width * height);
		ifs.read((char *)&texData[0], texData.size());
		ifs.close();*/

		auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_B8G8R8A8_UNORM, 256, 256, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE,
			D3D12_TEXTURE_LAYOUT_UNKNOWN, 0);
		D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_CUSTOM, D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 0, 0 };
		CHK(dev->CreateCommittedResource(
			//&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(mTexture.ReleaseAndGetAddressOf())));
		mTexture->SetName(L"Texure");
		D3D12_BOX box = {};
		box.right = 256;
		box.bottom = 256;
		box.back = 1;
		CHK(mTexture->WriteToSubresource(0, &box, texData.data(), 4 * 256, 4 * 256 * 256));
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0; // No MIP
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	auto cbvSrcUavDescHandle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
	//dev->CreateShaderResourceView(mTexture.Get(), &srvDesc, cbvSrcUavDescHandle);
	/*
	//シャドウマップ処理
	//srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	auto cbvSrvUavDescStep = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cbvSrcUavDescHandle.ptr += cbvSrvUavDescStep;
	dev->CreateShaderResourceView(mTexture.Get(), &srvDesc, cbvSrcUavDescHandle);
	//dev->(res,,);
	D3D12_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = FLT_MAX;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	dev->CreateSampler(&samplerDesc, mDescHeapSampler->GetCPUDescriptorHandleForHeapStart());
	*/

	return hr;
}

HRESULT Teapot::Update() {
	// Upload constant buffer
	static float rot = 0.0f;
	rot += 1.0f;
	if (rot >= 360.0f) rot = 0.0f;
	
	XMMATRIX worldMat, viewMat, projMat;
	worldMat = XMMatrixRotationY(XMConvertToRadians(rot));
	viewMat = XMMatrixLookAtLH({ 0, 1, -2.5f }, { 0, 0.5f, 0 }, { 0, 1, 0 });
	projMat = XMMatrixPerspectiveFovLH(45, (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.01f, 50.0f);
	XMFLOAT4X4 mat, world;
	XMStoreFloat4x4(&mat, XMMatrixTranspose(worldMat * viewMat * projMat));
	XMStoreFloat4x4(&world, XMMatrixTranspose(worldMat));
	//auto mvpMat = XMMatrixTranspose(worldMat * viewMat * projMat);
	//auto worldTransMat = XMMatrixTranspose(worldMat);
	
	// mCBUploadPtr is Write-Combine memory
	XMFLOAT4X4* buffer{};
	CHK(mConstantBuffer->Map(0, nullptr, (void**)&buffer));
	buffer[0] = mat;
	buffer[1] = world;
	mConstantBuffer->Unmap(0, nullptr);
	buffer = nullptr;
	//memcpy_s(mConstantBufferUploadPtr, 64, &mvpMat, 64);
	//memcpy_s(reinterpret_cast<char*>(mConstantBufferUploadPtr) + 64, 64, &worldTransMat, 64);

	return S_OK;
}

HRESULT Teapot::Draw(ID3D12GraphicsCommandList* cmdList) {
	// Draw
	//ID3D12DescriptorHeap* descHeaps[] = { mDescHeap.Get(), mDescHeapSampler.Get() };
	//cmdList->SetDescriptorHeaps(ARRAYSIZE(descHeaps), descHeaps);
	
	//cmdList->SetGraphicsRootConstantBufferView(0, mConstantBuffer->GetGPUVirtualAddress());
	//cmdList->SetDescriptorHeaps(1, mDescHeap.GetAddressOf());
	//cmdList->SetGraphicsRootDescriptorTable(2, mDescHeap->GetGPUDescriptorHandleForHeapStart());
	//cmdList->SetGraphicsRootDescriptorTable(1, mDescHeapSampler->GetGPUDescriptorHandleForHeapStart());
	cmdList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	cmdList->IASetIndexBuffer(&mIndexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//cmdList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);

	return S_OK;
}