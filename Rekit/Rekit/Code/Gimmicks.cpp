#include "Gimmicks.h"

#include <array>			// Use at collision.
#include <algorithm>		// Use std::remove_if.
#include <map>
#include <vector>			// use at collision.

#include "Donya/GeometricPrimitive.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.
#include "Donya/Sound.h"

#include "Common.h"
#include "FilePath.h"
#include "Music.h"

#undef max
#undef min

namespace GimmickUtility
{
	int ToInt( GimmickKind kind )
	{
		return scast<int>( kind );
	}
	GimmickKind ToKind( int kind )
	{
		if ( kind < 0 || ToInt( GimmickKind::GimmicksCount ) <= kind )
		{
			_ASSERT_EXPR( 0, L"Error : An out of range detected!" );
			return GimmickKind::GimmicksCount;
		}
		// else
		return scast<GimmickKind>( kind );
	}
	std::string ToString( GimmickKind kind )
	{
		switch ( kind )
		{
		case GimmickKind::Fragile:			return "Fragile";		// break;
		case GimmickKind::Hard:				return "Hard";			// break;
		case GimmickKind::Ice:				return "Ice";			// break;
		case GimmickKind::Spike:			return "Spike";			// break;
		case GimmickKind::TriggerKey:		return "TriggerKey";	// break;
		case GimmickKind::TriggerSwitch:	return "TriggerSwitch";	// break;
		case GimmickKind::TriggerPull:		return "TriggerPull";	// break;
		case GimmickKind::Shutter:			return "Shutter";		// break;
		default: _ASSERT_EXPR( 0, L"Error : Unexpected kind detected!" ); break;
		}

		return "ERROR_KIND";
	}
}

namespace GimmickStatus
{
	static std::map<int, bool> statuses{};

	void Reset()
	{
		statuses.clear();
	}
	void Register( int id, bool configure )
	{
		auto found =  statuses.find( id );
		if ( found == statuses.end() )
		{
			statuses.insert( std::pair<int, bool>( id, configure ) );
		}
		else
		{
			found->second = configure;
		}
	}
	bool Refer( int id )
	{
		auto found =  statuses.find( id );
		if ( found == statuses.end() ) { return false; }
		// else

		return found->second;
	}
	void Remove( int id )
	{
		statuses.erase( id );
	}
}

using namespace GimmickUtility;

#pragma region Base

GimmickBase::GimmickBase() :
	kind(), pos(), velocity()
{}
GimmickBase::~GimmickBase() = default;

void GimmickBase::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist )
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

	const AABBEx actualBody		= GetHitBox();
	const BoxEx  previousXYBody	= actualBody.Get2D();

	if ( Donya::Box::IsHitBox( accompanyBox, previousXYBody, ignoreHitBoxExist ) )
	{
		// Following to "accompanyBox".
		// My velocity consider to be as accompanyBox's velocity.

		velocity.x = accompanyBox.velocity.x;
		velocity.y = accompanyBox.velocity.y;
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
		}
		else // if ( !ZeroEqual( penetration.x ) ) is same as above this : " || ZeroEqual( penetration.x ) "
		{
			movedXYBody.pos.x += resolver.x;
			velocity.x = 0.0f;
			moveSign.x = scast<float>( Donya::SignBit( resolver.x ) );
		}

	}

	pos.x = movedXYBody.pos.x;
	pos.y = movedXYBody.pos.y;
}

void GimmickBase::BaseDraw( const Donya::Vector4x4 &matWVP, const Donya::Vector4x4 &matW, const Donya::Vector4 &lightDir, const Donya::Vector4 &materialColor ) const
{
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

// region Base
#pragma endregion

#pragma region Gimmick

bool Gimmick::HasSlipAttribute( const BoxEx  &gimmick )
{
	return ( ToKind( gimmick.attr ) == GimmickKind::Ice ) ? true : false;
}
bool Gimmick::HasSlipAttribute( const AABBEx &gimmick )
{
	return ( ToKind( gimmick.attr ) == GimmickKind::Ice ) ? true : false;
}

bool Gimmick::HasDangerAttribute( const BoxEx &gimmick )
{
	return ( ToKind( gimmick.attr ) == GimmickKind::Spike ) ? true : false;
}
bool Gimmick::HasDangerAttribute( const AABBEx &gimmick )
{
	return ( ToKind( gimmick.attr ) == GimmickKind::Spike ) ? true : false;
}

Gimmick::Gimmick() :
	stageNo(), pGimmicks()
{}
Gimmick::~Gimmick() = default;

void Gimmick::Init( int stageNumber )
{
	FragileBlock::ParameterInit();
	HardBlock::ParameterInit();
	IceBlock::ParameterInit();
	SpikeBlock::ParameterInit();
	Trigger::ParameterInit();
	Shutter::ParameterInit();

	LoadParameter();

	stageNo = stageNumber;
}
void Gimmick::Uninit()
{
	pGimmicks.clear();
}

void Gimmick::Update( float elapsedTime )
{
#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

	for ( auto &it : pGimmicks )
	{
		if ( !it ) { continue; }
		// else

		it->Update( elapsedTime );
	}
}
void Gimmick::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains )
{
	const size_t blockCount = pGimmicks.size();

	auto ToBox = []( const AABBEx &aabb )
	{
		BoxEx box{};
		box.pos.x		= aabb.pos.x;
		box.pos.y		= aabb.pos.y;
		box.size.x		= aabb.size.x;
		box.size.y		= aabb.size.y;
		box.velocity.x	= aabb.velocity.x;
		box.velocity.y	= aabb.velocity.y;
		box.exist		= aabb.exist;
		box.mass		= aabb.mass;
		return box;
	};

	// The "pGimmicks" will update at PhysicUpdate().
	// So I prepare a temporary vector of terrains and update this every time update elements.
	std::vector<BoxEx> boxes{ blockCount }; // Necessary for AABB to Box.
	for ( size_t i = 0; i < blockCount; ++i )
	{
		if ( !pGimmicks[i] ) { continue; }
		// else

		boxes[i] = ToBox( pGimmicks[i]->GetHitBox() );
	}

	std::vector<BoxEx> allTerrains = boxes; // [blocks][terrains]
	allTerrains.insert( allTerrains.end(), terrains.begin(), terrains.end() );

	for ( size_t i = 0; i < blockCount; ++i )
	{
		if ( !pGimmicks[i] ) { continue; }
		// else

		pGimmicks[i]->PhysicUpdate( player, accompanyBox, allTerrains );
		allTerrains[i] = ToBox( pGimmicks[i]->GetHitBox() );
	}

	// Erase the should remove blocks.
	{
		auto itr = std::remove_if
		(
			pGimmicks.begin(), pGimmicks.end(),
			[]( std::unique_ptr<GimmickBase> &pElement )
			{
				return ( !pElement ) ? false : pElement->ShouldRemove();
			}
		);
		pGimmicks.erase( itr, pGimmicks.end() );
	}
}

void Gimmick::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	for ( auto &it : pGimmicks )
	{
		if ( !it ) { continue; }
		// else

		it->Draw( V, P, lightDir );
	}
}

std::vector<AABBEx> Gimmick::RequireHitBoxes() const
{
	std::vector<AABBEx> boxes{};
	for ( const auto &it : pGimmicks )
	{
		if ( !it ) { continue; }
		// else

		boxes.emplace_back( it->GetHitBox() );
	}
	return boxes;
}

void Gimmick::LoadParameter( bool fromBinary )
{
	std::string filePath = GenerateSerializePath( SERIAL_ID, fromBinary );
	Donya::Serializer::Load( *this, filePath.c_str(), SERIAL_ID, fromBinary );
}

#if USE_IMGUI

void Gimmick::SaveParameter()
{
	bool useBinary = true;
	std::string filePath{};

	filePath = GenerateSerializePath( SERIAL_ID, useBinary );
	Donya::Serializer::Save( *this, filePath.c_str(), SERIAL_ID, useBinary );

	useBinary = false;

	filePath = GenerateSerializePath( SERIAL_ID, useBinary );
	Donya::Serializer::Save( *this, filePath.c_str(), SERIAL_ID, useBinary );
}

void Gimmick::UseImGui()
{
	FragileBlock::UseParameterImGui();
	HardBlock::UseParameterImGui();
	IceBlock::UseParameterImGui();
	SpikeBlock::UseParameterImGui();
	Trigger::UseParameterImGui();
	Shutter::UseParameterImGui();

	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"ギミック" ) )
		{
			static Donya::Vector3 shutterDirection;
			ImGui::SliderFloat3 ( u8"シャッターの向き", &shutterDirection.x, -1, 1 );
			// Resizing.
			{
				const std::string prefix{ u8"末尾に追加・" };

				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Fragile ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_unique<FragileBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::Fragile ), Donya::Vector3::Zero() );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Hard ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_unique<HardBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::Hard ), Donya::Vector3::Zero() );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Ice ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_unique<IceBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::Ice ), Donya::Vector3::Zero() );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Spike ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_unique<SpikeBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::Spike ), Donya::Vector3::Zero() );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::TriggerKey ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_unique<Trigger>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::TriggerKey ), Donya::Vector3::Zero() );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::TriggerSwitch ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_unique<Trigger>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::TriggerSwitch ), Donya::Vector3::Zero() );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::TriggerPull ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_unique<Trigger>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::TriggerPull ), Donya::Vector3::Zero() );
				}
				if (ImGui::Button ( (prefix + ToString ( GimmickKind::Shutter )).c_str () ))
				{
					pGimmicks.push_back ( std::make_unique<Shutter> ( 0, shutterDirection ) );
					pGimmicks.back ()->Init ( ToInt ( GimmickKind::Shutter ), Donya::Vector3::Zero () );
				}
				/*
				if ( ImGui::Button( ( prefix + ToString( GimmickKind:: ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_unique<XXX>() );
					pGimmicks.back()->Init( ToInt( GimmickKind:: ), Donya::Vector3::Zero() );
				}
				*/

				if ( pGimmicks.empty() )
				{
					// Align a line.
					ImGui::Text( "" );
				}
				else if ( ImGui::Button( u8"末尾を削除" ) )
				{
					pGimmicks.pop_back();
				}
			}

			// Show parameter nodes.
			{
				int i = 0;
				std::string caption{};
				for ( auto it = pGimmicks.begin(); it != pGimmicks.end(); )
				{
					bool doRemove = false;
					std::unique_ptr<GimmickBase> &elem = *it;

					if ( !elem ) { continue; }
					// else

					caption = ToString( ToKind( elem->GetKind() ) ) + "[" + std::to_string( i++ ) + "]";
					if ( ImGui::TreeNode( caption.c_str() ) )
					{
						if ( ImGui::Button( u8"取り除く" ) ) { doRemove = true; }

						elem->ShowImGuiNode();
						ImGui::TreePop();
					}

					if ( doRemove )
					{
						it = pGimmicks.erase( it );
						continue;
					}
					// else

					++it;
				}
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

// region Gimmick
#pragma endregion
