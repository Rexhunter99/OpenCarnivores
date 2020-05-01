#include "AI.h"
#include "Character.h"

#include <algorithm> // For min()/max()
#include <cmath>


// These are implemented in Game.cpp
float GetLandH(float x, float y);
float GetLandUpH(float x, float y);


std::string AI::getName() const
{
	return m_Name;
}

uint16_t AI::getVersion() const
{
	return m_Version;
}

uint32_t AI::getID() const
{
	return m_IndexAI; // This value is the same as the "ai" index in the _RES file.
}


/**
 * Play the current animation's soundFX if the player isn't more than 68 cells away
 */
void AI::activateCharacterFx(Character& character)
{
    if (character.m_IndexAI && UNDERWATER) //== not hunter ==//
	{
		// TODO: Add a better check for player underwater, and play a distorted effect instead
		return;
	}

    int fx = character.charInfo->Anifx[character.phase];

    if (fx == -1)
    {
    	return;
    }

    if ( glm::length(glm::vec3(PlayerPos - character.position)) ) > 68.0f * 256.0f )
    {
    	return;
    }

	TSound& sound = character.charInfo->SoundFX[fx]
    AddVoice3d(sound.length, sound.lpData, character.position.x, character.position.y, character.position.z);
}

void AI::setNewTargetPlace(Character& character, float range, TargetTerrainType type)
{
	vec3<float> p(0.0f);

	// Recursively attempt to set a new target location no more than 256 times
	for (int attempts = 0; attempts < 1024; attempts++)
	{
		p.x = std::max( 512, std::min(506*256, character.position.x + siRand((int)range) ));
		p.z = std::max( 512, std::min(506*256, character.position.z + siRand((int)range) ));
		p.y = GetLandH(p.x, p.z);

		attempts++;

		if ( type == TTT_GROUND )
		{
			if (attempts < 128)
			{
				if ( std::fabs(p.x - character.position.x) + std::fabs(p.z - character.position.z) < range / 2.f)
					continue;
			}

			// Increase the range for the next attempt
			range += 512.0f;

			if (attempts < 256)
			{
				if (this->checkPlaceCollisionP(p)) continue;
			}

			character.target = p;
			character.targetTime = 0;
		}
		else if ( type == TTT_SHALLOWS )
		{
			if (attempts < 16)
			{
				if ( std::fabs(p.x - character.position.x) + std::fabs(p.z - character.position.z) < range / 2.f)
					continue;
			}

			p.y = GetLandH(p.x, p.z);
			float heightDiff = GetLandUpH(p.x, p.z) - p.y;

			if (attempts < 32)
			{
				// TODO: Modify the depths of shallows based on the height of the creature
				if (heightDiff < 200 || heightDiff > 400) continue;
			}

			character.target = p;
			character.targetTime = 0;
		}
		// TODO: Implement other checks

		break;
	}
}

/**
 * Checks for a collision with water cells, objects, steep inclines and the borders
 * Return: false if there is no collision, true if there is
 */
bool AI::checkPlaceCollision(vec3<float> &v, bool check_water, bool check_objects)
{
    int ccx = (int)v.x / 256;
    int ccz = (int)v.z / 256;

    // If the coordinate is 4 cells away from any of the borders
    if (ccx<4 || ccz<4 || ccx>1018 || ccz>1018)
        return true;

	// If the cell or any surrounding cells are water
    if (check_water)
	{
        if ( (FMap[ccz][ccx-1] | FMap[ccz-1][ccx] | FMap[ccz-1][ccx-1] |
                FMap[ccz][ccx] |
                FMap[ccz+1][ccx] | FMap[ccz][ccx+1] | FMap[ccz+1][ccx+1]) & fmWater)
            return true;
	}


    float h = GetLandH(v.x, v.z);

    // Check if this is not a water cell and the incline is too high
    if ( !(FMap[ccz][ccx] & fmWater) && (std::fabs(h - v.y) > 64) )
	{
		return true;
	}

    v.y = h;

    // Check if the incline is too steep
    float hh = GetLandH(v.x-64, v.z-64);
    if ( std::fabs(hh-h) > 100 )
        return true;
    hh = GetLandH(v.x+64, v.z-64);
    if ( std::fabs(hh-h) > 100 )
        return true;
    hh = GetLandH(v.x-64, v.z+64);
    if ( std::fabs(hh-h) > 100 )
        return true;
    hh = GetLandH(v.x+64, v.z+64);
    if ( std::fabs(hh-h) > 100 )
        return true;

	// Check if the coordinate intersects with an object
    if (check_objects)
	{
		for (int z=-2; z<=2; z++)
		for (int x=-2; x<=2; x++)
			if (OMap[ccz+z][ccx+x]!=255)
			{
				int ob = OMap[ccz+z][ccx+x];
				if (MObjects[ob].info.Radius<10)
					continue;
				float CR = (float)MObjects[ob].info.Radius + 64;

				float oz = (ccz+z) * 256.f + 128.f;
				float ox = (ccx+x) * 256.f + 128.f;

				float r = (float) std::sqrt( (ox-v.x)*(ox-v.x) + (oz-v.z)*(oz-v.z) );
				if (r<CR)
					return true;
			}
	}

    return false;
}

/**
 */
bool AI::checkPlaceCollisionP(vec3<float> &v)
{
	int ccx = (int)v.x / 256;
	int ccz = (int)v.z / 256;

	// If the coordinate is 4 cells away from any of the borders
	if (ccx<4 || ccz<4 || ccx>1008 || ccz>1008)
	{
		return true;
	}

	// Combine the cell flags together to test them all at once
    int F = (FMap[ccz][ccx-1] | FMap[ccz-1][ccx] | FMap[ccz-1][ccx-1] |
             FMap[ccz][ccx] |
             FMap[ccz+1][ccx] | FMap[ccz][ccx+1] | FMap[ccz+1][ccx+1]);

	// If the cell or surrounding cells is a water cell or NOWAY cell
    if (F & (fmWater + fmNOWAY))
        return true;

    v.y = GetLandH(v.x, v.z);

    // Check if the surrounding area is too steep
    float hh = GetLandH(v.x-164, v.z-164);
    if ( std::fabs(hh-v.y) > 160 ) return true;
    hh = GetLandH(v.x+164, v.z-164);
    if ( std::fabs(hh-v.y) > 160 ) return true;
    hh = GetLandH(v.x-164, v.z+164);
    if ( std::fabs(hh-v.y) > 160 ) return true;
    hh = GetLandH(v.x+164, v.z+164);
    if ( std::fabs(hh-v.y) > 160 ) return true;

	// Check if the coordinate intersects an object
    for (int z=-2; z<=2; z++)
	for (int x=-2; x<=2; x++)
		if (OMap[ccz+z][ccx+x]!=255)
		{
			int ob = OMap[ccz+z][ccx+x];
			if (MObjects[ob].info.Radius<10)
				continue;
			float CR = (float)MObjects[ob].info.Radius + 64;

			float oz = (ccz+z) * 256.f + 128.f;
			float ox = (ccx+x) * 256.f + 128.f;

			float r = (float) std::sqrt( (ox-v.x)*(ox-v.x) + (oz-v.z)*(oz-v.z) );
			if (r<CR)
				return true;
		}

    return false;
}


bool AI::checkPlaceCollision2(glm::vec3 &v, bool check_water)
{
    int ccx = (int)v.x / 256;
    int ccz = (int)v.z / 256;

    if (ccx<4 || ccz<4 || ccx>1018 || ccz>1018)
        return true;

    if (check_water)
        if ( (FMap[ccz][ccx-1] | FMap[ccz-1][ccx] | FMap[ccz-1][ccx-1] |
                FMap[ccz][ccx] |
                FMap[ccz+1][ccx] | FMap[ccz][ccx+1] | FMap[ccz+1][ccx+1]) & fmWater)
            return true;

    float h = GetLandH(v.x, v.z);
    v.y = h;

    float hh = GetLandH(v.x-64, v.z-64);
    if ( std::fabs(hh-h) > 100 ) return true;
    hh = GetLandH(v.x+64, v.z-64);
    if ( std::fabs(hh-h) > 100 ) return true;
    hh = GetLandH(v.x-64, v.z+64);
    if ( std::fabs(hh-h) > 100 ) return true;
    hh = GetLandH(v.x+64, v.z+64);
    if ( std::fabs(hh-h) > 100 ) return true;

    return 0;
}

void AI::processPrevPhase(Character& character)
{
	uint32_t phase = character.phase;

	character.phasePrevMorphTime += TimeDt;

    if (character.phasePrevMorphTime > 256)
	{
		character.phasePrev = phase;
	}

	uint32_t aniTime = character.charInfo->Animation[phase].AniTime;
	character.phasePrevFrameTime += TimeDt;
	character.phasePrevFrameTime %= aniTime;
}

int AI::checkPossiblePath(Character &character, bool check_water, bool check_objects)
{
	vec3<float> p = character.position;

	float lookx = (float)std::cos(character.rotation.y);
	float lookz = (float)std::sin(character.rotation.y);

	int c = 0;

	for (int t=0; t<20; t++)
	{
		p.x += lookx * 64.f;
		p.z += lookz * 64.f;

		if ( this->checkPlaceCollision(p, check_water, check_objects) )
		{
			c++;
		}
	}

	return c;
}

#ifndef DEGTORAD
// NOTE: Don't use this... please...
#define DEGTORAD(d) (d * Math::pi / 180.0f)
#endif //DEGTORAD

void AI::lookForAWay(Character& character, bool check_water, bool check_objects)
{
	Vec3<float> rotation = character.m_targetAngle;
	Vec3<float> angleFound = rotation;
	float checkAngle = 15.f;
	int maxPath = 16;
	int curPath = 0;

	if ( !this->checkPossiblePath(character, check_water, check_objects))
	{
		character.m_foundPaths = 0;
		return;
	}

	character.m_foundPaths++;

	// Check in a 180 degree range around the character on left and right sides (12 * 15 == 180)
	for (int i=0; i<12; i++)
	{
		character.m_targetAngle.y = rotation.y + DEGTORAD(checkAngle);
		curPath = this->checkPossiblePath(character, check_water, check_objects) + (i>>1);

		if (!curPath)
		{
			return;
		}

		if (curPath < maxPath)
		{
			maxPath = curPath;
			angleFound = character.m_targetAngle;
		}

		character.m_targetAngle.y = rotation.y - DEGTORAD(checkAngle);
		curPath = this->checkPossiblePath(character, check_water, check_objects) + (i>>1);

		if (!curPath)
		{
			return;
		}

		if (curPath < maxPath)
		{
			maxPath = curPath;
			angleFound = character.m_targetAngle;
		}

		checkAngle += 15.f;
	}

	character.m_targetAngle = angleFound;
}

void AI::thinkY_Beta_Gamma(Character& character, float blook, float glook, float blim, float glim)
{
	// TODO: Figure this thing out... ugh
	character.m_position.y = GetLandH(character.m_position.x, character.m_position.z);

    // -- beta
    float hlook  = GetLandH(character.m_position.x + cptr->lookx * blook, character.m_position.z + cptr->lookz * blook);
    float hlook2 = GetLandH(character.m_position.x - cptr->lookx * blook, character.m_position.z - cptr->lookz * blook);
    DeltaFunc(cptr->beta, (hlook2 - hlook) / (blook * 3.2f), TimeDt / 800.f);

    if (cptr->beta > blim)
        cptr->beta = blim;
    if (cptr->beta <-blim)
        cptr->beta =-blim;

    // -- gamma
    hlook  = GetLandH(character.m_position.x + cptr->lookz * glook, character.m_position.z - cptr->lookx*glook);
    hlook2 = GetLandH(character.m_position.x - cptr->lookz * glook, character.m_position.z + cptr->lookx*glook);
    cptr->tggamma =(hlook - hlook2) / (glook * 3.2f);
    if (cptr->tggamma > glim)
        cptr->tggamma = glim;
    if (cptr->tggamma <-glim)
        cptr->tggamma =-glim;
}
