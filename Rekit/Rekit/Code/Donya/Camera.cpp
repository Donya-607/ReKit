#include "../../Code/Header/Camera.h" // The "Camera.h" is also used by Donya-library, so explicit.

#include <algorithm> // use std::min.

#include "Common.h"
#include "Header/Constant.h"
#include "FilePath.h"
#include "Stage.h" // Use for know width of stage.
#include "Header/Useful.h"

Camera::Camera() :
	stageNo( 0 ),
	stickMoveSpeed( 0 ), easePower( 0.1f ),
	pos(), prevTargetPos(),
	screenSize(),
	distanceFromEdge(),
	mostEdgePos(),
	isReachedEdgeX( false )
{

}

void Camera::Init( int stageNumber )
{
	LoadParameter();

	stageNo			= stageNumber;

	pos				= {};

	screenSize.x	= Common::ScreenWidthF();
	screenSize.y	= Common::ScreenHeightF();

	auto pStage = StageAccessor::GetStageOrNull( stageNo );
	if ( !pStage )
	{
		mostEdgePos = {};
	}
	else
	{
		Box lastChip  = pStage->GetLastChipBody();

		mostEdgePos.x = lastChip.cx + lastChip.w;
		mostEdgePos.y = lastChip.cy + lastChip.h;
	}
}

void Camera::Update( Donya::Vector2 inputVelocity )
{
#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

	inputVelocity.x = 0;
	inputVelocity *= stickMoveSpeed;
	//Scroll( prevTargetPos, inputVelocity, 0 );

	ClampPos();
}

void Camera::Scroll( Donya::Vector2 targetPos, Donya::Vector2 targetVelocity, int targetMoveDirectionSide )
{
	HorizontalScroll( targetPos, targetVelocity, targetMoveDirectionSide );
	VerticalScroll( targetPos, targetVelocity );

	// ScrollToCenter( targetPos, targetVelocity );

	prevTargetPos = targetPos;
}

void Camera::ClampPos()
{
	bool wasClampedX = false;

	if ( mostEdgePos.x <= pos.x + screenSize.x )
	{
		pos.x = mostEdgePos.x - screenSize.x;
		wasClampedX = true;
	}
	if ( pos.x <= 0.0f )
	{
		pos.x = 0.0f;
		wasClampedX = true;
	}

	isReachedEdgeX = wasClampedX;
	
	if ( mostEdgePos.y <= pos.y + screenSize.y )
	{
		pos.y = mostEdgePos.y - screenSize.y;
	}
	if ( pos.y <= 0.0f )
	{
		pos.y = 0.0f;
	}
}

void Camera::HorizontalScroll( Donya::Vector2 targetPos, Donya::Vector2 targetVelocity, int targetMoveDirectionSide )
{
	auto IsInsideOfRange = [&]( Donya::Vector2 screenPos, int horizontalMoveSign )->bool
	{
		if ( horizontalMoveSign == -1 && screenSize.x - distanceFromEdge.x <= screenPos.x )
		{
			return false;
		}
		if ( horizontalMoveSign == 1 && screenPos.x <= distanceFromEdge.x )
		{
			return false;
		}

		return true;
	};

	Donya::Vector2 scrTargetPos = targetPos - pos;
	if ( !IsInsideOfRange( scrTargetPos, targetMoveDirectionSide ) ) { return; }
	// else

	pos.x += targetVelocity.x;
	ClampPos();

	if ( fabsf( targetVelocity.x ) < enableLerpBorder.x ) { return; }
	// else

	float destination	= ( targetMoveDirectionSide == -1 )
						? targetPos.x - ( screenSize.x - distanceFromEdge.x )
						: targetPos.x - ( distanceFromEdge.x );

	float correction = destination - pos.x;

	pos.x += correction * easePower;
	ClampPos();
}
void Camera::VerticalScroll( Donya::Vector2 targetPos, Donya::Vector2 targetVelocity )
{
	auto IsInsideOfRange = [&]( Donya::Vector2 screenPos, int sign )->bool
	{
		if ( sign == -1 && screenSize.y - distanceFromEdge.y <= screenPos.y )
		{
			return false;
		}
		if ( sign == 1 && screenPos.y <= distanceFromEdge.y )
		{
			return false;
		}

		return true;
	};

	int moveSign = ( targetVelocity.y <= 0.0f ) ? -1 : 1;
	Donya::Vector2 scrTargetPos = targetPos - pos;

	// if ( fabsf( targetVelocity.y ) < enableLerpBorder.y ) { return; }
	// else

	pos.y += targetVelocity.y;
	ClampPos();

	// if ( !IsInsideOfRange( scrTargetPos, moveSign ) ) { return; }
	// else

	float destination = targetPos.y - ( screenSize.y - distanceFromEdge.y );
						// ? targetPos.y - ( screenSize.y - distanceFromEdge.y )
						// : targetPos.y - ( distanceFromEdge.y );
	float correction  = destination - pos.y;

	pos.y += correction; // *( easePower * 0.8f/*little weaken*/ );
	ClampPos();
}

void Camera::ScrollToCenter( Donya::Vector2 targetPosition, Donya::Vector2 targetVelocity )
{
	Donya::Vector2 normVel = targetVelocity;
	normVel.Normalize();

	constexpr float REGARD_MOVE_BORDER = 0.4f;
	if ( fabsf( normVel.y ) < REGARD_MOVE_BORDER ) { return; }
	// else

	// If you use "return when you are near the center", write here.

	float dist = targetPosition.x - ( pos.x + ( screenSize.x * 0.5f ) );

	pos.x += dist * easePower;
}

void Camera::LoadParameter( bool isBinary )
{
	Donya::Serializer::Extension ext =	( isBinary )
	? Donya::Serializer::Extension::BINARY
	: Donya::Serializer::Extension::JSON;
	std::string filePath = GenerateSerializePath( SERIAL_ID, ext );

	Donya::Serializer seria;
	seria.Load( ext, filePath.c_str(), SERIAL_ID, *this );
}

#if USE_IMGUI

void Camera::SaveParameter()
{
	Donya::Serializer::Extension bin  = Donya::Serializer::Extension::BINARY;
	Donya::Serializer::Extension json = Donya::Serializer::Extension::JSON;
	std::string binPath  = GenerateSerializePath( SERIAL_ID, bin );
	std::string jsonPath = GenerateSerializePath( SERIAL_ID, json );

	Donya::Serializer seria;
	seria.Save( bin,  binPath.c_str(),  SERIAL_ID, *this );
	seria.Save( json, jsonPath.c_str(), SERIAL_ID, *this );
}

void Camera::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"カメラ" ) )
		{
			ImGui::Text( u8"位置[X:%5.3f][Y:%5.3f]", pos.x, pos.y );

			ImGui::SliderFloat( u8"入力による移動の速さ", &stickMoveSpeed, 0.1f, 64.0f );
			ImGui::SliderFloat( u8"補間の強さ", &easePower, 0.0001f, 1.0f );
			ImGui::SliderFloat2( u8"[X, Y]この値より遅かったら補間をかけない", &enableLerpBorder.x, 0.0f, 128.0f );

			if ( ImGui::TreeNode( u8"ファイル（カメラ）" ) )
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
