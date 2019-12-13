#pragma once

#include "Donya/Collision.h"
#include "Donya/Serializer.h"

class BoxEx : public Donya::Box
{
public:
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
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	static BoxEx Nil()
	{
		return BoxEx{};
	}
};

class AABBEx : public Donya::AABB
{
public:
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
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	static AABBEx Nil()
	{
		return AABBEx{};
	}
};
