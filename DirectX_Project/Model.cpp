#include "Model.h"
#include "DX12Manager.h"

Model::Model()
{
}


Model::~Model()
{
}

bool Model::Initialize(ID3D12Device* dev, const WCHAR* modelFileName, const char* textureFileName) {
	//if (LoadModelAndCreateBuffers(dev, modelFileName) == false) {
		//return false;
	//}

	if (LoadModel(modelFileName) == false) {
		return false;
	}

	if (InitializeBuffers(dev) == false) {
		return false;
	}

	if (LoadTexture(dev, textureFileName) == false) {
		return false;
	}

	return true;
}

//テンプレ
bool Model::LoadModelAndCreateBuffers(ID3D12Device* dev, const WCHAR* modelFileName) {
	WaveFrontReader<uint16_t> mesh;
	if FAILED(mesh.Load(modelFileName)) {
		throw std::runtime_error("mesh not found");
		return false;
	}

	mIndexCount = static_cast<UINT>(mesh.indices.size());
	mVBIndexOffset = static_cast<UINT>(sizeof(mesh.vertices[0]) * mesh.vertices.size());
	UINT IBSize = static_cast<UINT>(sizeof(mesh.indices[0]) * mIndexCount);

	void* vbData = mesh.vertices.data();
	void* ibData = mesh.indices.data();
	if FAILED(dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mVBIndexOffset + IBSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,	
		IID_PPV_ARGS(mVertexBuffer.ReleaseAndGetAddressOf()))) {
		throw std::runtime_error("failed to create vertex buffer.");
		return false;
	}
	mVertexBuffer->SetName(L"VertexBuffer");
	char* vbUploadPtr = nullptr;
	if FAILED(mVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vbUploadPtr))) {
		return false;
	}
	memcpy_s(vbUploadPtr, mVBIndexOffset, vbData, mVBIndexOffset);
	memcpy_s(vbUploadPtr + mVBIndexOffset, IBSize, ibData, IBSize);
	mVertexBuffer->Unmap(0, nullptr);
	
	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = sizeof(mesh.vertices[0]);
	mVertexBufferView.SizeInBytes = mVBIndexOffset;
	mIndexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress() + mVBIndexOffset;
	mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	mIndexBufferView.SizeInBytes = IBSize;

	return true;
}

bool Model::LoadModel(const WCHAR* filename)
{
	ifstream fin;
	char input;
	int i;

	fin.open(filename);
	if (fin.fail())
	{
		return false;
	}

	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}

	fin >> mVertexCount;

	//頂点数=インデックス数
	mIndexCount = mVertexCount;

	mModel = new ModelType[mVertexCount];
	if (!mModel)
	{
		return false;
	}

	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin.get(input);
	fin.get(input);

	for (i = 0; i < mVertexCount; i++)
	{
		fin >> mModel[i].x >> mModel[i].y >> mModel[i].z;
		fin >> mModel[i].tu >> mModel[i].tv;
		fin >> mModel[i].nx >> mModel[i].ny >> mModel[i].nz;
	}

	fin.close();

	return true;
}

bool Model::InitializeBuffers(ID3D12Device* device)
{
	//vertex index
	Vertex* vertices;
	unsigned long* indices;
	D3D12_HEAP_PROPERTIES heapProperties{};
	D3D12_RESOURCE_DESC   resourceDesc{};
	HRESULT result;
	int i;

	vertices = new Vertex[mVertexCount];
	if (!vertices)
	{
		return false;
	}

	indices = new unsigned long[mIndexCount];
	if (!indices)
	{
		return false;
	}

	for (i = 0; i<mVertexCount; i++)
	{
		vertices[i].position = XMFLOAT3(mModel[i].x, mModel[i].y, mModel[i].z);
		vertices[i].uv = XMFLOAT2(mModel[i].tu, mModel[i].tv);
		vertices[i].normal = XMFLOAT3(mModel[i].nx, mModel[i].ny, mModel[i].nz);

		indices[i] = i;
	}

	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeof(Vertex) * mVertexCount;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;

	//頂点バッファ
	{
		result = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertexBuffer));
		if (FAILED(result)) {
			return result;
		}
	}
	//インデックスバッファ
	{
		resourceDesc.Width = sizeof(unsigned long) * mIndexCount;
		result = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mIndexBuffer));
		if (FAILED(result)) {
			return result;
		}
	}

	//頂点データの書き込み
	Vertex *vBuffer{};
	mVertexBuffer->Map(0, nullptr, (void**)&vBuffer);
	memcpy(&vBuffer[0], &vertices[0], sizeof(Vertex) * mVertexCount);
	vBuffer = nullptr;
	mVertexBuffer->Unmap(0, nullptr);

	//インデックスデータの書き込み
	unsigned long *ibuffer{};
	mIndexBuffer->Map(0, nullptr, (void**)&ibuffer);
	memcpy(&ibuffer[0], &indices[0], sizeof(unsigned long) * mIndexCount);
	ibuffer = nullptr;
	//閉
	mIndexBuffer->Unmap(0, nullptr);

	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}


bool Model::LoadTexture(ID3D12Device* dev, const char* textureFileName) {
	//texture指定なしもOK
	if (textureFileName == nullptr) {
		return true;
	}

	std::vector<char> texData(4 * 256 * 256);
	std::ifstream ifs(textureFileName, std::ios::binary);
	if (!ifs)
		throw std::runtime_error("Texture not found.");
	//ヘッダのスキップ
	ifs.seekg(128, std::ios::beg);
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
	if FAILED(dev->CreateCommittedResource(
		//&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mTexture.ReleaseAndGetAddressOf()))) {
		return false;
	}
	mTexture->SetName(L"Texure");
	D3D12_BOX box = {};
	box.right = 256;
	box.bottom = 256;
	box.back = 1;
	if FAILED(mTexture->WriteToSubresource(0, &box, texData.data(), 4 * 256, 4 * 256 * 256)) {
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if FAILED(dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(mDescHeapTexture.ReleaseAndGetAddressOf()))) {
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0; // No MIP
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	auto cbvSrcUavDescHandle = mDescHeapTexture->GetCPUDescriptorHandleForHeapStart();
	dev->CreateShaderResourceView(mTexture.Get(), &srvDesc, cbvSrcUavDescHandle);

	return true;
}

void Model::Render(ID3D12GraphicsCommandList* cmdList) {

	unsigned int stride;
	unsigned int offset;


	// Set vertex buffer stride and offset.
	stride = sizeof(Vertex);
	offset = 0;

	D3D12_VERTEX_BUFFER_VIEW vertex_view{};
	vertex_view.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	vertex_view.StrideInBytes = sizeof(Vertex);
	vertex_view.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * mVertexCount);

	D3D12_INDEX_BUFFER_VIEW index_view{};
	index_view.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	index_view.SizeInBytes = static_cast<UINT>(sizeof(unsigned long) * mIndexCount);
	index_view.Format = DXGI_FORMAT_R16_UINT;

	cmdList->IASetVertexBuffers(0, 1, &vertex_view);
	cmdList->IASetIndexBuffer(&index_view);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}