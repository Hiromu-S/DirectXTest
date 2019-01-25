#include "DirectXManager.h"

DirectXManager::DirectXManager(HWND hWnd)
{
	if (Initialize(hWnd)) {
		SafeRelease();
		exit(1);
	}
}

DirectXManager::~DirectXManager()
{
}

D3D_DRIVER_TYPE driverTypes[] = {
	D3D_DRIVER_TYPE_HARDWARE,
	D3D_DRIVER_TYPE_WARP,
	D3D_DRIVER_TYPE_REFERENCE,
};
UINT numDriverTypes = ARRAYSIZE(driverTypes);

struct Vertex3D {
	float pos[3];
	float col[4];
};

const int TYOUTEN = 3;

Vertex3D hVectorData[TYOUTEN] = {
	{ { +0.0f, +0.5f, +0.5f },{ 1, 1, 1, 1 } },
{ { +0.5f, -0.5f, +0.5f },{ 1, 1, 1, 1 } },
{ { -0.5f, -0.5f, +0.5f },{ 1, 1, 1, 1 } }
};

D3D11_INPUT_ELEMENT_DESC hInElementDesc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

HRESULT DirectXManager::CreateVertexShader(TCHAR* csoName, ID3D11VertexShader** res) {

	//バイナリファイルを読み込む//
	FILE* fp;
	_tfopen_s(&fp, csoName, _T("rb"));
	if (fp == NULL) {

		return -1;
	}
	fseek(fp, 0, SEEK_END);
	long cso_sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	unsigned char* cso_data = new unsigned char[cso_sz];
	fread(cso_data, cso_sz, 1, fp);
	fclose(fp);

	//この時点でコンパイル後のバイナリはcso_dataに格納されている//
	ID3D11InputLayout* hpInputLayout = NULL;

	if (FAILED(m_hpDevice->CreateInputLayout(hInElementDesc, ARRAYSIZE(hInElementDesc), cso_data, cso_sz, &hpInputLayout))) {
		delete[] cso_data;
		return -1;
	}

	//頂点レイアウトをコンテキストに設定
	m_hpDeviceContext->IASetInputLayout(hpInputLayout);

	// 頂点シェーダーオブジェクトの作成//
	ID3D11VertexShader* pVertexShader;
	HRESULT hr = m_hpDevice->CreateVertexShader(cso_data, cso_sz, NULL, &pVertexShader);

	//成功していればpVertexShaderが出来上がっている//
	*res = pVertexShader;

	delete[] cso_data;
	return hr;
}

HRESULT DirectXManager::CreatePixelShader(TCHAR* csoName, ID3D11PixelShader** res) {

	//バイナリファイルを読み込む//
	FILE* fp;
	_tfopen_s(&fp, csoName, _T("rb"));
	if (fp == NULL) {
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	long cso_sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	unsigned char* cso_data = new unsigned char[cso_sz];
	fread(cso_data, cso_sz, 1, fp);
	fclose(fp);

	//この時点でコンパイル後のバイナリはcso_dataに格納されている//

	// ピクセルシェーダーオブジェクトの作成//
	ID3D11PixelShader* pPixelShader;
	HRESULT hr = m_hpDevice->CreatePixelShader(cso_data, cso_sz, NULL, &pPixelShader);

	//成功していればpPixelShaderが出来上がっている//
	*res = pPixelShader;

	delete[] cso_data;
	return hr;
}

/*
TRUE...Failed to initialize
FALSE...Succeed to initialize
*/
bool DirectXManager::Initialize(HWND hWnd) {
	//**Create Device
	m_hDXGISwapChainDesc.BufferDesc.Width = 1024;
	m_hDXGISwapChainDesc.BufferDesc.Height = 768;
	m_hDXGISwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	m_hDXGISwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	m_hDXGISwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	m_hDXGISwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_hDXGISwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	m_hDXGISwapChainDesc.SampleDesc.Count = 1;
	m_hDXGISwapChainDesc.SampleDesc.Quality = 0;
	m_hDXGISwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	m_hDXGISwapChainDesc.BufferCount = 1;
	m_hDXGISwapChainDesc.OutputWindow = hWnd;
	m_hDXGISwapChainDesc.Windowed = TRUE;
	m_hDXGISwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	m_hDXGISwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	D3D_DRIVER_TYPE hDriverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL hFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex) {
		hDriverType = driverTypes[driverTypeIndex];
		m_hr = D3D11CreateDeviceAndSwapChain(NULL, hDriverType, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &m_hDXGISwapChainDesc, &m_hpDXGISwpChain, &m_hpDevice, &hFeatureLevel, &m_hpDeviceContext);
		if SUCCEEDED(m_hr) {
			break;
		}
	}

	/*winerror.h
	#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
	#define FAILED(hr) (((HRESULT)(hr)) < 0)
	*/
	if FAILED(m_hr) {
		//_T ha mosikasitara irankamo
		MessageBox(hWnd, _T("D3D11CreateDevice"), _T("Err"), MB_ICONSTOP);
		return true;
	}
	
	//**Get Interface
	if FAILED(m_hpDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&m_hpDXGI)) {
		MessageBox(hWnd, _T("QueryInterface"), _T("Err"), MB_ICONSTOP);
		return true;
	}
	//**Get Adapter
	if FAILED(m_hpDXGI->GetAdapter(&m_hpAdapter)) {
		MessageBox(hWnd, _T("GetAdapter"), _T("Err"), MB_ICONSTOP);
		return true;
	}
	//**Get Factory
	m_hpAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&m_hpFactory);
	if (m_hpFactory == NULL) {
		MessageBox(hWnd, _T("GetParent-GetFactory"), _T("Err"), MB_ICONSTOP);
		return true;
	}
	//**Let fullscreen to ALT+Enter key
	if FAILED(m_hpFactory->MakeWindowAssociation(hWnd, 0)) {
		MessageBox(hWnd, _T("MakeWindowAssociation"), _T("Err"), MB_ICONSTOP);
		return true;
	}

	
	if FAILED(m_hpFactory->CreateSwapChain(m_hpDevice, &m_hDXGISwapChainDesc, &m_hpDXGISwpChain)) {
		MessageBox(hWnd, _T("CreateSwapChain"), _T("Err"), MB_ICONSTOP);
		return true;
	}

	//**Get back buffer of the swap chain
	if FAILED(m_hpDXGISwpChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_hpBackBuffer)) {
		MessageBox(hWnd, _T("SwpChain GetBuffer"), _T("Err"), MB_ICONSTOP);
		return true;
	}

	//**Create render target from the back buffer 
	if FAILED(m_hpDevice->CreateRenderTargetView(m_hpBackBuffer, NULL, &m_hpRenderTargetView)) {
		MessageBox(hWnd, _T("CreateRenderTargetView"), _T("Err"), MB_ICONSTOP);
		return true;
	}

	//**Set the render target for context
	m_hpDeviceContext->OMSetRenderTargets(1, &m_hpRenderTargetView, NULL);

	//**Set view port
	m_vp.TopLeftX = 0;
	m_vp.TopLeftY = 0;
	m_vp.Width = 1024;
	m_vp.Height = 768;
	m_vp.MinDepth = 0.0f;
	m_vp.MaxDepth = 1.0f;
	m_hpDeviceContext->RSSetViewports(1, &m_vp);

	ID3D11RasterizerState* hpRasterizerState = NULL;
	D3D11_RASTERIZER_DESC hRasterizerDesc = {
		D3D11_FILL_SOLID,
		D3D11_CULL_NONE,
		FALSE,
		0,
		0.0f,
		FALSE,
		FALSE,
		FALSE,
		FALSE,
		FALSE
	};
	if FAILED(m_hpDevice->CreateRasterizerState(&hRasterizerDesc, &hpRasterizerState)) {
		MessageBox(hWnd, _T("CreateRasterizerState"), _T("Err"), MB_ICONSTOP);
		return true;
	}

	m_hpDeviceContext->RSSetState(hpRasterizerState);

	return false;
}

/*How to make vertex buffer
1...Make structure describing vertex.
2...Make new structure(step1)
3...Set value for D3D11_BUFFER_DESC structure.(BindFlags = D3D11_BIND_VERTEX_BUFFER)
	(ByteWidth = size of step1 structure)
4...Set value for D3D11_SUBRESOURCE_DATA structure.(pSysMem needs address for resource data on step2)
5...Call ID3D11Device::CreateBuffer to specify step3, step4 and address of initializing ID3D11Buffer interface
*/

void DirectXManager::draw() {
	float clearCol[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_hpDeviceContext->ClearRenderTargetView(m_hpRenderTargetView, clearCol);
	m_hpDeviceContext->Draw(TYOUTEN, 0);
	m_hpDXGISwpChain->Present(0, 0);
}

void DirectXManager::setTriangle(HWND hWnd) {

	TCHAR vs_file_name[] = "C:/Users/USER/source/repos/DirectX_Project/Debug/DefaultVertexShader.cso";
	TCHAR ps_file_name[] = "C:/Users/USER/source/repos/DirectX_Project/Debug/DefaultPixelShader.cso";

	//頂点バッファ作成
	D3D11_BUFFER_DESC hBufferDesc;
	hBufferDesc.ByteWidth = sizeof(Vertex3D) * TYOUTEN;
	hBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	hBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	hBufferDesc.CPUAccessFlags = 0;
	hBufferDesc.MiscFlags = 0;
	hBufferDesc.StructureByteStride = sizeof(float);

	D3D11_SUBRESOURCE_DATA hSubResourceData;
	hSubResourceData.pSysMem = hVectorData;
	hSubResourceData.SysMemPitch = 0;
	hSubResourceData.SysMemSlicePitch = 0;

	ID3D11Buffer* hpBuffer;
	if (FAILED(m_hpDevice->CreateBuffer(&hBufferDesc, &hSubResourceData, &hpBuffer))) {
		MessageBox(hWnd, _T("CreateBuffer"), _T("Err"), MB_ICONSTOP);
		SafeRelease();
		return;
	}

	//その頂点バッファをコンテキストに設定
	UINT hStrides = sizeof(Vertex3D);
	UINT hOffsets = 0;
	m_hpDeviceContext->IASetVertexBuffers(0, 1, &hpBuffer, &hStrides, &hOffsets);
	m_hpDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	/*https://msdn.microsoft.com/ja-jp/library/ee416275(v=vs.85).aspx*/
	D3D11_SHADER_RESOURCE_VIEW_DESC hpSRViewDesc;
	hpSRViewDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;//Data type
	hpSRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;//1D or 2D or 3D or array
	hpSRViewDesc.Texture2D.MipLevels = 1;
	hpSRViewDesc.Texture2D.MostDetailedMip = 0;

	ID3D11Resource* pResource = NULL;
	ID3D11ShaderResourceView* hpSRView = NULL;
	if FAILED(m_hpDevice->CreateShaderResourceView(pResource, &hpSRViewDesc, &hpSRView)) {
		MessageBox(hWnd, _T("CreateShaderResourceView"), _T("Err"), MB_ICONSTOP);
		SafeRelease();
		return;
	}

	ID3D11ShaderResourceView* hpShaderResourceViews[] = { hpSRView };
	m_hpDeviceContext->PSSetShaderResources(0, 1, hpShaderResourceViews);

	//頂点シェーダー生成
	ID3D11VertexShader* hpVertexShader;
	if (FAILED(CreateVertexShader(vs_file_name, &hpVertexShader))) {
		MessageBox(hWnd, _T("CreateVertexShader"), _T("Err"), MB_ICONSTOP);
		SafeRelease();
		return;
	}

	//ピクセルシェーダー生成
	ID3D11PixelShader* hpPixelShader;
	if (FAILED(CreatePixelShader(ps_file_name, &hpPixelShader))) {
		MessageBox(hWnd, _T("CreatePixelShader"), _T("Err"), MB_ICONSTOP);
		SafeRelease();
		return;
	}

	//頂点シェーダーをコンテキストに設定
	m_hpDeviceContext->VSSetShader(hpVertexShader, NULL, 0);

	//ピクセルシェーダーをコンテキストに設定
	m_hpDeviceContext->PSSetShader(hpPixelShader, NULL, 0);

	//no hull shader
	m_hpDeviceContext->HSSetShader(NULL, NULL, 0);

	//no domain shader
	m_hpDeviceContext->DSSetShader(NULL, NULL, 0);

	//no geometry shader
	m_hpDeviceContext->GSSetShader(NULL, NULL, 0);


}

void DirectXManager::SafeRelease() {
	SAFE_RELEASE(m_hpRenderTargetView);
	SAFE_RELEASE(m_hpBackBuffer);
	SAFE_RELEASE(m_hpDXGISwpChain);
	SAFE_RELEASE(m_hpFactory);
	SAFE_RELEASE(m_hpAdapter);
	SAFE_RELEASE(m_hpDXGI);
	SAFE_RELEASE(m_hpDeviceContext);
	SAFE_RELEASE(m_hpDevice);
}

