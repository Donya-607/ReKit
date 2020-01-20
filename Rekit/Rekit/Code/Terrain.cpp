#include "Terrain.h"

#include "Donya/Loader.h"
#include "Donya/StaticMesh.h"

namespace TerrainModel
{
	static Donya::StaticMesh block{};
	static bool wasLoaded = false;

	bool Load()
	{
		if ( wasLoaded ) { return true; }
		// else

		Donya::Loader loader{};
		bool  loadSucceeded = loader.Load( "./Data/Models/Gimmicks/Normal.bin", nullptr );
		if ( !loadSucceeded )
		{
			_ASSERT_EXPR( 0, L"Failed : Load a explosion model." );
			return false;
		}

		bool  createSucceeded = Donya::StaticMesh::Create( loader, block );
		if ( !createSucceeded )
		{
			_ASSERT_EXPR( 0, L"Failed : Create a explosion model." );
			return false;
		}

		if ( loadSucceeded && createSucceeded )
		{
			wasLoaded = true;
		}

		wasLoaded = true;
		return true;
	}

	Donya::StaticMesh &GetModel()
	{
		return block;
	}
}
bool Terrain::LoadModel()
{
	return TerrainModel::Load();
}

void Terrain::Init( const Donya::Vector3 &wsRoomOrigin, const std::vector<BoxEx> &terrain )
{
	worldOffset = wsRoomOrigin;
	source = boxes = terrain;
}
void Terrain::Uninit()
{
	source.clear();
	source.shrink_to_fit();
	boxes.clear();
	boxes.shrink_to_fit();
}

void Terrain::Draw( const Donya::Vector4x4 &matVP, const Donya::Vector4 &lightDir ) const
{
	constexpr Donya::Vector4 color{ 0.6f, 0.6f, 0.6f, 0.6f };

	Donya::StaticMesh model = TerrainModel::GetModel();
	Donya::Vector4x4 S{}, T{}, W{};
	for ( const auto &it : boxes )
	{
		S = Donya::Vector4x4::MakeScaling( Donya::Vector3{ it.size, 1.0f } );
		T = Donya::Vector4x4::MakeTranslation( Donya::Vector3{ it.pos, 0.0f } + worldOffset );
		W = S * T;

		model.Render
		(
			nullptr,
			/* useDefaultShading	= */ true,
			/* isEnableFill			= */ true,
			( W * matVP ), W,
			lightDir,
			color
		);
	}
}

void Terrain::Reset()
{
	boxes = source;
}

std::vector<BoxEx> Terrain::Acquire() const
{
	std::vector<BoxEx> wsHitBoxes = boxes;
	for ( auto &it : wsHitBoxes )
	{
		it.pos.x += worldOffset.x;
		it.pos.y += worldOffset.y;
	}
	return wsHitBoxes;
}

void Terrain::Append( const std::vector<BoxEx> &terrain )
{
	boxes.insert( boxes.end(), terrain.begin(), terrain.end() );
}
