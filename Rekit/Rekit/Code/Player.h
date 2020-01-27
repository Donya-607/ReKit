#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/StaticMesh.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"

class Player
{
private:
	static Donya::StaticMesh	drawModel;
	static bool					wasLoaded;
public:
	struct Input
	{
		Donya::Vector3 moveVelocity{};
		bool useJump{ false };
	};
private:
	enum class State
	{
		Normal,
		Dead,
	};
private:
	State						status;

	int							collideKeyCounter;	// Keep increase when colliding to a key.

	int							remainJumpCount;	// 0 is can not jump, 1 ~ is can jump.
	float						drawAlpha;			// Use for dead animation.

	Donya::Vector3				pos;				// World space.
	Donya::Vector3				velocity;

	bool						aboveSlipGround;
	bool						seeRight;

	float						viewOpenCount;
	bool						isCatchKey;

	size_t						idOpenDoor;
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

	/// <summary>
	/// Returns true if now is the timing that I caught the key.
	/// </summary>
	bool IsCatchKey() const;

	bool IsDead() const;
private:
	void LoadModel();

	void NormalUpdate( float elapsedTime, Input controller );
	void DeadUpdate( float elapsedTime, Input controller );

	void Move( float elapsedTime, Input controller );

	void Fall( float elapsedTime, Input controller );
	void JumpIfUsed( float elapsedTime, Input controller );

	void Landing();

	void KillMe();

	void UpdateOpenDoor(float elapsedTime);
	void DrawOfOpenDoor(const Donya::Vector4x4& matViewProjection)const;

#if USE_IMGUI
private:

	void UseImGui();

#endif // USE_IMGUI
};
