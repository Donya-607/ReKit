#pragma once

#include <memory>
#include <vector>

#undef max
#undef min
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Collision.h"
#include "Donya/UseImGui.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"

enum class GimmickKind
{
	Fragile = 0,
	Hard,
	Ice,
	Spike,
	TriggerKey,
	TriggerSwitch, // Linking to Shutter. Gather a block.
	TriggerPull,
	Shutter,

	GimmicksCount
};
namespace GimmickUtility
{
	int ToInt( GimmickKind kind );
	GimmickKind ToKind( int kind );
	std::string ToString( GimmickKind kind );
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
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	virtual void Init( int kind, const Donya::Vector3 &wsInitPos ) = 0;
	virtual void Uninit() = 0;

	virtual void Update( float elapsedTime ) = 0;
	/// <summary>
	/// The base class PhysicUpdate() provides only moves(by velocity) and resolving collision.
	/// </summary>
	virtual void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer = true, bool ignoreHitBoxExist = false );

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

/// <summary>
/// Compressable block. Movable.
/// </summary>
class FragileBlock : public GimmickBase
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
	bool	wasBroken;	// Use for a signal of want to remove.
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
			cereal::base_class<GimmickBase>( this )
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Init( int kind, const Donya::Vector3 &wsPos ) override;
	void Uninit() override;

	void Update( float elapsedTime ) override;
	void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist = false ) override;

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
private:
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;

	void Fall( float elapsedTime );

	void Brake( float elapsedTime );

	void AssignVelocity( const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains );
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( FragileBlock, 0 )
CEREAL_REGISTER_TYPE( FragileBlock )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, FragileBlock )

/// <summary>
/// Can not compress block. Movable.
/// </summary>
class HardBlock : public GimmickBase
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
	HardBlock();
	~HardBlock();
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
	void Init( int kind, const Donya::Vector3 &wsPos ) override;
	void Uninit() override;

	void Update( float elapsedTime ) override;
	void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist = false ) override;

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
private:
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;

	void Fall( float elapsedTime );

	void Brake( float elapsedTime );
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( HardBlock, 0 )
CEREAL_REGISTER_TYPE( HardBlock )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, HardBlock )

/// <summary>
/// Slipping block. Immovable.
/// </summary>
class IceBlock : public GimmickBase
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
	IceBlock();
	~IceBlock();
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
	void Init( int kind, const Donya::Vector3 &wsPos ) override;
	void Uninit() override;

	void Update( float elapsedTime ) override;
	void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist = false ) override;

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
private:
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( IceBlock, 0 )
CEREAL_REGISTER_TYPE( IceBlock )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, IceBlock )

/// <summary>
/// Danger block. Immovable.
/// </summary>
class SpikeBlock : public GimmickBase
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
	float radian; // Use for rotation.
public:
	SpikeBlock();
	~SpikeBlock();
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
	void Init( int kind, const Donya::Vector3 &wsPos ) override;
	void Uninit() override;

	void Update( float elapsedTime ) override;
	void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist = false ) override;

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
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( SpikeBlock, 0 )
CEREAL_REGISTER_TYPE( SpikeBlock )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, SpikeBlock )

/// <summary>
/// Provides some trigger to some gimmick.
/// </summary>
class Trigger : public GimmickBase
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
	void Init( int kind, const Donya::Vector3 &wsPos ) override;
	void Uninit() override;

	void Update( float elapsedTime ) override;
	void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist = false ) override;

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
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;
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
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Trigger, 0 )
CEREAL_REGISTER_TYPE( Trigger )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, Trigger )

/// <summary>
/// Open when gave a trigger by Trigger.
/// </summary>
class Shutter : public GimmickBase
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
	int				id;
	Donya::Vector3	direction;
	float			movedWidth;
public:
	Shutter();
	Shutter( int id, const Donya::Vector3& direction );
	~Shutter();
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive& archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<GimmickBase>( this ),
			CEREAL_NVP ( id )
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Init( int kind, const Donya::Vector3& wsPos ) override;
	void Uninit() override;

	void Update( float elapsedTime ) override;
	void PhysicUpdate( const BoxEx& player, const BoxEx& accompanyBox, const std::vector<BoxEx>& terrains, bool collideToPlayer, bool ignoreHitBoxExist = false ) override;

	void Draw( const Donya::Vector4x4& matView, const Donya::Vector4x4& matProjection, const Donya::Vector4& lightDirection ) const override;
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
private:
	/// <summary>
	/// Returns index is kind of triggers(following the GimmickKind, start by TriggerKey), 0-based.
	/// </summary>
	int GetTriggerKindIndex() const;
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode() override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Shutter, 0 )
CEREAL_REGISTER_TYPE( Shutter )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, Shutter )

/// <summary>
/// The gimmicks admin.
/// </summary>
class Gimmick
{
public:
	static bool HasSlipAttribute( const BoxEx  &gimmickHitBox );
	static bool HasSlipAttribute( const AABBEx &gimmickHitBox );
	
	static bool HasDangerAttribute( const BoxEx  &gimmickHitBox );
	static bool HasDangerAttribute( const AABBEx &gimmickHitBox );
private:
	int stageNo;
	std::vector<std::unique_ptr<GimmickBase>> pGimmicks;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( pGimmicks )
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
		if ( 2 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *SERIAL_ID = "Gimmick";
public:
	Gimmick();
	~Gimmick();
public:
	void Init( int stageNumber );
	void Uninit();

	void Update( float elapsedTime );
	void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains );

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
CEREAL_CLASS_VERSION( Gimmick, 0 )
