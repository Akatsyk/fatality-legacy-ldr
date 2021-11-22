#include <Windows.h>
#include "MinHook/MinHook.h"


using ofunc = BOOL( WINAPI * )( _In_opt_ LPCWSTR lpApplicationName,
                                _Inout_opt_ LPWSTR lpCommandLine,
                                _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                _In_ BOOL bInheritHandles,
                                _In_ DWORD dwCreationFlags,
                                _In_opt_ LPVOID lpEnvironment,
                                _In_opt_ LPCWSTR lpCurrentDirectory,
                                _In_ LPSTARTUPINFOW lpStartupInfo,
                                _Out_ LPPROCESS_INFORMATION lpProcessInformation );
ofunc original;
HMODULE _self;
bool queue_unload = false;

BOOL
WINAPI
_CreateProcessW(
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation
) {
    if( wcsstr( lpApplicationName, L"csgo.exe" ) )
        dwCreationFlags |= CREATE_SUSPENDED;

    auto result = original( lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation );

    if( wcsstr( lpApplicationName, L"csgo.exe" ) ) {
        VirtualAllocEx( lpProcessInformation->hProcess, ( void * )0x1420000, 0x326000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE ); //allocation for legacy module
        queue_unload = true;
    }

    return result;
}

unsigned long __stdcall thr( void * args ) {
    /*original = ( ofunc )DetourFunction( ( PBYTE )CreateProcessW, ( PBYTE )_CreateProcessW );*/
    MH_Initialize();

    MH_CreateHook(CreateProcessW, _CreateProcessW, reinterpret_cast<void**>(&original));

    MH_EnableHook(MH_ALL_HOOKS);
    while( !queue_unload )
        Sleep( 500 );

    MH_Uninitialize();
    MH_DisableHook(MH_ALL_HOOKS);
    //DetourRemove( ( PBYTE )original, ( PBYTE )_CreateProcessW );
    FreeLibraryAndExitThread( static_cast<HMODULE>(args), 0 );

    return 0;
}

bool __stdcall DllMain( HMODULE self, unsigned long reason, void *reserved ) {
	if( reason == 1 ) {
       if(const auto thread = CreateThread(0, 0, thr, self, 0, 0))
           CloseHandle(thread);
	}
	return true;
}