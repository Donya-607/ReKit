#pragma once

#include <vector> // Use for argument.

#undef max
#undef min
#include <cereal/types/polymorphic.hpp>

#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"
#include "GimmickBase.h"

class Elevator : public GimmickBase
{
public:
	/// <summary>
	/// Please call when a scene initialize.
	/// </summary>
	static void ParameterInit ();
#if USE_IMGUI
	/// <summary>
	/// Please call every frame.
	/// </summary>
	static void UseParameterImGui ();
#endif // USE_IMGUI
	enum class ElevatorState
	{
		Stay = 0,
		Go,
		Wait,
		GoBack,
	};
private:
	int				id;
	Donya::Vector3	direction;		// World space.
	float			moveAmount;		// Amount moved.
	float			maxMoveAmount;	// Maximum amount to move.
	int				waitCount;
	ElevatorState	state;
public:
	Elevator ();
	Elevator ( int id, const Donya::Vector3& direction, float moveAmount );
	~Elevator ();
private:
	friend class cereal::access;
	template<class Archive>
	void serialize ( Archive& archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<GimmickBase> ( this ),
			CEREAL_NVP ( id )
		);
		if (1 <= version)
		{
			archive
			(
				CEREAL_NVP( direction ),
				CEREAL_NVP( maxMoveAmount )
			);
		}
		if (2 <= version)
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Init ( int kind, float rollDegree, const Donya::Vector3& wsPos ) override;
	void Uninit () override;

	void Update ( float elapsedTime ) override;
	void PhysicUpdate ( const BoxEx& player, const BoxEx& accompanyBox, const std::vector<BoxEx>& terrains, bool collideToPlayer, bool ignoreHitBoxExist = false, bool allowCompress = false ) override;

	void Draw ( const Donya::Vector4x4& matView, const Donya::Vector4x4& matProjection, const Donya::Vector4& lightDirection ) const override;
public:
	void WakeUp () override;

	/// <summary>
	/// Returns a signal of want to remove.
	/// </summary>
	bool ShouldRemove () const override;
	/// <summary>
	/// Returns world space position.
	/// </summary>
	Donya::Vector3 GetPosition () const override;
	/// <summary>
	/// Returns world space hit-box.
	/// </summary>
	AABBEx GetHitBox () const override;
private:
	/// <summary>
	/// Returns index is kind of triggers(following the GimmickKind, start by TriggerKey), 0-based.
	/// </summary>
	int GetTriggerKindIndex () const;
	Donya::Vector4x4 GetWorldMatrix ( bool useDrawing = false ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode () override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION ( Elevator, 1 )
CEREAL_REGISTER_TYPE ( Elevator )
CEREAL_REGISTER_POLYMORPHIC_RELATION ( GimmickBase, Elevator )

