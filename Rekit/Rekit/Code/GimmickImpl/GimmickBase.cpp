#include "GimmickBase.h"

#include "Donya/Useful.h"	// Use SignBit(), ZeroEqual().
#include "Donya/Sound.h"

#if DEBUG_MODE
#include "Donya/GeometricPrimitive.h"	// Use for drawing a collision.
#endif // DEBUG_MODE

#include "Common.h"
#include "Music.h"
#include "GimmickUtil.h"

using namespace GimmickUtility;

GimmickBase::GimmickBase() :
	kind(), rollDegree(), pos(), velocity(), wasCompressed( false )
{}
GimmickBase::~GimmickBase() = default;

void GimmickBase::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	std::vector<BoxEx> wholeCollisions = terrains;
	if ( collideToPlayer )
	{
		wholeCollisions.emplace_back( player );
	}

	auto CalcCollidingBox = [&]( const BoxEx &myself, const BoxEx &previousMyself )->BoxEx
	{
		for ( const auto &it : wholeCollisions )
		{
			if ( it.mass < myself.mass ) { continue; }
			if ( it == previousMyself  ) { continue; }
			// else

			if ( Donya::Box::IsHitBox( it, myself, ignoreHitBoxExist ) )
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

	if ( Donya::Box::IsHitBox( accompanyBox, previousXYBody, ignoreHitBoxExist ) )
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
			: 0.0f;
		penetration.y
			= ( moveSign.y < 0.0f ) ? minusPenetration.y
			: ( moveSign.y > 0.0f ) ? plusPenetration.y
			: 0.0f;

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
			Donya::Vector2 influence{};
			enum Dir { Up = 1, Down = -1 };
			int  verticalSign = Donya::SignBit( velocity.y );
			if ( verticalSign == Down )
			{
				influence = HasInfluence( other );
			}

			movedXYBody.pos.y += resolver.y;
			velocity.y = 0.0f;
			moveSign.y = scast<float>( Donya::SignBit( resolver.y ) );

			pushDirection = Donya::Vector2{ 0.0f, moveSign.y };

			if ( !influence.IsZero() )
			{
				movedXYBody.pos += influence;
				const Donya::Int2 signs
				{
					Donya::SignBit( influence.x ),
					Donya::SignBit( influence.y )
				};
				if ( signs.x != 0 ) { moveSign.x = scast<float>( signs.x ); }
				if ( signs.y != 0 ) { moveSign.y = scast<float>( signs.y ); }
			}
		}
		else // if ( !ZeroEqual( penetration.x ) ) is same as above this : " || ZeroEqual( penetration.x ) "
		{
			movedXYBody.pos.x += resolver.x;
			velocity.x = 0.0f;
			moveSign.x = scast<float>( Donya::SignBit( resolver.x ) );

			pushDirection = Donya::Vector2{ moveSign.x, 0.0f };
		}

		if ( allowCompress && JudgeWillCompressed( pushDirection ) )
		{
			Donya::Sound::Play( Music::Insert );
			wasCompressed = true;
		}
	}

	pos.x = movedXYBody.pos.x;
	pos.y = movedXYBody.pos.y;
}

void GimmickBase::BaseDraw( const Donya::Vector4x4 &matWVP, const Donya::Vector4x4 &matW, const Donya::Vector4 &lightDir, const Donya::Vector4 &materialColor ) const
{
	Donya::StaticMesh *pModel = GetModelAddress( ToKind( kind ) );
	if ( !pModel ) { return; }
	// else

	pModel->Render
	(
		nullptr,
		/* useDefaultShading	= */ true,
		/* isEnableFill			= */ true,
		matWVP, matW, lightDir, materialColor
	);

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static Donya::Geometric::Cube cube = Donya::Geometric::CreateCube();

		cube.Render
		(
			nullptr,
			/* useDefaultShading	= */ true,
			/* isEnableFill			= */ true,
			matWVP, matW, lightDir, materialColor
		);
	}
#endif // DEBUG_MODE
}

int				GimmickBase::GetKind()		const { return kind;	}
Donya::Vector3	GimmickBase::GetPosition()	const { return pos;		}

bool GimmickBase::HasMultipleHitBox() const { return false; }
std::vector<AABBEx> GimmickBase::GetAnotherHitBoxes() const
{
	return std::vector<AABBEx>();
}
