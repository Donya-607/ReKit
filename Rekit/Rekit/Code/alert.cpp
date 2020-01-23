#include "Alert.h"

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

class AlertParam final : public Donya::Singleton<AlertParam>
{
	friend Donya::Singleton<AlertParam>;
public:
	struct Member
	{
		float flowingSpeed{};

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize ( Archive& archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP ( flowingSpeed )
			);
			if (1 <= version)
			{
				// archive( CEREAL_NVP( x ) )
			}
		}
	};
private:
	static constexpr const char* SERIAL_ID = "Alert";
	Member m;
private:
	AlertParam () : m () {}
public:
	~AlertParam () = default;
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
			if (ImGui::TreeNode ( u8"アラート・調整データ" ))
			{
				ImGui::DragFloat ( u8"流れる速度", &m.flowingSpeed, 0.1f );

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

CEREAL_CLASS_VERSION ( AlertParam::Member, 1 )


Alert::Alert () :
	emergency (), flowingEmergency (), pos (), alpha (), state ()
{}
Alert::~Alert () = default;

void Alert::Init ()
{
	AlertParam::Get ().Init ();

	emergency = Donya::Sprite::Load ( GetSpritePath ( SpriteAttribute::Gear ) );
	flowingEmergency = Donya::Sprite::Load ( GetSpritePath ( SpriteAttribute::Gear ) );

	pos[0].x = Common::HalfScreenWidthF ();
	pos[0].y = Common::HalfScreenHeightF ();
	pos[1] = pos[0];
	pos[1].x += Common::ScreenWidthF ();
	alpha = 0.0f;
}
void Alert::Uninit ()
{
	AlertParam::Get ().Uninit ();
}

void Alert::Update ( float elapsedTime )
{
#if USE_IMGUI

	AlertParam::Get ().UseImGui ();
	UseImGui ();

#endif // USE_IMGUI

	for (int i = 0; i < 2; i++)
	{
		pos[i].x += AlertParam::Get ().Data ().flowingSpeed * -1;

		if (pos[i].x < -(Common::HalfScreenHeightF ()))
		{
			pos[i].x += Common::HalfScreenWidthF () + Common::ScreenWidthF ();
		}
	}

	alpha=sin()
}

void Alert::Draw () const
{
	Donya::Sprite::Draw ( emergency, Common::HalfScreenWidthF (), Common::HalfScreenHeightF (), 0.0f, Donya::Sprite::Origin::CENTER, alpha );

	for (int i = 0; i < 2; i++)
	{
		Donya::Sprite::Draw ( flowingEmergency, pos[i].x, pos[i].y, 0.0f, Donya::Sprite::Origin::CENTER );
	}
}

#if USE_IMGUI

void Alert::UseImGui ()
{
	if (ImGui::BeginIfAllowed ())
	{
		if (ImGui::TreeNode ( u8"アラート・今のデータ" ))
		{
			for (int i = 0; i < 2; i++)
			{
				ImGui::SliderFloat2 ( (u8"ワールド座標・テロップ" + std::to_string ( i )).c_str (), &pos[i].x, 0.0f, Common::HalfScreenWidthF () );
			}

			ImGui::TreePop ();
		}

		ImGui::End ();
	}
}

#endif // USE_IMGUI
