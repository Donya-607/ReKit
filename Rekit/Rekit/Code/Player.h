#pragma once

#include <vector>

#include "Donya/CBuffer.h"
#include "Donya/Collision.h"
#include "Donya/GeometricPrimitive.h"
#include "Donya/Shader.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"

class Player
{
public:
	struct Input
	{
		Donya::Vector3 moveVelocity{};
		bool useJump{ false };
	};
private:
	struct Constants
	{
		DirectX::XMFLOAT4X4	worldViewProjection;
		DirectX::XMFLOAT4X4	world;
		DirectX::XMFLOAT4	lightDirection;
		DirectX::XMFLOAT4	lightColor;
		DirectX::XMFLOAT4	materialColor;
	};
private:
	int							remainJumpCount;	// 0 is can not jump, 1 ~ is can jump.
	Donya::Vector3				pos;				// World space.
	Donya::Vector3				velocity;

	Donya::Geometric::Sphere	drawModel;
	Donya::CBuffer<Constants>	cbuffer;
	Donya::VertexShader			VSDemo;
	Donya::PixelShader			PSDemo;
public:
	Player();
	~Player();
public:
	void Init( const Donya::Vector3 &wsInitPos );
	void Uninit();

	void Update( float elpasedTime, Input controller );
	void PhysicUpdate( const std::vector<BoxEx> &terrains );

	void Draw( const Donya::Vector4x4 &matViewProjection, const Donya::Vector4 &lightDirection, const Donya::Vector4 &lightColor ) const;
public:
	/// <summary>
	/// Returns position is world space.
	/// </summary>
	Donya::Vector3 GetPosition() const;
	/// <summary>
	/// Returns hit-box is world space.
	/// </summary>
	AABBEx GetHitBox() const;
private:
	void CreateRenderingObjects();

	void Move( float elapsedTime, Input controller );

	void Fall( float elapsedTime, Input controller );
	void JumpIfUsed( float elapsedTime, Input controller );

	void Landing();

#if USE_IMGUI
private:

	void UseImGui();

#endif // USE_IMGUI
};
