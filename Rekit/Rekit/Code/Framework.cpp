#include "Framework.h"

#include <array>

#include "Header/Blend.h"
#include "Header/Constant.h"
#include "Header/Donya.h"
#include "Header/Keyboard.h"
#include "Header/Mouse.h"
#include "Header/Resource.h"
#include "Header/ScreenShake.h"
#include "Header/Sound.h"
#include "Header/Useful.h"
#include "Header/UseImgui.h"

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
	pSceneMng->Init( Scene::Type::ChiProject );
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

	DebugShowInformation();

#endif // DEBUG_MODE

	pSceneMng->Update( elapsedTime );
}

#define ENABLE_3D_TEST ( true && DEBUG_MODE && USE_IMGUI )
#if ENABLE_3D_TEST
#include <memory>
#include "Header/Camera.h"
#include "Header/Collision.h"
#include "Header/GeometricPrimitive.h"
#include "Header/Loader.h"
#include "Header/StaticMesh.h"
#include "Header/Useful.h"
#endif // ENABLE_3D_TEST
void Framework::Draw( float elapsedTime/*Elapsed seconds from last frame*/ )
{
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA );

	pSceneMng->Draw( elapsedTime );

#if ENABLE_3D_TEST

	auto Draw3DModelTest = [&]()
	{
		auto InitializedCameraPointer = []()
		{
			std::shared_ptr<Donya::Camera> pCamera = std::make_shared<Donya::Camera>();
			pCamera->Init
			(
				Common::ScreenWidthF(),
				Common::ScreenHeightF(),
				ToRadian( 30.0f ) // FOV
			);
			pCamera->SetFocusCoordinate( { 0.0f, 0.0f, 2.0f } );
			return pCamera;
		};
		static std::shared_ptr<Donya::Camera> pCamera = InitializedCameraPointer();
	
		auto MakeControlStructWithMouse = []()
		{
			static Donya::Int2 prevMouse{};
			static Donya::Int2 currMouse{};

			prevMouse = currMouse;

			auto nowMouse = Donya::Mouse::Coordinate();
			currMouse.x = scast<int>( nowMouse.x );
			currMouse.y = scast<int>( nowMouse.y );

			auto IsEqual = []( const Donya::Int2 &lhs, const Donya::Int2 &rhs )
			{
				if ( lhs.x != rhs.x ) { return false; }
				if ( lhs.y != rhs.y ) { return false; }
				return true;
			};
			bool isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
			bool isDriveMouse = ( !IsEqual( prevMouse, currMouse ) ) || Donya::Mouse::WheelRot() || isInputMouseButton;
			if ( !isDriveMouse )
			{
				Donya::Camera::Controller noop{};
				noop.SetNoOperation();
				return noop;
			}

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
				movement.y = diff.y * MOVE_SPEED;
			}

			constexpr float FRONT_SPEED = 3.5f;
			movement.z = FRONT_SPEED * scast<float>( Donya::Mouse::WheelRot() );

			Donya::Quaternion rotYaw	= Donya::Quaternion::Make( Donya::Vector3::Up(), rotation.x );

			Donya::Vector3 right = Donya::Vector3::Right();
			right = rotYaw.RotateVector( right );
			Donya::Quaternion rotPitch	= Donya::Quaternion::Make( right, rotation.y );

			Donya::Quaternion rotQ		= rotYaw * rotPitch;

			static Donya::Vector3 front = Donya::Vector3::Front();

			if ( !rotation.IsZero() )
			{
				front = rotQ.RotateVector( front );
				front.Normalize();
			}

			Donya::Camera::Controller ctrl{};
			ctrl.moveVelocity		= movement;
			ctrl.rotation			= rotation;
			ctrl.slerpPercent		= 1.0f;
			ctrl.moveAtLocalSpace	= true;

			return ctrl;
		};
		pCamera->Update( MakeControlStructWithMouse() );
		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			pCamera->SetPosition( { 0.0f, 0.0f, 0.0f } );
		}

		static AABB	baseAABB
		{
			Donya::Vector3{ -1.0f, 0.0f, 6.0f },
			Donya::Vector3{ 0.5f, 0.5f, 0.5f }, true
		};
		static Sphere baseSphere
		{
			Donya::Vector3{ -1.0f, 2.0f, 6.0f },
			0.5f, true
		};
		static AABB	ctrlAABB
		{
			Donya::Vector3{},
			Donya::Vector3{ 0.5f, 0.5f, 0.5f }, true
		};
		static Sphere ctrlSphere
		{
			Donya::Vector3{ 0.0f, 1.0f, 0.0f },
			0.5f, true
		};
		{
			constexpr float MOVE_ADD = 0.1f;
			Donya::Vector3 velocity{};
			if ( Donya::Keyboard::Press( 'G' ) ) { velocity.y += MOVE_ADD; }
			if ( Donya::Keyboard::Press( 'B' ) ) { velocity.y -= MOVE_ADD; }
			if ( Donya::Keyboard::Press( 'V' ) ) { velocity.x -= MOVE_ADD; }
			if ( Donya::Keyboard::Press( 'N' ) ) { velocity.x += MOVE_ADD; }
			if ( Donya::Keyboard::Press( 'F' ) ) { velocity.z += MOVE_ADD; }
			if ( Donya::Keyboard::Press( 'H' ) ) { velocity.z -= MOVE_ADD; }

			// auto cameraPosture = pCamera->GetPosture();
			// velocity = cameraPosture.RotateVector( velocity );

			ctrlAABB.pos += velocity;
			ctrlSphere.pos += velocity;
		}

		XMMATRIX W_Cube{}, W_B_Cube{}, W_C_Cube{};
		XMMATRIX W_Sphere{}, W_B_Sphere{}, W_C_Sphere{};
		XMMATRIX W_TexBoard{};
		XMMATRIX W_Mesh{};
		{
			static float scale = 1.0f; // 0.1f;
			static float angleX = 0.0f; // -10.0f;	// Radian.
			static float angleY = 0.0f; // -160.0f;	// Radian.
			static float angleZ = 0.0f; // 0;		// Radian.
			static float moveX = 0.0f; // 2.0f;
			static float moveY = 0.0f; // -2.0f;
			static float moveZ = 0.0f; // 0;

			if ( 1 )
			{
				constexpr float SCALE_ADD = 0.01f;
				constexpr float ANGLE_ADD = ToRadian( 5.0f );
				constexpr float MOVE_ADD = 0.1f;

				if ( Donya::Keyboard::Press( 'W'		) ) { scale  += SCALE_ADD; }
				if ( Donya::Keyboard::Press( 'S'		) ) { scale  -= SCALE_ADD; }
				if ( Donya::Keyboard::Press( VK_UP		) ) { angleX += ANGLE_ADD; }
				if ( Donya::Keyboard::Press( VK_DOWN	) ) { angleX -= ANGLE_ADD; }
				if ( Donya::Keyboard::Press( VK_RIGHT	) ) { angleY += ANGLE_ADD; }
				if ( Donya::Keyboard::Press( VK_LEFT	) ) { angleY -= ANGLE_ADD; }
				if ( Donya::Keyboard::Press( 'A'		) ) { angleZ += ANGLE_ADD; }
				if ( Donya::Keyboard::Press( 'D'		) ) { angleZ -= ANGLE_ADD; }
				if ( Donya::Keyboard::Press( 'I'		) ) { moveY  += MOVE_ADD;  }
				if ( Donya::Keyboard::Press( 'K'		) ) { moveY  -= MOVE_ADD;  }
				if ( Donya::Keyboard::Press( 'L'		) ) { moveX  += MOVE_ADD;  }
				if ( Donya::Keyboard::Press( 'J'		) ) { moveX  -= MOVE_ADD;  }
			}

			XMMATRIX S	= XMMatrixScaling( scale, scale, scale );

			XMMATRIX R{};	// Make by Euler-angles(radian).
			XMMATRIX RQ{};	// Make by Quaternion.
			{
				auto q		= Donya::Quaternion::Make( angleX, angleY, angleZ );
				RQ			= XMLoadFloat4x4( &q.RequireRotationMatrix() );
			}
			{
				XMMATRIX RX	= XMMatrixRotationX( angleX );
				XMMATRIX RY	= XMMatrixRotationY( angleY );
				XMMATRIX RZ	= XMMatrixRotationZ( angleZ );
				R			= RZ * RY * RX;
			}

			constexpr float SHIFT = 1.0f;
			XMMATRIX T_Cube		= XMMatrixTranslation( moveX - SHIFT, moveY, moveZ );
			XMMATRIX T_Sphere	= XMMatrixTranslation( moveX + SHIFT, moveY, moveZ );
			XMMATRIX T_TexBoard	= XMMatrixTranslation( moveX, moveY, moveZ );

			XMMATRIX T_B_AABB	= XMMatrixTranslation( baseAABB.pos.x,   baseAABB.pos.y,   baseAABB.pos.z );
			XMMATRIX T_B_Sphere	= XMMatrixTranslation( baseSphere.pos.x, baseSphere.pos.y, baseSphere.pos.z );
			XMMATRIX T_C_AABB	= XMMatrixTranslation( ctrlAABB.pos.x,   ctrlAABB.pos.y,   ctrlAABB.pos.z );
			XMMATRIX T_C_Sphere	= XMMatrixTranslation( ctrlSphere.pos.x, ctrlSphere.pos.y, ctrlSphere.pos.z );

			W_Cube		= S * R * T_Cube;
			W_Sphere	= S * R * T_Sphere;

			W_B_Cube	= S * R * T_B_AABB;
			W_B_Sphere	= S * R * T_B_Sphere;
			W_C_Cube	= S * R * T_C_AABB;
			W_C_Sphere	= S * R * T_C_Sphere;

			W_TexBoard	= S * R * T_TexBoard;
			W_Mesh		= W_TexBoard;
		}

		XMFLOAT4X4 worldViewProjection_Cube{},		worldViewProjection_B_Cube{},	worldViewProjection_C_Cube{};
		XMFLOAT4X4 worldViewProjection_Sphere{},	worldViewProjection_B_Sphere{},	worldViewProjection_C_Sphere{};
		XMFLOAT4X4 worldViewProjection_TexBoard{};
		XMFLOAT4X4 worldViewProjection_Mesh{};
		{
			XMMATRIX V = pCamera->CalcViewMatrix();
			XMMATRIX P = pCamera->GetProjectionMatrix();

			XMStoreFloat4x4
			(
				&worldViewProjection_Cube,
				XMMatrixMultiply( W_Cube, XMMatrixMultiply( V, P ) )
			);
			XMStoreFloat4x4
			(
				&worldViewProjection_Sphere,
				XMMatrixMultiply( W_Sphere, XMMatrixMultiply( V, P ) )
			);

			XMStoreFloat4x4
			(
				&worldViewProjection_B_Cube,
				XMMatrixMultiply( W_B_Cube, XMMatrixMultiply( V, P ) )
			);
			XMStoreFloat4x4
			(
				&worldViewProjection_B_Sphere,
				XMMatrixMultiply( W_B_Sphere, XMMatrixMultiply( V, P ) )
			);
			XMStoreFloat4x4
			(
				&worldViewProjection_C_Cube,
				XMMatrixMultiply( W_C_Cube, XMMatrixMultiply( V, P ) )
			);
			XMStoreFloat4x4
			(
				&worldViewProjection_C_Sphere,
				XMMatrixMultiply( W_C_Sphere, XMMatrixMultiply( V, P ) )
			);

			XMStoreFloat4x4
			(
				&worldViewProjection_TexBoard,
				XMMatrixMultiply( W_TexBoard, XMMatrixMultiply( V, P ) )
			);

			worldViewProjection_Mesh = worldViewProjection_TexBoard;
		}

		XMFLOAT4X4 world_Cube{},	world_B_Cube{},		world_C_Cube{};
		XMFLOAT4X4 world_Sphere{},	world_B_Sphere{},	world_C_Sphere{};
		XMFLOAT4X4 world_TexBoard{};
		XMFLOAT4X4 world_Mesh{};
		{
			XMStoreFloat4x4( &world_Cube, W_Cube );
			XMStoreFloat4x4( &world_Sphere, W_Sphere );
			XMStoreFloat4x4( &world_B_Cube, W_B_Cube );
			XMStoreFloat4x4( &world_B_Sphere, W_B_Sphere );
			XMStoreFloat4x4( &world_C_Cube, W_C_Cube );
			XMStoreFloat4x4( &world_C_Sphere, W_C_Sphere );
			XMStoreFloat4x4( &world_TexBoard, W_TexBoard );
			world_Mesh = world_TexBoard;
		}

		static XMFLOAT4 lightDirection{ 0.0f, 0.0f, 1.0f, 0.0f };
		static XMFLOAT4 lightColor{ 1.0f, 1.0f, 1.0f, 1.0f };		// The fourth-parameter(w) is light-intensity.
		static XMFLOAT4 mtlColor{ 0.4f, 1.0f, 0.7f, 1.0f };
		int drawCount = 1;
	#if USE_IMGUI
		pCamera->ShowParametersToImGui();
		if ( ImGui::BeginIfAllowed() )
		{
			if ( ImGui::TreeNode( u8"ÇRÇcÅEÉeÉXÉg" ) )
			{
				ImGui::SliderFloat3( "Light Direction", &lightDirection.x, -1.0f, 1.0f );
				ImGui::ColorEdit4( "Light Color(A is light-intensity)", &lightColor.x );
				ImGui::ColorEdit4( "Material Color", &mtlColor.x );
				ImGui::SliderInt( u8"ï`âÊâÒêî", &drawCount, 0, 1024 );

				ImGui::TreePop();
			}

			ImGui::End();
		}
	#endif // USE_IMGUI
	
		auto InitializedCube = []()
		{
			Donya::Geometric::Cube cube{};
			cube.Init();
			return cube;
		};
		auto InitializedSphere = []()
		{
			Donya::Geometric::Sphere sphere{};
			sphere.Init();
			return sphere;
		};
		auto InitializedTexBoard = []()
		{
			Donya::Geometric::TextureBoard texBoard{ L"./Data/Images/Rights/FMOD Logo White - Black Background.png" };
			texBoard.Init();
			return texBoard;
		};
		auto InitializedStaticMesh = []( std::string fullPath )
		{
			Donya::Loader loader{};
			loader.Load( fullPath, nullptr );

			return Donya::StaticMesh::Create( loader );
		};
		static Donya::Geometric::Cube				cube			= InitializedCube();
		static Donya::Geometric::Sphere				sphere			= InitializedSphere();
		static Donya::Geometric::TextureBoard		texBoard		= InitializedTexBoard();
		static std::shared_ptr<Donya::StaticMesh>	pStaticMesh		= InitializedStaticMesh( "D:\\Captures\\Player.bin" );
		static Donya::Geometric::Cube				baseShowCube	= InitializedCube();
		static Donya::Geometric::Sphere				baseShowSphere	= InitializedSphere();
		static Donya::Geometric::Cube				colCube			= InitializedCube();
		static Donya::Geometric::Sphere				colSphere		= InitializedSphere();
		Donya::Vector4 collisionColorCube{ 1.0f, 1.0f, 1.0f, 0.8f };
		Donya::Vector4 collisionColorSphere{ 1.0f, 1.0f, 1.0f, 0.8f };
		// if ( AABB::IsHitAABB( baseAABB, ctrlAABB ) || AABB::IsHitSphere( baseAABB, ctrlSphere ) )
		// if ( AABB::IsHitPoint( baseAABB, ctrlAABB.pos ) || AABB::IsHitPoint( baseAABB, ctrlSphere.pos ) )
		{
			// collisionColorCube = Donya::Vector4{ 1.0f, 0.5f, 0.0f, 0.9f };
		}
		// if ( Sphere::IsHitAABB( baseSphere, ctrlAABB ) || Sphere::IsHitSphere( baseSphere, ctrlSphere ) )
		// if ( Sphere::IsHitPoint( baseSphere, ctrlAABB.pos ) || Sphere::IsHitPoint( baseSphere, ctrlSphere.pos ) )
		{
			// collisionColorSphere = Donya::Vector4{ 1.0f, 0.5f, 0.0f, 0.9f };
		}

		// Primitives
		for ( int i = 0; i < drawCount; ++i )
		{
			cube.Render				( nullptr, true, true, worldViewProjection_Cube,		world_Cube,		lightDirection,	mtlColor );
			sphere.Render			( nullptr, true, true, worldViewProjection_Sphere,	world_Sphere,	lightDirection,	mtlColor );

			baseShowCube.Render		( nullptr, true, true, worldViewProjection_B_Cube,	world_B_Cube,	lightDirection,	collisionColorCube		);
			baseShowSphere.Render	( nullptr, true, true, worldViewProjection_B_Sphere, world_B_Sphere, lightDirection,	collisionColorSphere	);

			colCube.Render			( nullptr, true, true, worldViewProjection_C_Cube,	world_C_Cube,	lightDirection,	collisionColorCube		);
			colSphere.Render		( nullptr, true, true, worldViewProjection_C_Sphere, world_C_Sphere, lightDirection,	collisionColorSphere	);
		}

		// TextureBoards
		for ( int i = 0; i < drawCount; ++i )
		{
			texBoard.Render( nullptr, true, true, worldViewProjection_TexBoard, world_TexBoard, lightDirection, mtlColor );
		}

		if ( pStaticMesh )
		{
			for ( int i = 0; i < drawCount; ++i )
			{
				// pStaticMesh->Render( worldViewProjection_Mesh, world_Mesh, lightDirection, lightColor, mtlColor );
			}
		}
	};
	// Draw3DModelTest();

#endif // ENABLE_3D_TEST
}

#undef ENABLE_3D_TEST

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

	const std::array<Bundle, ID::TERMINATION_OF_MUSIC_ID> bandles =
	{
		{	// ID, FilePath, isEnableLoop
			{ ID::BGM_Title,			"./Data/Sounds/BGM/Title.wav",					true  },
			{ ID::BGM_Stage0,			"./Data/Sounds/BGM/Stage0.wav",					true  },
			{ ID::BGM_Stage1,			"./Data/Sounds/BGM/Stage1.wav",					true  },
			{ ID::BGM_Stage2,			"./Data/Sounds/BGM/Stage2.wav",					true  },
			{ ID::BGM_Stage3,			"./Data/Sounds/BGM/Stage3.wav",					true  },
			{ ID::BGM_Stage4,			"./Data/Sounds/BGM/Stage4.wav",					true  },
			{ ID::BGM_Result,			"./Data/Sounds/BGM/Result.wav",					false },

			{ ID::PlayerAtk,			"./Data/Sounds/SE/Player/Attack.wav",			false },
			{ ID::PlayerAtkHit,			"./Data/Sounds/SE/Player/AttackHit.wav",		false },
			{ ID::PlayerDamage,			"./Data/Sounds/SE/Player/ReceiveDamage.wav",	false },
			{ ID::PlayerFullCharge,		"./Data/Sounds/SE/Player/FullCharge.wav",		false },
			{ ID::PlayerHitToNeedle,	"./Data/Sounds/SE/Player/HitToNeedle.wav",		false },
			{ ID::PlayerJump,			"./Data/Sounds/SE/Player/Jump.wav",				false },

			{ ID::BreakRock,			"./Data/Sounds/SE/Player/BreakRock.wav",		false },

			{ ID::TouchRespawnPoint,	"./Data/Sounds/SE/Player/TouchRespawnPoint.wav",false },

			{ ID::ItemChoose,			"./Data/Sounds/SE/UI/ChooseItem.wav",			false },
			{ ID::ItemDecision,			"./Data/Sounds/SE/UI/DecisionItem.wav",			false },
		},
	};

	bool result = true, successed = true;

	for ( size_t i = 0; i < ID::TERMINATION_OF_MUSIC_ID; ++i )
	{
		result = Load( bandles[i].id, bandles[i].fileName.c_str(), bandles[i].isEnableLoop );
		if ( !result ) { successed = false; }
	}

	return successed;
}

#if DEBUG_MODE && USE_IMGUI
#include "Header/Easing.h"
#endif // DEBUG_MODE && USE_IMGUI
void Framework::DebugShowInformation()
{
#if DEBUG_MODE && USE_IMGUI

	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( "Information" ) )
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

		if ( ImGui::TreeNode( "Easing Test" ) )
		{
			using namespace Donya::Easing;

			static float time		= 0.0f;
			ImGui::SliderFloat( u8"éûä‘", &time, 0.0f, 1.0f );
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

		if ( ImGui::TreeNode( "Screen Shake" ) )
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

#endif // DEBUG_MODE && USE_IMGUI
}