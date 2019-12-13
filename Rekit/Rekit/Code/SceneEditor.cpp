#include "SceneEditor.h"

#include <cereal/types/vector.hpp>

SceneEditor::SceneEditor()
{

}

SceneEditor::~SceneEditor() = default;

void SceneEditor::Init()
{

}

void SceneEditor::Uninit()
{

}

Scene::Result SceneEditor::Update(float elapsedTime)
{
	return Result();
}

void SceneEditor::Draw(float elapsedTime)
{

}

Scene::Result SceneEditor::ReturnResult()
{
	return Result();
}

#if USE_IMGUI
void SceneEditor::UseImGui()
{

}
#endif // USE_IMGUI