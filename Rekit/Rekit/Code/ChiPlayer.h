#pragma once

#include "Header/CBuffer.h"
#include "Header/Quaternion.h"
#include "Header/Serializer.h"
#include "Header/Shader.h"
#include "Header/StaticMesh.h"
#include "Header/Template.h"
#include "Header/UseImgui.h"
#include "Header/Vector.h"

/// <summary>
/// For Chi's player parameter.
/// </summary>
class ChiPlayerParam final : public Donya::Singleton<ChiPlayerParam>
{
	friend Donya::Singleton<ChiPlayerParam>;
private:
	float scale;			// Usually 1.0f.
	float runSpeed;			// Scalar.
	float rotSlerpFactor;	// Use player's rotation.
private:
	ChiPlayerParam();
public:
	~ChiPlayerParam();
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( scale ),
			CEREAL_NVP( runSpeed )
		);

		if ( 1 <= version )
		{
			archive( CEREAL_NVP( rotSlerpFactor ) );
		}
		if ( 2 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *SERIAL_ID = "ChiPlayer";
public:
	void Init();
	void Uninit();
public:
	float Scale()		const { return scale;			}
	float RunSpeed()	const { return runSpeed;		}
	float SlerpFactor()	const { return rotSlerpFactor;	}
public:
	void LoadParameter( bool isBinary = true );

#if USE_IMGUI

	void SaveParameter();

	void UseImGui();

#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( ChiPlayerParam, 1 )

/// <summary>
/// For Chi's player.
/// </summary>
class ChiPlayer
{
public:
	struct Input
	{
		Donya::Vector3 moveVector; // Normalized.
		bool doDefend;
		bool doAttack;
	public:
		Input() :
			moveVector(),
			doDefend( false ),
			doAttack( false )
		{}
		Input( Donya::Vector3 moveVector, bool doDefend, bool doAttack ) :
			moveVector( moveVector ),
			doDefend( doDefend ),
			doAttack( doAttack )
		{}
	};
private:
	enum class State
	{
		Idle,
		Run,
	};
	struct ConstantBuffer
	{
		DirectX::XMFLOAT4X4	worldViewProjection;
		DirectX::XMFLOAT4X4	world;
		DirectX::XMFLOAT4	cameraPosition;
		DirectX::XMFLOAT4	lightDirection;
		DirectX::XMFLOAT4	lightColor;
		DirectX::XMFLOAT4	materialColor;
	};
	struct MaterialConstantBuffer
	{
		DirectX::XMFLOAT4	ambient;
		DirectX::XMFLOAT4	diffuse;
		DirectX::XMFLOAT4	specular;
	};
	struct FogCBuffer
	{
		DirectX::XMFLOAT4	fogColor;
		float				fogFar;
		float				fogNear;
		float				paddings[2];
	};
private:
	State									status;
	Donya::Vector3							pos;			// In world space.
	Donya::Vector3							velocity;		// In world space.
	Donya::Vector3							lookDirection;	// In world space.
	Donya::Quaternion						orientation;
	Donya::VertexShader						vertexShader;
	Donya::PixelShader						pixelShader;
	mutable Donya::CBuffer<ConstantBuffer>			cbuffer;
	mutable Donya::CBuffer<MaterialConstantBuffer>	mtlCBuffer;
	mutable Donya::CBuffer<FogCBuffer>		fogCBuffer;
	MaterialConstantBuffer					mtlCBufferParam;
	FogCBuffer								fogCBufferParam;
	std::shared_ptr<Donya::StaticMesh>		pModel;
public:
	ChiPlayer();
	~ChiPlayer();
public:
	void Init();
	void Uninit();

	void Update( Input input );

	void Draw
	(
		const Donya::Vector4x4	&matView,
		const Donya::Vector4x4	&matProjection,
		const Donya::Vector4	&cameraPosition,
		const Donya::Vector4	&lightDirection,
		const Donya::Vector4	&lightColor,
		const Donya::Vector4	&materialColor = { 1.0f, 1.0f, 1.0f, 1.0f }
	) const;
private:
	void LoadModel();

	void Run( Input input );

	/// <summary>
	/// The position("pos") is only changed by this method.
	/// </summary>
	void ApplyVelocity();
private:
#if USE_IMGUI

	void UseImGui();

#endif // USE_IMGUI
};
