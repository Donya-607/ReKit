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
#include "SceneEditor.h"	// Use StageConfiguration.

#undef max
#undef min

using namespace DirectX;

class GameParam final : public Donya::Singleton<GameParam>
{
	friend Donya::Singleton<GameParam>;
public:
	struct Member
	{
	public:
		Donya::Vector4		lightColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		Donya::Vector4		lightDirection{ 0.0f, -1.0f, 1.0f, 0.0f };

		float				cameraDolly{ -10.0f };		// Use for z-position of camera.
		float				cameraSlerp{ 0.2f };		// Use for the interpolation of camera move.
		Donya::Vector2		roomSize{ 10.0f, 10.0f };	// Whole-size. Use for check to "is the player on outside place?"
		Donya::Int2			roomCounts{ 4, 5 };			// 1-based. Represent the row and column count of neighboring rooms.
		
		Donya::Vector3		initPlayerPos{};

		BoxEx debugClearTrigger{ { -10.0f, -10.0f, 6.0f, 6.0f, true }, 0 }; // Does not serialize.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( lightColor ),
				CEREAL_NVP( lightDirection ),
				CEREAL_NVP( cameraDolly ),
				CEREAL_NVP( roomSize ),
				CEREAL_NVP( roomCounts ),
				CEREAL_NVP( initPlayerPos )
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( cameraSlerp ) );
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "GameConfig";
	Member m;
private:
	GameParam() : m() {}
public:
	~GameParam() = default;
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
private:
	void LoadParameter( bool fromBinary = true )
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
		if ( ImGui::BeginIfAllowed() )
		{
			if ( ImGui::TreeNode( u8"ゲーム・設定" ) )
			{
				if ( ImGui::TreeNode( u8"ライト" ) )
				{
					ImGui::SliderFloat3( u8"方向性ライト・向き", &m.lightDirection.x, -1.0f, 1.0f );
					ImGui::ColorEdit4( u8"方向性ライト・カラー", &m.lightColor.x );

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"部屋設定" ) )
				{
					ImGui::DragFloat( u8"カメラのＺ位置",			&m.cameraDolly,		1.0f );
					ImGui::SliderFloat( u8"カメラの補間速度",		&m.cameraSlerp,		0.01f, 1.0f );
					ImGui::DragFloat2( u8"ルームサイズ（全体）",	&m.roomSize.x,		0.1f, 0.1f );
					ImGui::SliderInt2( u8"ルームの数（縦横）",	&m.roomCounts.x,	1, 16 );
					ImGui::Text( "" );
					ImGui::DragFloat3( u8"自機の初期位置",		&m.initPlayerPos.x );

					if ( ImGui::TreeNode( u8"デバッグ用・クリア判定エリア" ) )
					{
						ImGui::DragFloat2( u8"座標", &m.debugClearTrigger.pos.x );
						ImGui::DragFloat2( u8"サイズ（半分）", &m.debugClearTrigger.size.x );
						ImGui::Checkbox( u8"当たり判定をつける", &m.debugClearTrigger.exist );
						m.debugClearTrigger.velocity = 0.0f;
						m.debugClearTrigger.mass = 0;
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

CEREAL_CLASS_VERSION( GameParam::Member, 1 )

SceneGame::SceneGame() :
	stageCount( -1 ), currentStageNo( 0 ),
	iCamera(),
	controller( Donya::Gamepad::PAD_1 ),
	roomOriginPos(),
	bg(), player(), pHook( nullptr ),
	terrains(), gimmicks(),
	useCushion( true )
{}
SceneGame::~SceneGame() = default;

void SceneGame::Init()
{
	Donya::Sound::Play( Music::BGM_Game );

	BG::ParameterInit();

	Terrain::LoadModel();

	Gimmick::LoadModels();
	Gimmick::InitParameters();

	GameParam::Get().Init();

	LoadAllStages();
	currentStageNo = 0;

	CameraInit();

	player.Init( GameParam::Get().Data().initPlayerPos );
	Hook::Init();
	bg.Init ();
}
void SceneGame::Uninit()
{
	Donya::Sound::Stop( Music::BGM_Game );

	GameParam::Get().Uninit();

	bg.Uninit();
	player.Uninit();
	Hook::Uninit();

	for ( auto &it : gimmicks )
	{
		it.Uninit();
	}
}

Scene::Result SceneGame::Update( float elapsedTime )
{
	if ( useCushion )
	{
		useCushion = false;
		CameraUpdate();
		return Scene::Result{ Scene::Request::NONE, Scene::Type::Null };
	}
	// else

#if USE_IMGUI

	UseImGui();
	GameParam::Get().UseImGui();
	BG::UseParameterImGui();

#endif // USE_IMGUI

	auto AppendGimmicksBox	= []( std::vector<BoxEx> *pTerrains, const Gimmick &gimmicks )
	{
		const auto boxes = gimmicks.RequireHitBoxes();
		for ( const auto &it : boxes )
		{
			pTerrains->emplace_back( it.Get2D() );
		}
	};
	auto ExtractHitBoxes	= []( const Gimmick &gimmicks )
	{
		const auto boxes = gimmicks.RequireHitBoxes();
		const size_t boxCount = boxes.size();

		std::vector<BoxEx> hitBoxes{ boxCount };
		for ( size_t i = 0; i < boxCount; ++i )
		{
			hitBoxes[i] = boxes[i].Get2D();
		}

		return hitBoxes;
	};

	controller.Update();

	bg.Update (elapsedTime);

	/*
	Update-order memo:
	1.	Prepare and reset "debugAllTerrains" with "debugTerrains". This way prevent continuously emplace_back to "debugAllTerrains" every frame. You do not add or remove the "debugTerrains".
	2.	Update only a velocity(a position does not update) of all objects.
	3.	Update a position(PhysicUpdate) of the hook with terrains.
	4.	Update a position(PhysicUpdate) of the gimmicks with the player's hit-box(and hook's hit-box if existed) that contain calculated velocity. That hit-box of the player is not latest, but I want to update the gimmicks before the update of the player.
	5.	Register the updated hit-boxes of the gimmicks to "debugAllTerrains".
	6.	Update a position(PhysicUpdate) of the player with updated "debugAllTerrains".
	*/

	auto &refTerrain = terrains[currentStageNo];
	auto &refGimmick = gimmicks[currentStageNo];
	
	// 1. Reset the registered hit-boxes in "debugAllTerrains".
	refTerrain.Reset();

	// 2. Update velocity of all objects.
	refGimmick.Update( elapsedTime );
	PlayerUpdate( elapsedTime ); // This update does not call the PhysicUpdate().
	HookUpdate( elapsedTime ); // This update does not call the PhysicUpdate().

	// 3. The hook's PhysicUpdate().
	if ( pHook )
	{
		std::vector<BoxEx>   terrainsForHook = refTerrain.Acquire();
		AppendGimmicksBox(  &terrainsForHook,  refGimmick );

		pHook->PhysicUpdate( terrainsForHook,  player.GetPosition() );
	}

	// 4. The gimmicks PhysicUpdate().
	{
		AABBEx wsPlayerAABB = player.GetHitBox();

		BoxEx accompanyBox{};
		if ( pHook )
		{
			accompanyBox = pHook->GetHitBox().Get2D();
		}
		else
		{
			accompanyBox.exist = false;
		}

		refGimmick.PhysicUpdate( wsPlayerAABB.Get2D(), accompanyBox, refTerrain.Acquire() );
	}

	// 5. Add the gimmicks block.
	refTerrain.Append( ExtractHitBoxes( refGimmick ) );
	
	// 6. The player's PhysicUpdate().
	player.PhysicUpdate( refTerrain.Acquire() );
	if ( player.IsDead() )
	{
		if ( !Fader::Get().IsExist() )
		{
			StartFade();
		}
	}
	else if ( IsPlayerOutFromRoom() )
	{
		UpdateCurrentStage();
	}

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

	if ( DetectClearMoment() )
	{
		StartFade();
	}

	return ReturnResult();
}

void SceneGame::Draw( float elapsedTime )
{
	{
		constexpr FLOAT BG_COLOR[4]{ 0.4f, 0.4f, 0.4f, 1.0f };
		Donya::ClearViews( BG_COLOR );
	}

	{
		const float prevDepth = Donya::Sprite::GetDrawDepth();
		Donya::Sprite::SetDrawDepth( 1.0f );
		bg.Draw();
		Donya::Sprite::SetDrawDepth( prevDepth );
	}

	const Donya::Vector4x4	V = iCamera.CalcViewMatrix();
	const Donya::Vector4x4	P = iCamera.GetProjectionMatrix();
	const Donya::Vector4	cameraPos{ iCamera.GetPosition(), 1.0f };
	const Donya::Vector4	lightDir	= GameParam::Get().Data().lightDirection;
	const Donya::Vector4	lightColor	= GameParam::Get().Data().lightColor;

	gimmicks[currentStageNo].Draw( V, P, lightDir );

	player.Draw( V * P, lightDir, lightColor );
	if ( pHook )
	{
		pHook->Draw(V * P, lightDir, lightColor );
	}

#if DEBUG_MODE
	// Drawing the line that represent the room size.
	{
		static Donya::Geometric::Line line{ 32U };
		static bool initialized = false;
		if ( !initialized )
		{
			line.Init();
			initialized = true;
		}

		const auto param = GameParam::Get().Data();
		const Donya::Vector2 roomHalfSize = param.roomSize * 0.5f;
		const Donya::Vector3 center{ roomOriginPos.x, -roomOriginPos.y, player.GetPosition().z }; // The Y is should convert to world space from screen space.
		const Donya::Vector3 side{ roomHalfSize.x, 0.0f, 0.0f };
		const Donya::Vector3 vert{ 0.0f, roomHalfSize.y, 0.0f };

		constexpr Donya::Vector4 color{ 1.0f, 0.0f, 0.0f, 1.0f };
		line.Reserve( center - side, center + side, color );
		line.Reserve( center - vert, center + vert, color );

		line.Flush( V * P );
	}
#endif // DEBUG_MODE

	terrains[currentStageNo].Draw( V * P, lightDir );

// #if DEBUG_MODE
	// if ( Common::IsShowCollision() )
	{
		static auto cube = Donya::Geometric::CreateCube();

		// Drawing area of clear-trigger.
		{
			constexpr Donya::Vector4 cubeColor{ 1.0f, 1.0f, 1.0f, 1.0f };
			const auto box = GameParam::Get().Data().debugClearTrigger;
			Donya::Vector4x4 cubeT = Donya::Vector4x4::MakeTranslation( Donya::Vector3{ box.pos, 1.0f } );
			Donya::Vector4x4 cubeS = Donya::Vector4x4::MakeScaling( Donya::Vector3{ box.size * 2.0f, 1.0f } );
			Donya::Vector4x4 cubeW = cubeS * cubeT;

			cube.Render
			(
				nullptr,
				/* useDefaultShading	= */ true,
				/* isEnableFill			= */ true,
				( cubeW * V * P ), cubeW,
				lightDir,
				cubeColor
			);
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

			//texBoard.RenderPart
			//(
			//	texPos, texSize,
			//	nullptr, // Specify use library's device-context.
			//	/* useDefaultShading = */ true,
			//	/* isEnableFill      = */ true,
			//	( TB_W * V * P ), TB_W,
			//	lightDir, boardColor
			//);
		}
	#endif // DEBUG_MODE
	}

	{
		constexpr const wchar_t* texturePathOfGear = L"./Data/Images/title_gear.png";
		constexpr const wchar_t* texturePathOfTitle = L"./Data/Images/title_text.png";
		std::wstring pass = L"./Data/Images/title_text.png";
		std::wstring passGear = L"./Data/Images/title_gear.png";

		static Donya::Geometric::TextureBoard	gear = Donya::Geometric::CreateTextureBoard(texturePathOfGear);
		static Donya::Geometric::TextureBoard	title = Donya::Geometric::CreateTextureBoard(texturePathOfTitle);
		size_t textureIndex = Donya::Sprite::Load(pass);
		size_t texIndexGear = Donya::Sprite::Load(passGear);

		static Donya::Vector2	texPos{};
		static Donya::Vector2	texSize{ 1280.0f, 320.0f };

		static Donya::Vector3	boardScale{ 10.0f, 10.0f, 10.0f };
		static Donya::Vector3	boardPos{};
		static float			boardRadian{};

		static Donya::Vector4	boardColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		static Donya::Vector4	lightDir{ 0.0f,-1.0f, 1.0f, 0.0f };

		Donya::Vector4x4 TB_S = Donya::Vector4x4::MakeScaling(boardScale);
		Donya::Vector4x4 TB_R = title.CalcBillboardRotation((iCamera.GetPosition() - boardPos).Normalized(), boardRadian);
		Donya::Vector4x4 TB_T = Donya::Vector4x4::MakeTranslation(boardPos);
		Donya::Vector4x4 TB_W = TB_S * TB_T;

		{
			Donya::Sprite::SetDrawDepth(0.0f);
			static int animTime = 0;
			static int animFrame = 0;
			if (++animTime % 6 == 0)
			{
				animTime = 0;
				if (++animFrame >= 5)
					animFrame = 0;
			}
			Donya::Sprite::DrawPart(textureIndex, 1000, 500, 0, 320 * animFrame, 1280.0f, 320.0f);
		}
		{
			Donya::Sprite::SetDrawDepth(0.1f);
			static int animTime = 0;
			static int animFrame = 0;
			if (++animTime % 30 == 0)
			{
				animTime = 0;
				if (++animFrame >= 3)
					animFrame = 0;
			}
			Donya::Sprite::DrawPart(texIndexGear, 1150, 350, 0, 480 * animFrame, 480.0f, 480.0f);
			Donya::Sprite::DrawPart(texIndexGear, 820, 630, 0, 480 * (2 - animFrame), 480.0f, 480.0f);
		}


		//)itle.RenderPart
		//)
		//)	texPos, texSize,
		//)	nullptr, // Specify use library's device-context.
		//)	/* useDefaultShading = */ true,
		//)	/* isEnableFill      = */ true,
		//)	( TB_W * V * P ), TB_W,
		//)	lightDir, boardColor
		//);

	}
// #endif // DEBUG_MODE
}

void SceneGame::LoadAllStages()
{
	auto MakeIdentifier = []( int stageNo )->std::string
	{
		return std::string{ StageConfiguration::FILE_NAME + std::to_string( stageNo ) };
	};
	auto MakeFilePath = [&MakeIdentifier]( int stageNo )->std::string
	{
		return GenerateSerializePath
		(
			MakeIdentifier( stageNo ),
			/* useBinaryExtension = */ true
		);
	};
	auto LoadGimmicksFile = []( const std::string &filePath, const std::string &identifier )->StageConfiguration
	{
		StageConfiguration stage{};
		Donya::Serializer  seria{};

		bool succeeded = Donya::Serializer::Load
		(
			stage, filePath.c_str(),
			identifier.c_str(),
			/* fromBinary = */ true
		);
		if ( !succeeded )
		{
			_ASSERT_EXPR( 0, L"Failed : Load a gimmicks file." );
		}

		return stage;
	};

	terrains.clear();
	gimmicks.clear();

	int stageNo = 0; // 0-based.
	std::string filePath = MakeFilePath( stageNo );
	StageConfiguration config{};

	const Donya::Vector2 roomSize = GameParam::Get().Data().roomSize;
	Donya::Int2    roomIndex{};
	Donya::Vector3 roomOrigin{};

	while ( Donya::IsExistFile( filePath ) )
	{
		config = LoadGimmicksFile( filePath, MakeIdentifier( stageNo ) );

		roomIndex    = CalcRoomIndex( stageNo );
		roomOrigin.x = roomSize.x *  roomIndex.x;
		roomOrigin.y = roomSize.y * -roomIndex.y; // Convert Y from screen space -> world space.
		roomOrigin.z = 0.0f;
		
		terrains.push_back( {} );
		terrains[stageNo].Init( roomOrigin, config.editBlocks );

		gimmicks.push_back( {} );
		gimmicks[stageNo].Init // == gimmicks.back()
		(
			stageNo,
			config
		);

		stageNo++;
		filePath = MakeFilePath( stageNo );
	}

	stageCount = stageNo;
}

Donya::Int2 SceneGame::CalcRoomIndex( int stageNo ) const
{
	const auto param = GameParam::Get().Data();
	const int roomCount = param.roomCounts.x * param.roomCounts.y;
	_ASSERT_EXPR( 0 <= stageNo && stageNo < roomCount, L"Error : Passed stage-number without stage-count! " );

	Donya::Int2 index{};
	index.x = stageNo % param.roomCounts.x;
	index.y = ( !param.roomCounts.x ) ? 0 : stageNo / param.roomCounts.x;
	index.x = std::min( param.roomCounts.x - 1, index.x );
	index.y = std::min( param.roomCounts.y - 1, index.y );

	return index;
}

void SceneGame::CameraInit()
{
	iCamera.Init( Donya::ICamera::Mode::Look );
	iCamera.SetZRange( 0.1f, 1000.0f );
	iCamera.SetFOV( ToRadian( 30.0f ) );
	iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
	iCamera.SetPosition( Donya::Vector3{ 0.0f, 0.0f, GameParam::Get().Data().cameraDolly } );
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
	// Adjust position to origin of current room.
	{
		const auto param = GameParam::Get().Data();
		const Donya::Int2 roomIndex = CalcRoomIndex( currentStageNo );

		Donya::Vector3 currentPos{};
		currentPos.x = param.roomSize.x *  roomIndex.x;
		currentPos.y = param.roomSize.y * -roomIndex.y; // Convert Y from screen space -> world space.
		currentPos.z = param.cameraDolly;
		iCamera.SetPosition( currentPos );

		roomOriginPos.x =  currentPos.x;
		roomOriginPos.y = -currentPos.y; // Convert Y from world space -> screen space.
	}

	Donya::ICamera::Controller input{};
	input.slerpPercent = GameParam::Get().Data().cameraSlerp;

#if DEBUG_MODE
	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		const int wheel = Donya::Mouse::WheelRot();
		if ( wheel != 0 )
		{
			constexpr float Z_SPEED = 3.5f;
			input.moveVelocity.z = scast<float>( wheel ) * Z_SPEED;
		}

		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			iCamera.SetPosition( { 0.0f, 0.0f, -5.0f } );
		}
	}
#endif // DEBUG_MODE

	iCamera.Update( input );
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

		if ( controller.Trigger( Pad::LT ) ) { useJump = true; }
	}
	else
	{
		bool pressLeft  = Donya::Keyboard::Press( 'Z' )/* || Donya::Keyboard::Press( VK_LEFT  )*/;
		bool pressRight = Donya::Keyboard::Press( 'X' )/* || Donya::Keyboard::Press( VK_RIGHT )*/;
		if ( pressLeft  ) { moveLeft  = true; }
		if ( pressRight ) { moveRight = true; }
		
		bool trgJump = Donya::Keyboard::Trigger( VK_SPACE )/* || Donya::Keyboard::Trigger( VK_LSHIFT )*/;
		if ( trgJump ) { useJump = true; }
	}

	if ( moveLeft  ) { input.moveVelocity.x -= 1.0f; }
	if ( moveRight ) { input.moveVelocity.x += 1.0f; }
	if ( useJump   ) { input.useJump = true; }

	player.Update( elapsedTime, input );
}
bool SceneGame::IsPlayerOutFromRoom() const
{
	const auto param	= GameParam::Get().Data();
	const Donya::Int2 roomIndex = CalcRoomIndex( currentStageNo );

	Donya::Box roomBox{};
	roomBox.pos			= roomOriginPos;
	roomBox.size		= param.roomSize * 0.5f;

	Donya::Box playerBox = player.GetHitBox().Get2D();

	return ( Donya::Box::IsHitPoint( roomBox, playerBox.pos.x, playerBox.pos.y ) ) ? false : true;
}
void SceneGame::UpdateCurrentStage()
{
	/*
	The room is forming to matrix.
	Like this:
	----- ----- ----- -----
	| 0 | | 1 | | 2 | | 3 |
	----- ----- ----- -----
	| 4 | | 5 | | 6 | | 7 |
	----- ----- ----- -----
	| 8 | | 9 | | 11| | 12|
	----- ----- ----- -----

	And the "roomOriginPos" is the center of the rooms.
	Like this:
	--------- ---------
	|       | |       | // W : The whole width  of the rooms.
	|   X   | |   X   | // H : The whole height of the rooms.
	| (0,0) | | (W,0) |
	--------- ---------
	--------- ---------
	|       | |       |
	|   X   | |   X   |
	| (0,H) | | (W,H) |
	--------- ---------

	So the border of the rooms is:
	: half  size if the index of rooms is 0 -> 1.
	: whole size if the index of rooms is 1 -> N.
	*/

	const auto param = GameParam::Get().Data();
	const Donya::Vector2 roomHalfSize = param.roomSize * 0.5f;

	Donya::Vector2 playerPos = player.GetHitBox().Get2D().pos;
	playerPos.y *= -1.0f; // Think as screen space.
	playerPos   -= roomOriginPos;

	Donya::Int2 index = CalcRoomIndex( currentStageNo );
	if ( roomHalfSize.x	<  playerPos.x		) { index.x++; }
	if ( playerPos.x	< -roomHalfSize.x	) { index.x--; }
	if ( roomHalfSize.y	<  playerPos.y		) { index.y++; }
	if ( playerPos.y	< -roomHalfSize.y	) { index.y--; }

	index.x = std::max( 0, std::min( param.roomCounts.x - 1, index.x ) );
	index.y = std::max( 0, std::min( param.roomCounts.y - 1, index.y ) );

	const int prevStageNo	= currentStageNo;
	const int roomCount		= param.roomCounts.x * param.roomCounts.y;

	currentStageNo = index.x + ( param.roomCounts.x * index.y );

	if ( roomCount <= currentStageNo )
	{
		// Fail-safe.
		currentStageNo = prevStageNo;
	}
}

void SceneGame::HookUpdate( float elapsedTime )
{
#if USE_IMGUI
	Hook::UseImGui();
#endif // USE_IMGUI

	Hook::Input input{};

	Donya::Vector2		stick{};
	bool				useAction	= false;
	bool				extend		= false;
	bool				shrink		= false;
	bool				create		= false;
	bool				erase		= false; // A User can erase the hook arbitally.

	if ( controller.IsConnected() )
	{
		using Pad = Donya::Gamepad;

		stick = controller.RightStick();

		if (controller.Trigger ( Pad::RT )) { useAction = true; }
		if (stick.Length () != 0) {
			create = true;
			extend = true;
		}
		else { shrink = true; }
		if (controller.Trigger ( Pad::RB )) { erase = true; }
	}
	else
	{
		if ( Donya::Keyboard::Press  ( VK_LEFT		) ) { stick.x	-= 1.0f; }
		if ( Donya::Keyboard::Press  ( VK_RIGHT		) ) { stick.x	+= 1.0f; }
		if ( Donya::Keyboard::Press  ( VK_UP		) ) { stick.y	+= 1.0f; }
		if ( Donya::Keyboard::Press  ( VK_DOWN		) ) { stick.y	-= 1.0f; }
		if ( stick.IsZero() )
		{
			shrink = true;
		}
		else
		{
			create = true;
			extend = true;
		}

		if ( Donya::Keyboard::Trigger( VK_RSHIFT	) ) { useAction	= true; }
		if ( Donya::Keyboard::Trigger( VK_END		) ) { erase		= true; }
	}

	if ( create )
	{
		if( !pHook ) 
		{ 
			pHook = std::make_unique<Hook>( player.GetPosition() );
			Donya::Sound::Play( Music::Throw );
		}
	}
	if ( erase  )
	{
		pHook.reset();

		// The sound is temporary. so TODO : change this.
		Donya::Sound::Play( Music::Jump );
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
	input.extend	= extend;
	input.shrink	= shrink;

	pHook->Update(elapsedTime, input);
}

bool SceneGame::DetectClearMoment() const
{
	if ( Fader::Get().IsExist() ) { return false; }
	// else

	const auto clearBox		= GameParam::Get().Data().debugClearTrigger;
	const auto playerBox	= player.GetHitBox();
	BoxEx xyPlayer{};
	{
		xyPlayer.pos.x		= playerBox.pos.x;
		xyPlayer.pos.y		= playerBox.pos.y;
		xyPlayer.size.x		= playerBox.size.x;
		xyPlayer.size.y		= playerBox.size.y;
		xyPlayer.exist		= true;
	}

	return ( Donya::Box::IsHitBox( xyPlayer, clearBox ) ) ? true : false;
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
		change.sceneType = Scene::Type::Game;
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
		if ( ImGui::TreeNode( u8"ゲーム・デバッグ" ) )
		{
			ImGui::Text( u8"ポーズ画面へ : <Press P>" );

			ImGui::TreePop();
		}
		
		ImGui::End();
	}
}

#endif // USE_IMGUI
