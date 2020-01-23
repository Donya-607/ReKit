#include "BG.h"

#include <array>
#include <vector>

#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Constant.h"	// Use DEBUG_MODE, scast macros.
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.
#include "Donya/Sprite.h"

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#endif // DEBUG_MODE

#include "FilePath.h"
#include "Common.h"

class BGParam final : public Donya::Singleton<BGParam>
{
	friend Donya::Singleton<BGParam>;
public:
	struct Member
	{
		float										rotationSpeed{};
		std::array <Donya::Vector2, BG::GEAR_NUM>	pos{};
		std::array <bool, BG::GEAR_NUM>				clockwise{};
		std::array <float, BG::GEAR_NUM>			initDegree{};

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize ( Archive& archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP ( rotationSpeed ),
				CEREAL_NVP ( pos ),
				CEREAL_NVP ( clockwise ),
				CEREAL_NVP ( initDegree )
			);
			if (1 <= version)
			{
				// archive( CEREAL_NVP( x ) )
			}
		}
	};
private:
	static constexpr const char* SERIAL_ID = "BG";
	Member m;
private:
	BGParam () : m () {}
public:
	~BGParam () = default;
public:
	void Init ()
	{
		LoadParameter ();
	}
	void Uninit ()
	{

	}
public:
	Member Data () const
	{
		return m;
	}
private:
	void LoadParameter ( bool fromBinary = true )
	{
		std::string filePath = GenerateSerializePath ( SERIAL_ID, fromBinary );
		Donya::Serializer::Load ( m, filePath.c_str (), SERIAL_ID, fromBinary );
	}

#if USE_IMGUI

	void SaveParameter ()
	{
		bool useBinary = true;
		std::string filePath{};

		filePath = GenerateSerializePath ( SERIAL_ID, useBinary );
		Donya::Serializer::Save ( m, filePath.c_str (), SERIAL_ID, useBinary );

		useBinary = false;

		filePath = GenerateSerializePath ( SERIAL_ID, useBinary );
		Donya::Serializer::Save ( m, filePath.c_str (), SERIAL_ID, useBinary );
	}

public:
	void UseImGui ()
	{
		if (ImGui::BeginIfAllowed ())
		{
			if (ImGui::TreeNode ( u8"背景・調整データ" ))
			{
				ImGui::SliderFloat ( u8"Gearの回転速度", &m.rotationSpeed, 0.0f, 100.0f );
				for (int i = 0; i < BG::GEAR_NUM; i++)
				{
					ImGui::SliderFloat2 ( (u8"位置・Gear" + std::to_string ( i )).c_str (), &m.pos[i].x, -10.0f, Common::HalfScreenWidthF () * 2 );
					ImGui::SliderFloat ( (u8"初期回転値・Gear" + std::to_string ( i )).c_str (), &m.initDegree[i], 0.0f, 360.0f );
					ImGui::Checkbox ( (u8"右回り・Gear" + std::to_string ( i )).c_str() , &m.clockwise[i] );
				}

				if (ImGui::TreeNode ( u8"ファイル" ))
				{
					static bool isBinary = true;
					if (ImGui::RadioButton ( "Binary", isBinary )) { isBinary = true; }
					if (ImGui::RadioButton ( "JSON", !isBinary )) { isBinary = false; }
					std::string loadStr{ "読み込み " };
					loadStr += (isBinary) ? "Binary" : "JSON";

					if (ImGui::Button ( u8"保存" ))
					{
						SaveParameter ();
					}
					if (ImGui::Button ( Donya::MultiToUTF8 ( loadStr ).c_str () ))
					{
						LoadParameter ( isBinary );
					}

					ImGui::TreePop ();
				}

				ImGui::TreePop ();
			}

			ImGui::End ();
		}
	}

#endif // USE_IMGUI
};

CEREAL_CLASS_VERSION ( BGParam::Member, 1 )


void BG::ParameterInit ()
{
	BGParam::Get ().Init ();
}
#if USE_IMGUI
void BG::UseParameterImGui ()
{
	BGParam::Get ().UseImGui ();
}
#endif // USE_IMGUI

BG::BG () :
	gear (), pos (), degree ()
{}
BG::~BG () = default;

void BG::Init ()
{
	gear = Donya::Sprite::Load ( GetSpritePath ( SpriteAttribute::Gear ) );

	for (int i = 0; i < GEAR_NUM; i++)
	{
		pos[i] = BGParam::Get ().Data ().pos[i];
		degree[i] = BGParam::Get ().Data ().initDegree[i];
	}
}
void BG::Uninit ()
{
	BGParam::Get ().Uninit ();
}

void BG::Update ( float elapsedTime )
{
#if USE_IMGUI

	ShowImGuiNode ();

#endif // USE_IMGUI

	for (int i = 0; i < GEAR_NUM; i++)
	{
		if (BGParam::Get ().Data ().clockwise[i])
		{
			degree[i] += BGParam::Get ().Data ().rotationSpeed;
			if (degree[i] >= 360.0f) { degree[i] -= 360.0f; }
		}
		else
		{
			degree[i] -= BGParam::Get ().Data ().rotationSpeed;
			if (degree[i] <= 0.0f) { degree[i] += 360.0f; }
		}
	}
}

void BG::Draw () const
{
	for (int i = 0; i < GEAR_NUM; i++)
	{
		Donya::Sprite::Draw ( gear, pos[i].x, pos[i].y, degree[i], 0.8f );
	}
}

#if USE_IMGUI

void BG::ShowImGuiNode ()
{
	if (ImGui::BeginIfAllowed ())
	{
		if (ImGui::TreeNode ( u8"背景・今のデータ" ))
		{
			for (int i = 0; i < GEAR_NUM; i++)
			{
				ImGui::SliderFloat2 ( (u8"ワールド座標・Gear" + std::to_string ( i )).c_str (), &pos[i].x, -10.0f, Common::HalfScreenWidthF () * 2 );
				ImGui::DragFloat  ( (u8"回転量・Gear" + std::to_string ( i )).c_str (), &degree[i] );
			}

			ImGui::TreePop ();
		}

		ImGui::End ();
	}
}

#endif // USE_IMGUI
