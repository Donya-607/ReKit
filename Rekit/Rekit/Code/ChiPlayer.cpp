#include "ChiPlayer.h"

#include <array>

#include "Header/Color.h"
#include "Header/Loader.h"		// Use for loading model.
#include "Header/Useful.h"		// Use convert character-code function.

#include "FilePath.h"

ChiPlayerParam::ChiPlayerParam() :
	scale( 0.1f ), runSpeed( 0.2f ), rotSlerpFactor( 0.3f )
{}
ChiPlayerParam::~ChiPlayerParam() = default;

void ChiPlayerParam::Init()
{
	LoadParameter();
}
void ChiPlayerParam::Uninit()
{
	// No op.
}

void ChiPlayerParam::LoadParameter( bool isBinary )
{
	Donya::Serializer::Extension ext = ( isBinary )
	? Donya::Serializer::Extension::BINARY
	: Donya::Serializer::Extension::JSON;
	std::string filePath = GenerateSerializePath( SERIAL_ID, ext );

	Donya::Serializer seria;
	seria.Load( ext, filePath.c_str(), SERIAL_ID, *this );
}

#if USE_IMGUI

void ChiPlayerParam::SaveParameter()
{
	Donya::Serializer::Extension bin  = Donya::Serializer::Extension::BINARY;
	Donya::Serializer::Extension json = Donya::Serializer::Extension::JSON;
	std::string binPath  = GenerateSerializePath( SERIAL_ID, bin );
	std::string jsonPath = GenerateSerializePath( SERIAL_ID, json );

	Donya::Serializer seria;
	seria.Save( bin,  binPath.c_str(),  SERIAL_ID, *this );
	seria.Save( json, jsonPath.c_str(), SERIAL_ID, *this );
}

void ChiPlayerParam::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"プレイヤー・調整データ" ) )
		{
			ImGui::SliderFloat( u8"スケール", &scale, 0.0f, 8.0f );
			ImGui::DragFloat( u8"走行速度", &runSpeed );
			ImGui::SliderFloat( u8"回転の補間速度", &rotSlerpFactor, 0.05f, 1.0f );

			if ( ImGui::TreeNode( u8"ファイル" ) )
			{
				static bool isBinary = true;
				if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true; }
				if ( ImGui::RadioButton( "JSON", !isBinary ) ) { isBinary = false; }
				std::string loadStr{ "読み込み " };
				loadStr += ( isBinary ) ? "Binary" : "JSON";

				if ( ImGui::Button( u8"保存" ) )
				{
					SaveParameter();
				}
				if ( ImGui::Button( Donya::MultiToUTF8( loadStr ).c_str() ) )
				{
					LoadParameter( isBinary );
				}

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}
}

#endif // USE_IMGUI

ChiPlayer::ChiPlayer() :
	status( State::Idle ),
	pos(), velocity(), lookDirection(),
	orientation(),
	vertexShader(), pixelShader(),
	cbuffer(), mtlCBuffer(), fogCBuffer(),
	mtlCBufferParam(), fogCBufferParam(),
	pModel( nullptr )
{}
ChiPlayer::~ChiPlayer() = default;

void ChiPlayer::Init()
{
	ChiPlayerParam::Get().Init();

	LoadModel();

	lookDirection = Donya::Vector3::Front();
	orientation   = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( 180.0f ) );

	// Create Shader objects.
	{
		std::string csoPathVS{ "./Data/Shader/StaticMeshVS.cso" };
		std::string csoPathPS{ "./Data/Shader/StaticMeshPS.cso" };

		const std::vector<D3D11_INPUT_ELEMENT_DESC> INPUT_ELEMENTS
		{
			D3D11_INPUT_ELEMENT_DESC{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			D3D11_INPUT_ELEMENT_DESC{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		assert( vertexShader.CreateByCSO( csoPathVS, INPUT_ELEMENTS ) );
		assert( pixelShader.CreateByCSO( csoPathPS ) );
	}

	cbuffer.Create();
	mtlCBuffer.Create();
	fogCBuffer.Create();

	mtlCBufferParam.ambient		= { 0.0f, 0.0f, 0.0f, 1.0f };
	mtlCBufferParam.diffuse		= { 1.0f, 1.0f, 1.0f, 1.0f };
	mtlCBufferParam.specular	= { 0.0f, 0.0f, 0.0f, 0.0f };

	fogCBufferParam.fogColor	= { 1.0f, 1.0f, 1.0f, 1.0f };
	fogCBufferParam.fogFar		= 1000.0f;
	fogCBufferParam.fogNear		= 1.0f;
}
void ChiPlayer::Uninit()
{
	ChiPlayerParam::Get().Uninit();

	pModel.reset();
}

void ChiPlayer::Update( Input input )
{
#if USE_IMGUI

	ChiPlayerParam::Get().UseImGui();
	UseImGui();

#endif // USE_IMGUI

	Run( input );

	ApplyVelocity();
}

void ChiPlayer::Draw( const Donya::Vector4x4 &matView, const Donya::Vector4x4 &matProjection, const Donya::Vector4 &cameraPosition, const Donya::Vector4 &lightDirection, const Donya::Vector4 &lightColor, const Donya::Vector4 &materialColor ) const
{
	if ( !pModel )
	{
		_ASSERT_EXPR( 0, L"Error : The chi-player's model is not loaded !" );
		return;
	}
	// else

	const auto &PARAM = ChiPlayerParam::Get();

	Donya::Vector4x4 S = Donya::Vector4x4::MakeScaling( PARAM.Scale() );
	Donya::Vector4x4 R = orientation.RequireRotationMatrix();
	Donya::Vector4x4 T = Donya::Vector4x4::MakeTranslation( pos );
	Donya::Vector4x4 W = S * R * T;
	Donya::Vector4x4 WVP = W * matView * matProjection;


	vertexShader.Activate();
	pixelShader.Activate();

	cbuffer.data.worldViewProjection	= WVP.XMFloat();
	cbuffer.data.world					= W.XMFloat();
	cbuffer.data.cameraPosition			= cameraPosition;
	cbuffer.data.lightDirection			= lightDirection;
	cbuffer.data.lightColor				= lightColor;
	cbuffer.data.materialColor			= materialColor;
	cbuffer.data.materialColor.w		= Donya::Color::FilteringAlpha( cbuffer.data.materialColor.w );
	cbuffer.Activate( 0, /* setVS = */ true, /* setPS = */ true );
	
	mtlCBuffer.data.ambient				= mtlCBufferParam.ambient;
	mtlCBuffer.data.diffuse				= mtlCBufferParam.diffuse;
	mtlCBuffer.data.specular			= mtlCBufferParam.specular;
	mtlCBuffer.Activate( 1, /* setVS = */ true, /* setPS = */ true );

	fogCBuffer.data.fogColor			= fogCBufferParam.fogColor;
	fogCBuffer.data.fogFar				= fogCBufferParam.fogFar;
	fogCBuffer.data.fogNear				= fogCBufferParam.fogNear;
	fogCBuffer.Activate( 2, /* setVS = */ true, /* setPS = */ true );

	pModel->Render( Donya::GetImmediateContext(), /* useDefaultShading = */ false );

	vertexShader.Deactivate();
	pixelShader.Deactivate();
	cbuffer.Deactivate();
	mtlCBuffer.Deactivate();
	fogCBuffer.Deactivate();
}

void ChiPlayer::LoadModel()
{
	Donya::Loader loader{};
	bool result = loader.Load( GetModelPath( ModelAttribute::ChiPlayer ), nullptr );

	_ASSERT_EXPR( result, L"Failed : Load chi-player model." );

	pModel = Donya::StaticMesh::Create( loader );

	_ASSERT_EXPR( pModel, L"Failed : Load chi-player model." );
}

void ChiPlayer::Run( Input input )
{
	if ( input.moveVector.IsZero() )
	{
		status = State::Idle;

		velocity = 0.0f; // Each member set to zero.
		return;
	}
	// else

	status = State::Run;

	velocity = input.moveVector;
	velocity *= ChiPlayerParam::Get().RunSpeed();
}

void ChiPlayer::ApplyVelocity()
{
	if ( !velocity.IsZero() )
	{
		pos += velocity;
		lookDirection = velocity.Normalized();
	}

	if ( lookDirection == orientation.LocalFront() ) { return; }
	// else

	Donya::Quaternion destination = Donya::Quaternion::LookAt( orientation, lookDirection, Donya::Quaternion::Freeze::Up );
	destination.Normalize();

	orientation = Donya::Quaternion::Slerp( orientation, destination, ChiPlayerParam::Get().SlerpFactor() );
	orientation.Normalize();

	if ( orientation.IsSameRotation( destination ) )
	{
		orientation   = destination;
		lookDirection = orientation.LocalFront();
	}
}

#if USE_IMGUI

void ChiPlayer::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"プレイヤー・今のパラメータ" ) )
		{
			const std::string vec3Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f]" };
			const std::string vec4Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f][W:%5.3f]" };
			auto ShowVec3 = [&vec3Info]( std::string name, const Donya::Vector3 &param )
			{
				ImGui::Text( ( name + vec3Info ).c_str(), param.x, param.y, param.z );
			};
			auto ShowVec4 = [&vec4Info]( std::string name, const Donya::Vector4 &param )
			{
				ImGui::Text( ( name + vec4Info ).c_str(), param.x, param.y, param.z, param.w );
			};
			auto ShowQuaternion = [&vec4Info]( std::string name, const Donya::Quaternion &param )
			{
				ImGui::Text( ( name + vec4Info ).c_str(), param.x, param.y, param.z, param.w );
			};

			auto AdjustVec3 = []( std::string name, Donya::Vector3 *pV )
			{
				ImGui::DragFloat3( name.c_str(), &pV->x, 0.05f );
			};
			auto AdjustVec4 = []( std::string name, Donya::Vector4 *pV )
			{
				ImGui::DragFloat4( name.c_str(), &pV->x, 0.05f );
			};
			auto AdjustQuaternion = []( std::string name, Donya::Quaternion *pQ )
			{
				ImGui::DragFloat4( name.c_str(), &pQ->x, 0.05f );
			};

			AdjustVec3( "Position", &pos );
			AdjustVec3( "Velocity", &velocity );
			AdjustVec3( "LookDirection", &lookDirection );

			AdjustQuaternion( "Orientation", &orientation );
			if ( ImGui::Button( u8"Reset : Orientation" ) )
			{
				orientation = Donya::Quaternion::Identity();
			}

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"プレイヤー・マテリアルパラメータ" ) )
		{
			ImGui::ColorEdit3( "Ambient.RGB", &mtlCBufferParam.ambient.x );
			ImGui::ColorEdit3( "Diffuse.RGB", &mtlCBufferParam.diffuse.x );
			ImGui::ColorEdit3( "Specular.RGB", &mtlCBufferParam.specular.x );
			ImGui::Text( "" );

			ImGui::SliderFloat( "Ambient.A", &mtlCBufferParam.ambient.w, 0.0f, 3.0f );
			ImGui::SliderFloat( "Diffuse.A", &mtlCBufferParam.diffuse.w, 0.0f, 3.0f );
			ImGui::SliderFloat( "Specular.A", &mtlCBufferParam.specular.w, 0.0f, 3.0f );
			ImGui::Text( "" );

			ImGui::ColorEdit4( "FogColor.RGB", &fogCBufferParam.fogColor.x );
			ImGui::DragFloat( "Fog.Near", &fogCBufferParam.fogNear );
			ImGui::DragFloat( "Fog.Far", &fogCBufferParam.fogFar );
			ImGui::Text( "" );

			ImGui::TreePop();
		}

		ImGui::End();
	}
}

#endif // USE_IMGUI
