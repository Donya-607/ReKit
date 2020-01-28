#include "ScenePause.h"

#include <algorithm>
#include <string>
#include <vector>

#include <cereal/types/vector.hpp>

#include "Donya/Constant.h"
#include "Donya/Keyboard.h"
#include "Donya/Serializer.h"
#include "Donya/Sprite.h"
#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Vector.h"

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"

#undef max
#undef min


class PauseParam final : public Donya::Singleton<PauseParam>
{
	friend Donya::Singleton<PauseParam>;
public:
	struct Member
	{
	public:
		int							sentenceCount{ 2 };
		std::vector<Donya::Vector2>	sentencePositions{};	// Screen space.
		std::vector<Donya::Int2>	sentenceTexOrigins{};	// Left top.
		std::vector<Donya::Int2>	sentenceTexSizes{};		// Whole size.
		Donya::Vector2				arrowRelativePos{};		// Related by sentencePosition.
		Donya::Int2					arrowTexOrigin{};
		Donya::Int2					arrowTexSize{};
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( sentenceCount ),
				CEREAL_NVP( sentencePositions ),
				CEREAL_NVP( sentenceTexOrigins ),
				CEREAL_NVP( sentenceTexSizes ),
				CEREAL_NVP( arrowRelativePos ),
				CEREAL_NVP( arrowTexOrigin ),
				CEREAL_NVP( arrowTexSize )
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "PauseConfig";
	Member m;
private:
	PauseParam() : m() {}
public:
	~PauseParam() = default;
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
public:
	static std::string GetSentenceName( int sentenceIndex )
	{
		switch ( sentenceIndex )
		{
		case 0:  return "Resume";
		case 1:  return "BackToTitle";
		case 2:  return "Retry";
		default: return "Error!";
		}
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
			if ( ImGui::TreeNode( u8"ポーズ・設定" ) )
			{
				if ( ImGui::TreeNode( u8"文章関連" ) )
				{
					const int oldCount = m.sentenceCount;
					ImGui::InputInt( u8"文章の数", &m.sentenceCount );
					if ( oldCount != m.sentenceCount )
					{
						m.sentencePositions.resize( m.sentenceCount );
						m.sentenceTexSizes.resize( m.sentenceCount );
						m.sentenceTexOrigins.resize( m.sentenceCount );
					}

					if ( ImGui::TreeNode( u8"スクリーン座標" ) )
					{
						for ( int i = 0; i < m.sentenceCount; ++i )
						{
							ImGui::DragFloat2( GetSentenceName( i ).c_str(), &m.sentencePositions[i].x );
						}
						ImGui::TreePop();
					}
					if ( ImGui::TreeNode( u8"切り取り原点（左上）" ) )
					{
						for ( int i = 0; i < m.sentenceCount; ++i )
						{
							ImGui::DragInt2( GetSentenceName( i ).c_str(), &m.sentenceTexOrigins[i].x );
						}
						ImGui::TreePop();
					}
					if ( ImGui::TreeNode( u8"切り取りサイズ（全体幅）" ) )
					{
						for ( int i = 0; i < m.sentenceCount; ++i )
						{
							ImGui::DragInt2( GetSentenceName( i ).c_str(), &m.sentenceTexSizes[i].x );
						}
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"矢印関連" ) )
				{
					ImGui::DragFloat2( u8"文章からのオフセット位置",	&m.arrowRelativePos.x );
					ImGui::DragInt2( u8"テクスチャ原点（左上）",		&m.arrowTexOrigin.x );
					ImGui::DragInt2( u8"テクスチャサイズ（全体幅）",	&m.arrowTexSize.x );

					ImGui::TreePop();
				}

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
};
CEREAL_CLASS_VERSION( PauseParam::Member, 0 )


ScenePause::ScenePause() :
	choice( Choice::Resume ),
	sprBoard(), sprSentences(),
	nextSceneType(),
	controller( Donya::XInput::PadNumber::PAD_1 ),
	idBoard( NULL ), idSentences( NULL ),
	pos()
{}
ScenePause::~ScenePause() = default;

void ScenePause::Init()
{
	PauseParam::Get().Init();

	sprBoard.LoadSheet		( GetSpritePath( SpriteAttribute::PauseBoard ),		16U );
	sprSentences.LoadSheet	( GetSpritePath( SpriteAttribute::PauseSentences ),	16U );

	sprBoard.SetPartCount( { 1, 1 } );
	sprSentences.SetPartCount( { 1, PauseParam::Get().Data().sentenceCount } );

	idBoard = Donya::Sprite::Load( GetSpritePath( SpriteAttribute::PauseBoard ) );
	idSentences = Donya::Sprite::Load( GetSpritePath( SpriteAttribute::PauseSentences ) );

	pos.x = Common::HalfScreenWidthF() - 475.0f;
	pos.y = 450.0f;
}

void ScenePause::Uninit()
{
	
}

Scene::Result ScenePause::Update( float elapsedTime )
{
#if USE_IMGUI
	UseImGui();
#endif

	controller.Update();

	if ( !Fader::Get().IsExist() )
	{
		UpdateChooseItem();
	}

#if DEBUG_MODE
	// Scene Transition Demo.
	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			if ( !Fader::Get().IsExist() )
			{
				nextSceneType = Scene::Type::Game;
				StartFade();
			}
		}
		else if ( Donya::Keyboard::Trigger( 'T' ) )
		{
			if ( !Fader::Get().IsExist() )
			{
				nextSceneType = Scene::Type::Title;
				StartFade();
			}
		}
	}
#endif // DEBUG_MODE

	pos.y = scast<float>( choice ) * 150.0f + 450.0f;

	return ReturnResult();
}

void ScenePause::Draw( float elapsedTime )
{
	Donya::Sprite::Draw( idBoard, 0.0f, 0.0f, 0.0f, Donya::Sprite::Origin::LEFT_TOP );
	Donya::Sprite::DrawPart( idSentences, Common::HalfScreenWidthF()+50.0f, Common::HalfScreenHeightF()+60.0f, 0.0f, 116.0f, 864.0f, 680.0f, 0.0f, Donya::Sprite::Origin::CENTER );
	Donya::Sprite::DrawPart( idSentences, pos.x, pos.y, 0.0f, 0.0f, 116.0f, 116.0f, 0.0f, Donya::Sprite::Origin::CENTER );
}

void ScenePause::UpdateChooseItem()
{
	bool up{}, down{};
	if ( controller.IsConnected() )
	{
		up		= controller.Trigger( Donya::Gamepad::Button::UP	);
		down	= controller.Trigger( Donya::Gamepad::Button::DOWN	);
		if ( !up )
		{
			up		= controller.TriggerStick( Donya::Gamepad::StickDirection::UP	);
		}
		if ( !down )
		{
			down	= controller.TriggerStick( Donya::Gamepad::StickDirection::DOWN	);
		}
	}
	else
	{
		up		= Donya::Keyboard::Trigger( VK_UP	);
		down	= Donya::Keyboard::Trigger( VK_DOWN	);
	}

	int index		= scast<int>( choice );
	int oldIndex	= index;

	if ( up		) { index--; }
	if ( down	) { index++; }

	index = std::max( 0, std::min( scast<int>( Choice::ItemCount ) - 1, index ) );

	if ( index != oldIndex )
	{
		Donya::Sound::Play( Music::ItemChoose );
	}

	choice = scast<Choice>( index );
}

void ScenePause::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result ScenePause::ReturnResult()
{
	bool useResume{};
	bool useDecision{};
	if ( controller.IsConnected() )
	{
		useResume	= controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT );
		useDecision	= controller.Trigger( Donya::Gamepad::Button::A ) || controller.Trigger( Donya::Gamepad::Button::B );
	}
	else
	{
		useResume	= Donya::Keyboard::Trigger( 'P' );
		useDecision	= Donya::Keyboard::Trigger( VK_RETURN );
	}
	if ( useResume )
	{
		Donya::Sound::Play( Music::ItemDecision );

		Scene::Result change{};
		change.AddRequest( Scene::Request::REMOVE_ME );
		return change;
	}

	if ( useDecision )
	{
		Donya::Sound::Play( Music::ItemDecision );
//		StartFade();
		Scene::Result change{};
		switch (choice)
		{
		case Choice::Resume:
			change.AddRequest( Scene::Request::REMOVE_ME );
			break;
		case Choice::BackToTitle:
			if (!Fader::Get().IsExist())
			{
				nextSceneType = Scene::Type::Title;
				StartFade();
			}
		//	change.AddRequest( Scene::Request::ADD_SCENE );
		//	change.sceneType = Scene::Type::Title;
			break;
		case Choice::Retry:
			if (!Fader::Get().IsExist())
			{
				nextSceneType = Scene::Type::Game;
				StartFade();
			}
		//	change.AddRequest( Scene::Request::ADD_SCENE );
		//	change.sceneType = Scene::Type::Game;
			break;
		}
		return change;
	}

	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.sceneType = nextSceneType;
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ALL );
		return change;
	}

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
void ScenePause::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"ポーズ・確認用(empty)" ) )
		{

			ImGui::TreePop();
		}
		ImGui::End();
	}
}
#endif // USE_IMGUI