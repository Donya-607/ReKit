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
#include "DerivedCollision.h"

#pragma region EditParam
class EditParam final : public Donya::Singleton<EditParam>
{
	friend Donya::Singleton<EditParam>;
public:
	struct Member
	{
	public:
		std::vector<BoxEx>	debugTerrains{};
		std::vector<BoxEx>	debugAllTerrains{};		// Use for collision and drawing.

		Donya::Vector3		transformMousePos{};

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
	void GenerateBlock()
	{
		auto mousePos = EditParam::Get().Data().transformMousePos;
		BoxEx changeable{ { mousePos.x, mousePos.y, 2.0f, 2.0f, true }, 1 };
		EditParam::Get().DataRef().debugTerrains.emplace_back(changeable);
	}
	void EraseBlock()
	{
		auto mousePos = EditParam::Get().Data().transformMousePos;
		for (auto itr = m.debugTerrains.begin(); itr != m.debugTerrains.end(); )
		{
			auto box = *itr;
			auto mousePos = m.transformMousePos;
			if (box.pos.x - box.size.x > mousePos.x) { itr++; continue; }
			if (box.pos.x + box.size.x < mousePos.x) { itr++; continue; }
			if (box.pos.y - box.size.y > mousePos.y) { itr++; continue; }
			if (box.pos.y + box.size.y < mousePos.y) { itr++; continue; }

			itr = m.debugTerrains.erase(itr);
			break;
		}
	}

public:
	void UseImGui()
	{
		m.debugCompressor.pos += m.debugCompressor.velocity;
		if (ImGui::BeginIfAllowed())
		{
			if (ImGui::TreeNode(u8"地形エディタ"))
			{
				static int stageNum = 0;
				ImGui::InputInt(u8"ステージ", &stageNum);
				if (ImGui::TreeNode(u8"ファイル操作"))
				{
					static bool isBinary = true;
					if (ImGui::RadioButton( "Binary", isBinary) ) { isBinary = true; }
					if (ImGui::RadioButton( "JSON", !isBinary) ) { isBinary = false; }
					std::string loadStr{ "読み込み " };
					loadStr += (isBinary) ? "Binary" : "JSON";

					if (ImGui::Button( u8"保存") )
					{
						SaveParameter();
					}
					if (ImGui::Button( Donya::MultiToUTF8( loadStr ).c_str()) )
					{
						LoadParameter( isBinary );
					}
					ImGui::TreePop();
				}
				ImGui::Text(" ");

				if (ImGui::TreeNode(u8"オブジェクト種類"))
				{
					ImGui::Text(u8"to be continued");
					ImGui::TreePop();
				}
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

	iCamera.Init(Donya::ICamera::Mode::Free);
	iCamera.SetPosition(Donya::Vector3(0.0f, 0.0f, -80.0f));
	//iCamera.SetPosition(Donya::Vector3(0.0f, 0.0f, -320.0f));
	line.Init();

}

void SceneEditor::Uninit()
{

}

Scene::Result SceneEditor::Update(float elapsedTime)
{
	// Mouseの状態取得関数
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
#if USE_IMGUI
	UseImGui();
#endif

	GenerateBlockIfCleck();
	EraseBlockIfRightCleck();

	return ReturnResult();
}

void SceneEditor::Draw(float elapsedTime)
{
	bool isPressG = Donya::Keyboard::Press( 'G' );

	const Donya::Vector4x4 V = iCamera.CalcViewMatrix();
	const Donya::Vector4x4 P = iCamera.GetProjectionMatrix();

	// Drawing BackGround
	{
		constexpr FLOAT BG_COLOR[4]{ 0.2f, 0.2f, 0.2f, 1.0f };
		Donya::ClearViews(BG_COLOR);
	}

	static auto cirsolBox = Donya::Geometric::CreateCube();
	// Drawing cirsol box.
	{
		constexpr Donya::Vector4 cubeColor{ 0.9f, 0.6f, 0.6f, 0.6f };
		Donya::Vector4x4 cubeT{};
		Donya::Vector4x4 cubeS{};
		Donya::Vector4x4 cubeW{};

		// スクリーン座標 -> ワールド座標
		CalcScreenToXY(&EditParam::Get().DataRef().transformMousePos, scast<int>(mousePos.x), scast<int>(mousePos.y), Common::ScreenWidth(), Common::ScreenHeight(), V, P);

		cubeT = Donya::Vector4x4::MakeTranslation(EditParam::Get().DataRef().transformMousePos);
		cubeS = Donya::Vector4x4::MakeScaling(Donya::Vector3{ 1.0f*2.0f, 1.0f*2.0f, 1.0f });
		cubeW = cubeS * cubeT;

		cirsolBox.Render
		(
			nullptr,
			/* useDefaultShading	= */ true,
			/* isEnableFill			= */ true,
			(cubeW * V * P), cubeW,
			dirLight.dir,
			cubeColor
		);

	}

	// Drawing Block.
	{
		static auto cube = Donya::Geometric::CreateCube();

		constexpr Donya::Vector4 cubeColor{ 0.9f, 0.9f, 0.3f, 0.6f };
		Donya::Vector4x4 cubeT{};
		Donya::Vector4x4 cubeS{};
		Donya::Vector4x4 cubeW{};
		for (const auto& it : EditParam::Get().Data().debugTerrains)
		{
			// The drawing size is whole size.
			// But a collision class's size is half size.
			// So we should to double it size.

			cubeT = Donya::Vector4x4::MakeTranslation(Donya::Vector3{ it.pos, 0.0f });
			cubeS = Donya::Vector4x4::MakeScaling(Donya::Vector3{ it.size, 1.0f });
			cubeW = cubeS * cubeT;

			cube.Render
			(
				nullptr,
				/* useDefaultShading	= */ true,
				/* isEnableFill			= */ true,
				(cubeW * V * P), cubeW,
				dirLight.dir,
				cubeColor
			);
		}
	}


	Donya::Vector3 start = Donya::Vector3(0.0f, 0.0f, 0.0f);
	Donya::Vector3 end = Donya::Vector3(10.0f, 0.0f, 0.0f);
	line.Reserve(start, end, Donya::Vector4(1.0f,1.0f,1.0f,1.0f));

	start = Donya::Vector3(0.0f, 10.0f, 0.0f);
	end = Donya::Vector3(10.0f, 10.0f, 0.0f);
	line.Reserve(start, end, Donya::Vector4(1.0f, 0.0f, 0.0f, 1.0f));

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


void SceneEditor::GenerateBlockIfCleck()
{
	bool cleckLeftButton = Donya::Mouse::Trigger(Donya::Mouse::LEFT);

	if ( !cleckLeftButton )return;

	EditParam::Get().GenerateBlock();
}

void SceneEditor::EraseBlockIfRightCleck()
{
	bool cleckRightButton = Donya::Mouse::Trigger(Donya::Mouse::RIGHT);

	if (!cleckRightButton)return;

	EditParam::Get().EraseBlock();
}


Donya::Vector3* SceneEditor::CalcScreenToWorld(
	Donya::Vector3* pout, 
	int screenPosX, int screenPosY, float fZ, 
	int screenWidth, int screenHeight, 
	Donya::Vector4x4 view, Donya::Vector4x4 projection)
{
	// 行列を使って頂点座標変換する関数
	auto TransformCoord = [&](Donya::Vector3* pout, const Donya::Vector3* pv, const Donya::Vector4x4* pm)
	{
		Donya::Vector3 out;
		float norm;

		norm = pm->m[0][3] * pv->x + pm->m[1][3] * pv->y + pm->m[2][3] * pv->z + pm->m[3][3];

		out.x = (pm->m[0][0] * pv->x + pm->m[1][0] * pv->y + pm->m[2][0] * pv->z + pm->m[3][0]) / norm;
		out.y = (pm->m[0][1] * pv->x + pm->m[1][1] * pv->y + pm->m[2][1] * pv->z + pm->m[3][1]) / norm;
		out.z = (pm->m[0][2] * pv->x + pm->m[1][2] * pv->y + pm->m[2][2] * pv->z + pm->m[3][2]) / norm;

		*pout = out;
		return pout;
	};

	// 各行列の逆行列を算出
	using namespace Donya;
	Vector4x4 invView, invPrj, vp, invVp;

	invView = view.Inverse();
	invPrj = projection.Inverse();

	vp = vp.Identity();
	vp.m[0][0] = screenWidth / 2.0f;	vp.m[1][1] = -screenHeight / 2.0f;
	vp.m[3][0] = screenWidth / 2.0f;	vp.m[3][1] = screenHeight  / 2.0f;
	invVp = vp.Inverse();

	Donya::Vector4x4 inverseMatrixes = invVp * invPrj * invView;

#if 0
	Donya::Vector4 tmp = Donya::Vector4(scast<float>(screenPosX), scast<float>(screenPosY), fZ, 0.0f);
	Donya::Vector4 out = tmp * inverseMatrixes;

	pout->x = out.x;
	pout->y = out.y;
	pout->z = out.z;

#else
	TransformCoord(pout, &Donya::Vector3(scast<float>(screenPosX), scast<float>(screenPosY), fZ), &inverseMatrixes);
#endif
	return pout;
}


Donya::Vector3* SceneEditor::CalcScreenToXY(
	Donya::Vector3* pout, 
	int Sx, int Sy, 
	int screenWidth, int screenHeight, 
	Donya::Vector4x4 view, Donya::Vector4x4 projection)
{
	using namespace Donya;

	Vector4 nearPos, farPos;
	Vector3 vec3NearPos, vec3FarPos, ray;

	// 頂点変換用の行列を作成
	Vector4x4 invView, invPrj, vp, invVp;

	// ビューポート行列の作成
	vp = vp.Identity();
	vp.m[0][0] = screenWidth / 2.0f;	vp.m[1][1] = -screenHeight / 2.0f;
	vp.m[3][0] = screenWidth / 2.0f;	vp.m[3][1] = screenHeight / 2.0f;

	invView = view.Inverse();
	invPrj = projection.Inverse();
	invVp = vp.Inverse();

	Donya::Vector4x4 inverseMatrixes = invVp * invPrj * invView;


	// 最近値と最遠値をそれぞれ座標変換
	nearPos = Vector4(Sx, Sy, 0.0f, 1.0f);
	nearPos = nearPos * inverseMatrixes;
	vec3NearPos = Vector3(nearPos.x / nearPos.w, nearPos.y / nearPos.w, nearPos.z / nearPos.w);

	farPos  = Vector4(Sx, Sy, 1.0f, 1.0f);
	farPos = farPos * inverseMatrixes;
	vec3FarPos = Vector3(farPos.x / farPos.w, farPos.y / farPos.w, farPos.z / farPos.w);


	// 線分を取得
	ray = vec3FarPos - vec3NearPos;
	ray.Normalize();

	// XY平面との交点を求める
	float Lray = Vector3::Dot(ray, Vector3(0.0f, 0.0f, -1.0f));
	float LP0 = Vector3::Dot(-vec3NearPos, Vector3(0.0f, 0.0f, -1.0f));
	*pout = vec3NearPos + (LP0 / Lray) * ray;


	return pout;
}


#if USE_IMGUI
void SceneEditor::UseImGui()
{
	if (ImGui::BeginIfAllowed())
	{
		if (ImGui::TreeNode(u8"エディターの数値見る用"))
		{

			ImGui::SliderFloat3(u8"変換後のマウス座標", &EditParam::Get().DataRef().transformMousePos.x, -5.0f, 5.0f);

			ImGui::TreePop();
		}
		ImGui::End();
	}

	// エディタImGui
	EditParam::Get().UseImGui();
}
#endif // USE_IMGUI