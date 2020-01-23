#include "Alert.h"

#include <array>
#include <vector>

#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Constant.h"	// Use DEBUG_MODE, scast macros.
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.
#include "Donya/Sprite.h"
#include "Donya/Sound.h"
#include "Donya/Blend.h"

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#endif // DEBUG_MODE

#include "FilePath.h"
#include "Common.h"
#include "Music.h"

class AlertParam final : public Donya::Singleton<AlertParam>
{
	friend Donya::Singleton<AlertParam>;
public:
	struct Member
	{
		float fadeCount{};
		float flashingSpeed{};
		float flowingSpeed{};
		float fadeSpeed{};

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize ( Archive& archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP ( fadeCount ),
				CEREAL_NVP ( flashingSpeed ),
				CEREAL_NVP ( flowingSpeed ),
				CEREAL_NVP ( fadeSpeed )
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
			if (ImGui::TreeNode ( u8"�A���[�g�E�����f�[�^" ))
			{
				ImGui::DragFloat ( u8"�_�ŉ�", &m.fadeCount, 1.0f );
				ImGui::DragFloat ( u8"�_�ő��x", &m.flashingSpeed, 0.1f );
				ImGui::DragFloat ( u8"����鑬�x", &m.flowingSpeed, 0.1f );
				ImGui::DragFloat ( u8"�t�F�[�h�A�E�g���x", &m.fadeSpeed, 0.1f );

				if (ImGui::TreeNode ( u8"�t�@�C��" ))
				{
					static bool isBinary = true;
					if (ImGui::RadioButton ( "Binary", isBinary )) { isBinary = true; }
					if (ImGui::RadioButton ( "JSON", !isBinary )) { isBinary = false; }
					std::string loadStr{ "�ǂݍ��� " };
					loadStr += (isBinary) ? "Binary" : "JSON";

					if (ImGui::Button ( u8"�ۑ�" ))
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
	flashingEmergency (), flowingEmergency (),
	pos (), alpha ( 0.0f ), degree ( 0.0f ), fadeCount ( 0.0f ), state ( 0 )
{}
Alert::~Alert () = default;

void Alert::Init ()
{
	AlertParam::Get ().Init ();

	flashingEmergency = Donya::Sprite::Load ( GetSpritePath ( SpriteAttribute::Emergerncy ) );
	flowingEmergency = Donya::Sprite::Load ( GetSpritePath ( SpriteAttribute::EmergerncySoloFrame ) );

	pos[0].x = Common::HalfScreenWidthF ();
	pos[0].y = Common::HalfScreenHeightF ();
	pos[1] = pos[0];
	pos[1].x += Common::ScreenWidthF ();

	Donya::Sound::Play ( Music::ID::Alert );
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

	switch (state)
	{
	case 0:
		alpha = sinf ( ToRadian ( degree ) );
		if (alpha < 0) { alpha *= -1; }

		degree += AlertParam::Get ().Data ().flashingSpeed;
		if (degree > 180.0f)
		{
			degree -= 180.0f;
		}

		fadeCount += elapsedTime;
		if (fadeCount >= AlertParam::Get ().Data ().fadeCount * 0.8f)
		{
//		Donya::Sound::AppendFadePoint ( Music::ID::Alert, 1.3f, 0.0f, false );
			fadeCount = 1.0f;
			alpha = 1.0f;
			Donya::Sound::Stop ( Music::ID::Alert );
			state++;
		}
		break;

	case 1:
		fadeCount -= AlertParam::Get ().Data ().fadeSpeed;
		alpha -= AlertParam::Get ().Data ().fadeSpeed;
		if (fadeCount <= 0.0f) { fadeCount = 0.0f; }
		if (alpha <= 0.0f) { alpha = 0.0f; }
		break;
	}

	for (int i = 0; i < 2; i++)
	{
		pos[i].x += AlertParam::Get ().Data ().flowingSpeed * -1;

		if (pos[i].x < -Common::HalfScreenWidthF ())
		{
			pos[i].x += Common::ScreenWidthF () * 2;
		}
	}
}

void Alert::Draw () const
{
	Donya::Blend::Activate ( Donya::Blend::Mode::ALPHA_NO_ATC );
	Donya::Sprite::Draw ( flashingEmergency, Common::HalfScreenWidthF (), Common::HalfScreenHeightF (), 0.0f, Donya::Sprite::Origin::CENTER, alpha );

	for (int i = 0; i < 2; i++)
	{
		Donya::Sprite::Draw ( flowingEmergency, pos[i].x, pos[i].y - 240.0f, 0.0f, Donya::Sprite::Origin::CENTER, fadeCount );
		Donya::Sprite::Draw ( flowingEmergency, -pos[i].x + Common::ScreenWidthF () , pos[i].y + 240.0f, 0.0f, Donya::Sprite::Origin::CENTER, fadeCount );
	}
	Donya::Blend::Activate ( Donya::Blend::Mode::ALPHA );
}

#if USE_IMGUI

void Alert::UseImGui ()
{
	if (ImGui::BeginIfAllowed ())
	{
		if (ImGui::TreeNode ( u8"�A���[�g�E���̃f�[�^" ))
		{
			for (int i = 0; i < 2; i++)
			{
				ImGui::SliderFloat2 ( (u8"���[���h���W�E�e���b�v" + std::to_string ( i )).c_str (), &pos[i].x, 0.0f, Common::HalfScreenWidthF () );
			}
			ImGui::DragFloat ( u8"�A���t�@�l", &alpha, 0.1f );
			ImGui::DragFloat ( u8"�_�ő��x", &degree, 0.1f );
			ImGui::DragFloat ( u8"�_�ŉ�", &fadeCount, 0.1f );

			ImGui::TreePop ();
		}

		ImGui::End ();
	}
}

#endif // USE_IMGUI
