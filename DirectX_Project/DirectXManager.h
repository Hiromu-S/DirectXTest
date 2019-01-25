#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <tchar.h>
#include <Windows.h>
#include <iostream>
#include <cstdio>

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

class DirectXManager
{
public:
	DirectXManager() {}
	DirectXManager(HWND hWnd);
	~DirectXManager();

	bool Initialize(HWND hWnd);

	void SafeRelease();

	HRESULT CreateVertexShader(TCHAR* csoName, ID3D11VertexShader** res);
	HRESULT CreatePixelShader(TCHAR* csoName, ID3D11PixelShader** res);
	void setTriangle(HWND hWnd);

	void draw();

private:
	HRESULT m_hr;
	ID3D11Device* m_hpDevice = NULL;
	ID3D11DeviceContext* m_hpDeviceContext = NULL;
	IDXGIDevice1* m_hpDXGI = NULL;
	IDXGIAdapter* m_hpAdapter = NULL;//Interface of display subsystem(video card, etc...)
	IDXGIFactory* m_hpFactory = NULL;
	IDXGISwapChain* m_hpDXGISwpChain = NULL;
	DXGI_SWAP_CHAIN_DESC m_hDXGISwapChainDesc;
	ID3D11Texture2D* m_hpBackBuffer = NULL;
	ID3D11RenderTargetView* m_hpRenderTargetView = NULL;
	D3D11_VIEWPORT m_vp;
};

