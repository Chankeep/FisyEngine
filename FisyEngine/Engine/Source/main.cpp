#include <windows.h>
#include "d3dApplication.h"

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{

	CoInitialize(nullptr);
#if defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		d3dApplication dxApp(hInstance);
		if (!dxApp.Initialize())
			return 0;

		return dxApp.Run();
	}
	catch (HrException& e)
	{
		MessageBox(nullptr, std::to_wstring(e.Error()).c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

