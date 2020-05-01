#pragma once

#include <cstdint>
#include <string>

#include "Math.h" // for vec3<T>


class Character;


class AI {
protected:

	std::string		m_Name;
	const uint16_t	m_Version;
	const uint32_t	m_IndexAI;

public:

	enum TargetTerrainType
	{
		TTT_GROUND = 0,		// Anywhere that has ground and is not too deep in water
		TTT_GROUNDONLY,		// Only on ground, never in liquids
		TTT_SHALLOWS,		// For "Brachio" types
		TTT_FLYING,			// Flyers (duh)
		TTT_WATER,			// General water
		TTT_DEEPWATER		// Deep water location only
	};

	virtual void animate(Character& character) = 0;
	virtual void animateDead(Character& character) = 0;

	std::string getName() const;
	uint16_t	getVersion() const;
	uint32_t	getID() const;

	/* Play the associated soundFX of the character's animation */
	void 	activateCharacterFx(Character& character);
	/* Check of the point is colliding with steep terrain, water, objects, etc. */
	bool 	checkPlaceCollision(vec3<float> &v, bool check_water = false, bool check_objects = false);
	bool 	checkPlaceCollisionP(vec3<float> &v);
	bool 	checkPlaceCollision2(vec3<float> &v, bool check_water);
	/* Return the amount of collisions detected along a defined path */
	int		checkPossiblePath(Character &character, bool check_water, bool check_objects);
	/* Try and find a new way around the character */
	void	lookForAWay(Character& character, bool check_water, bool check_objects);
	/* Handle the Phase state change from current to previous */
	void	processPrevPhase(Character& character);
	/* Choose a new place to become the target of the character */
	void 	setNewTargetPlace(Character& character, float range, TargetTerrainType type = TTT_GROUND);
};
