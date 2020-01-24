#pragma once

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"

class GimmickBase
{
protected:
	int				kind;
	float			rollDegree;	// The rotation amount with Z-axis.
	Donya::Vector3	pos;		// World space.
	Donya::Vector3	velocity;
	bool			wasCompressed;
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
			archive( CEREAL_NVP( rollDegree ) );
		}
		if ( 2 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	virtual void Init( int kind, float rollDegree, const Donya::Vector3 &wsInitPos ) = 0;
	void AddOffset(const Donya::Vector3& worldOffset)
	{
		pos += worldOffset;
	}
	virtual void Uninit() = 0;

	virtual void Update( float elapsedTime ) = 0;
	/// <summary>
	/// The base class PhysicUpdate() provides only moves(by velocity) and resolving collision.
	/// </summary>
	virtual void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer = true, bool ignoreHitBoxExist = false, bool allowCompress = false );

	virtual void Draw( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &lightDirection ) const = 0;
protected:
	void BaseDraw( const Donya::Vector4x4 &matWVP, const Donya::Vector4x4 &matW, const Donya::Vector4 &lightDir, const Donya::Vector4 &materialColor ) const;
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
	/// Returns integer link to "GimmickKind".
	/// </summary>
	virtual int GetKind() const;
	/// <summary>
	/// Returns world space position.
	/// </summary>
	virtual Donya::Vector3 GetPosition() const;
	/// <summary>
	/// Returns world space hit-box.
	/// </summary>
	virtual AABBEx GetHitBox() const = 0;
	/// <summary>
	/// If returns true, please also fetch the another hit-boxes with GetAnotherHitBoxes().
	/// </summary>
	virtual bool HasMultipleHitBox() const;
	/// <summary>
	/// Usually returns empty. If the HasMultipleHitBox() returns true, I returns another hit-boxes(the hit-box that returns by GetHitBox() isn't contain).
	/// </summary>
	virtual std::vector<AABBEx> GetAnotherHitBoxes() const;

#if USE_IMGUI
	virtual void ShowImGuiNode() {}
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( GimmickBase, 1 )

