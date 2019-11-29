#pragma once

#include "Header/Serializer.h"
#include "Header/UseImgui.h"
#include "Header/Vector.h"

class Camera
{
private:
	int stageNo;
	float stickMoveSpeed;				// Speed of move by input of stick.
	float easePower;					// 0.0f ~ 1.0f
	Donya::Vector2 pos;					// Left-Top. This will be subtract to pos of every object.
	Donya::Vector2 prevTargetPos;
	Donya::Vector2 screenSize;
	Donya::Vector2 distanceFromEdge;	// The target's position place in screen.
	Donya::Vector2 enableLerpBorder;
	Donya::Vector2 mostEdgePos;
	bool isReachedEdgeX;
public:
	Camera();
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( easePower ),
			CEREAL_NVP( enableLerpBorder )
		);

		if ( 1 <= version )
		{
			archive( CEREAL_NVP( stickMoveSpeed ) );
		}
	}
	static constexpr const char *SERIAL_ID = "Camera";
public:
	void Init( int stageNumber );

	void Update( Donya::Vector2 inputVelocity );

	/// <summary>
	/// The "targetMoveDirectionSide" specify the "targetPos" place of side in screen, 1 or -1.
	/// </summary>
	void Scroll( Donya::Vector2 targetPosition, Donya::Vector2 targetVelocity, int targetMoveDirectionSide );
public:
	void SetPosition( Donya::Vector2 wsPosition )
	{
		pos = wsPosition;
		ClampPos();
	}
	void SetDistanceFromEdge( Donya::Vector2 distance )
	{
		distanceFromEdge = distance;
	}

	bool IsReachedEdgeX() const
	{
		return isReachedEdgeX;
	}

	/// <summary>
	/// Scroll value, offset.
	/// </summary>
	Donya::Vector2 GetPos() const
	{
		return pos;
	}
private:
	void ClampPos();

	void HorizontalScroll( Donya::Vector2 targetPosition, Donya::Vector2 targetVelocity, int targetMoveDirectionSide );
	void VerticalScroll( Donya::Vector2 targetPosition, Donya::Vector2 targetVelocity );

	void ScrollToCenter( Donya::Vector2 targetPosition, Donya::Vector2 targetVelocity );

	void LoadParameter( bool isBinary = true );

#if USE_IMGUI

	void SaveParameter();

	void UseImGui();

#endif // USE_IMGUI
};

CEREAL_CLASS_VERSION( Camera, 1 )
