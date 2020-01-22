#include "GimmickUtil.h"

#include <map>

#include "Donya/Loader.h"
#include "Donya/Useful.h"

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
		case GimmickKind::SwitchBlock:		return "SwitchBlock";	// break;
		case GimmickKind::FlammableBlock:	return "FlammableBlock";// break;
		case GimmickKind::Lift:				return "Lift";			// break;
		case GimmickKind::TriggerKey:		return "TriggerKey";	// break;
		case GimmickKind::TriggerSwitch:	return "TriggerSwitch";	// break;
		case GimmickKind::TriggerPull:		return "TriggerPull";	// break;
		case GimmickKind::Bomb:				return "Bomb";			// break;
		case GimmickKind::BombGenerator:	return "BombGenerator";	// break;
		case GimmickKind::Shutter:			return "Shutter";		// break;
		case GimmickKind::Door:				return "Door";			// break;
		case GimmickKind::Elevator:			return "Elevator";		// break;
		case GimmickKind::BeltConveyor:		return "BeltConveyor";	// break;
		case GimmickKind::OneWayBlock:		return "OneWayBlock";	// break;
		default: _ASSERT_EXPR( 0, L"Error : Unexpected kind detected!" ); break;
		}

		return "ERROR_KIND";
	}

	void InitParameters()
	{
		FragileBlock::ParameterInit();
		HardBlock::ParameterInit();
		IceBlock::ParameterInit();
		SpikeBlock::ParameterInit();
		SwitchBlock::ParameterInit();
		FlammableBlock::ParameterInit();
		Lift::ParameterInit();
		Trigger::ParameterInit();
		Bomb::ParameterInit();
		BombGenerator::ParameterInit();
		Shutter::ParameterInit();
		Door::ParameterInit();
		Elevator::ParameterInit();
		BeltConveyor::ParameterInit();
		OneWayBlock::ParameterInit();
	}
	
	namespace Instance
	{
		// This is in the order of GimmickKind.
		static std::array<Donya::StaticMesh, scast<int>( GimmickKind::GimmicksCount )> models{};
		static bool wasLoaded{ false };
	}
	bool LoadModels()
	{
		if ( Instance::wasLoaded ) { return true; }
		// eles

		const std::vector<GimmickKind> loadKinds
		{
			GimmickKind::Fragile,
			GimmickKind::Hard,
			GimmickKind::Ice,
			GimmickKind::Spike,
			GimmickKind::SwitchBlock,
			GimmickKind::FlammableBlock,
			GimmickKind::Lift,
			GimmickKind::TriggerKey,
			GimmickKind::TriggerSwitch,
			GimmickKind::TriggerPull,
			GimmickKind::Bomb,
			GimmickKind::BombGenerator,
			GimmickKind::Shutter,
			GimmickKind::Door,
			GimmickKind::Elevator,
			GimmickKind::BeltConveyor,
			GimmickKind::OneWayBlock,
		};

		auto MakeModelPath			= []( const std::string &kindName )
		{
			const std::string directory{ "./Data/Models/Gimmicks/" };
			const std::string extension{ ".bin" };

			return directory + kindName + extension;
		};
		auto AssertAboutLoading		= []( const std::string &kindName )
		{
			const std::wstring errMsg{ L"Failed : Load a gimmicks model. That is : " };
			_ASSERT_EXPR( 0, ( errMsg + Donya::MultiToWide( kindName ) ).c_str() );
		};
		auto AssertAboutCreation	= []( const std::string &kindName )
		{
			const std::wstring errMsg{ L"Failed : Create a gimmicks model. That is : " };
			_ASSERT_EXPR( 0, ( errMsg + Donya::MultiToWide( kindName ) ).c_str() );
		};

		Donya::Loader	loader{};
		std::string		kindName{};
		bool result{};
		bool succeeded = true;
		for ( const auto &it : loadKinds )
		{
			kindName = ToString( it );

			result = loader.Load( MakeModelPath( kindName ), nullptr );
			if ( !result )
			{
				AssertAboutLoading( kindName );

				succeeded = false;
				continue;
			}
			// else

			Donya::StaticMesh *pModel = GetModelAddress( it );
			result = Donya::StaticMesh::Create( loader, *pModel );
			if ( !result )
			{
				AssertAboutCreation( kindName );

				succeeded = false;
				continue;
			}
			// else
		}

		if ( succeeded )
		{
			Instance::wasLoaded = true;
		}

		return true;
	}
	Donya::StaticMesh *GetModelAddress( GimmickKind kind )
	{
		const int index = ToInt( kind );
		_ASSERT_EXPR( 0 <= index && index < ToInt( GimmickKind::GimmicksCount ), L"Error : The passed index over than conut of models! You should check the contents of models." );

		return &Instance::models[index];
	}

#if USE_IMGUI
	void UseGimmicksImGui()
	{
		FragileBlock::UseParameterImGui();
		HardBlock::UseParameterImGui();
		IceBlock::UseParameterImGui();
		SpikeBlock::UseParameterImGui();
		SwitchBlock::UseParameterImGui();
		FlammableBlock::UseParameterImGui();
		Lift::UseParameterImGui();
		Trigger::UseParameterImGui();
		Bomb::UseParameterImGui();
		BombGenerator::UseParameterImGui();
		Shutter::UseParameterImGui();
		Door::UseParameterImGui();
		Elevator::UseParameterImGui();
		BeltConveyor::UseParameterImGui();
		OneWayBlock::UseParameterImGui();
	}
#endif // USE_IMGUI

	bool HasSlipAttribute( const BoxEx  &gimmick )
	{
		return HasAttribute( GimmickKind::Ice, gimmick );
	}
	bool HasSlipAttribute( const AABBEx &gimmick )
	{
		return HasAttribute( GimmickKind::Ice, gimmick );
	}

	bool HasDangerAttribute( const BoxEx  &gimmick )
	{
		return HasAttribute( GimmickKind::Spike, gimmick );
	}
	bool HasDangerAttribute( const AABBEx &gimmick )
	{
		return HasAttribute( GimmickKind::Spike, gimmick );
	}

	bool HasGatherAttribute( const BoxEx  &gimmick )
	{
		return Trigger::IsGatherBox( gimmick );
	}
	bool HasGatherAttribute( const AABBEx &gimmick )
	{
		return Trigger::IsGatherBox( gimmick );
	}

	Donya::Vector2 HasInfluence( const BoxEx  &gimmick )
	{
		return BeltConveyor::HasInfluence( gimmick );
	}
	Donya::Vector3 HasInfluence( const AABBEx &gimmick )
	{
		return BeltConveyor::HasInfluence( gimmick );
	}

	bool HasAttribute( GimmickKind attribute, const BoxEx  &gimmick )
	{
		return ( ToKind( gimmick.attr ) == attribute ) ? true : false;
	}
	bool HasAttribute( GimmickKind attribute, const AABBEx &gimmick )
	{
		return ( ToKind( gimmick.attr ) == attribute ) ? true : false;
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
		auto found = statuses.find( id );
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
		auto found = statuses.find( id );
		if ( found == statuses.end() ) { return false; }
		// else

		return found->second;
	}
	void Remove( int id )
	{
		statuses.erase( id );
	}
}
