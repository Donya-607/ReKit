#include "SceneGame.h"

#include <vector>

#include <cereal/types/vector.hpp>

#include "Donya/Camera.h"
#include "Donya/CBuffer.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"		// Use GetFPS().
#include "Donya/GeometricPrimitive.h"
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Quaternion.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"

using namespace DirectX;

#pragma region AlphaParam
class AlphaParam final : public Donya::Singleton<AlphaParam>
{
	friend Donya::Singleton<AlphaParam>;
public:
	struct Member
	{
	public:
		std::vector<BoxEx> debugTerrains{};
		std::vector<BoxEx> debugAllTerrains{};		// Use for collision and drawing.

		BoxEx debugCompressor{ { 0.0f, 0.0f, 1.5f, 1.5f, true }, 30 };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( debugTerrains )
			);
			if ( 1 <= version )
			{
				// CEREAL_NVP( x )
			}
		}
	};
private:
	static constexpr const char* SERIAL_ID = "AlphaStageBlocks";
	Member m;
private:
	AlphaParam() : m() {}
public:
	~AlphaParam() = default;
public:
	void Init()
	{
		LoadParameter();
	}
	void Uninit()
	{

	}
public:
	Member Data() const
	{
		return m;
	}
	Member& DataRef()
	{
		return m;
	}
private:
	void LoadParameter(bool fromBinary = true)
	{
		std::string filePath = GenerateSerializePath( SERIAL_ID, fromBinary );
		Donya::Serializer::Load( m, filePath.c_str(), SERIAL_ID, fromBinary );
	}

#if USE_IMGUI

	void SaveParameter()
	{
		bool useBinary = true;
		std::string filePath{};

		filePath = GenerateSerializePath( SERIAL_ID, useBinary );
		Donya::Serializer::Save( m, filePath.c_str(), SERIAL_ID, useBinary );

		useBinary = false;

		filePath = GenerateSerializePath( SERIAL_ID, useBinary );
		Donya::Serializer::Save( m, filePath.c_str(), SERIAL_ID, useBinary );
	}

public:
	void UseImGui()
	{
		m.debugCompressor.pos += m.debugCompressor.velocity;
		if ( ImGui::BeginIfAllowed() )
		{
			if ( ImGui::TreeNode( u8"地形エディタ" ) )
			{
				if ( ImGui::TreeNode( u8"パラメーター設定" ) )
				{
					bool pressCtrl = Donya::Keyboard::Press( VK_LCONTROL ) || Donya::Keyboard::Press( VK_RCONTROL );
					bool triggerDebugButton = Donya::Keyboard::Trigger( 'G' );
					if ( ( pressCtrl && triggerDebugButton ) || ImGui::Button( u8"ブロック生成" ) )
					{
						BoxEx changeable{ { 0.0f, 0.0f, 2.0f, 2.0f, true }, 1 };
						AlphaParam::Get().DataRef().debugTerrains.emplace_back( changeable );
					}
					ImGui::Text( "" );

					if ( ImGui::TreeNode( u8"設定" ) )
					{
						int index = 0;
						for ( auto itr = m.debugTerrains.begin(); itr != m.debugTerrains.end(); )
						{
							index++;

							std::string strPos	= u8"座標"   + std::to_string( index );
							std::string strSize	= u8"サイズ" + std::to_string( index );
							std::string strMass	= u8"質量"   + std::to_string( index );

							ImGui::DragFloat2( strPos.c_str(),  &itr->pos.x  );
							ImGui::DragFloat2( strSize.c_str(), &itr->size.x );
							ImGui::DragInt   ( strMass.c_str(), &itr->mass   );

							std::string strErase = u8"削除"  + std::to_string( index );

							if ( ImGui::Button( strErase.c_str() ) )
							{
								itr = m.debugTerrains.erase( itr );
							}
							else
							{
								itr++;
							}
							ImGui::Text( "" );
						}

						ImGui::TreePop();
					}

					if ( ImGui::TreeNode( u8"コンプレッサ" ) )
					{
						ImGui::DragFloat2( u8"座標",				&m.debugCompressor.pos.x				);
						ImGui::DragFloat2( u8"サイズ（半分）",	&m.debugCompressor.size.x				);
						ImGui::DragFloat2( u8"速度",				&m.debugCompressor.velocity.x,	0.001f	);
						ImGui::DragInt   ( u8"質量",				&m.debugCompressor.mass,		1.0f, 0	);

						ImGui::TreePop();
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"ファイル" ) )
				{
					static bool isBinary = true;
					if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true; }
					if ( ImGui::RadioButton( "JSON", !isBinary ) ) { isBinary = false; }
					std::string loadStr{ "読み込み " };
					loadStr += ( isBinary ) ? "Binary" : "JSON";

					if ( ImGui::Button( u8"保存" ) )
					{
						SaveParameter();
					}
					if ( ImGui::Button( Donya::MultiToUTF8( loadStr ).c_str() ) )
					{
						LoadParameter( isBinary );
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			ImGui::End();
		}
	}

#endif // USE_IMGUI
};

CEREAL_CLASS_VERSION( AlphaParam::Member, 0 )
#pragma endregion

SceneGame::SceneGame() :
	dirLight(), iCamera(),
	controller( Donya::Gamepad::PAD_1 ),
	player(), gimmicks(),
	pHook( nullptr )
{}
SceneGame::~SceneGame() = default;

void SceneGame::Init()
{
	Donya::Sound::Play( Music::BGM_Game );

	CameraInit();

	gimmicks.Init( NULL );
	AlphaParam::Get().Init();

	player.Init();
	Hook::Init();
}
void SceneGame::Uninit()
{
	Donya::Sound::Stop( Music::BGM_Game );

	gimmicks.Uninit();
	AlphaParam::Get().Uninit();

	player.Uninit();
	Hook::Uninit();
}

Scene::Result SceneGame::Update( float elapsedTime )
{
#if USE_IMGUI

	UseImGui();
	AlphaParam::Get().UseImGui();

#endif // USE_IMGUI

	auto ToBox = []( const AABBEx &aabb )
	{
		BoxEx box{};
		box.pos.x		= aabb.pos.x;
		box.pos.y		= aabb.pos.y;
		box.size.x		= aabb.size.x;
		box.size.y		= aabb.size.y;
		box.exist		= aabb.exist;
		box.velocity.x	= aabb.velocity.x;
		box.velocity.y	= aabb.velocity.y;
		box.mass		= aabb.mass;
		return box;
	};
	auto AppendGimmicksBox = [&ToBox]( std::vector<BoxEx> *pTerrains, const Gimmick &gimmicks )
	{
		const auto boxes = gimmicks.RequireHitBoxes();
		for ( const auto &it : boxes )
		{
			pTerrains->emplace_back( ToBox( it ) );
		}
	};

	controller.Update();

	/*
	Update-order memo:
	1.	Prepare and reset "debugAllTerrains" with "debugTerrains". This way prevent continuously emplace_back to "debugAllTerrains" every frame. You do not add or remove the "debugTerrains".
	2.	Update only a velocity(a position does not update) of all objects.
	3.	Update a position(PhysicUpdate) of the hook with terrains.
	4.	Update a position(PhysicUpdate) of the gimmicks with the player's hit-box(and hook's hit-box if existed) that contain calculated velocity. That hit-box of the player is not latest, but I want to update the gimmicks before the update of the player.
	5.	Register the updated hit-boxes of the gimmicks to "debugAllTerrains".
	6.	Update a position(PhysicUpdate) of the player with updated "debugAllTerrains".
	*/

	auto &refStage = AlphaParam::Get().DataRef();
	// 1. Reset the registered hit-boxes in "debugAllTerrains".
	{
		refStage.debugAllTerrains.clear();
		refStage.debugAllTerrains.emplace_back( refStage.debugCompressor );
		refStage.debugAllTerrains.insert( refStage.debugAllTerrains.end(), refStage.debugTerrains.begin(), refStage.debugTerrains.end() );
	}

	// 2. Update velocity of all objects.
	gimmicks.Update( elapsedTime );
	PlayerUpdate( elapsedTime ); // This update does not call the PhysicUpdate().
	HookUpdate( elapsedTime ); // This update does not call the PhysicUpdate().

	// 3. The hook's PhysicUpdate().
	if ( pHook )
	{
		std::vector<BoxEx>   terrainsForHook = refStage.debugAllTerrains;
		AppendGimmicksBox(  &terrainsForHook, gimmicks );

		pHook->PhysicUpdate( terrainsForHook, player.GetPosition() );
	}

	// 4. The gimmicks PhysicUpdate().
	{
		AABBEx wsPlayerAABB = player.GetHitBox();

		std::vector<BoxEx> terrainsForGimmicks = refStage.debugAllTerrains;
		terrainsForGimmicks.emplace_back( ToBox( wsPlayerAABB ) );

		if ( pHook )
		{
			terrainsForGimmicks.emplace_back( ToBox( pHook->GetHitBox() ) );
		}

		gimmicks.PhysicUpdate( terrainsForGimmicks );
	}

	// 5. Add the gimmicks block.
	AppendGimmicksBox( &refStage.debugAllTerrains, gimmicks );
	
	// 6. The player's PhysicUpdate().
	player.PhysicUpdate( AlphaParam::Get().DataRef().debugAllTerrains );

	CameraUpdate();

#if DEBUG_MODE
	// Scene Transition Demo.
	{
		bool pressCtrl = Donya::Keyboard::Press( VK_LCONTROL ) || Donya::Keyboard::Press( VK_RCONTROL );
		bool triggerDebugButton = ( Donya::Keyboard::Trigger( VK_RETURN ) || controller.Trigger( Donya::Gamepad::Button::A ) || controller.Trigger( Donya::Gamepad::Button::START ) );
		if ( pressCtrl && triggerDebugButton )
		{
			if ( !Fader::Get().IsExist() )
			{
				StartFade();
			}
		}
	}
#endif // DEBUG_MODE

	return ReturnResult();
}

void SceneGame::Draw( float elapsedTime )
{
	{
		constexpr FLOAT BG_COLOR[4]{ 0.4f, 0.4f, 0.4f, 1.0f };
		Donya::ClearViews( BG_COLOR );
	}

	const Donya::Vector4x4 V = iCamera.CalcViewMatrix();
	const Donya::Vector4x4 P = iCamera.GetProjectionMatrix();
	const Donya::Vector4 cameraPos{ iCamera.GetPosition(), 1.0f };

	gimmicks.Draw( V, P, dirLight.dir );

	player.Draw( V * P, dirLight.dir, dirLight.color );
	if (pHook)
	{
		pHook->Draw(V * P, dirLight.dir, dirLight.color);
	}

// #if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		// Drawing Test Terrains that use to player's collision.
		{
			static auto cube = Donya::Geometric::CreateCube();

			constexpr Donya::Vector4 cubeColor{ 1.0f, 0.8f, 0.0f, 0.6f };
			Donya::Vector4x4 cubeT{};
			Donya::Vector4x4 cubeS{};
			Donya::Vector4x4 cubeW{};
			for ( const auto &it : AlphaParam::Get().Data().debugAllTerrains )
			{
				// The drawing size is whole size.
				// But a collision class's size is half size.
				// So we should to double it size.

				cubeT = Donya::Vector4x4::MakeTranslation( Donya::Vector3{ it.pos, 0.0f } );
				cubeS = Donya::Vector4x4::MakeScaling( Donya::Vector3{ it.size * 2.0f, 1.0f } );
				cubeW = cubeS * cubeT;

				cube.Render
				(
					nullptr,
					/* useDefaultShading	= */ true,
					/* isEnableFill			= */ true,
					( cubeW * V * P ), cubeW,
					dirLight.dir,
					cubeColor
				);
			}
		}

	#if DEBUG_MODE
		// Drawing TextureBoard Demo.
		{
			constexpr const wchar_t *texturePath	= L"./Data/Images/Rights/FMOD Logo White - Black Background.png";
			static Donya::Geometric::TextureBoard	texBoard = Donya::Geometric::CreateTextureBoard( texturePath );
			static Donya::Vector2	texPos{};
			static Donya::Vector2	texSize{ 728.0f, 192.0f };

			static Donya::Vector3	boardScale{ 1.0f, 1.0f, 1.0f };
			static Donya::Vector3	boardPos{};
			static float			boardRadian{};

			static Donya::Vector4	boardColor{ 1.0f, 1.0f, 1.0f, 1.0f };
			static Donya::Vector4	lightDir  { 0.0f,-1.0f, 1.0f, 0.0f };

		#if USE_IMGUI

			if ( ImGui::BeginIfAllowed() )
			{
				if ( ImGui::TreeNode( u8"板ポリ描画テスト" ) )
				{
					ImGui::DragFloat2( u8"切り取り位置・左上", &texPos.x );
					ImGui::DragFloat2( u8"切り取りサイズ・全体", &texSize.x );
					ImGui::Text( "" );
					ImGui::DragFloat3( u8"スケール", &boardScale.x );
					ImGui::DragFloat3( u8"ワールド位置", &boardPos.x );
					ImGui::DragFloat( u8"Z回転", &boardRadian, ToRadian( 10.0f ) );
					ImGui::Text( "" );
					ImGui::ColorEdit4( u8"ブレンド色", &boardColor.x );
					ImGui::SliderFloat3( u8"板ポリのライト方向", &lightDir.x, -1.0f, 1.0f );
					ImGui::Text( "" );

					ImGui::TreePop();
				}

				ImGui::End();
			}

		#endif // USE_IMGUI

			Donya::Vector4x4 TB_S = Donya::Vector4x4::MakeScaling( boardScale );
			Donya::Vector4x4 TB_R = texBoard.CalcBillboardRotation( ( iCamera.GetPosition() - boardPos ).Normalized(), boardRadian );
			Donya::Vector4x4 TB_T = Donya::Vector4x4::MakeTranslation( boardPos );
			Donya::Vector4x4 TB_W = TB_S * TB_R * TB_T;

			texBoard.RenderPart
			(
				texPos, texSize,
				nullptr, // Specify use library's device-context.
				/* useDefaultShading = */ true,
				/* isEnableFill      = */ true,
				( TB_W * V * P ), TB_W,
				lightDir, boardColor
			);
		}
	#endif // DEBUG_MODE
	}
// #endif // DEBUG_MODE
}

void SceneGame::CameraInit()
{
	iCamera.Init( Donya::ICamera::Mode::Look );
	iCamera.SetZRange( 0.1f, 1000.0f );
	iCamera.SetFOV( ToRadian( 30.0f ) );
	iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
	iCamera.SetPosition( { 0.0f, 0.0f, -64.0f } );
	iCamera.SetFocusPoint( { 0.0f, 0.0f, 0.0f } );
	iCamera.SetProjectionPerspective();

	// I can setting a configuration,
	// but current data is not changed by this immediately.
	// So update here.

	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;
	iCamera.Update( moveInitPoint );
}
void SceneGame::CameraUpdate()
{
	auto MakeControlStructWithMouse = []()
	{
		constexpr float SLERP_FACTOR = 0.2f;

		auto NoOperation = [&SLERP_FACTOR]()
		{
			Donya::ICamera::Controller noop{};
			noop.SetNoOperation();
			noop.slerpPercent = SLERP_FACTOR;
			return noop;
		};

		if ( !Donya::Keyboard::Press( VK_MENU ) ) { return NoOperation(); }
		// else

		static Donya::Int2 prevMouse{};
		static Donya::Int2 currMouse{};

		prevMouse = currMouse;

		auto nowMouse = Donya::Mouse::Coordinate();
		currMouse.x = scast<int>( nowMouse.x );
		currMouse.y = scast<int>( nowMouse.y );

		bool isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
		bool isDriveMouse = ( prevMouse != currMouse ) || Donya::Mouse::WheelRot() || isInputMouseButton;
		if ( !isDriveMouse ) { return NoOperation(); }
		// else

		Donya::Vector3 diff{};
		{
			Donya::Vector2 vec2 = ( currMouse - prevMouse ).Float();

			diff.x = vec2.x;
			diff.y = vec2.y;
		}

		Donya::Vector3 movement{};
		Donya::Vector3 rotation{};

		if ( Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) )
		{
			constexpr float ROT_AMOUNT = ToRadian( 1.0f );
			rotation.x = diff.x * ROT_AMOUNT;
			rotation.y = diff.y * ROT_AMOUNT;
		}
		else
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
		{
			constexpr float MOVE_SPEED = 0.1f;
			movement.x = diff.x * MOVE_SPEED;
			movement.y = -diff.y * MOVE_SPEED;
		}

		constexpr float FRONT_SPEED = 3.5f;
		movement.z = FRONT_SPEED * scast<float>( Donya::Mouse::WheelRot() );

		Donya::ICamera::Controller ctrl{};
		ctrl.moveVelocity		= movement;
		ctrl.yaw				= rotation.x;
		ctrl.pitch				= rotation.y;
		ctrl.roll				= 0.0f;
		ctrl.slerpPercent		= SLERP_FACTOR;
		ctrl.moveInLocalSpace	= true;

		return ctrl;
	};
	auto input = MakeControlStructWithMouse();
	input.moveVelocity.x *= -1.0f;
	input.moveVelocity.y *= -1.0f;
	iCamera.Update( input );

#if DEBUG_MODE
	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			iCamera.SetPosition( { 0.0f, 0.0f, -5.0f } );
		}
	}
#endif // DEBUG_MODE
}

void SceneGame::PlayerUpdate( float elapsedTime )
{
	Player::Input input{};

	bool moveLeft	= false;
	bool moveRight	= false;
	bool useJump	= false;

	if ( controller.IsConnected() )
	{
		using Pad  = Donya::Gamepad;

		bool left  = controller.Press( Pad::LEFT  ) || controller.PressStick( Pad::StickDirection::LEFT,  /* leftStick = */ true );
		bool right = controller.Press( Pad::RIGHT ) || controller.PressStick( Pad::StickDirection::RIGHT, /* leftStick = */ true );

		if ( left  ) { moveLeft  = true; }
		if ( right ) { moveRight = true; }

		if ( controller.Trigger( Pad::A  ) ) { useJump = true; }
	}
	else
	{
		bool pressLeft  = Donya::Keyboard::Press( 'A' )/* || Donya::Keyboard::Press( VK_LEFT  )*/;
		bool pressRight = Donya::Keyboard::Press( 'D' )/* || Donya::Keyboard::Press( VK_RIGHT )*/;
		if ( pressLeft  )		{ moveLeft  = true; }
		if ( pressRight )		{ moveRight = true; }
		
		bool trgJump    = Donya::Keyboard::Trigger( 'S' )/* || Donya::Keyboard::Trigger( VK_LSHIFT )*/;
		if ( trgJump )			{ useJump = true; }
	}

	if ( moveLeft  ) { input.moveVelocity.x -= 1.0f; }
	if ( moveRight ) { input.moveVelocity.x += 1.0f; }
	if ( useJump   ) { input.useJump = true; }

	player.Update( elapsedTime, input );
}

void SceneGame::HookUpdate(float elapsedTime)
{
#if USE_IMGUI
	Hook::UseImGui();
#endif // USE_IMGUI

	Hook::Input input{};

	Donya::Vector2		stick{};
	bool				useAction	= false;
	bool				trigger		= false;
	bool				erase		= false; // A User can erase the hook arbitally.

	if ( controller.IsConnected() )
	{
		using Pad = Donya::Gamepad;

		stick = controller.RightStick();

		if ( controller.Press  ( Pad::RT ) ) { useAction	= true; }
		if ( controller.Trigger( Pad::RT ) ) { trigger		= true; }
		if ( controller.Trigger( Pad::LT ) ) { erase		= true; }
	}
	else
	{
		// OLD
		/*
		POINT mousePoint = Donya::Mouse::Coordinate();
		stick.x = scast<float>(mousePoint.x) - player.GetPosition().x;
		stick.y = scast<float>(mousePoint.y) - player.GetPosition().y;

		if (Donya::Mouse::Press(Donya::Mouse::LEFT) && !Donya::Keyboard::Press(VK_SPACE))
		{
			useAction = true;
		}
		if (Donya::Mouse::Trigger(Donya::Mouse::LEFT) && !Donya::Keyboard::Press(VK_SPACE))
		{
			trigger = true;
		}
		*/

		if ( Donya::Keyboard::Press  ( VK_LEFT   ) ) { stick.x		-= 1.0f; }
		if ( Donya::Keyboard::Press  ( VK_RIGHT  ) ) { stick.x		+= 1.0f; }
		if ( Donya::Keyboard::Press  ( VK_UP     ) ) { stick.y		+= 1.0f; }
		if ( Donya::Keyboard::Press  ( VK_DOWN   ) ) { stick.y		-= 1.0f; }

		if ( Donya::Keyboard::Press  ( VK_RSHIFT ) ) { useAction	= true; }
		if ( Donya::Keyboard::Trigger( VK_RSHIFT ) ) { trigger		= true; }
		if ( Donya::Keyboard::Trigger( VK_END    ) ) { erase		= true; }
	}

	if ( trigger )
	{
		if( !pHook ) 
		{ 
			pHook = std::make_unique<Hook>( player.GetPosition() );
			Donya::Sound::Play( Music::Throw );
		}
	}
	if ( erase )
	{
		pHook.reset();
	}

	if ( !pHook ) { return; }
	// else

	if ( !pHook->IsExist() )
	{
		pHook.reset();
		return;
	}
	// else

	input.playerPos = player.GetPosition();
	input.currPress = useAction;
	input.stickVec  = stick.Normalized();

	pHook->Update(elapsedTime, input);
}

void SceneGame::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneGame::ReturnResult()
{
	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Clear;
		return change;
	}

	bool requestPause	= controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT ) || Donya::Keyboard::Trigger( 'P' );
	bool allowPause		= !Fader::Get().IsExist();
	if ( requestPause && allowPause )
	{
		Donya::Sound::Play( Music::ItemDecision );

		Scene::Result pause{};
		pause.AddRequest( Scene::Request::ADD_SCENE );
		pause.sceneType = Scene::Type::Pause;
		return pause;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI

void SceneGame::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"ゲーム・設定" ) )
		{
			ImGui::Text( u8"ポーズ画面へ : <Press P>" );
			ImGui::Text( u8"クリア画面へ : <Press Ctrl + Enter>" );

			ImGui::SliderFloat3( u8"方向性ライト・向き", &dirLight.dir.x, -1.0f, 1.0f );
			ImGui::ColorEdit4( u8"方向性ライト・カラー", &dirLight.color.x );

			ImGui::TreePop();
		}
		
		ImGui::End();
	}
}

#endif // USE_IMGUI
