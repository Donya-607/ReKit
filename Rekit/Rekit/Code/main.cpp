#include <locale.h>
#include <string>
#include <time.h>
#include <windows.h>

#include "Donya/Constant.h"	// Use DEBUG_MODE, scast macros.
#include "Donya/Donya.h"

#include "Common.h"
#include "Framework.h"
#include "Icon.h"

INT WINAPI wWinMain( _In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ LPWSTR cmdLine, _In_ INT cmdShow )
{
#if DEBUG_MODE
	// reference:https://docs.microsoft.com/ja-jp/visualstudio/debugger/crt-debug-heap-details?view=vs-2015
	_CrtSetDbgFlag
	(
		_CRTDBG_ALLOC_MEM_DF
		| _CRTDBG_LEAK_CHECK_DF
		| _CRTDBG_CHECK_ALWAYS_DF
	);
	// When memory leak detected, if you assign the output number to "_crtBreakAlloc",
	// program will be stop in that memory allocate place. ex : _crtBreakAlloc = 123;
	// _crtBreakAlloc = ;
#endif // DEBUG_MODE

	setlocale( LC_ALL, "JPN" );

	srand( scast<unsigned int>( time( NULL ) ) );

#if DEBUG_MODE
	constexpr bool fullScreenMode = false;
#else
	// constexpr bool fullScreenMode = true;
	constexpr bool fullScreenMode = false;
#endif // DEBUG_MODE

	std::string title{ "エクスプローラー" };
	Donya::Init( cmdShow, Common::ScreenWidth(), Common::ScreenHeight(), title.c_str(), fullScreenMode );

	Donya::SetWindowIcon( instance, IDI_ICON );

	Framework framework{};
	framework.Init();

	while ( Donya::MessageLoop() )
	{
		Donya::ClearViews();

		Donya::SystemUpdate();
		framework.Update( Donya::GetElapsedTime() );

		framework.Draw( Donya::GetElapsedTime() );
		Donya::Present( 1 );
	}

	framework.Uninit();

	auto   returnValue = Donya::Uninit();
	return returnValue;
}