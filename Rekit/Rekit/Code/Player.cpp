#include "Player.h"

#include <array>
#include <algorithm>		// Use std::min(), max().
#include <vector>

#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "FilePath.h"

#undef max
#undef min

class Param final : public Donya::Singleton<Param>
{
	friend Donya::Singleton<Param>;
public:
	struct Member
	{
		int			maxJumpCount{};	// 0 is can not jump, 1 ~ is can jump.
		float		moveSpeed{};	// Use for a horizontal move. It will influenced by "elapsedTime".
		float		jumpPower{};	// Use for a just moment of using a jump.
		float		maxFallSpeed{};	// Use for a limit of falling speed.
		float		gravity{};		// Always use to fall. It will influenced by "elapsedTime".

		Donya::AABB	hitBoxPhysic{};	// Hit-Box of using to the collision to the stage.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( maxJumpCount ),
				CEREAL_NVP( moveSpeed ),
				CEREAL_NVP( jumpPower ),
				CEREAL_NVP( maxFallSpeed ),
				CEREAL_NVP( gravity ),
				CEREAL_NVP( hitBoxPhysic )
			);
			if ( 1 <= version )
			{
				// CEREAL_NVP( x )
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "Player";
	Member m;
private:
	Param() : m() {}
public:
	~Param() = default;
public:
	void Init()
	{
		LoadParameter();
	}
	void Uninit()
	{

	}
private:
	void LoadParameter( bool fromBinary = true )
	{
		std::string filePath = GenerateSerializePath( SERIAL_ID, fromBinary );
		Donya::Serializer::Load( m, filePath.c_str(), SERIAL_ID, fromBinary );
	}

#if USE_IMGUI

	void SaveParameter()
	{
		bool useBinary = true;
		std::string filePath{};

		filePath = GenerateSerializePath( SERIAL_ID, useBinary );
		Donya::Serializer::Save( m, filePath.c_str(), SERIAL_ID, useBinary );

		useBinary = false;
		
		filePath = GenerateSerializePath( SERIAL_ID, useBinary );
		Donya::Serializer::Save( m, filePath.c_str(), SERIAL_ID, useBinary );
	}

public:
	void UseImGui()
	{
		if ( ImGui::BeginIfAllowed() )
		{
			if ( ImGui::TreeNode( u8"�v���C���[�E�����f�[�^" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, Donya::AABB *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"���S�ʒu�̃I�t�Z�b�g" ).c_str(), &pHitBox->pos.x  );
					ImGui::DragFloat2( ( prefix + u8"�T�C�Y�i�������w��j" ).c_str(), &pHitBox->size.x );
					ImGui::Checkbox  ( ( prefix + u8"�����蔻��͗L����"   ).c_str(), &pHitBox->exist );
				};

				ImGui::DragInt( u8"�ő�W�����v��",		&m.maxJumpCount,	1.0f, 0		);
				ImGui::DragFloat( u8"���ړ����x",		&m.moveSpeed,		1.0f, 0.0f	);
				ImGui::DragFloat( u8"�W�����v����",		&m.jumpPower,		1.0f, 0.0f	);
				ImGui::DragFloat( u8"�ő嗎�����x",		&m.maxFallSpeed,	1.0f, 0.0f	);
				ImGui::DragFloat( u8"�d��",				&m.gravity,			1.0f, 0.0f	);

				AdjustAABB( u8"�����蔻��F�u�r�n�`", &m.hitBoxPhysic );

				if ( ImGui::TreeNode( u8"�t�@�C��" ) )
				{
					static bool isBinary = true;
					if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true; }
					if ( ImGui::RadioButton( "JSON", !isBinary ) ) { isBinary = false; }
					std::string loadStr{ "�ǂݍ��� " };
					loadStr += ( isBinary ) ? "Binary" : "JSON";

					if ( ImGui::Button( u8"�ۑ�" ) )
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
};

CEREAL_CLASS_VERSION( Param::Member, 0 )

Player::Player() :
	remainJumpCount( 1 ),
	pos(), velocity(),
	drawModel( Donya::Geometric::CreateSphere() ), cbuffer(), VSDemo(), PSDemo()
{}
Player::~Player() = default;

void Player::Init()
{
	Param::Get().Init();

	CreateRenderingObjects();
}
void Player::Uninit()
{
	Param::Get().Uninit();
}

void Player::Update( float elapsedTime, Input controller )
{
#if USE_IMGUI

	Param::Get().UseImGui();
	UseImGui();

#endif // USE_IMGUI

	
}

void Player::Draw( const Donya::Vector4x4 &matViewProjection, const Donya::Vector4 &lightDirection, const Donya::Vector4 &lightColor ) const
{

}

void Player::CreateRenderingObjects()
{
	cbuffer.Create();

	constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputElements
	{
		D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	// The function requires argument is std::vector, so convert.
	const std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementsVector{ inputElements.begin(), inputElements.end() };
	VSDemo.CreateByCSO( GetShaderPath( ShaderAttribute::Demo, /* wantVS */ true ), inputElementsVector );
	PSDemo.CreateByCSO( GetShaderPath( ShaderAttribute::Demo, /* wantVS */ false ) );
}

#if USE_IMGUI

void Player::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"�v���C���[�E���̃f�[�^" ) )
		{
			ImGui::DragInt( u8"�̂���W�����v��",	&remainJumpCount	);
			ImGui::DragFloat3( u8"���[���h���W",		&pos.x				);
			ImGui::DragFloat3( u8"�ړ����x",			&velocity.x			);

			ImGui::TreePop();
		}

		ImGui::End();
	}
}

#endif // USE_IMGUI
