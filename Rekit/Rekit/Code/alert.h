#pragma once

#include <vector>

#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

class Alert
{
private:
	size_t emergency;			// Flashing emergency
	size_t flowingEmergency;	// Flowing emergency

	Donya::Vector2		pos[2];	// Screen space.
	float				alpha;
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
