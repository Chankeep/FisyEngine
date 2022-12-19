#include "d3dApplication.h"

//for the imgui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd,
	UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return d3dApplication::GetD3dApp()->WndProc(hWnd, message, wParam, lParam);
}

d3dApplication* d3dApplication::d3dApp = nullptr;

d3dApplication::d3dApplication(HINSTANCE hInstance) : instanceHandle(hInstance)
{
	assert(d3dApp == nullptr);
	d3dApp = this;
}

d3dApplication::~d3dApplication()
{
	dx->OnDestroy();
}

bool d3dApplication::Initialize()
{
	if (!InitMainWindow())
		return false;
	if (!InitializeD3d())
		return false;

	return true;
}

int d3dApplication::Run()
{
	MSG msg = { 0 };

	Timer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Timer.Tick();
			TickLogic(Timer.GetDeltaTime());
			TickRender(Timer.GetDeltaTime());
		}
	}

	return static_cast<int>(msg.wParam);
}

void d3dApplication::TickRender(float deltaTime)
{
	dx->OnRender();
}
void d3dApplication::TickLogic(float deltaTime)
{
	CalculateFPS();
	OnKeyboradInput();
	camera.UpdateViewMatrix();
	dx->rotateBox(Timer.GetDeltaTime());
	dx->OnUpdate(camera.GetViewMatrix(), camera.GetProjMatrix());
}

int d3dApplication::InitMainWindow()
{
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WindowProc;
	wc.style = CS_GLOBALCLASS;
	wc.hInstance = instanceHandle;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszClassName = L"d3dWndClass";
	if (!RegisterClassEx(&wc))
	{
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return 0;
	}

	RECT R = { 0, 0, WindowWidth, WindowHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	hwnd = CreateWindowW(L"d3dWndClass",
		L"DirectX 12 made by chankkep",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width, height,
		nullptr, nullptr,
		instanceHandle, nullptr);

	if (!hwnd)
		return FALSE;

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	ImGui_ImplWin32_Init(hwnd);

	return true;
}

bool d3dApplication::InitializeD3d()
{
	dx = new DxCore(hwnd);
	dx->OnInit();
	dx->OnResize(WindowWidth, WindowHeight);

	return true;
}

LRESULT d3dApplication::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	const ImGuiIO imio = ImGui::GetIO();

	switch (msg)
	{
	case WM_CLOSE:
		if (MessageBox(hwnd, L"要退出吗？", L"Fisy", MB_OKCANCEL) == IDOK)
		{
			DestroyWindow(hwnd);
		}
		// Else: User canceled. Do nothing.
		return 0;
	case WM_SIZE:
		WindowWidth = LOWORD(lParam);
		WindowHeight = HIWORD(lParam);
		if (wParam != SIZE_MINIMIZED && DxCore::isInitialized)
			OnResize();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSEMOVE:
		if (imio.WantCaptureMouse)
			break;
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void d3dApplication::OnResize()
{
	dx->OnResize(WindowWidth, WindowHeight);
}

void d3dApplication::OnMouseDown(WPARAM wParam, int x, int y)
{
	lastMousePosX = x;
	lastMousePosY = y;

	SetCapture(hwnd);
}

void d3dApplication::OnMouseUp(WPARAM wParam, int x, int y)
{
	ReleaseCapture();
}

void d3dApplication::OnMouseMove(WPARAM wParam, int x, int y)
{
	if ((wParam & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - lastMousePosX));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - lastMousePosY));

		camera.Pitch(dy);
		camera.Yaw(dx);


	}
	// else if ((wParam & MK_RBUTTON) != 0)
	// {
	// 	float dx = XMConvertToRadians(static_cast<float>(x - lastMousePosX));
	// 	float dy = XMConvertToRadians(static_cast<float>(y - lastMousePosY));
	// }
	lastMousePosX = x;
	lastMousePosY = y;
}

void d3dApplication::OnKeyboradInput()
{
	const float deltatime = Timer.GetDeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
	{
		camera.Walk(10.0f * deltatime);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		camera.Walk(-10.0f * deltatime);
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		camera.Strafe(-10.0f * deltatime);
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		camera.Strafe(10.0f * deltatime);
	}
}

void d3dApplication::CalculateFPS()
{
	SetWindowText(hwnd, dx->GetAdapterName());
}
