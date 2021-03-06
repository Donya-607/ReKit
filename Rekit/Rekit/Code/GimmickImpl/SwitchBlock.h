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
/// The block use for the switch of trigger.
/// </summary>
class SwitchBlock : public GimmickBase
{
public:
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
	bool			wasBroken;
	float			scale;			// Use for the performance of appear.
	Donya::Vector3	initPos;
public:
	SwitchBlock();
	~SwitchBlock();
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<GimmickBase>( this )
		);
		if ( 1 <= version )
		{
			archive( CEREAL_NVP( initPos ) );
		}
		if ( 2 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Init( int kind, float rollDegree, const Donya::Vector3 &wsPos ) override;
	void AddOffset( const Donya::Vector3 &worldOffset ) override;
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
	/// Returns world space hit-box.
	/// </summary>
	AABBEx GetHitBox() const override;
private:
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;

	void Fall( float elapsedTime );

	void Brake( float elapsedTime );

	bool WasBroken() const;
	void BreakMe();

	void Scale( float elapsedTime );
	void Respawn();

	bool GatherToTheTarget( const std::vector<BoxEx> &terrains );
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( SwitchBlock, 1 )
CEREAL_REGISTER_TYPE( SwitchBlock )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, SwitchBlock )

