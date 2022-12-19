#pragma once

#include "DxCore.h"
#include "Camera.h"

class d3dApplication
{
public:
	d3dApplication(HINSTANCE hInstance);
	virtual ~d3dApplication();

	virtual bool Initialize();

	int Run();
	virtual void TickRender(float deltaTime);
	virtual void TickLogic(float deltaTime);
	virtual int InitMainWindow();
	virtual bool InitializeD3d();

	DxCore* GetDXCore() { return dx; }
	static d3dApplication* GetD3dApp() { return d3dApp; }
	virtual LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnResize();
	void OnMouseDown(WPARAM wParam, int x, int y);
	void OnMouseUp(WPARAM wParam, int x, int y);
	void OnMouseMove(WPARAM wParam, int x, int y);
	void OnKeyboradInput();

	void CalculateFPS();

protected:

	long WindowWidth = 1280;
	long WindowHeight = 720;

	static d3dApplication* d3dApp;

	imguiManager imgui;

	DxCore* dx;
	Camera camera = { WindowWidth, WindowHeight };
	d3dTimer Timer;

	HWND hwnd = nullptr;
	HINSTANCE instanceHandle = nullptr;


	int lastMousePosX;
	int lastMousePosY;

	float MouseSpeed = 0.25f;
};
