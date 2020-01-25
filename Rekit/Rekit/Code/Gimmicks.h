#pragma once

#include <memory>
#include <vector>

#include "Donya/UseImGui.h"
#include "Donya/Serializer.h"
#include "Donya/StaticMesh.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"

#include "GimmickImpl/GimmickBase.h"	// HACK : This include is not necessary.

struct StageConfiguration; // This is declared at SceneEditor.h

/// <summary>
/// The gimmicks admin.
/// </summary>
class Gimmick
{
private:
	int stageNo;
	std::vector<std::shared_ptr<GimmickBase>> pGimmicks;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		/*archive
		(
			CEREAL_NVP( pGimmicks )
		);*/
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
	void Init( int stageNumber, const StageConfiguration &stageConfig, const Donya::Vector3 &worldOffset );
	void Uninit();

	void Update( float elapsedTime, bool alsoElevators = true, bool useImGui = false );
	void UpdateElevators( float elapsedTime );
	void PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains );

	void Draw( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &lightDirection, bool alsoElevators = true ) const;
	void DrawElevators( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &lightDirection ) const;
public:
	bool HasElevators() const;
	std::vector<AABBEx> RequireHitBoxes() const;
private:
	/// <summary>
	/// Replace the gimmicks.
	/// </summary>
	void ApplyConfig( const StageConfiguration &stageConfig, const Donya::Vector3 &worldOffset );
	
	void LoadParameter( bool fromBinary = true );
#if USE_IMGUI
	void SaveParameter();
	void UseImGui();
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Gimmick, 0 )
