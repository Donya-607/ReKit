#pragma once

#include <vector>

#include <cereal/types/vector.hpp>

#include "Donya/Collision.h"
#include "Donya/UseImGui.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"

class GimmickBase
{
protected:
	int				kind;
	Donya::Vector3	pos;			// World space.
	Donya::Vector3	velocity;
public:
	GimmickBase();
	~GimmickBase();
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( kind ),
			CEREAL_NVP( pos ),
			CEREAL_NVP( velocity )
		);
		if ( 1 <= version )
		{
			// CEREAL_NVP( x )
		}
	}
public:
	virtual void Init( int kind, const Donya::Vector3 &wsInitPos ) = 0;
	virtual void Uninit() = 0;

	virtual void Update( float elapsedTime ) = 0;
	virtual void PhysicUpdate( const std::vector<BoxEx> &terrains ) = 0;

	void BaseDraw( const Donya::Vector4x4 &matWVP, const Donya::Vector4x4 &matW, const Donya::Vector4 &lightDir, const Donya::Vector4 &materialColor ) const;
	virtual void Draw( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &lightDirection ) const = 0;
public:
	/// <summary>
	/// Tell something trigger to the gimmick.
	/// </summary>
	virtual void WakeUp() = 0;

	/// <summary>
	/// Returns a signal of want to remove.
	/// </summary>
	virtual bool ShouldRemove() const = 0;
	/// <summary>
	/// Returns world space position.
	/// </summary>
	virtual Donya::Vector3 GetPosition() const = 0;
	/// <summary>
	/// Returns world space hit-box.
	/// </summary>
	virtual AABBEx GetHitBox() const = 0;

#if USE_IMGUI
	virtual void ShowImGuiNode() {}
#endif // USE_IMGUI
};

// TODO : If you create another block or like that,
// You should separate this to BlockBase,
// then inherit that and recreate FragileBlock.
// After that, you create another block.
class FragileBlock
{
private:
	Donya::Vector3	pos;		// World space.
	Donya::Vector3	velocity;
	bool			wasBroken;	// Use for a signal of want to remove.
public:
	FragileBlock();
	~FragileBlock();
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( pos )
		);
		if ( 1 <= version )
		{
			// CEREAL_NVP( x )
		}
	}
public:
	void Init( const Donya::Vector3 &wsPos );
	void Uninit();

	void Update( float elapsedTime );
	void PhysicUpdate( const std::vector<BoxEx> &terrains );

	void Draw( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &lightDirection ) const;
public:
	/// <summary>
	/// Returns a signal of want to remove.
	/// </summary>
	bool ShouldRemove() const;
	/// <summary>
	/// Returns world space position.
	/// </summary>
	Donya::Vector3 GetPosition() const;
	/// <summary>
	/// Returns world space hit-box.
	/// </summary>
	AABBEx GetHitBox() const;
private:
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;

	void Fall( float elapsedTime );

	void AssignVelocity( const std::vector<BoxEx> &terrains );
public:
#if USE_IMGUI
	void ShowImGuiNode();
#endif // USE_IMGUI
};

class Gimmick
{
private:
	int stageNo;
	std::vector<FragileBlock> fragileBlocks;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( fragileBlocks )
		);
		if ( 1 <= version )
		{
			// CEREAL_NVP( x )
		}
	}
	static constexpr const char *SERIAL_ID = "DebugBlocks";
public:
	Gimmick();
	~Gimmick();
public:
	void Init( int stageNumber );
	void Uninit();

	void Update( float elapsedTime );
	void PhysicUpdate( const std::vector<BoxEx> &terrains );

	void Draw( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &lightDirection ) const;
public:
	std::vector<AABBEx> RequireHitBoxes() const;
private:
	void LoadParameter( bool fromBinary = true );
#if USE_IMGUI
	void SaveParameter();
	void UseImGui();
#endif // USE_IMGUI
};
