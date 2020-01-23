#pragma once

#include <string>

#include "Donya/StaticMesh.h"
#include "Donya/UseImGui.h"

#include "DerivedCollision.h"

enum class GimmickKind
{
	Fragile = 0,
	Hard,
	Ice,
	Spike,
	SwitchBlock,
	FlammableBlock,
	Lift,
	TriggerKey,			// Linking to the Door.
	TriggerSwitch,		// Linking to the Shutter. Gather the switch block.
	TriggerPull,		// Linking to the Elevator.
	Bomb,
	BombGenerator,
	BombDuct,
	Shutter,
	Door,
	Elevator,
	BeltConveyor,
	OneWayBlock,
	JammerArea,
	JammerOrigin,

	GimmicksCount
};

namespace GimmickUtility
{
	int ToInt( GimmickKind kind );
	GimmickKind ToKind( int kind );
	std::string ToString( GimmickKind kind );

	/// <summary>
	/// Please call when a scene initialize.
	/// </summary>
	void InitParameters();
	/// <summary>
	/// Retuns the result of loadings.
	/// </summary>
	bool LoadModels();
	Donya::StaticMesh *GetModelAddress( GimmickKind kind );

#if USE_IMGUI
	void UseGimmicksImGui();
#endif // USE_IMGUI

	bool HasSlipAttribute( const BoxEx  &gimmickHitBox );
	bool HasSlipAttribute( const AABBEx &gimmickHitBox );

	bool HasDangerAttribute( const BoxEx  &gimmickHitBox );
	bool HasDangerAttribute( const AABBEx &gimmickHitBox );

	bool HasGatherAttribute( const BoxEx  &gimmickHitBox );
	bool HasGatherAttribute( const AABBEx &gimmickHitBox );

	/// <summary>
	/// Returns influence velocity if has, zero if don't has.
	/// </summary>
	Donya::Vector2 HasInfluence( const BoxEx  &gimmickHitBox );
	/// <summary>
	/// Returns influence velocity if has, zero if don't has.
	/// </summary>
	Donya::Vector3 HasInfluence( const AABBEx &gimmickHitBox );

	bool HasAttribute( GimmickKind attribute, const BoxEx  &gimmickHitBox );
	bool HasAttribute( GimmickKind attribute, const AABBEx &gimmickHitBox );
}

namespace GimmickStatus
{
	/// <summary>
	/// Remove all statuses.
	/// </summary>
	void Reset();
	/// <summary>
	/// Register a status with identifier.
	/// </summary>
	void Register( int id, bool configure );
	/// <summary>
	/// Return the specified status, or false if the identifier is invalid.
	/// </summary>
	bool Refer( int id );
	/// <summary>
	/// Remove the specified status from a list.
	/// </summary>
	void Remove( int id );
}
