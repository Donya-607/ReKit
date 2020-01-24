#include "Gimmicks.h"

#include <array>			// Use at collision.
#include <algorithm>		// Use std::remove_if.
#include <map>
#include <vector>			// Use at collision, and load models.

#include "Donya/GeometricPrimitive.h"
#include "Donya/Loader.h"
#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "Common.h"
#include "FilePath.h"
#include "GimmickUtil.h"
#include "Music.h"
#include "SceneEditor.h"	// Use The "StageConfiguration".

#undef max
#undef min

using namespace GimmickUtility;

Gimmick::Gimmick() :
	stageNo(), pGimmicks()
{}
Gimmick::~Gimmick() = default;

void Gimmick::Init(int stageNumber, const StageConfiguration& stageConfig, const Donya::Vector3 &worldOffset )
{
	// LoadParameter(); // Currently didn't use serialize.

	stageNo = stageNumber;
	ApplyConfig( stageConfig, worldOffset );
}
void Gimmick::Uninit()
{
	pGimmicks.clear();
}

void Gimmick::Update( float elapsedTime, bool useImGui )
{
#if USE_IMGUI
	if ( useImGui )
	{
		UseImGui();
	}
#endif // USE_IMGUI

	for ( auto &it : pGimmicks )
	{
		if ( !it ) { continue; }
		// else

		it->Update( elapsedTime );
	}
}
void Gimmick::UpdateElevators(float elapsedTime)
{
	for (auto& it : pGimmicks)
	{
		if (GimmickUtility::ToKind(it->GetKind()) == GimmickKind::Elevator)
		{
			it->Update(elapsedTime);
		}
	}
}

void Gimmick::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains )
{
	// The "pGimmicks" will update at PhysicUpdate().
	// So I prepare a temporary vector of terrains and update this every time update elements.

	const size_t gimmickCount = pGimmicks.size();

	std::vector<BoxEx> boxes{};			// Contains main hit-boxes of all gimmicks.
	std::vector<BoxEx> anotherBoxes{};	// Contains another hit-boxes of all gimmicks.

	// Prepare the blocks hit-boxes.
	for ( size_t i = 0; i < gimmickCount; ++i )
	{
		const auto &pElement = pGimmicks[i];
		if ( !pElement ) { continue; }
		// else

		boxes.emplace_back( pElement->GetHitBox().Get2D() );

		if ( pElement->HasMultipleHitBox() )
		{
			auto anotherHitBoxes = pElement->GetAnotherHitBoxes();
			for ( const auto &it : anotherHitBoxes )
			{
				anotherBoxes.emplace_back( it.Get2D() );
			}
		}
	}

	// This "allTerrains" stores boxes arranged in the order : [gimmicks][anothers][terrains],
	// so I can access to the gimmicks hit-box by index.
	// The reason for that arranges is I should update a hit-box after every PhysicUpdate().
	// Because that method will moves the gimmicks.
	// I want to update is the gimmicks, but I should send to gimmicks all hit-boxes.
	std::vector<BoxEx>  allTerrains = boxes; // [gimmicks][anothers][terrains]
	allTerrains.insert( allTerrains.end(), anotherBoxes.begin(), anotherBoxes.end() );
	allTerrains.insert( allTerrains.end(), terrains.begin(),     terrains.end()     );

	for ( size_t i = 0; i < gimmickCount; ++i )
	{
		if ( !pGimmicks[i] ) { continue; }
		// else

		pGimmicks[i]->PhysicUpdate( player, accompanyBox, allTerrains );
		allTerrains[i] = pGimmicks[i]->GetHitBox().Get2D();
	}

	// Erase the should remove blocks.
	{
		auto itr = std::remove_if
		(
			pGimmicks.begin(), pGimmicks.end(),
			[]( std::shared_ptr<GimmickBase> &pElement )
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
	std::vector<AABBEx> anotherBoxes{};
	for ( const auto &it : pGimmicks )
	{
		if ( !it ) { continue; }
		// else

		boxes.emplace_back( it->GetHitBox() );

		if ( it->HasMultipleHitBox() )
		{
			anotherBoxes = it->GetAnotherHitBoxes();
			for ( const auto &itr : anotherBoxes )
			{
				boxes.emplace_back( itr );
			}
		}
	}
	return boxes;
}

void Gimmick::LoadParameter( bool fromBinary )
{
	std::string filePath = GenerateSerializePath( SERIAL_ID, fromBinary );
	Donya::Serializer::Load( *this, filePath.c_str(), SERIAL_ID, fromBinary );
}

void Gimmick::ApplyConfig( const StageConfiguration &stageConfig, const Donya::Vector3 &worldOffset )
{
	pGimmicks.clear();
	
	const size_t gimmickCount = stageConfig.pEditGimmicks.size();
	pGimmicks.resize( gimmickCount );

	for ( size_t i = 0; i < gimmickCount; ++i )
	{
		pGimmicks[i] = stageConfig.pEditGimmicks[i];
		pGimmicks[i]->AddOffset(worldOffset);
	}
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

// This includes is the order in the GimmickKind.
#include "GimmickImpl/FragileBlock.h"
#include "GimmickImpl/HardBlock.h"
#include "GimmickImpl/IceBlock.h"
#include "GimmickImpl/Spike.h"
#include "GimmickImpl/SwitchBlock.h"
#include "GimmickImpl/FlammableBlock.h"
#include "GimmickImpl/Lift.h"
#include "GimmickImpl/Trigger.h"
#include "GimmickImpl/Bomb.h"
#include "GimmickImpl/Shutter.h"
#include "GimmickImpl/Door.h"
#include "GimmickImpl/Elevator.h"
#include "GimmickImpl/BeltConveyor.h"
#include "GimmickImpl/OneWayBlock.h"
#include "GimmickImpl/Jammer.h"

void Gimmick::UseImGui()
{
	GimmickUtility::UseGimmicksImGui();

	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"ギミック" ) )
		{
			static int				id{};			// For triggers.
			static float			rollDegree{};	// For everyone.
			static float			moveAmount{};	// For lift and elevator.
			static float			appearSec{};	// For jammer.
			static float			intervalSec{};	// For jammer.
			static Donya::Vector3	direction{ 0.0f, 1.0f, 0.0f };
			if ( ImGui::TreeNode( u8"設置オプション" ) )
			{
				ImGui::DragFloat	( u8"Ｚ軸回転量", &rollDegree );
				ImGui::SliderFloat3	( u8"動作方向", &direction.x, -1.0f, 1.0f );
				ImGui::DragInt		( u8"ID", &id );
				ImGui::DragFloat	( u8"移動量", &moveAmount );
				ImGui::DragFloat	( u8"開始時間（秒）", &appearSec );
				ImGui::DragFloat	( u8"点滅間隔（秒）", &intervalSec );

				ImGui::TreePop();
			}

			// Resizing.
			{
				constexpr Donya::Vector3 GENERATE_POS = Donya::Vector3::Zero();
				const std::string prefix{ u8"末尾に追加・" };

				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Fragile			) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<FragileBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::Fragile ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Hard				) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<HardBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::Hard ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Ice				) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<IceBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::Ice ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Spike				) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<SpikeBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::Spike ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::SwitchBlock		) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<SwitchBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::SwitchBlock ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::FlammableBlock	) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<FlammableBlock>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::FlammableBlock ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Lift				) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<Lift>( direction.Normalized(), moveAmount ) );
					pGimmicks.back()->Init( ToInt( GimmickKind::Lift ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::TriggerKey		) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<Trigger>( id, false ) );
					pGimmicks.back()->Init( ToInt( GimmickKind::TriggerKey ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::TriggerSwitch		) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<Trigger>( id, false ) );
					pGimmicks.back()->Init( ToInt( GimmickKind::TriggerSwitch ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::TriggerPull		) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<Trigger>( id, false ) );
					pGimmicks.back()->Init( ToInt( GimmickKind::TriggerPull ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Bomb				) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<Bomb>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::Bomb ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::BombGenerator		) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<BombGenerator>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::BombGenerator ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::BombDuct			) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<BombDuct>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::BombDuct ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Shutter			) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<Shutter>( id, direction.Normalized() ) );
					pGimmicks.back()->Init( ToInt( GimmickKind::Shutter ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Door				) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<Door>( id, direction.Normalized() ) );
					pGimmicks.back()->Init( ToInt( GimmickKind::Door ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::Elevator			) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<Elevator>( id, direction.Normalized(), moveAmount ) );
					pGimmicks.back()->Init( ToInt( GimmickKind::Elevator ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::BeltConveyor		) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<BeltConveyor>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::BeltConveyor ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::OneWayBlock		) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<OneWayBlock>( direction.Normalized() ) );
					pGimmicks.back()->Init( ToInt( GimmickKind::OneWayBlock ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::JammerArea		) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<JammerArea>( appearSec, intervalSec  ) );
					pGimmicks.back()->Init( ToInt( GimmickKind::JammerArea ), rollDegree, GENERATE_POS );
				}
				if ( ImGui::Button( ( prefix + ToString( GimmickKind::JammerOrigin ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<JammerOrigin>() );
					pGimmicks.back()->Init( ToInt( GimmickKind::JammerOrigin ), rollDegree, GENERATE_POS );
				}
				/*
				if ( ImGui::Button( ( prefix + ToString( GimmickKind:: ) ).c_str() ) )
				{
					pGimmicks.push_back( std::make_shared<XXX>() );
					pGimmicks.back()->Init( ToInt( GimmickKind:: ), rollDegree, GENERATE_POS );
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
					auto &elem = *it;

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
