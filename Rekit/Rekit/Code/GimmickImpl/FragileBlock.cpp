#include "FragileBlock.h"

#include <algorithm>		// Use std::max, min.
#include <string>

#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "FilePath.h"
#include "Music.h"

#undef max
#undef min

struct ParamFragileBlock final : public Donya::Singleton<ParamFragileBlock>
{
	friend Donya::Singleton<ParamFragileBlock>;
public:
	struct Member
	{
		float	drawScale{ 1.0f };
		float	gravity{};
		float	maxFallSpeed{};
		float	brakeSpeed{};		// Affect to inverse speed of current velocity(only X-axis).
		float	stopThreshold{};	// The threshold of a judge to stop instead of the brake.
		AABBEx	hitBox{};			// Hit-Box of using to the collision to the stage.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( hitBox )
			);
			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( gravity ),
					CEREAL_NVP( maxFallSpeed )
				);
			}
			if ( 2 <= version )
			{
				archive
				(
					CEREAL_NVP( brakeSpeed ),
					CEREAL_NVP( stopThreshold )
				);
			}
			if ( 3 <= version )
			{
				archive( CEREAL_NVP( drawScale ) );
			}
			if ( 4 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "FragileBlock";
	Member m;
private:
	ParamFragileBlock() : m() {}
public:
	~ParamFragileBlock() = default;
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
			if ( ImGui::TreeNode( u8"ギミック[Fragile]・調整データ" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"質量" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox  ( ( prefix + u8"当たり判定は有効か" ).c_str(), &pHitBox->exist );
				};

				ImGui::DragFloat( u8"描画スケール",			&m.drawScale,		0.1f	);
				ImGui::DragFloat( u8"重力加速度",			&m.gravity,			0.1f	);
				ImGui::DragFloat( u8"最大落下速度",			&m.maxFallSpeed,	0.1f	);
				ImGui::DragFloat( u8"ブレーキ速度（Ｘ軸）",	&m.brakeSpeed,		0.5f	);
				ImGui::DragFloat( u8"停止する閾値（Ｘ軸）",	&m.stopThreshold,	0.1f	);

				AdjustAABB( u8"当たり判定", &m.hitBox );

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
CEREAL_CLASS_VERSION( ParamFragileBlock::Member, 3 )

void FragileBlock::ParameterInit()
{
	ParamFragileBlock::Get().Init();
}
#if USE_IMGUI
void FragileBlock::UseParameterImGui()
{
	ParamFragileBlock::Get().UseImGui();
}
#endif // USE_IMGUI

FragileBlock::FragileBlock() : GimmickBase()
{}
FragileBlock::~FragileBlock() = default;

void FragileBlock::Init( int gimmickKind, float roll, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	rollDegree	= roll;
	pos			= wsPos;
	velocity	= 0.0f;
}
void FragileBlock::Uninit()
{
	// No op.
}

void FragileBlock::Update( float elapsedTime )
{
	Fall( elapsedTime );

	Brake( elapsedTime );
}
void FragileBlock::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	GimmickBase::PhysicUpdate( player, accompanyBox, terrains, true, false, true );
	
	// Store also the player(player is not contain to terrains).
	// std::vector<BoxEx> wholeCollisions = terrains;
	// wholeCollisions.emplace_back( player );
	// AssignVelocity( accompanyBox, wholeCollisions );
}

void FragileBlock::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

	BaseDraw( WVP, W, lightDir, color );
}

void FragileBlock::WakeUp()
{
	// No op.
}

bool FragileBlock::ShouldRemove() const
{
	return wasCompressed;
}

Donya::Vector3 FragileBlock::GetPosition() const
{
	return pos;
}
AABBEx FragileBlock::GetHitBox() const
{
	AABBEx base = ParamFragileBlock::Get().Data().hitBox;
	base.pos		+= pos;
	base.velocity	=  velocity;
	base.attr		=  kind;
	return base;
}

Donya::Vector4x4 FragileBlock::GetWorldMatrix( bool useDrawing ) const
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
	mat._11 =
	mat._22 =
	mat._33 = ParamFragileBlock::Get().Data().drawScale;
	mat *= R;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

void FragileBlock::Fall( float elapsedTime )
{
	const auto DATA = ParamFragileBlock::Get().Data();
	velocity.y -= DATA.gravity * elapsedTime;
	velocity.y =  std::max( DATA.maxFallSpeed, velocity.y );
}

void FragileBlock::Brake( float elapsedTime )
{
	const float moveSign = scast<float>( Donya::SignBit( velocity.x ) );
	if ( ZeroEqual( moveSign ) ) { return; }
	// else

	const float nowSpeed = fabsf( velocity.x );
	if ( nowSpeed <= ParamFragileBlock::Get().Data().stopThreshold )
	{
		velocity.x = 0.0f;
		return;
	}
	// else

	const float brakeSpeed = std::min( nowSpeed, ParamFragileBlock::Get().Data().brakeSpeed );
	velocity.x -= brakeSpeed * moveSign;
}

void FragileBlock::AssignVelocity( const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains )
{
#if 1 // VER_4, Calc a penetration every colliding hit-boxes. Then resolve only lowest penetrating axis. Then recheck a collision.
	auto CalcCollidingBox = [&]( const BoxEx &myself, const BoxEx &previousMyself )->BoxEx
	{
		for ( const auto &it : terrains )
		{
			if ( it.mass < myself.mass ) { continue; }
			if ( it == previousMyself  ) { continue; }
			// else

			if ( Donya::Box::IsHitBox( it, myself ) )
			{
				return it;
			}
		}

		return BoxEx::Nil();
	};

	std::vector<Donya::Vector2> pushedDirections{}; // Store a normalized-vector of [wall->myself].
	// Returns true if it is determined to compressed. The "pushDir" expect {0, 1} or {1, 0}.
	auto JudgeWillCompressed = [&pushedDirections]( const Donya::Vector2 pushDir )->bool
	{
		pushedDirections.emplace_back( pushDir );
		if ( pushedDirections.size() < 2U ) { return false; } // The myself does not compress if a vectors count less than two.
		// else

		float angle{};
		for ( const auto &it : pushedDirections )
		{
			angle = Donya::Vector2::Dot( pushDir, it );
			if ( angle < 0.0f ) // If these direction is against.
			{
				return true;
			}
		}

		return false;
	};

	const AABBEx actualBody		= GetHitBox();
	const BoxEx  previousXYBody	= actualBody.Get2D();

	if ( Donya::Box::IsHitBox( accompanyBox, previousXYBody ) )
	{
		// Following to "accompanyBox".
		// My velocity consider to be as accompanyBox's velocity.

		velocity.x = accompanyBox.velocity.x;
		velocity.y = accompanyBox.velocity.y;

		// The "accompanyBox" is external factor.
		// But I regard as that is not contained to "terrains",
		// so I should register to a list of compress-factors("pushedDirections") here.

		const Donya::Int2 moveSign
		{
			Donya::SignBit( accompanyBox.velocity.x ),
			Donya::SignBit( accompanyBox.velocity.y )
		};
		if ( moveSign.x != 0 )
		{
			pushedDirections.emplace_back( Donya::Vector2{ scast<float>( moveSign.x ), 0.0f } );
		}
		if ( moveSign.y != 0 )
		{
			pushedDirections.emplace_back( Donya::Vector2{ 0.0f, scast<float>( moveSign.y ) } );
		}
	}

	Donya::Vector2 xyVelocity{ velocity.x, velocity.y };
	Donya::Vector2 moveSign // The moving direction of myself. Take a value of +1.0f or -1.0f.
	{
		scast<float>( Donya::SignBit( xyVelocity.x ) ),
		scast<float>( Donya::SignBit( xyVelocity.y ) )
	};

	BoxEx movedXYBody = previousXYBody;
	movedXYBody.pos  += xyVelocity;

	BoxEx other{};

	constexpr unsigned int MAX_LOOP_COUNT = 1000U;
	unsigned int loopCount{};
	while ( ++loopCount < MAX_LOOP_COUNT )
	{
		other = CalcCollidingBox( movedXYBody, previousXYBody );
		if ( other == BoxEx::Nil() ) { break; } // Does not detected a collision.
		// else

		if ( other.mass < movedXYBody.mass ) { continue; }
		// else

		if ( ZeroEqual( moveSign.x ) && !ZeroEqual( other.velocity.x ) )
		{
			// The myself's moving direction is considered the inverse of other's moving direction.
			moveSign.x = scast<float>( Donya::SignBit( -other.velocity.x ) );
		}
		if ( ZeroEqual( moveSign.y ) && !ZeroEqual( other.velocity.y ) )
		{
			// The myself's moving direction is considered the inverse of other's moving direction.
			moveSign.y = scast<float>( Donya::SignBit( -other.velocity.y ) );
		}

		if ( moveSign.IsZero() ) { continue; } // Each other does not move, so collide is no possible.
		// else

		Donya::Vector2 penetration{}; // Store absolute value.
		Donya::Vector2 plusPenetration
		{
			fabsf( ( movedXYBody.pos.x + movedXYBody.size.x ) - ( other.pos.x - other.size.x ) ),
			fabsf( ( movedXYBody.pos.y + movedXYBody.size.y ) - ( other.pos.y - other.size.y ) )
		};
		Donya::Vector2 minusPenetration
		{
			fabsf( ( movedXYBody.pos.x - movedXYBody.size.x ) - ( other.pos.x + other.size.x ) ),
			fabsf( ( movedXYBody.pos.y - movedXYBody.size.y ) - ( other.pos.y + other.size.y ) )
		};
		penetration.x
			= ( moveSign.x < 0.0f ) ? minusPenetration.x
			: ( moveSign.x > 0.0f ) ? plusPenetration.x
			: 0.0f; // std::min( minusPenetration.x, plusPenetration.x );
		penetration.y
			= ( moveSign.y < 0.0f ) ? minusPenetration.y
			: ( moveSign.y > 0.0f ) ? plusPenetration.y
			: 0.0f; // std::min( minusPenetration.y, plusPenetration.y );

		// Safety. Prevent the penetration to be too big.
		// if ( fabsf( xyVelocity.x ) < penetration.x ) { penetration.x = fabsf( xyVelocity.x ); }
		// if ( fabsf( xyVelocity.y ) < penetration.y ) { penetration.y = fabsf( xyVelocity.y ); }

		constexpr float ERROR_MARGIN = 0.0001f; // Prevent the two edges onto same place(the collision detective allows same(equal) value).

		Donya::Vector2 pushDirection{};
		Donya::Vector2 resolver
		{
			( penetration.x + ERROR_MARGIN ) * -moveSign.x,
			( penetration.y + ERROR_MARGIN ) * -moveSign.y
		};

		// Repulse to the more little(but greater than zero) axis side of penetration.
		if ( penetration.y < penetration.x || ZeroEqual( penetration.x ) )
		{
			movedXYBody.pos.y += resolver.y;
			velocity.y = 0.0f;
			moveSign.y = scast<float>( Donya::SignBit( resolver.y ) );

			pushDirection = Donya::Vector2{ 0.0f, moveSign.y };
		}
		else // if ( !ZeroEqual( penetration.x ) ) is same as above this : " || ZeroEqual( penetration.x ) "
		{
			movedXYBody.pos.x += resolver.x;
			velocity.x = 0.0f;
			moveSign.x = scast<float>( Donya::SignBit( resolver.x ) );

			pushDirection = Donya::Vector2{ moveSign.x, 0.0f };
		}

		if ( JudgeWillCompressed( pushDirection ) )
		{
			Donya::Sound::Play( Music::Insert );
			wasCompressed = true;
			break; // Break from hit-boxes loop.
		}
		// else
	}

	pos.x = movedXYBody.pos.x;
	pos.y = movedXYBody.pos.y;

#elif 0 // VER_3, Calc a penetration every colliding hit-boxes. Then recheck a collision.
	auto CalcCollidingBox = [&]( const BoxEx &myself, const BoxEx &previousMyself )->BoxEx
	{
		for ( const auto &it : terrains )
		{
			if ( it.mass < myself.mass ) { continue; }
			if ( it == previousMyself  ) { continue; }
			// else

			if ( Donya::Box::IsHitBox( it, myself ) )
			{
				return it;
			}
		}

		return BoxEx::Nil();
	};

	const AABBEx actualBody = GetHitBox();
	BoxEx previousXYBody{};
	{
		previousXYBody.pos.x		= actualBody.pos.x;
		previousXYBody.pos.y		= actualBody.pos.y;
		previousXYBody.size.x		= actualBody.size.x;
		previousXYBody.size.y		= actualBody.size.y;
		previousXYBody.velocity.x	= actualBody.velocity.x;
		previousXYBody.velocity.y	= actualBody.velocity.y;
		previousXYBody.exist		= actualBody.exist;
		previousXYBody.mass			= actualBody.mass;
	}

	Donya::Vector2 xyVelocity{ velocity.x, velocity.y };
	Donya::Vector2 moveSign // The moving direction of myself. Take a value of +1.0f or -1.0f.
	{
		scast<float>( Donya::SignBit( xyVelocity.x ) ),
		scast<float>( Donya::SignBit( xyVelocity.y ) )
	};

	BoxEx movedXYBody = previousXYBody;
	movedXYBody.pos  += xyVelocity;

	BoxEx other{};

	std::vector<Donya::Vector2> pushedDirections{}; // Store a normalized-vector of [wall->myself].
	// Returns true if it is determined to compressed. The "pushDir" expect {0, 1} or {1, 0}.
	auto JudgeWillCompressed = [&pushedDirections]( const Donya::Vector2 pushDir )->bool
	{
		pushedDirections.emplace_back( pushDir );
		if ( pushedDirections.size() < 2U ) { return false; } // The myself does not compress if a vectors count less than two.
		// else

		float angle{};
		for ( const auto &it : pushedDirections )
		{
			angle = Donya::Vector2::Dot( pushDir, it );
			if ( angle < 0.0f ) // If these direction is against.
			{
				return true;
			}
		}

		return false;
	};

	constexpr unsigned int MAX_LOOP_COUNT = 1000U;
	unsigned int loopCount{};
	bool    nowColliding = true;
	while ( nowColliding )
	{
		if ( MAX_LOOP_COUNT <= ++loopCount ) { break; }
		// else

		other = CalcCollidingBox( movedXYBody, previousXYBody );
		if ( other == BoxEx::Nil() ) { break; }
		// else

		if ( ZeroEqual( moveSign.x ) && !ZeroEqual( other.velocity.x ) )
		{
			// The myself's moving direction is considered the inverse of other's moving direction.
			moveSign.x = scast<float>( Donya::SignBit( -other.velocity.x ) );
		}
		if ( ZeroEqual( moveSign.y ) && !ZeroEqual( other.velocity.y ) )
		{
			// The myself's moving direction is considered the inverse of other's moving direction.
			moveSign.y = scast<float>( Donya::SignBit( -other.velocity.y ) );
		}

		Donya::Vector2 penetration{}; // Store absolute value.
		Donya::Vector2 plusPenetration
		{
			fabsf( ( movedXYBody.pos.x + movedXYBody.size.x ) - ( other.pos.x - other.size.x ) ),
			fabsf( ( movedXYBody.pos.y + movedXYBody.size.y ) - ( other.pos.y - other.size.y ) )
		};
		Donya::Vector2 minusPenetration
		{
			fabsf( ( movedXYBody.pos.x - movedXYBody.size.x ) - ( other.pos.x + other.size.x ) ),
			fabsf( ( movedXYBody.pos.y - movedXYBody.size.y ) - ( other.pos.y + other.size.y ) )
		};
		penetration.x
			= ( moveSign.x < 0.0f ) ? minusPenetration.x
			: ( moveSign.x > 0.0f ) ? plusPenetration.x
			: std::min( minusPenetration.x, plusPenetration.x );
		penetration.y
			= ( moveSign.y < 0.0f ) ? minusPenetration.y
			: ( moveSign.y > 0.0f ) ? plusPenetration.y
			: std::min( minusPenetration.y, plusPenetration.y );

		// Safety. Prevent the penetration to be too big.
		// if ( fabsf( xyVelocity.x ) < penetration.x ) { penetration.x = fabsf( xyVelocity.x ); }
		// if ( fabsf( xyVelocity.y ) < penetration.y ) { penetration.y = fabsf( xyVelocity.y ); }

		penetration += 0.0001f; // Prevent the two edges onto same place(the collision detective allows same(equal) value).

		Donya::Vector2 resolver
		{
			penetration.x * -moveSign.x,
			penetration.y * -moveSign.y
		};

		if ( Donya::SignBit( resolver.y ) != 0 ) { velocity.y = 0.0f; } // My preference.
		
		movedXYBody.pos += resolver;
		moveSign.x = scast<float>( Donya::SignBit( resolver.x ) );
		moveSign.y = scast<float>( Donya::SignBit( resolver.y ) );

		std::vector<Donya::Vector2> pushDirections{};
		Donya::Int2 resolveSign
		{
			Donya::SignBit( resolver.x ),
			Donya::SignBit( resolver.y )
		};
		if ( resolveSign.x == +1 ) { pushDirections.emplace_back( Donya::Vector2{ +1.0f, 0.0f } ); }
		if ( resolveSign.x == -1 ) { pushDirections.emplace_back( Donya::Vector2{ -1.0f, 0.0f } ); }
		if ( resolveSign.y == +1 ) { pushDirections.emplace_back( Donya::Vector2{ 0.0f, +1.0f } ); }
		if ( resolveSign.y == -1 ) { pushDirections.emplace_back( Donya::Vector2{ 0.0f, -1.0f } ); }

		for ( const auto &it : pushDirections )
		{
			if ( JudgeWillCompressed( it ) )
			{
				wasBroken = true;
				Donya::Sound::Play( Music::Insert );
				break; // Break from directions loop.
			}
		}
		if ( wasBroken ) { break; }
		// else
	}

	pos.x =  movedXYBody.pos.x;
	pos.y =  movedXYBody.pos.y;
	pos.z += velocity.z;		// Z-axis does not check a collision(unnecessary).

#elif 0 // VER_2, WIP, Calc intersection-point by a line of movement vs edges of terrains hit-box.
	auto CalcCollidingBox		= [&]( const BoxEx &prevMyself, const Donya::Vector2 &velocity )->BoxEx
	{		
		BoxEx movedMyself = prevMyself;
		movedMyself.pos  += velocity;

		for ( const auto &it : terrains )
		{
			if ( it.mass < movedMyself.mass ) { continue; }
			// else

			if ( Donya::Box::IsHitBox( it, movedMyself ) )
			{
				return it;
			}
		}

		return BoxEx::Nil();
	};
	auto CalcIntersectionPoint	= [&]( const BoxEx &prevMyself, const BoxEx &collideBox )->Donya::Line::Result
	{
		std::array<Donya::Line, 4> otherEdges{}; // [0:LT->RT][1:RT->RB][2:RB->LB][3:LB->LT]
		auto AssignEdges		= [&otherEdges]( const BoxEx &other )
		{
			Donya::Vector2 LT = other.pos - other.size;
			Donya::Vector2 LB = LT; LT.y += other.size.y * 2.0f;
			Donya::Vector2 RT = LT; RT.x += other.size.x * 2.0f;
			Donya::Vector2 RB = other.pos - other.size;

			otherEdges[0].pos = LT;
			otherEdges[0].vec = RT - otherEdges[0].pos;

			otherEdges[1].pos = RT;
			otherEdges[1].vec = RB - otherEdges[1].pos;

			otherEdges[2].pos = RB;
			otherEdges[2].vec = LB - otherEdges[2].pos;

			otherEdges[3].pos = LB;
			otherEdges[3].vec = LT - otherEdges[3].pos;
		};

		Donya::Line movement{};
		movement.pos			=	prevMyself.pos;
		movement.vec			=	Donya::Vector2{ velocity.x, velocity.y };

		// Convert to point vs AABB.
		Donya::Vector2 myPoint	=	prevMyself.pos;
		BoxEx exOther			=	collideBox; // Extended other(collideBox) by myself's size.
		exOther.size			+=	prevMyself.size;

		AssignEdges( exOther );

		Donya::Line::Result result{};
		for ( const auto &it : otherEdges )
		{
			result = Donya::Line::CalcIntersectionPoint( it, movement );
			if ( result.wasHit ) { break; }
		}
		
		return result;
	};

	Donya::Vector2 moveVelocity{ velocity.x, velocity.y };
	bool    wasHit = true;
	while ( wasHit )
	{
		const AABBEx actualBody = GetHitBox();
		BoxEx previousXYBody{}; // Use for check "a wall is myself?".
		{
			previousXYBody.pos.x		= actualBody.pos.x;
			previousXYBody.pos.y		= actualBody.pos.y;
			previousXYBody.size.x		= actualBody.size.x;
			previousXYBody.size.y		= actualBody.size.y;
			previousXYBody.velocity.x	= actualBody.velocity.x;
			previousXYBody.velocity.y	= actualBody.velocity.y;
			previousXYBody.exist		= actualBody.exist;
			previousXYBody.mass			= actualBody.mass;
		}

		BoxEx collideBox = CalcCollidingBox( previousXYBody, moveVelocity );

		wasHit = false;
	}

#else // VER_1, Apply the velocity and check the collision per axis.

	/// <summary>
	/// The "xyNAxis" is specify moving axis. please only set to { 1, 0 } or { 0, 1 }. This function  to be able to handle any axis.
	/// </summary>
	auto MoveSpecifiedAxis = [&]( Donya::Vector2 xyNAxis, float moveSpeed, const AABBEx &baseHitBox )->bool
	{
		bool corrected = false;

		BoxEx previousXYBody{}; // Use for check "a wall is myself?".
		{
			previousXYBody.pos.x		= baseHitBox.pos.x;
			previousXYBody.pos.y		= baseHitBox.pos.y;
			previousXYBody.size.x		= baseHitBox.size.x;
			previousXYBody.size.y		= baseHitBox.size.y;
			previousXYBody.velocity.x	= baseHitBox.velocity.x;
			previousXYBody.velocity.y	= baseHitBox.velocity.y;
			previousXYBody.exist		= true;
			previousXYBody.mass			= baseHitBox.mass;
		}

		// Only either X or Y is valid.
		const Donya::Vector2 xyVelocity = xyNAxis * moveSpeed;
		pos.x += xyVelocity.x;
		pos.y += xyVelocity.y;

		// The player's hit box of stage is circle, but doing with rectangle for easily correction.
		BoxEx xyBody{};
		{
			xyBody.pos.x	= GetPosition().x;
			xyBody.pos.y	= GetPosition().y;
			xyBody.size.x	= baseHitBox.size.x;
			xyBody.size.y	= baseHitBox.size.y;
			xyBody.exist	= true;
			xyBody.mass		= baseHitBox.mass;
		}
		Donya::Vector2 xyBodyCenter = xyBody.pos;
		Donya::Vector2 bodySize{ xyBody.size.x * xyNAxis.x, xyBody.size.y * xyNAxis.y }; // Only either X or Y is valid.
		const float bodyWidth = bodySize.Length(); // Extract valid member by Length().

		float moveSign{}; // The moving direction of myself. Take a value of +1 or -1.
		std::vector<Donya::Vector2> pushedDirections{}; // Store a normalized-vector of [wall->myself].

		// Returns true if it is determined to compressed.
		auto JudgeWillCompressed = [&pushedDirections]( const BoxEx &myself, const BoxEx &other, const Donya::Vector2 &xyNAxis, bool rejectLightOther = true )->bool
		{
			if ( rejectLightOther && other.mass < myself.mass ) { return false; }
			// else

			Donya::Vector2 currentPushDir = ( myself.pos - other.pos ).Normalized();
			currentPushDir.x *= xyNAxis.x;
			currentPushDir.y *= xyNAxis.y;

			pushedDirections.emplace_back( currentPushDir );
			if ( pushedDirections.size() < 2U ) { return false; } // The myself does not compress if a vectors count less than two.
			// else

			float angle{};
			for ( const auto &it : pushedDirections )
			{
				angle = Donya::Vector2::Dot( currentPushDir, it );
				if ( angle < 0.0f ) // If these direction is against.
				{
					Donya::Sound::Play( Music::Insert );
					
					return true;
				}
			}

			return false;
		};

		const size_t wallCount = terrains.size();
		int loop = 0;
		for ( size_t i = 0; i < wallCount; ++i )
		{
			loop++;
			const BoxEx &wall = terrains[i];

			if ( previousXYBody == wall ) { continue; } // The terrains contain also myself.
			if ( !Donya::Box::IsHitBox( xyBody, wall ) ) { continue; }
			// else

			moveSign = scast<float>( Donya::SignBit( xyVelocity.x ) + Donya::SignBit( xyVelocity.y ) );

			Donya::Vector2 xyWallCenter = wall.pos;
			Donya::Vector2 wallSize{ wall.size.x * xyNAxis.x, wall.size.y * xyNAxis.y }; // Only either X or Y is valid.
			Donya::Vector2 wallVelocity{ wall.velocity.x * xyNAxis.x, wall.velocity.y * xyNAxis.y }; // Only either X or Y is valid.
			float wallWidth = wallSize.Length();		// Extract valid member by Length().
			float wallSpeed = wallVelocity.Length();	// Extract valid member by Length().

			if ( ZeroEqual( moveSign ) )
			{
				// Usually I decide a repulse direction by moving direction("moveSign") of myself.
				// But I can not decide a repulse direction when the myself does not moving.
				// So use the other("wall")'s moving direction.

				// If the myself is heavier than wall, so the myself does not repulse.
				if ( wall.mass < xyBody.mass ) { continue; }
				// else

				// Each other does not move, it is not colliding movement of now axis.
				if ( ZeroEqual( wallSpeed )  )
				{
					wasBroken = JudgeWillCompressed( xyBody, wall, xyNAxis );

					// Break/Continue from hit-boxes loop.
					if ( wasBroken ) { break; }
					// else
					continue;
				}
				// else

				moveSign  = scast<float>( Donya::SignBit( wallVelocity.x ) + Donya::SignBit( wallVelocity.y ) );
				moveSign *= -1.0f; // This "moveSign" represent the moving direction of myself, so I should reverse.
				moveSpeed = wallSpeed * -moveSign;
			}

			wasBroken = JudgeWillCompressed( xyBody, wall, xyNAxis );
			if ( wasBroken ) { break; } // Break from hit-boxes loop.
			// else

			// Calculate colliding length.
			// First, calculate body's edge of moving side.
			// Then, calculate wall's edge of inverse moving side.
			// After that, calculate colliding length from two edges.
			// Finally, correct the position to inverse moving side only that length.

			Donya::Vector2 bodyEdge	= xyBodyCenter + ( xyNAxis * bodyWidth *  moveSign );
			Donya::Vector2 wallEdge	= xyWallCenter + ( xyNAxis * wallWidth * -moveSign );
			Donya::Vector2 diff		= bodyEdge - wallEdge;
			Donya::Vector2 axisDiff{ diff.x * xyNAxis.x, diff.y * xyNAxis.y };
			float collidingLength	= axisDiff.Length(); // Extract valid member by Length().
		
			Donya::Vector2 xyCorrection = xyNAxis * ( collidingLength * -moveSign );
			pos.x += xyCorrection.x;
			pos.y += xyCorrection.y;
			// Prevent the two edges onto same place(the collision detective allows same(equal) value).
			pos.x += 0.0001f * scast<float>( Donya::SignBit( xyCorrection.x ) );
			pos.y += 0.0001f * scast<float>( Donya::SignBit( xyCorrection.y ) );
			// We must apply the repulsed position to hit-box for next collision.
			xyBody.pos.x = GetPosition().x;
			xyBody.pos.y = GetPosition().y;

			corrected = true;

			// Recheck from the beginning with updated position.
			i = 0;
		}

		return corrected;
	};

	const AABBEx actualBody = GetHitBox();

	// Move to X-axis with collision.
	MoveSpecifiedAxis( Donya::Vector2{ 1.0f, 0.0f }, velocity.x, actualBody );

	// Move to Y-axis with collision.
	bool wasCorrected = MoveSpecifiedAxis( Donya::Vector2{ 0.0f, 1.0f }, velocity.y, actualBody );
	if ( wasCorrected )
	{
		velocity.y = 0.0f;
	}

	// Move to Z-axis only.
	pos.z += velocity.z;

#endif // VERSION
}

#if USE_IMGUI

void FragileBlock::ShowImGuiNode()
{
	ImGui::Text( u8"種類：%d[FragileBlock]", kind );
	ImGui::DragFloat ( u8"Ｚ軸回転量",	&rollDegree,	1.0f	);
	ImGui::DragFloat3( u8"ワールド座標",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI
