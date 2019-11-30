#include "Framework.h"

#include <array>

#include "Donya/Blend.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Resource.h"
#include "Donya/ScreenShake.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"

#include "Common.h"
#include "Music.h"

using namespace DirectX;

Framework::Framework() :
	pSceneMng( nullptr )
{}
Framework::~Framework() = default;

bool Framework::Init()
{
	LoadSounds();

	pSceneMng = std::make_unique<SceneMng>();

#if DEBUG_MODE
	pSceneMng->Init( Scene::Type::Game );
#else
	pSceneMng->Init( Scene::Type::Logo );
#endif // DEBUG_MODE

	return true;
}

void Framework::Uninit()
{
	pSceneMng->Uninit();
}

void Framework::Update( float elapsedTime/*Elapsed seconds from last frame*/ )
{
#if DEBUG_MODE

	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Keyboard::Trigger( 'C' ) )
		{
			char debugstopper = 0;
		}
		if ( Donya::Keyboard::Trigger( 'T' ) )
		{
			Donya::ToggleShowStateOfImGui();
		}
		if ( Donya::Keyboard::Trigger( 'H' ) )
		{
			Common::ToggleShowCollision();
		}
	}

	DebugShowInformation();

#endif // DEBUG_MODE

	pSceneMng->Update( elapsedTime );
}

void Framework::Draw( float elapsedTime/*Elapsed seconds from last frame*/ )
{
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA );

	pSceneMng->Draw( elapsedTime );
}

bool Framework::LoadSounds()
{
	using Donya::Sound::Load;
	using Music::ID;

	struct Bundle
	{
		ID id;
		std::string fileName;
		bool isEnableLoop;
	public:
		Bundle() : id(), fileName(), isEnableLoop( false ) {}
		Bundle( ID id, const char *fileName, bool isEnableLoop ) : id( id ), fileName( fileName ), isEnableLoop( isEnableLoop ) {}
		Bundle( ID id, const std::string &fileName, bool isEnableLoop ) : id( id ), fileName( fileName ), isEnableLoop( isEnableLoop ) {}
	};

	const std::array<Bundle, ID::MUSIC_COUNT> bandles =
	{
		{	// ID, FilePath, isEnableLoop
			{ ID::BGM_Title,			"./Data/Sounds/BGM/Title.wav",					true  },
			
			{ ID::ItemChoose,			"./Data/Sounds/SE/UI/ChooseItem.wav",			false },
			{ ID::ItemDecision,			"./Data/Sounds/SE/UI/DecisionItem.wav",			false },
		},
	};

	bool result = true, successed = true;

	for ( size_t i = 0; i < ID::MUSIC_COUNT; ++i )
	{
		result = Load( bandles[i].id, bandles[i].fileName.c_str(), bandles[i].isEnableLoop );
		if ( !result ) { successed = false; }
	}

	return successed;
}

#if USE_IMGUI
#include "Donya/Easing.h"
#endif // USE_IMGUI
void Framework::DebugShowInformation()
{
#if USE_IMGUI

	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"情報" ) )
		{
			ImGui::Text( "FPS[%f]", Donya::GetFPS() );

			int x = 0, y = 0;
			Donya::Mouse::Coordinate( &x, &y );
			ImGui::Text( "Mouse[X:%d][Y:%d]", x, y );
			ImGui::Text( "Wheel[%d]", Donya::Mouse::WheelRot() );

			int LB = 0, MB = 0, RB = 0;
			LB = Donya::Mouse::Press( Donya::Mouse::LEFT );
			MB = Donya::Mouse::Press( Donya::Mouse::MIDDLE );
			RB = Donya::Mouse::Press( Donya::Mouse::RIGHT );
			ImGui::Text( "LB : %d, MB : %d, RB : %d", LB, MB, RB );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"イージングデモ" ) )
		{
			using namespace Donya::Easing;

			static float time		= 0.0f;
			ImGui::SliderFloat( u8"時間", &time, 0.0f, 1.0f );
			ImGui::Text( "" );
			static Donya::Easing::Type type = Donya::Easing::Type::In;
			{
				int iType = scast<int>( type );

				std::string caption = "Type : ";
				if ( type == Donya::Easing::Type::In ) { caption += "In"; }
				if ( type == Donya::Easing::Type::Out ) { caption += "Out"; }
				if ( type == Donya::Easing::Type::InOut ) { caption += "InOut"; }

				ImGui::SliderInt( caption.c_str(), &iType, 0, 2 );

				type = scast<Donya::Easing::Type>( iType );
			}

			constexpr unsigned int SIZE = scast<unsigned int>( Kind::ENUM_TERMINATION );
			constexpr std::array<Kind, SIZE> KINDS
			{
				Kind::Linear,
				Kind::Back,
				Kind::Bounce,
				Kind::Circular,
				Kind::Cubic,
				Kind::Elastic,
				Kind::Exponential,
				Kind::Quadratic,
				Kind::Quartic,
				Kind::Quintic,
				Kind::Sinusoidal,
				Kind::Smooth,
				Kind::SoftBack,
				Kind::Step,
			};
			std::array<float, SIZE> RESULTS
			{
				Ease( KINDS[ 0],	type,	time ),
				Ease( KINDS[ 1],	type,	time ),
				Ease( KINDS[ 2],	type,	time ),
				Ease( KINDS[ 3],	type,	time ),
				Ease( KINDS[ 4],	type,	time ),
				Ease( KINDS[ 5],	type,	time ),
				Ease( KINDS[ 6],	type,	time ),
				Ease( KINDS[ 7],	type,	time ),
				Ease( KINDS[ 8],	type,	time ),
				Ease( KINDS[ 9],	type,	time ),
				Ease( KINDS[10],	type,	time ),
				Ease( KINDS[11],	type,	time ),
				Ease( KINDS[12],	type,	time ),
				Ease( KINDS[13],	type,	time ),
			};

			auto MakeCaption = [&]( Kind kind )->std::string
			{
				std::string rv{};
				switch ( kind )
				{
				case Donya::Easing::Kind::Linear:
					rv = "Linear"; break;
				case Donya::Easing::Kind::Back:
					rv = "Back"; break;
				case Donya::Easing::Kind::Bounce:
					rv = "Bounce"; break;
				case Donya::Easing::Kind::Circular:
					rv = "Circular"; break;
				case Donya::Easing::Kind::Cubic:
					rv = "Cubic"; break;
				case Donya::Easing::Kind::Elastic:
					rv = "Elastic"; break;
				case Donya::Easing::Kind::Exponential:
					rv = "Exponential"; break;
				case Donya::Easing::Kind::Quadratic:
					rv = "Quadratic"; break;
				case Donya::Easing::Kind::Quartic:
					rv = "Quartic"; break;
				case Donya::Easing::Kind::Quintic:
					rv = "Quintic"; break;
				case Donya::Easing::Kind::Smooth:
					rv = "Smooth"; break;
				case Donya::Easing::Kind::Sinusoidal:
					rv = "Sinusoidal"; break;
				case Donya::Easing::Kind::SoftBack:
					rv = "SoftBack"; break;
				case Donya::Easing::Kind::Step:
					rv = "Step"; break;
				default:
					rv = "Error Type"; break;
				}

				return rv;
			};

			for ( unsigned int i = 0; i < SIZE; ++i )
			{
				float result = RESULTS[i];
				std::string caption = MakeCaption( KINDS[i] );
				ImGui::SliderFloat( caption.c_str(), &result, 0.0f, 2.0f );
			}

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"画面シェイクデモ" ) )
		{
			static float power = 20.0f;
			static float decel = 5.0f;
			static float time = 1.0f;
			static float interval = 0.05f;
			static Donya::ScreenShake::Kind kind = Donya::ScreenShake::Kind::MOMENT;

			ImGui::Text( "now X : %f\n", Donya::ScreenShake::GetX() );
			ImGui::Text( "now Y : %f\n", Donya::ScreenShake::GetY() );

			ImGui::SliderFloat( "Power", &power, 6.0f, 128.0f );
			ImGui::SliderFloat( "Deceleration", &decel, 0.2f, 64.0f );
			ImGui::SliderFloat( "ShakeTime", &time, 0.1f, 10.0f );
			ImGui::SliderFloat( "Interval", &interval, 0.1f, 3.0f );
			if ( ImGui::Button( "Toggle the kind" ) )
			{
				kind = ( kind == Donya::ScreenShake::MOMENT )
					? Donya::ScreenShake::PERMANENCE
					: Donya::ScreenShake::MOMENT;
			}
			ImGui::Text( "Now Kind : %s", ( kind == Donya::ScreenShake::MOMENT ) ? "Moment" : "Permanence" );

			if ( ImGui::Button( "Activate Shake X" ) )
			{
				if ( Donya::Keyboard::Shifts() )
				{
					Donya::ScreenShake::SetX( kind, power );
				}
				else
				{
					Donya::ScreenShake::SetX( kind, power, decel, time, interval );
				}
			}
			if ( ImGui::Button( "Activate Shake Y" ) )
			{
				if ( Donya::Keyboard::Shifts() )
				{
					Donya::ScreenShake::SetY( kind, power );
				}
				else
				{
					Donya::ScreenShake::SetY( kind, power, decel, time, interval );
				}
			}
			if ( ImGui::Button( "Stop Shake" ) )
			{
				Donya::ScreenShake::StopX();
				Donya::ScreenShake::StopY();
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}

#endif // USE_IMGUI
}