#pragma once

#include <vector> // Use for argument.

#undef max
#undef min
#include <cereal/types/polymorphic.hpp>

#include "Donya/StaticMesh.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"
#include "GimmickBase.h"

/// <summary>
/// A movable block. This will explosion when compressed by something.
/// </summary>
class Bomb : public GimmickBase
{
private:
	// It model can't receive from Gimmick's staiic method.
	static Donya::StaticMesh modelExplosion;
public:
	static bool IsExplosionBox( const BoxEx  &source );
	static bool IsExplosionBox( const AABBEx &source );
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
	enum class State
	{
		Bomb,
		Explosion
	};
private:
	State status;
	float scale;
	float alpha;
public:
	Bomb();
	~Bomb();
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
	/// Returns world space hit-box.
	/// </summary>
	AABBEx GetHitBox() const override;
private:
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;

	void Fall( float elapsedTime );

	void Brake( float elapsedTime );
private:
	void BombUpdate( float elapsedTime );
	void ExplosionUpdate( float elapsedTime );

	void BombPhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist = false, bool allowCompress = false );
	void ExplosionPhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist = false, bool allowCompress = false );

	bool NowExplosioning() const;
	void Explosion();
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Bomb, 0 )
CEREAL_REGISTER_TYPE( Bomb )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, Bomb )



/// <summary>
/// This class generates the bomb class.
/// </summary>
class BombGenerator : public GimmickBase
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
	float generateTimer;
	std::vector<Bomb> bombs;
public:
	BombGenerator();
	~BombGenerator();
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
	/// Returns world space hit-box.
	/// </summary>
	AABBEx GetHitBox() const override;
	bool HasMultipleHitBox() const override;
	std::vector<AABBEx> GetAnotherHitBoxes() const override;
private:
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;
private:
	void CountDown( float elapsedTime );
	void Generate();

	void UpdateBombs( float elapsedTime );
	void PhysicUpdateBombs( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist = false );
	void EraseBombs();
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( BombGenerator, 0 )
CEREAL_REGISTER_TYPE( BombGenerator )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, BombGenerator )


/// <summary>
/// An eraser of the Bomb. Ungravity. Immovable.
/// </summary>
class BombDuct : public GimmickBase
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
public:
	BombDuct();
	~BombDuct();
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
	void WakeUp() override {}

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
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( BombDuct, 0 )
CEREAL_REGISTER_TYPE( BombDuct )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, BombDuct )

