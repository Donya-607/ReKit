#pragma once

#include <vector>

#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

class Alert
{
private:
	size_t flashingEmergency;
	size_t flowingEmergency;

	Donya::Vector2	pos[2];	// Screen space.
	float			alpha;
	float			degree;
	float			fadeCount;
	int				state;
public:
	Alert ();
	~Alert ();
public:
	void Init ();
	void Uninit ();

	void Update ( float elpasedTime );

	void Draw () const;

#if USE_IMGUI
private:

	void UseImGui ();

#endif // USE_IMGUI
};
