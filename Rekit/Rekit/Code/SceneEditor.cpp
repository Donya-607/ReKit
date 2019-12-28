#include "SceneEditor.h"

#include <cereal/types/vector.hpp>

#include "Donya/Keyboard.h"
#include "Donya/Camera.h"
#include "Donya/CBuffer.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"		// Use GetFPS().
#include "Donya/GeometricPrimitive.h"
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Quaternion.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"

#pragma region EditParam
class EditParam final : public Donya::Singleton<EditParam>
{
	friend Donya::Singleton<EditParam>;
public:
	struct Member
	{
	public:
		std::vector<BoxEx> debugTerrains{};
		std::vector<BoxEx> debugAllTerrains{};		// Use for collision and drawing.

		BoxEx debugCompressor{ { 0.0f, 0.0f, 0.0f, 0.0f, false }, 0 };
		BoxEx debugClearTrigger{ { 0.0f, 0.0f, 0.0f, 0.0f, false }, 0 };

		Donya::Vector3 initCameraPos{};
		Donya::Vector3 initPlayerPos{};
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& archive, std::uint32_t version)
		{
			archive
			(
				CEREAL_NVP(debugTerrains)
			);
			if (1 <= version)
			{
				archive(CEREAL_NVP(debugClearTrigger));
			}
			if (2 <= version)
			{
				archive
				(
					CEREAL_NVP(initPlayerPos),
					CEREAL_NVP(initCameraPos)
				);
			}
			if (3 <= version)
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char* SERIAL_ID = "EditStageBlocks";
	Member m;
private:
	EditParam() : m() {}
public:
	~EditParam() = default;
public:
	void Init()
	{
		LoadParameter();
	}
	void Uninit()
	{

	}
public:
	Member Data() const
	{
		return m;
	}
	Member& DataRef()
	{
		return m;
	}
private:
	void LoadParameter(bool fromBinary = true)
	{
		std::string filePath = GenerateSerializePath(SERIAL_ID, fromBinary);
		Donya::Serializer::Load(m, filePath.c_str(), SERIAL_ID, fromBinary);
	}

#if USE_IMGUI

	void SaveParameter()
	{
		bool useBinary = true;
		std::string filePath{};

		filePath = GenerateSerializePath(SERIAL_ID, useBinary);
		Donya::Serializer::Save(m, filePath.c_str(), SERIAL_ID, useBinary);

		useBinary = false;

		filePath = GenerateSerializePath(SERIAL_ID, useBinary);
		Donya::Serializer::Save(m, filePath.c_str(), SERIAL_ID, useBinary);
	}

public:
	void UseImGui()
	{
		m.debugCompressor.pos += m.debugCompressor.velocity;
		if (ImGui::BeginIfAllowed())
		{
			if (ImGui::TreeNode(u8"地形エディタ"))
			{

				ImGui::TreePop();
			}

			ImGui::End();
		}
	}

#endif // USE_IMGUI
};

CEREAL_CLASS_VERSION(EditParam::Member, 2)
#pragma endregion


SceneEditor::SceneEditor() :
	iCamera(),
	nextSceneType(Scene::Type::Null),
	controller(Donya::Gamepad::PAD_1)
{

}

SceneEditor::~SceneEditor() = default;

void SceneEditor::Init()
{
	CameraInit();

	gimmicks.Init(NULL);
	player.Init(EditParam::Get().Data().initPlayerPos);

}

void SceneEditor::Uninit()
{

}

Scene::Result SceneEditor::Update(float elapsedTime)
{
	// Mouseの状態取得関数
	Donya::Vector2 mousePos;
	Donya::Mouse::Coordinate(&mousePos.x, &mousePos.y);

	Donya::ICamera::Controller ctrl{};
	iCamera.Update(ctrl);

	// Scene Transition Demo.
	{
		bool pressCtrl = Donya::Keyboard::Press(VK_LCONTROL) || Donya::Keyboard::Press(VK_RCONTROL);
		bool triggerG = Donya::Keyboard::Trigger('G');
		bool triggerT = Donya::Keyboard::Trigger('T');
		if (pressCtrl && triggerT)
		{
			if (!Fader::Get().IsExist())
			{
				nextSceneType = Scene::Type::Title;
				StartFade();
			}
		}
		else if (pressCtrl && triggerG)
		{
			if (!Fader::Get().IsExist())
			{
				nextSceneType = Scene::Type::Game;
				StartFade();
			}
		}
	}
	return ReturnResult();
}

void SceneEditor::Draw(float elapsedTime)
{
	bool isPressG = Donya::Keyboard::Press( 'G' );

	Donya::Box cirsolBox;
//	cirsolBox.Set(カーソルX, カーソルY, 1.0f, 1.0f, false);

	Donya::Geometric::Line line;
	line.Init();

	Donya::Vector3 start = Donya::Vector3(0.0f, 0.0f, 0.0f);
	Donya::Vector3 end = Donya::Vector3(1.0f, 0.0f, 0.0f);
	line.Reserve(start, end);

	Donya::Vector4x4 V = iCamera.CalcViewMatrix();
	Donya::Vector4x4 P = iCamera.GetProjectionMatrix();
	Donya::Vector4x4 VP = V * P;
	line.Flush(VP);

}

void SceneEditor::StartFade()const
{
	Fader::Configuration config{};
	config.type = Fader::Type::Gradually;
	config.closeFrame = Fader::GetDefaultCloseFrame();
	config.SetColor(Donya::Color::Code::BLACK);
	Fader::Get().StartFadeOut(config);
}

void SceneEditor::CameraInit()
{
	iCamera.Init(Donya::ICamera::Mode::Look);
	iCamera.SetZRange(0.1f, 1000.0f);
	iCamera.SetFOV(ToRadian(30.0f));
	iCamera.SetScreenSize({ Common::ScreenWidthF(), Common::ScreenHeightF() });
	iCamera.SetPosition(EditParam::Get().Data().initCameraPos);
	iCamera.SetFocusPoint({ 0.0f, 0.0f, 0.0f });
	iCamera.SetProjectionPerspective();

	// I can setting a configuration,
	// but current data is not changed by this immediately.
	// So update here.

	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;
	iCamera.Update(moveInitPoint);
}

Scene::Result SceneEditor::ReturnResult()
{
	if (Fader::Get().IsClosed())
	{
		Scene::Result change{};
		change.AddRequest(Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME);
		change.sceneType = nextSceneType;
		return change;
	}

	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
void SceneEditor::UseImGui()
{

}
#endif // USE_IMGUI