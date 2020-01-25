#include "Trigger.h"

#include <algorithm>		// Use std::max, min.
#include <string>

#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "FilePath.h"
#include "GimmickUtil.h"	// Use for the GimmickKind, a namespaces.
#include "Music.h"

#undef max
#undef min

namespace
{
	const int GATHER_SIGN_ID = GimmickUtility::ToInt( GimmickKind::TriggerSwitch );
	BoxEx	ToGatherBox( BoxEx  source )
	{
		source.attr  = GATHER_SIGN_ID;
		source.mass  = GATHER_SIGN_ID;
		source.exist = false;
		return source;
	}
	AABBEx	ToGatherBox( AABBEx source )
	{
		source.attr  = GATHER_SIGN_ID;
		source.mass  = GATHER_SIGN_ID;
		source.exist = false;
		return source;
	}
	bool	IsGatherBox( const BoxEx  &source )
	{
		if ( source.attr  != GATHER_SIGN_ID )	{ return false; }
		if ( source.mass  != GATHER_SIGN_ID )	{ return false; }
		if ( source.exist != false )			{ return false; }
		return true;
	}
	bool	IsGatherBox( const AABBEx &source )
	{
		if ( source.attr  != GATHER_SIGN_ID )	{ return false; }
		if ( source.mass  != GATHER_SIGN_ID )	{ return false; }
		if ( source.exist != false )			{ return false; }
		return true;
	}
}

struct ParamTrigger final : public Donya::Singleton<ParamTrigger>
{
	friend Donya::Singleton<ParamTrigger>;
public:
	struct Member
	{
		struct KeyMember
		{
			float drawScale{ 1.0f };
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				// archive();

				if ( 1 <= version )
				{
					archive( CEREAL_NVP( drawScale ) );
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct SwitchMember
		{
			// The switch gather a block in direction to me.
			// And the switch have the collisions look like 'U'.
			// The "hitBoxLeft", "Right" roles a side wall, the "Member::hitBoxSwitch" roles bottom wall.
			// The "gatheringArea" represents a gathering area only, has not collision.

			float drawScale{ 1.0f };
			Donya::Vector3 gatheringPos{};
			AABBEx gatheringArea{};
			AABBEx hitBoxLeft{};
			AABBEx hitBoxRight{};
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				// archive();

				if ( 1 <= version )
				{
					archive
					(
						CEREAL_NVP( gatheringPos ),
						CEREAL_NVP( gatheringArea ),
						CEREAL_NVP( hitBoxLeft ),
						CEREAL_NVP( hitBoxRight )
					);
				}
				if ( 2 <= version )
				{
					archive( CEREAL_NVP( drawScale ) );
				}
				if ( 3 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct PullMember
		{
			float drawScale;
			float stretchMax;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( stretchMax )
				);
				if ( 1 <= version )
				{
					archive( CEREAL_NVP( drawScale ) );
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		KeyMember		mKey{};
		SwitchMember	mSwitch{};
		PullMember		mPull{};

		AABBEx	hitBoxKey{};
		AABBEx	hitBoxSwitch{};
		AABBEx	hitBoxPull{};
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( hitBoxKey ),
				CEREAL_NVP( hitBoxSwitch ),
				CEREAL_NVP( hitBoxPull )
			);
			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( mKey ),
					CEREAL_NVP( mSwitch ),
					CEREAL_NVP( mPull )
				);
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "Trigger";
	Member m;
private:
	ParamTrigger() : m() {}
public:
	~ParamTrigger() = default;
public:
	void Init()
	{
		LoadParameter();

		m.mSwitch.gatheringArea = ToGatherBox( m.mSwitch.gatheringArea );
	}
	void Uninit()
	{
		// No op.
	}
public:
	Member Data() const { return m; }
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
			if ( ImGui::TreeNode( u8"ギミック[Trigger]・調整データ" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt( ( prefix + u8"質量" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox( ( prefix + u8"当たり判定は有効か" ).c_str(), &pHitBox->exist );
					ImGui::Text( "" );
				};

				if ( ImGui::TreeNode( u8"カギ" ) )
				{
					ImGui::DragFloat( u8"描画スケール", &m.mKey.drawScale, 0.1f );
					ImGui::TreePop();
				}
				if ( ImGui::TreeNode( u8"スイッチ" ) )
				{
					ImGui::DragFloat( u8"描画スケール", &m.mSwitch.drawScale, 0.1f );
					ImGui::DragFloat3( u8"引き寄せる位置（相対）",		&m.mSwitch.gatheringPos.x,			0.1f		);
					ImGui::DragFloat2( u8"引き寄せるサイズ（半分を指定）",	&m.mSwitch.gatheringArea.size.x,	0.1f, 0.0f	);
					{
						m.mSwitch.gatheringArea = ToGatherBox( m.mSwitch.gatheringArea );
						m.mSwitch.gatheringArea.pos = m.mSwitch.gatheringPos;
						m.mSwitch.gatheringArea.velocity = 0.0f;
					}
					ImGui::Text( "" );

					AdjustAABB( u8"当たり判定・左壁",		&m.mSwitch.hitBoxLeft		);
					AdjustAABB( u8"当たり判定・右壁",		&m.mSwitch.hitBoxRight		);

					ImGui::TreePop();
				}
				if ( ImGui::TreeNode( u8"引き手" ) )
				{
					ImGui::DragFloat( u8"描画スケール", &m.mPull.drawScale, 0.1f );
					ImGui::DragFloat( u8"引っ張る長さ", &m.mPull.stretchMax, 1.0f, 0.0f );

					ImGui::TreePop();
				}

				AdjustAABB( u8"当たり判定・鍵",				&m.hitBoxKey	);
				AdjustAABB( u8"当たり判定・スイッチ（底）",	&m.hitBoxSwitch	);
				AdjustAABB( u8"当たり判定・引き手",			&m.hitBoxPull	);

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
CEREAL_CLASS_VERSION( ParamTrigger::Member, 1 )
CEREAL_CLASS_VERSION( ParamTrigger::Member::KeyMember, 1 )
CEREAL_CLASS_VERSION( ParamTrigger::Member::SwitchMember, 2 )
CEREAL_CLASS_VERSION( ParamTrigger::Member::PullMember, 1 )



bool Trigger::IsGatherBox( const BoxEx  &source )
{
	return ::IsGatherBox( source );
}
bool Trigger::IsGatherBox( const AABBEx &source )
{
	return ::IsGatherBox( source );
}

void Trigger::ParameterInit()
{
	ParamTrigger::Get().Init();
}
#if USE_IMGUI
void Trigger::UseParameterImGui()
{
	ParamTrigger::Get().UseImGui();
}
#endif // USE_IMGUI

Trigger::Trigger() : GimmickBase(),
	id ( -1 ), enable( false ),
	mKey(), mSwitch(), mPull()
{}
Trigger::Trigger( int id, bool enable ) : GimmickBase(),
	id ( id ), enable( enable ),
	mKey(), mSwitch(), mPull()
{}
Trigger::~Trigger() = default;

void Trigger::Init( int gimmickKind, float roll, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	rollDegree	= roll;
	pos			= wsPos;
	velocity	= 0.0f;

	switch ( kind )
	{
	case scast<int>( GimmickKind::TriggerKey ) :
		InitKey();
		break;
	case scast<int>( GimmickKind::TriggerSwitch ) :
		InitSwitch();
		break;
	case scast<int>( GimmickKind::TriggerPull ) :
		InitPull();
		break;
	default: return;
	}
}
void Trigger::Uninit()
{
	switch ( kind )
	{
	case scast<int>( GimmickKind::TriggerKey ) :
		UninitKey();
		break;
	case scast<int>( GimmickKind::TriggerSwitch ) :
		UninitSwitch();
		break;
	case scast<int>( GimmickKind::TriggerPull ) :
		UninitPull();
		break;
	default: return;
	}
}

void Trigger::Update( float elapsedTime )
{
	switch ( kind )
	{
	case scast<int>( GimmickKind::TriggerKey ) :
		UpdateKey( elapsedTime );
		break;
	case scast<int>( GimmickKind::TriggerSwitch ) :
		UpdateSwitch( elapsedTime );
		break;
	case scast<int>( GimmickKind::TriggerPull ) :
		UpdatePull( elapsedTime );
		break;
	default: return;
	}
}
void Trigger::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	switch ( kind )
	{
	case scast<int>( GimmickKind::TriggerKey ) :
		PhysicUpdateKey( player, accompanyBox, terrains );
		break;
	case scast<int>( GimmickKind::TriggerSwitch ) :
		PhysicUpdateSwitch( player, accompanyBox, terrains );
		break;
	case scast<int>( GimmickKind::TriggerPull ) :
		PhysicUpdatePull( player, accompanyBox, terrains );
		break;
	default: return;
	}
}

#if DEBUG_MODE
#include "Donya/GeometricPrimitive.h"
#include "Common.h"
#endif // DEBUG_MODE
void Trigger::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	auto DrawSwitch = [&]()
	{
		enum DrawParts
		{
			Base = 0,
			Left,
			Right,
			Area,

			DrawCount
		};

		// This hit-boxes should be contain like this: [0]:left, [1]:right, [2]:area.
		const auto anotherHitBoxes = GetAnotherHitBoxes();
		_ASSERT_EXPR( anotherHitBoxes.size() == scast<size_t>( DrawCount - 1 ), L"LogicalError : The switch's configuration parts count is not matching!" );

		const AABBEx wsHitBoxes[DrawCount]
		{
			GetHitBox(),
			anotherHitBoxes[0],
			anotherHitBoxes[1],
			anotherHitBoxes[2],
		};
		const Donya::Vector4 colors[DrawCount]
		{
			{ 0.8f, 0.8f, 0.8f, 0.8f },
			{ 0.8f, 0.8f, 0.8f, 0.8f },
			{ 0.8f, 0.8f, 0.8f, 0.8f },
			{ 1.0f, 1.0f, 1.0f, 0.0f },
		};
		const Donya::Vector4 lightenFactors[DrawCount]
		{
			{ 0.2f, 0.2f, 0.2f, 0.2f },
			{ 0.2f, 0.2f, 0.2f, 0.2f },
			{ 0.2f, 0.2f, 0.2f, 0.2f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
		};

		constexpr int BASE_INDEX = 0;

		Donya::Vector4		color	=  colors[BASE_INDEX];
		if ( IsEnable() ) { color	+= lightenFactors[BASE_INDEX]; }

		Donya::Vector4x4	W		=  GetWorldMatrix( wsHitBoxes[BASE_INDEX], /* useDrawing = */ true, /* enableRotation = */ true );
		W._41 = pos.x; // Discard the offset of hit-box for draw.
		W._42 = pos.y; // Discard the offset of hit-box for draw.
		W._43 = pos.z; // Discard the offset of hit-box for draw.
		Donya::Vector4x4	VP		=  V * P;
		Donya::Vector4x4	WVP		=  W * VP;

		BaseDraw( WVP, W, lightDir, color );

	#if DEBUG_MODE
		if ( Common::IsShowCollision() )
		{
			static Donya::Geometric::Cube cube = Donya::Geometric::CreateCube();

			// Donya::Quaternion rotation{};
			// Donya::Vector4x4  R{};
			for ( int i = BASE_INDEX + 1; i < DrawCount; ++i )
			{
				W = Donya::Vector4x4::Identity();

				// rotation = Donya::Quaternion::Make( Donya::Vector3::Front(), ToRadian( rollDegree ) );
				// R = rotation.RequireRotationMatrix();
				W._11 = wsHitBoxes[i].size.x;
				W._22 = wsHitBoxes[i].size.y;
				W._33 = wsHitBoxes[i].size.z;
				// W *= R;
				W._41 = wsHitBoxes[i].pos.x;
				W._42 = wsHitBoxes[i].pos.y;
				W._43 = wsHitBoxes[i].pos.z;

				WVP = W * VP;

				color = colors[i];
				if ( IsEnable() ) { color += lightenFactors[i]; }

				cube.Render
				(
					nullptr,
					/* useDefaultShading	= */ true,
					/* isEnableFill			= */ true,
					WVP, W, lightDir, color
				);
			}
		}
	#endif // DEBUG_MODE
	};
	auto DrawOther  = [&]()
	{
		Donya::Vector4x4 W = GetWorldMatrix( GetHitBox(), /* useDrawing = */ true );
		Donya::Vector4x4 WVP = W * V * P;

		constexpr Donya::Vector4 colors[]
		{
			{ 0.8f, 0.8f, 0.8f, 0.8f },		// Key
			{},								// Switch
			{ 0.8f, 0.8f, 0.8f, 0.8f }		// Pull
		};
		constexpr Donya::Vector4 lightenFactors[]
		{
			{ 0.2f, 0.2f, 0.2f, 0.2f },		// Key
			{},								// Switch
			{ 0.2f, 0.2f, 0.2f, 0.2f }		// Pull
		};
		const int kindIndex  = GetTriggerKindIndex();

		Donya::Vector4 color = colors[kindIndex];
		if ( IsEnable() ) { color += lightenFactors[kindIndex]; }

		BaseDraw( WVP, W, lightDir, color );
	};

	if ( GimmickUtility::ToKind( kind ) == GimmickKind::TriggerSwitch )
	{
		DrawSwitch();
	}
	else
	{
		DrawOther();
	}
}

void Trigger::WakeUp()
{
	// No op.
}

bool Trigger::ShouldRemove() const
{
	return false;
}

Donya::Vector3 Trigger::GetPosition() const
{
	return pos;
}
AABBEx Trigger::GetHitBox() const
{
	const auto &param = ParamTrigger::Get().Data();
	const AABBEx hitBoxes[]
	{
		param.hitBoxKey,
		RollHitBox( param.hitBoxSwitch ),
		param.hitBoxPull
	};
	const bool hitBoxExists[]
	{
		false,	// Key
		true,	// Switch
		false	// Pull
	};
	const int kindIndex =  GetTriggerKindIndex();

	AABBEx wsHitBox		=  hitBoxes[kindIndex];
	wsHitBox.pos		+= pos;
	wsHitBox.velocity	=  velocity;
	wsHitBox.exist		=  hitBoxExists[kindIndex];
	wsHitBox.attr		=  kind;
	return wsHitBox;
}
bool Trigger::HasMultipleHitBox() const
{
	return ( GimmickUtility::ToKind( kind ) == GimmickKind::TriggerSwitch ) ? true : false;
}
std::vector<AABBEx> Trigger::GetAnotherHitBoxes() const
{
	const auto &param = ParamTrigger::Get().Data();
	const AABBEx hitBoxes[]
	{
		RollHitBox( param.mSwitch.hitBoxLeft ),
		RollHitBox( param.mSwitch.hitBoxRight ),
		param.mSwitch.gatheringArea
	};

	auto ToWorldSpace = [&]( AABBEx lsHitBox )
	{
		lsHitBox.pos		+= pos;
		lsHitBox.velocity	=  velocity;
		lsHitBox.exist		=  true;
		lsHitBox.attr		=  kind;
		return lsHitBox;
	};

	std::vector<AABBEx> multiHitBoxes{};
	multiHitBoxes.reserve( ArraySize( hitBoxes ) );

	bool isGathering = false;
	for ( const auto &it : hitBoxes )
	{
		isGathering = IsGatherBox( it ); // Must be judge before ToWorldSpace().
		multiHitBoxes.emplace_back( ToWorldSpace( it ) );

		if ( isGathering )
		{
			multiHitBoxes.back() = ToGatherBox( multiHitBoxes.back() );
		}
	}

	return multiHitBoxes;
}

int Trigger::GetTriggerKindIndex() const
{
	const int kindIndex = kind - scast<int>( GimmickKind::TriggerKey );
	_ASSERT_EXPR( 0 <= kindIndex, L"Error : A trigger's kind is invalid!" );
	return kindIndex;
}

Donya::Vector4x4 Trigger::GetWorldMatrix( const AABBEx &inputBox, bool useDrawing, bool enableRotation ) const
{
	auto wsBox = inputBox;
	if ( useDrawing )
	{
		// The AABB size is half, but drawing object's size is whole.
		// wsBox.size *= 2.0f;
	}

	const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Front(), ToRadian( rollDegree ) );
	const Donya::Vector4x4 R = rotation.RequireRotationMatrix();
	Donya::Vector4x4 mat{};
	const float drawScales[]
	{
		ParamTrigger::Get().Data().mKey.drawScale,		// Key
		ParamTrigger::Get().Data().mSwitch.drawScale,	// Switch
		ParamTrigger::Get().Data().mPull.drawScale		// Pull
	};
	const int kindIndex = GetTriggerKindIndex();

	mat._11 =
	mat._22 =
	mat._33 = drawScales[kindIndex];

	if ( enableRotation ) { mat *= R; }

	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

AABBEx Trigger::RollHitBox( AABBEx box ) const
{
	const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Front(), ToRadian( rollDegree ) );

	auto Rotate = [&rotation]( Donya::Vector3 *pVec )
	{
		*pVec = rotation.RotateVector( *pVec );
	};

	Rotate( &box.pos		);
	Rotate( &box.size		);
	Rotate( &box.velocity	);

	box.size.x = fabsf( box.size.x );
	box.size.y = fabsf( box.size.y );

	return box;
}

void Trigger::InitKey()
{

}
void Trigger::InitSwitch()
{

}
void Trigger::InitPull()
{
	mPull.initPos = pos;
}

void Trigger::UninitKey()
{

}
void Trigger::UninitSwitch()
{

}
void Trigger::UninitPull()
{

}

void Trigger::UpdateKey( float elapsedTime )
{

}
void Trigger::UpdateSwitch( float elapsedTime )
{

}
void Trigger::UpdatePull( float elapsedTime )
{

}

void Trigger::PhysicUpdateKey( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains )
{
	GimmickBase::PhysicUpdate( player, accompanyBox, terrains, /* collideToPlayer = */ false, /* ignoreHitBoxExist = */ true );

	if ( !IsEnable() && Donya::Box::IsHitBox( player, GetHitBox().Get2D(), /* ignoreHitBoxExist = */ true ) )
	{
		TurnOn();
	}
}
void Trigger::PhysicUpdateSwitch( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains )
{
	if ( IsEnable() ) { return; }
	// else

	BoxEx triggerArea = BoxEx::Nil();
	{
		const auto anotherBoxes = GetAnotherHitBoxes();
		for ( const auto &it : anotherBoxes )
		{
			if ( !IsGatherBox( it ) ) { continue; }
			// else
			triggerArea = it.Get2D();
		}
	
		if ( triggerArea == BoxEx::Nil() ) { return; }
		// else
	}

	std::vector<BoxEx> correspondingBoxes{};
	{
		for ( const auto &it : terrains )
		{
			if ( !GimmickUtility::HasAttribute( GimmickKind::SwitchBlock, it ) ) { continue; }
			// else

			correspondingBoxes.emplace_back( it );
		}
	}

	for ( const auto &it : correspondingBoxes )
	{
		if ( !Donya::Box::IsHitBox( triggerArea, it, /* ignoreExistFlag = */ true ) ) { continue; }
		// else

		TurnOn();
		break;
	}
}
void Trigger::PhysicUpdatePull( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains )
{
	GimmickBase::PhysicUpdate( player, accompanyBox, terrains, /* collideToPlayer = */ false, /* ignoreHitBoxExist = */ true );

	// Restrict the direction of stretch.
	pos.x = mPull.initPos.x;

	const Donya::Vector3 stretched = pos - mPull.initPos;
	const float stretchMax = ParamTrigger::Get().Data().mPull.stretchMax;
	if ( stretchMax < stretched.Length() )
	{
		pos = mPull.initPos + ( stretched.Normalized() * stretchMax );

		if ( !IsEnable() ) { TurnOn(); }
	}
}

void Trigger::TurnOn()
{
	enable = true;
	GimmickStatus::Register( id, true );
	Donya::Sound::Play( Music::GetKey );
}

#if USE_IMGUI

void Trigger::ShowImGuiNode()
{
	using namespace GimmickUtility;

	ImGui::Text( u8"種類：%d[%s]", kind, ToString( ToKind( kind ) ).c_str() );
	ImGui::DragFloat ( u8"Ｚ軸回転量",	&rollDegree,	1.0f	);
	ImGui::DragFloat3( u8"ワールド座標",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI
