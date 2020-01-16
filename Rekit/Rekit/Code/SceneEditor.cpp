#include "SceneEditor.h"

#include <cereal/types/vector.hpp>
#include <memory>
#include <vector>


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
#include "Gimmicks.h"

#pragma region EditParam
class EditParam final : public Donya::Singleton<EditParam>
{
	friend Donya::Singleton<EditParam>;
public:
	struct Member
	{
	public:
		std::vector<BoxEx>	editBlocks{};
		std::vector<BoxEx>	debugAllTerrains{};		// Use for collision and drawing.
		std::vector<std::shared_ptr<GimmickBase>> pEditGimmicks;

		Donya::Vector3		transformMousePos{};
		int					stageNum = 1;
		int					doorID = 0;
		SelectGimmick		nowSelect;

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
				CEREAL_NVP(editBlocks)
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
				 archive( CEREAL_NVP( pEditGimmicks ) );
			}
			if (4 <= version)
			{
				//archive(CEREAL_NVP( x ));
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
	public:
	void LoadParameter(bool fromBinary = true)
	{
		std::string id = "EdittedStage:" + std::to_string(m.stageNum);

		std::string filePath = GenerateSerializePath(id, fromBinary);
		Donya::Serializer::Load(m, filePath.c_str(), id.c_str(), fromBinary);
	}

#if USE_IMGUI
public:
	void SaveParameter()
	{
		bool useBinary = true;
		std::string filePath{};

		std::string id = "EdittedStage:" + std::to_string(m.stageNum);

		filePath = GenerateSerializePath(id, useBinary);
		Donya::Serializer::Save(m, filePath.c_str(), id.c_str(), useBinary);

		useBinary = false;

		filePath = GenerateSerializePath(id, useBinary);
		Donya::Serializer::Save(m, filePath.c_str(), id.c_str(), useBinary);
	}
	void GenerateBlock()
	{
#if 0
		auto mousePos = EditParam::Get().Data().transformMousePos;
		BoxEx changeable{ { mousePos.x, mousePos.y, 4.0f, 4.0f, true }, 1 };
		EditParam::Get().DataRef().editBlocks.emplace_back(changeable);
#else

		auto ToInt = [&](GimmickKind kind)
		{
			return scast<int>(kind);
		};

		auto debug = EditParam::Data().nowSelect;
		auto mousePos = EditParam::Get().Data().transformMousePos;


		BoxEx changeable{ { mousePos.x, mousePos.y, 4.0f, 4.0f, true }, 1 };
		switch (EditParam::Data().nowSelect)
		{
		case SelectGimmick::Normal:
			EditParam::Get().DataRef().editBlocks.emplace_back(changeable);
			break;
		case SelectGimmick::Fragile:
			m.pEditGimmicks.push_back(std::make_shared<FragileBlock>());
			m.pEditGimmicks.back()->Init(ToInt(GimmickKind::Fragile), Donya::Vector3(mousePos.x, mousePos.y, 0.0f));
			break;
		case SelectGimmick::Hard:
			m.pEditGimmicks.push_back(std::make_shared<HardBlock>());
			m.pEditGimmicks.back()->Init(ToInt(GimmickKind::Hard), Donya::Vector3(mousePos.x, mousePos.y, 0.0f));
			break;
		case SelectGimmick::TriggerKey:
			m.pEditGimmicks.push_back(std::make_shared<Trigger>(m.doorID, false));
			m.pEditGimmicks.back()->Init(ToInt(GimmickKind::TriggerKey), Donya::Vector3(mousePos.x, mousePos.y, 0.0f));
			break;
		case SelectGimmick::TriggerSwitch:
			m.pEditGimmicks.push_back(std::make_shared<Trigger>(m.doorID, false));
			m.pEditGimmicks.back()->Init(ToInt(GimmickKind::TriggerSwitch), Donya::Vector3(mousePos.x, mousePos.y, 0.0f));
			break;
		case SelectGimmick::TriggerPull:
			m.pEditGimmicks.push_back(std::make_shared<Trigger>(m.doorID, false));
			m.pEditGimmicks.back()->Init(ToInt(GimmickKind::TriggerPull), Donya::Vector3(mousePos.x, mousePos.y, 0.0f));
			break;
		case SelectGimmick::Ice:
			m.pEditGimmicks.push_back(std::make_shared<IceBlock>());
			m.pEditGimmicks.back()->Init(ToInt(GimmickKind::Ice), Donya::Vector3(mousePos.x, mousePos.y, 0.0f));
			break;
		default:
			break;
		}
#endif
	}
	void EraseBlock()
	{
		auto mousePos = EditParam::Get().Data().transformMousePos;
		for (auto itr = m.editBlocks.begin(); itr != m.editBlocks.end(); )
		{
			auto box = *itr;
			//auto mousePos = m.transformMousePos;
			if (box.pos.x - box.size.x/2 > mousePos.x) { itr++; continue; }
			if (box.pos.x + box.size.x/2 < mousePos.x) { itr++; continue; }
			if (box.pos.y - box.size.y/2 > mousePos.y) { itr++; continue; }
			if (box.pos.y + box.size.y/2 < mousePos.y) { itr++; continue; }

			itr = m.editBlocks.erase(itr);
			break;
		}

		for (auto itr = m.pEditGimmicks.begin(); itr != m.pEditGimmicks.end();)
		{
			auto obj = *itr;
			auto pos = obj->GetPosition();
			Donya::Vector3 size(2.0f, 2.0f, 2.0f);

			if (pos.x - size.x / 2 > mousePos.x) { itr++; continue; }
			if (pos.x + size.x / 2 < mousePos.x) { itr++; continue; }
			if (pos.y - size.y / 2 > mousePos.y) { itr++; continue; }
			if (pos.y + size.y / 2 < mousePos.y) { itr++; continue; }

			itr = m.pEditGimmicks.erase(itr);
			break;
		}
	}
	void EraseBlockAll()
	{
		for (auto itr = m.editBlocks.begin(); itr != m.editBlocks.end(); )
		{
			itr = m.editBlocks.erase(itr);
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
				static int lastStageNum;
				lastStageNum = m.stageNum;
				ImGui::InputInt(u8"ステージ", &m.stageNum);
				if (m.stageNum <= 1)m.stageNum = 1;
				if (m.stageNum >= 20)m.stageNum = 20;
				// ステージ切り替えた時にブロックを全消去する
				if (lastStageNum != m.stageNum)
				{
					SceneEditor::isChanges = false;
					EraseBlockAll();
				}

				//if (ImGui::TreeNode(u8"ファイル操作"))
				//{
					static bool isBinary = true;
					if (ImGui::RadioButton( "Binary", isBinary) ) { isBinary = true; }
					ImGui::SameLine(170);
					if (ImGui::RadioButton( "JSON", !isBinary) ) { isBinary = false; }
					std::string loadStr{ "読み込み " };
					loadStr += (isBinary) ? "Binary" : "JSON";

					if (ImGui::Button( u8"保存") )
					{
						SceneEditor::isChanges = false;
						SaveParameter();
					}
					ImGui::SameLine(100);
					if (ImGui::Button( Donya::MultiToUTF8( loadStr ).c_str()) )
					{
						SceneEditor::isChanges = false;
						LoadParameter( isBinary );
					}
					//ImGui::TreePop();
				//}
				ImGui::Text(" ");

				if (ImGui::TreeNode(u8"オブジェクト"))
				{
					ImGui::InputInt(u8"ドアのID", &m.doorID);
					if (m.doorID >= 5)m.doorID = 5;
					if (m.doorID <= 0)m.doorID = 0;

					ImGui::Text("");

					// Select Gimmicks
					{
						ImGui::BeginChild(u8"Select"/*ImGui::GetID((void*)0)*/, ImVec2(250, 100));
						auto data = EditParam::DataRef().nowSelect;
						if (ImGui::Button(u8"Normal Block"))
						{
							data = SelectGimmick::Normal;
						}
						if (ImGui::Button(u8"Fragile"))
						{
							data = SelectGimmick::Fragile;
						}
						if (ImGui::Button(u8"Hard"))
						{
							data = SelectGimmick::Hard;
						}
						if (ImGui::Button(u8"TriggerKey"))
						{
							data = SelectGimmick::TriggerKey;
						}
						if (ImGui::Button(u8"TriggerSwitch"))
						{
							data = SelectGimmick::TriggerSwitch;
						}
						if (ImGui::Button(u8"TriggerPull"))
						{
							data = SelectGimmick::TriggerPull;
						}
						if (ImGui::Button(u8"Ice"))
						{
							data = SelectGimmick::Ice;
						}
						EditParam::DataRef().nowSelect = data;
						ImGui::EndChild();
					}
					if (SceneEditor::isChanges)
					{
						ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), u8"変更されていない箇所があります");
					}
					ImGui::TreePop();
				}
				ImGui::Text("");
				if (ImGui::TreeNode(u8"操作方法"))
				{
					ImGui::Text(u8"左クリック : 選択中のブロックを配置");
					ImGui::Text(u8"右クリック : カーソル上のブロックを削除");
					ImGui::Text(u8"Ctrl + S  : 現在の状態を、選択中のステージデータに上書き保存");
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			ImGui::End();
		}
	}

#endif // USE_IMGUI
};

CEREAL_CLASS_VERSION(EditParam::Member, 3)
#pragma endregion


bool SceneEditor::isChanges;
SceneEditor::SceneEditor() :
	controller(Donya::Gamepad::PAD_1),
	iCamera(),
	dirLight{},
	player{},
	gimmicks{},
	line(256),
	mousePos(Donya::Vector2(0.0f,0.0f)),
	nextSceneType(Scene::Type::Null),
	isPressG(false)
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
	SaveEditParameter();
	LoadEditParameter();
	isPressG = Donya::Keyboard::Press('G');


	return ReturnResult();
}

void SceneEditor::Draw(float elapsedTime)
{

	const Donya::Vector4x4 V = iCamera.CalcViewMatrix();
	const Donya::Vector4x4 P = iCamera.GetProjectionMatrix();

	// Drawing BackGround
	{
		constexpr FLOAT BG_COLOR[4]{ 0.2f, 0.2f, 0.2f, 1.0f };
		Donya::ClearViews(BG_COLOR);
	}

	static auto cirsolBox = Donya::Geometric::CreateCube();

	// Drawing cursor box.
	{
		constexpr Donya::Vector4 cubeColor{ 0.9f, 0.6f, 0.6f, 0.2f };
		Donya::Vector4x4 cubeT{};
		Donya::Vector4x4 cubeS{};
		Donya::Vector4x4 cubeW{};

		// スクリーン座標 -> ワールド座標
		CalcScreenToXY(&EditParam::Get().DataRef().transformMousePos, scast<int>(mousePos.x), scast<int>(mousePos.y), Common::ScreenWidth(), Common::ScreenHeight(), V, P);
		if( !isPressG ) CorrectionGridCursor();
		auto pos = EditParam::Get().DataRef().transformMousePos;
		cubeT = Donya::Vector4x4::MakeTranslation(EditParam::Get().DataRef().transformMousePos);
		cubeS = Donya::Vector4x4::MakeScaling(Donya::Vector3{ 1.0f * 4.0f, 1.0f * 4.0f, 1.0f });
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
		for (const auto& it : EditParam::Get().Data().editBlocks)
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

	// Drawing Objects.
	{
		for (auto& it : EditParam::Get().Data().pEditGimmicks)
		{
			if (!it) { continue; }
			// else

			it->Draw(V, P, dirLight.dir);
		}
	}

	if (isPressG)return;
	// Drawing GridLine
	{
		Donya::Vector3 start = Donya::Vector3(0.0f, 0.0f, 0.0f);
		Donya::Vector3 end = Donya::Vector3(0.0f, 0.0f, 0.0f);


		auto rightUpPos = Donya::Vector2(-38.101f, 21.392f);
		for (int c = 0; c < 6; c++) // 縦
		{
			start = Donya::Vector3(-30.101f, c * 4.0f, 0.0f);
			end =	Donya::Vector3(34.101f, c * 4.0f, 0.0f);

			line.Reserve(start, end, Donya::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
		}
		for (int c = 0; c < 5; c++)
		{
			start = Donya::Vector3(-30.101f, c * -4.0f, 0.0f);
			end = Donya::Vector3(34.101f, c *    -4.0f, 0.0f);

			line.Reserve(start, end, Donya::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
		}

		for (int r = 0; r < 9; r++) // 横
		{
			start = Donya::Vector3(r * 4.0f, 22.392f, 0.0f);
			end = Donya::Vector3(r * 4.0f, -18.392f, 0.0f);

			line.Reserve(start, end, Donya::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
		}

		for (int r = 0; r < 8; r++) // 横
		{
			start = Donya::Vector3(r * -4.0f, 22.392f, 0.0f);
			end = Donya::Vector3(r * -4.0f, -18.392f, 0.0f);
			line.Reserve(start, end, Donya::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
		}


		Donya::Vector4x4 VP = V * P;
		line.Flush(VP);
	}

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
	//bool pushCtrlButton = Donya::Keyboard::Press(VK_LCONTROL) | Donya::Keyboard::Press(VK_RCONTROL);
	bool pressZButton = Donya::Keyboard::Press('Z');
	
	if(ImGui::IsMouseHoveringAnyWindow())return;

	if ( !cleckLeftButton || pressZButton)return;

	SceneEditor::isChanges = true;
	EditParam::Get().GenerateBlock();
}

void SceneEditor::EraseBlockIfRightCleck()
{
	bool cleckRightButton = Donya::Mouse::Trigger(Donya::Mouse::RIGHT);

	if (!cleckRightButton)return;

	SceneEditor::isChanges = true;
	EditParam::Get().EraseBlock();
}

void SceneEditor::CorrectionGridCursor()
{
	auto mousePos = EditParam::Get().DataRef().transformMousePos;

	int div{};
//	if (mousePos.x >= 0.0f)
//	{
//		div = mousePos.x / 2.0f;
//	}
//	else
//	{
//		div = mousePos.x / 2.0f;
//	}

	div = mousePos.x / 4.0f;
	mousePos.x = div * 4.0f;

//	if (mousePos.y >= 0.0f)
//	{
//		div = mousePos.y / 2.0f;
//	}
//	else
//	{
//		div = mousePos.y / 2.0f;
//	}

	div = mousePos.y / 4.0f;
	mousePos.y = div * 4.0f;

	EditParam::Get().DataRef().transformMousePos = mousePos;
}

void SceneEditor::SaveEditParameter()
{
	bool pressCtrl = Donya::Keyboard::Press(VK_LCONTROL);
	bool pressSButton = Donya::Keyboard::Press('S');

	if (!pressCtrl || !pressSButton) return;

	isChanges = false;
	EditParam::Get().SaveParameter();
}

void SceneEditor::LoadEditParameter()
{
	bool pressCtrl = Donya::Keyboard::Press(VK_LCONTROL);
	bool pressRButton = Donya::Keyboard::Press('R');

	if (!pressCtrl || !pressRButton) return;

	isChanges = false;
	EditParam::Get().LoadParameter();
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
//	if (ImGui::BeginIfAllowed())
//	{
//		if (ImGui::TreeNode(u8"エディターの数値見る用"))
//		{
//
//			ImGui::SliderFloat3(u8"変換後のマウス座標", &EditParam::Get().DataRef().transformMousePos.x, -5.0f, 5.0f);
//
//			ImGui::TreePop();
//		}
//		ImGui::End();
//	}
//
	// エディタImGui
	EditParam::Get().UseImGui();
	ImGui::ShowDemoWindow();
}
#endif // USE_IMGUI