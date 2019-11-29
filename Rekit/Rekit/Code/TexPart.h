#ifndef _INCLUDED_TEXEL_H_
#define _INCLUDED_TEXEL_H_

#include <cstdint>	// use for std::uint32_t
#include <DirectXMath.h>

#include "cereal/cereal.hpp"

/// <summary>
/// This is base class.
/// </summary>
class TexPart
{
public:
	struct Int2
	{
		int x = 0;
		int y = 0;
	public:
		Int2() : x( 0 ), y( 0 ) {}
		Int2( int x, int y ) : x( x ), y( y ) {}
	public:
		DirectX::XMFLOAT2 Float() const
		{
			return { static_cast<float>( x ), static_cast<float>( y ) };
		}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, const std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ) );

			if ( 1 <= version )
			{
				// archive()
			}
		}
	};
	/// <summary>
	/// coord : The sprite part's coordinate of left-top.
	/// size  : The sprite part's whole size.
	/// </summary>
	struct TexInfo
	{
		Int2 coord;
		Int2 size;
	public:
		TexInfo() : coord(), size() {}
		TexInfo( int coordX, int coordY, int sizeX, int sizeY ) : coord( coordX, coordY ), size( sizeX, sizeY ) {}
		TexInfo( Int2 coord, Int2 size ) : coord( coord ), size( size ) {}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, const std::uint32_t version )
		{
			archive( CEREAL_NVP( coord ), CEREAL_NVP( size ) );

			if ( 1 <= version )
			{
				// archive()
			}
		}
	};
public:
	TexPart() = default;
	virtual ~TexPart() = default;
};

CEREAL_CLASS_VERSION( TexPart::Int2, 1 );
CEREAL_CLASS_VERSION( TexPart::TexInfo, 1 );

#endif // _INCLUDED_TEXEL_H_