#include "core/core.h"

/* ############################################# */
/* #██████╗ ██████╗ ███████╗██╗███████╗███████╗# */
/* #██╔══██╗██╔══██╗██╔════╝██║██╔════╝██╔════╝# */
/* #██████╔╝██████╔╝█████╗  ██║███████╗█████╗  # */
/* #██╔═══╝ ██╔══██╗██╔══╝  ██║╚════██║██╔══╝  # */
/* #██║     ██║  ██║███████╗██║███████║███████╗# */
/* #╚═╝     ╚═╝  ╚═╝╚══════╝╚═╝╚══════╝╚══════╝# */
/* ############################################# */

#ifdef _WINDLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	HANDLE hThread;
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		o::handle = hModule;
		hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)load, hModule, 0, nullptr);

		if (hThread != NULL)
			CloseHandle(hThread);

		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#else
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	load();
	return EXIT_SUCCESS;
}
#endif