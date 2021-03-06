#pragma once

#include <vector> // Use for argument.

#undef max
#undef min
#include <cereal/types/polymorphic.hpp>

#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"
#include "GimmickBase.h"

/// <summary>
/// Provides some trigger to some gimmick.
/// </summary>
class Trigger : public GimmickBase
{
public:
	static bool IsGatherBox( const BoxEx  &source );
	static bool IsGatherBox( const AABBEx &source );
	/// <summary>
	/// Please call when a scene initialize.
	/// </summary>
	static void ParameterInit();
#if USE_IMGUI
	/// <summary>
	/// Please call every frame.
	/// </summary>
	static void UseParameterImGui();
#endif // USE_IMGUI
private:
	struct KeyMember
	{

	};
	struct SwitchMember
	{

	};
	struct PullMember
	{
		Donya::Vector3 initPos;
	};
private:
	int				id;
	bool			enable;

	KeyMember		mKey;
	SwitchMember	mSwitch;
	PullMember		mPull;
public:
	Trigger();
	Trigger( int id, bool enable );
	~Trigger();
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<GimmickBase>( this ),
			CEREAL_NVP( id )
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Init( int kind, float rollDegree, const Donya::Vector3 &wsPos ) override;
	void Uninit() override;

	void Update( float elapsedTime ) override;
	void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist = false, bool allowCompress = false ) override;

	void Draw( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &lightDirection ) const override;
public:
	void WakeUp() override;

	/// <summary>
	/// Returns a signal of want to remove.
	/// </summary>
	bool ShouldRemove() const override;
	/// <summary>
	/// Returns world space position.
	/// </summary>
	Donya::Vector3 GetPosition() const override;
	/// <summary>
	/// Returns world space hit-box.
	/// </summary>
	AABBEx GetHitBox() const override;
	bool HasMultipleHitBox() const override;
	std::vector<AABBEx> GetAnotherHitBoxes() const override;
private:
	/// <summary>
	/// Returns index is kind of triggers(following the GimmickKind, start by TriggerKey), 0-based.
	/// </summary>
	int GetTriggerKindIndex() const;
	Donya::Vector4x4 GetWorldMatrix( const AABBEx &wsBox, bool useDrawing = false, bool enableRotation = true ) const;
private:
	AABBEx RollHitBox( AABBEx source ) const;
private:
	void InitKey();
	void InitSwitch();
	void InitPull();

	void UninitKey();
	void UninitSwitch();
	void UninitPull();

	void UpdateKey( float elapsedTime );
	void UpdateSwitch( float elapsedTime );
	void UpdatePull( float elapsedTime );

	void PhysicUpdateKey( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains );
	void PhysicUpdateSwitch( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains );
	void PhysicUpdatePull( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains );
private:
	void TurnOn();
	bool IsEnable() const { return enable; }
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Trigger, 0 )
CEREAL_REGISTER_TYPE( Trigger )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, Trigger )

