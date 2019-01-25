#include "App.h"

App::App()//LRESULT CALLBACK WndProc)
{
	//m_WndProc = WndProc;
}

App::~App()
{
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) {
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			PostMessage(hWnd, WM_DESTROY, 0, 0);
			return 0;
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/*
HINSTANCE...Data type of storing the handle
LPSTR...Windows basic string type
nCmdShow...Display state
*/

static HWND setupWindow(int width, int height)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = (HMODULE)GetModuleHandle(0);
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = _T("WindowClass");
	wcex.hIconSm = nullptr;
	if (!RegisterClassEx(&wcex)) {
		throw runtime_error("RegisterClassEx()");
	}

	RECT rect = { 0, 0, width, height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	const int windowWidth = (rect.right - rect.left);
	const int windowHeight = (rect.bottom - rect.top);

	HWND hWnd = CreateWindow(_T("WindowClass"), _T("Window"),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, windowWidth, windowHeight,
		nullptr, nullptr, nullptr, nullptr);
	if (!hWnd) {
		throw runtime_error("CreateWindow()");
	}

	return hWnd;
}

int App::Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	MSG  msg;
	ZeroMemory(&msg, sizeof msg);
	ID3D12Device* dev = nullptr;

#ifdef NDEBUG

	try

#endif	
	{
		g_mainWindowHandle = setupWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
		ShowWindow(g_mainWindowHandle, SW_SHOW);
		UpdateWindow(g_mainWindowHandle);
		DX12Manager mDX12Manager(WINDOW_WIDTH, WINDOW_HEIGHT, g_mainWindowHandle);
		dev = mDX12Manager.GetDevice();

		while (msg.message != WM_QUIT) {
			BOOL r = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
			if (r == 0) {
				mDX12Manager.Render();
			}
			else {
				DispatchMessage(&msg);
			}
		}
	}

#ifdef NDEBUG

	catch (std::exception &e) {
		MessageBoxA(g_mainWindowHandle, e.what(), "Exception occuured.", MB_ICONSTOP);
	}

#endif
	if (dev)
		dev->Release();

	return static_cast<int>(msg.wParam);
}

/*
int App::main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	//**Register window class
	m_wc.style = CS_HREDRAW | CS_VREDRAW;//Window style
	m_wc.lpfnWndProc = WndProc;//Pointer of callback function
	m_wc.cbClsExtra = 0;//Usually assign 0
	m_wc.cbWndExtra = 0;//Usually assign 0
	m_wc.hInstance = hInstance;//Instance handle
	m_wc.hIcon = NULL;//Handle of the icon
	m_wc.hCursor = NULL;//Handle of the cursor
	m_wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);//Color of window's background
	m_wc.lpszMenuName = NULL;//Default name of menu
	TCHAR szWindowClass[] = "3DDISPPG";//Name of this window class
	m_wc.lpszClassName = szWindowClass;
	RegisterClass(&m_wc);

	//**Create window
	m_hWnd = CreateWindow(szWindowClass,
		"3D Disp Pg",//Title
		WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,//Window style
		0,//X coordinate of window's upper left
		0,//Y coordinate of window's upper left
		1024,//Window's width
		768,//Window's height
		NULL,//Handle of parent's window
		NULL,//Handle of menu
		hInstance,//Instance handle of the module creating window or etc...
		NULL//NULL
	);

	//**Error process
	if (m_hWnd == NULL) {
		std::cerr << "Failed to create window." << std::endl;
		exit(1);
	}

	m_DXManager = DirectXManager(m_hWnd);

	m_DXManager.setTriangle(m_hWnd);

	//**Main loop
	while (true) {
		//Check the message in queue and if it exist, store it in hMsg
		while (PeekMessageW(&m_hMsg, NULL, 0, 0, PM_REMOVE)) {
			if (m_hMsg.message == WM_QUIT) {
				m_DXManager.SafeRelease();
				return 0;
			}
			TranslateMessage(&m_hMsg);
			DispatchMessage(&m_hMsg);
		}
		
		m_DXManager.draw();
	}

	return 0;
}*/