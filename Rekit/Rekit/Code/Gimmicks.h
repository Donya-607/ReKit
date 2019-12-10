#pragma once

#include <vector>

#include <cereal/types/vector.hpp>

#include "Donya/Collision.h"
#include "Donya/UseImGui.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

class HeavyBlock
{
private:
	Donya::Vector3	pos;	// World space.
public:
	HeavyBlock();
	~HeavyBlock();
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

	void Draw( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &lightDirection ) const;
public:
	/// <summary>
	/// Returns world space hit-box.
	/// </summary>
	Donya::AABB GetHitBox() const;
private:
	Donya::Vector4x4 GetWorldMatrix( bool useDrawing = false ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode();
#endif // USE_IMGUI
};

class Gimmick
{
private:
	int stageNo;
	std::vector<HeavyBlock> heavyBlocks;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( heavyBlocks )
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

	void Draw( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &lightDirection ) const;
public:
	std::vector<Donya::AABB> RequireHItBoxes() const;
private:
	void LoadParameter( bool fromBinary = true );
#if USE_IMGUI
	void SaveParameter();
	void UseImGui();
#endif // USE_IMGUI
};
