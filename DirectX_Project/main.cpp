#include "App.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	App app;
	//app.main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	app.Run(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}