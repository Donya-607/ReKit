#include "Gimmicks.h"

#include <algorithm>		// Use std::max, min, remove_if
#include <string>

#include "Donya/Loader.h"	// Use the explosion's model.
#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "FilePath.h"
#include "Music.h"

#undef max
#undef min

#pragma region Bomb

struct ParamBomb final : public Donya::Singleton<ParamBomb>
{
	friend Donya::Singleton<ParamBomb>;
public:
	struct Member
	{
		float	gravity{};
		float	maxFallSpeed{};
		float	brakeSpeed{};		// Affect to inverse speed of current velocity(only X-axis).
		float	stopThreshold{};	// The threshold of a judge to stop instead of the brake.

		AABBEx	hitBoxBomb{};		// Hit-Box of using to the collision to the stage.
		AABBEx	hitBoxExpl{};		// Hit-Box of using to the explosion.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( gravity ),
				CEREAL_NVP( maxFallSpeed ),
				CEREAL_NVP( brakeSpeed ),
				CEREAL_NVP( stopThreshold ),
				CEREAL_NVP( hitBoxBomb ),
				CEREAL_NVP( hitBoxExpl )
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "Bomb";
	Member m;
private:
	ParamBomb() : m() {}
public:
	~ParamBomb() = default;
public:
	void Init()
	{
		LoadParameter();
	}
	void Uninit()
	{
		// No op.
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
			if ( ImGui::TreeNode( u8"�M�~�b�N[Bomb]�E�����f�[�^" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"���S�ʒu�̃I�t�Z�b�g" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"�T�C�Y�i�������w��j" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"����" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox  ( ( prefix + u8"�����蔻��͗L����" ).c_str(), &pHitBox->exist );
				};

				ImGui::DragFloat( u8"�d�͉����x",			&m.gravity,			0.1f	);
				ImGui::DragFloat( u8"�ő嗎�����x",			&m.maxFallSpeed,	0.1f	);
				ImGui::DragFloat( u8"�u���[�L���x�i�w���j",	&m.brakeSpeed,		0.5f	);
				ImGui::DragFloat( u8"��~����臒l�i�w���j",	&m.stopThreshold,	0.1f	);
				
				AdjustAABB( u8"�����蔻��F�{���F", &m.hitBoxBomb );
				AdjustAABB( u8"�����蔻��F�����F", &m.hitBoxExpl );

				if ( ImGui::TreeNode( u8"�t�@�C��" ) )
				{
					static bool isBinary = true;
					if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true; }
					if ( ImGui::RadioButton( "JSON", !isBinary ) ) { isBinary = false; }
					std::string loadStr{ "�ǂݍ��� " };
					loadStr += ( isBinary ) ? "Binary" : "JSON";

					if ( ImGui::Button( u8"�ۑ�" ) )
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
CEREAL_CLASS_VERSION( ParamBomb::Member, 0 )

Donya::StaticMesh Bomb::modelExplosion{};
void Bomb::ParameterInit()
{
	ParamBomb::Get().Init();

	static bool wasCreated = false;
	if ( wasCreated ) { return; }
	// else

	Donya::Loader loader{};
	bool  loadSucceeded = loader.Load( "./Data/Models/Gimmicks/BombExplosion.bin", nullptr );
	if ( !loadSucceeded )
	{
		_ASSERT_EXPR( 0, L"Failed : Load a explosion model." );
	}

	bool  createSucceeded = Donya::StaticMesh::Create( loader, modelExplosion );
	if ( !createSucceeded )
	{
		_ASSERT_EXPR( 0, L"Failed : Create a explosion model." );
	}

	if ( loadSucceeded && createSucceeded )
	{
		wasCreated = true;
	}
}
#if USE_IMGUI
void Bomb::UseParameterImGui()
{
	ParamBomb::Get().UseImGui();
}
#endif // USE_IMGUI

Bomb::Bomb() : GimmickBase(),
	status( State::Bomb )
{}
Bomb::~Bomb() = default;

void Bomb::Init( int gimmickKind, float roll, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	rollDegree	= roll;
	pos			= wsPos;
	velocity	= 0.0f;
}
void Bomb::Uninit()
{
	// No op.
}

void Bomb::Update( float elapsedTime )
{
	switch ( status )
	{
	case Bomb::State::Bomb:
		BombUpdate( elapsedTime );
		return;
	case Bomb::State::Explosion:
		ExplosionUpdate( elapsedTime );
		return;
	default: return;
	}
}
void Bomb::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist )
{
	switch ( status )
	{
	case Bomb::State::Bomb:
		BombPhysicUpdate( player, accompanyBox, terrains, collideToPlayer, ignoreHitBoxExist );
		return;
	case Bomb::State::Explosion:
		ExplosionPhysicUpdate( player, accompanyBox, terrains, collideToPlayer, ignoreHitBoxExist );
		return;
	default: return;
	}
}

void Bomb::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 color{ 0.4f, 0.1f, 0.1f, 0.8f };

	BaseDraw( WVP, W, lightDir, color );
}

void Bomb::WakeUp()
{
	// No op.
}

bool Bomb::ShouldRemove() const
{
	// TODO : Implement this.
	return false;
}

AABBEx Bomb::GetHitBox() const
{
	AABBEx base = ParamBomb::Get().Data().hitBoxBomb;
	base.pos		+= pos;
	base.velocity	=  velocity;
	base.attr		=  kind;
	return base;
}

Donya::Vector4x4 Bomb::GetWorldMatrix( bool useDrawing ) const
{
	auto wsBox = GetHitBox();
	if ( useDrawing )
	{
		// The AABB size is half, but drawing object's size is whole.
		wsBox.size *= 2.0f;
	}

	const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Front(), ToRadian( rollDegree ) );
	const Donya::Vector4x4 R = rotation.RequireRotationMatrix();
	Donya::Vector4x4 mat{};
	mat._11 = wsBox.size.x;
	mat._22 = wsBox.size.y;
	mat._33 = wsBox.size.z;
	mat *= R;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

void Bomb::Fall( float elapsedTime )
{
	const auto DATA = ParamBomb::Get().Data();
	velocity.y -= DATA.gravity * elapsedTime;
	velocity.y =  std::max( DATA.maxFallSpeed, velocity.y );
}

void Bomb::Brake( float elapsedTime )
{
	const float moveSign = scast<float>( Donya::SignBit( velocity.x ) );
	if ( ZeroEqual( moveSign ) ) { return; }
	// else

	const float nowSpeed = fabsf( velocity.x );
	if ( nowSpeed <= ParamBomb::Get().Data().stopThreshold )
	{
		velocity.x = 0.0f;
		return;
	}
	// else

	const float brakeSpeed = std::min( nowSpeed, ParamBomb::Get().Data().brakeSpeed );
	velocity.x -= brakeSpeed * moveSign;
}

void Bomb::BombUpdate( float elapsedTime )
{
	Fall( elapsedTime );

	Brake( elapsedTime );
}
void Bomb::ExplosionUpdate( float elapsedTime )
{

}

void Bomb::BombPhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist )
{
	GimmickBase::PhysicUpdate( player, accompanyBox, terrains );
}
void Bomb::ExplosionPhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist )
{
	// No op.
}

#if USE_IMGUI

void Bomb::ShowImGuiNode()
{
	using namespace GimmickUtility;

	ImGui::Text( u8"��ށF%d[%s]", kind, ToString( ToKind( kind ) ).c_str() );
	ImGui::Text( u8"��ԁF%s", ( status == State::Explosion ) ? u8"����" : u8"�{��" );
	ImGui::DragFloat ( u8"�y����]��",	&rollDegree,	1.0f	);
	ImGui::DragFloat3( u8"���[���h���W",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"���x",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI

// region Bomb
#pragma endregion



#pragma region Generator

struct ParamBombGenerator final : public Donya::Singleton<ParamBombGenerator>
{
	friend Donya::Singleton<ParamBombGenerator>;
public:
	struct Member
	{
		float			generateFrame{};
		Donya::Vector3	generateOffset{};
		AABBEx			hitBox{};			// Hit-Box of using to the collision to the stage.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( generateFrame ),
				CEREAL_NVP( generateOffset ),
				CEREAL_NVP( hitBox )
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "BombGenerator";
	Member m;
private:
	ParamBombGenerator() : m() {}
public:
	~ParamBombGenerator() = default;
public:
	void Init()
	{
		LoadParameter();
	}
	void Uninit()
	{
		// No op.
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
			if ( ImGui::TreeNode( u8"�M�~�b�N[BombGenerator]�E�����f�[�^" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"���S�ʒu�̃I�t�Z�b�g" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"�T�C�Y�i�������w��j" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"����" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox  ( ( prefix + u8"�����蔻��͗L����" ).c_str(), &pHitBox->exist );
				};

				ImGui::DragFloat ( u8"�����Ԋu�i�b�j",	&m.generateFrame,		0.1f, 0.0f	);
				ImGui::DragFloat3( u8"�����ʒu�i���΁j",	&m.generateOffset.x,	0.1f		);
				
				AdjustAABB( u8"�����蔻��", &m.hitBox );

				if ( ImGui::TreeNode( u8"�t�@�C��" ) )
				{
					static bool isBinary = true;
					if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true; }
					if ( ImGui::RadioButton( "JSON", !isBinary ) ) { isBinary = false; }
					std::string loadStr{ "�ǂݍ��� " };
					loadStr += ( isBinary ) ? "Binary" : "JSON";

					if ( ImGui::Button( u8"�ۑ�" ) )
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
CEREAL_CLASS_VERSION( ParamBombGenerator::Member, 0 )

void BombGenerator::ParameterInit()
{
	ParamBombGenerator::Get().Init();
}
#if USE_IMGUI
void BombGenerator::UseParameterImGui()
{
	ParamBombGenerator::Get().UseImGui();
}
#endif // USE_IMGUI

BombGenerator::BombGenerator() : GimmickBase(),
	generateTimer(), bombs()
{}
BombGenerator::~BombGenerator() = default;

void BombGenerator::Init( int gimmickKind, float roll, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	rollDegree	= roll;
	pos			= wsPos;
	velocity	= 0.0f;

	generateTimer = ParamBombGenerator::Get().Data().generateFrame;
}
void BombGenerator::Uninit()
{
	// No op.
}

void BombGenerator::Update( float elapsedTime )
{
	CountDown( elapsedTime );
	Generate();

	UpdateBombs( elapsedTime );
}
void BombGenerator::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist )
{
	PhysicUpdateBombs( player, accompanyBox, terrains, collideToPlayer, ignoreHitBoxExist );
}

void BombGenerator::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 color{ 0.7f, 0.7f, 0.7f, 0.8f };

	BaseDraw( WVP, W, lightDir, color );

	for ( const auto &it : bombs )
	{
		it.Draw( V, P, lightDir );
	}
}

void BombGenerator::WakeUp()
{
	// No op.
}

bool BombGenerator::ShouldRemove() const
{
	// Don't destroy.
	return false;
}

AABBEx BombGenerator::GetHitBox() const
{
	AABBEx base = ParamBombGenerator::Get().Data().hitBox;
	base.pos		+= pos;
	base.velocity	=  velocity;
	base.attr		=  kind;
	return base;
}

bool BombGenerator::HasMultipleHitBox() const
{
	return true;
}
std::vector<AABBEx> BombGenerator::GetAnotherHitBoxes() const
{
	const size_t boxCount = bombs.size();
	std::vector<AABBEx> multipleHitBoxes{ boxCount };

	for ( size_t i = 0; i < boxCount; ++i )
	{
		multipleHitBoxes[i] = bombs[i].GetHitBox();
	}

	return multipleHitBoxes;
}

void BombGenerator::CountDown( float elapsedTime )
{
	generateTimer -= elapsedTime;
}
void BombGenerator::Generate()
{
	if ( 0.0f < generateTimer ) { return; }
	// else

	const auto &param = ParamBombGenerator::Get().Data();
	generateTimer = param.generateFrame;

	bombs.push_back( Bomb{} );
	bombs.back().Init( scast<int>( GimmickKind::Bomb ), 0.0f, pos + param.generateOffset );
}

void BombGenerator::UpdateBombs( float elapsedTime )
{
	for ( auto &it : bombs )
	{
		it.Update( elapsedTime );
	}

	EraseBombs();
}
void BombGenerator::PhysicUpdateBombs( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist )
{
	for ( auto &it : bombs )
	{
		it.PhysicUpdate( player, accompanyBox, terrains, collideToPlayer, ignoreHitBoxExist );
	}
}
void BombGenerator::EraseBombs()
{
	auto itr = std::remove_if
	(
		bombs.begin(), bombs.end(),
		[]( Bomb &element )
		{
			return element.ShouldRemove();
		}
	);
	
	bombs.erase( itr, bombs.end() );
}

Donya::Vector4x4 BombGenerator::GetWorldMatrix( bool useDrawing ) const
{
	auto wsBox = GetHitBox();
	if ( useDrawing )
	{
		// The AABB size is half, but drawing object's size is whole.
		// wsBox.size *= 2.0f;
	}

	const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Front(), ToRadian( rollDegree ) );
	const Donya::Vector4x4 R = rotation.RequireRotationMatrix();
	Donya::Vector4x4 mat{};
	mat._11 = wsBox.size.x;
	mat._22 = wsBox.size.y;
	mat._33 = wsBox.size.z;
	mat *= R;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

#if USE_IMGUI

void BombGenerator::ShowImGuiNode()
{
	using namespace GimmickUtility;

	ImGui::Text( u8"��ށF%d[%s]", kind, ToString( ToKind( kind ) ).c_str() );
	ImGui::DragFloat ( u8"�y����]��",	&rollDegree,	1.0f	);
	ImGui::DragFloat3( u8"���[���h���W",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"���x",			&velocity.x,	0.01f	);

	const size_t bombCount = bombs.size();

	const std::string nodeCaption = u8"�������F[" + std::to_string( bombCount ) + u8"��]";
	if ( ImGui::TreeNode(  nodeCaption.c_str() ) )
	{
		ImGui::BeginChild( nodeCaption.c_str(), ImVec2{ 0.0f, 270.0f } );
		{
			std::string instanceCaption{};
			for ( size_t i = 0; i < bombCount; ++i )
			{
				instanceCaption = "[" + std::to_string( i ) + "]";
				if ( ImGui::TreeNode( instanceCaption.c_str() ) )
				{
					bombs[i].ShowImGuiNode();

					ImGui::TreePop();
				}
			}
		}
		ImGui::EndChild();

		ImGui::TreePop();
	}
}

#endif // USE_IMGUI

// region Generator
#pragma endregion
