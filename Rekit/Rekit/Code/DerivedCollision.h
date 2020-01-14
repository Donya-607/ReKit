#pragma once

#include "Donya/Collision.h"
#include "Donya/Serializer.h"

class BoxEx : public Donya::Box
{
public:
	int attr{};	// Will used for identify an attribute.
	int mass{};	// Will used for consider an object will be compressed.
public:
	BoxEx() : Box(), mass() {}
	BoxEx( const Donya::Box &box, int mass ) : Box( box ), mass( mass ) {}
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<Donya::Box>( this ),
			CEREAL_NVP( mass )
		);

		if ( 1 <= version )
		{
			archive( CEREAL_NVP( attr ) );
		}
		if ( 2 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	static BoxEx Nil()
	{
		return BoxEx{};
	}
};
CEREAL_CLASS_VERSION( BoxEx, 1 )

class AABBEx : public Donya::AABB
{
public:
	int attr{};	// Will used for identify an attribute.
	int mass{};	// Will used for consider an object will be compressed.
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<Donya::AABB>( this ),
			CEREAL_NVP( mass )
		);

		if ( 1 <= version )
		{
			archive( CEREAL_NVP( attr ) );
		}
		if ( 2 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	/// <summary>
	/// A Vector2 members are builded by (x, y).
	/// </summary>
	BoxEx Get2D() const
	{
		BoxEx xy{};
		xy.pos.x		= pos.x;
		xy.pos.y		= pos.y;
		xy.size.x		= size.x;
		xy.size.y		= size.y;
		xy.velocity.x	= velocity.x;
		xy.velocity.y	= velocity.y;
		xy.exist		= exist;
		xy.attr			= attr;
		xy.mass			= mass;
		return xy;
	}
public:
	static AABBEx Nil()
	{
		return AABBEx{};
	}
};
CEREAL_CLASS_VERSION( AABBEx, 1 )
