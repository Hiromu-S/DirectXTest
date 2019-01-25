#pragma once

#include <tchar.h>
#include <Windows.h>
#include <iostream>
//#include "DirectXManager.h"
#include "DX12Manager.h"

class App
{
public:
	App();//LRESULT CALLBACK WndProc);
	~App();

	//int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);

	int Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
private:
	WNDCLASS m_wc;//Window class
	HWND m_hWnd;//Handle of window
	//DirectXManager m_DXManager;
	//DX12Manager mDX12Manager;
	LRESULT CALLBACK m_WndProc;
	MSG m_hMsg;
};

