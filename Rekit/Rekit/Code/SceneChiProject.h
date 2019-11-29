#pragma once

#include <memory>

#include "Scene.h"

/// <summary>
/// For another project's scene test.
/// </summary>
class SceneChiProject : public Scene
{
public:
	struct Impl;
private:
	std::unique_ptr<Impl> pImpl;
public:
	SceneChiProject();
	~SceneChiProject();
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	Result	ReturnResult();
};
