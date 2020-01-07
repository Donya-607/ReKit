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
	TriggerKey,
	TriggerSwitch,
	TriggerPull,

	GimmicksCount
};
namespace GimmickUtility
{
	int ToInt( GimmickKind kind );
	GimmickKind ToKind( int kind );
	std::string ToString( GimmickKind kind );
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
	virtual void PhysicUpdate( const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains );

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

#if USE_IMGUI
	virtual void ShowImGuiNode() {}
#endif // USE_IMGUI
};

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
	void PhysicUpdate( const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains ) override;

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
	void PhysicUpdate( const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains ) override;

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
	int		ID;
	bool	enable;
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
			CEREAL_NVP( ID )
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
	void PhysicUpdate( const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains ) override;

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
CEREAL_CLASS_VERSION( Trigger, 0 )
CEREAL_REGISTER_TYPE( Trigger )
CEREAL_REGISTER_POLYMORPHIC_RELATION( GimmickBase, Trigger )

class Gimmick
{
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
	void PhysicUpdate( const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains );

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
