
#include "AIParasaur.hpp"
#include "Character.hpp"

#include <algorithm>


AIParasaur::AIParasaur() :
	m_name("Parasaurolophus AI"),
	m_version(1),
	m_indexAI(10)	// AI_PARA
{
	/*
	This AI is suitable for almost any Hadrosaur type creature,
	It wanders around, grazing from time to time and will flee
	if danger is detected
	*/
}

void AIParasaur::animate(const Character& character)
{
	bool NewPhase = false; // Define this in AI.hpp as "extern bool NewPhase" implement it in AI.cpp
	int _Phase = character.phase;
	int _FTime = character.frameTime;
	//float _tgalpha = cptr->tgalpha; // _tgalpha is the target angle on the Y (vertical) axis
	float angleY = character.rotation.y;
    glm::vec3 nv;
    std::shared_ptr<TCharacterInfo> chInfo = character.m_charInfo;

    if ( character.afraidTime )
	{
		character.afraidTime = std::max(0, character.afraidTime - TimeDt);
	}

    if ( character.state == 2 )
    {
        NewPhase = true;
        character.state = 1;
    }

    glm::vec3 target			= character.target;
	glm::vec3 targetDifference	= target - character.position;
	glm::vec3 playerDifference	= glm::vec3(PlayerX, PlayerY, PlayerZ) - character.position;
	float targetDistance		= glm::length(targetDifference);
	float playerDistance		= glm::length(playerDifference);

	while ( true )
	{
		if ( character.state )
		{
			// Run away

			if (!character.afraidTime)
			{
				character.state = 0;
				this->setNewTargetPlace(character, 8048.f);
				continue; // Start the loop again
			}

			nv = playerDifference;
			glm::normalize( nv ) * 2048.f;
			glm::vec3 nt = character.position - nv;
			character.target = nt;
			character.targetTime = 0;
		}
		else if ( !character.state )
		{
			// Exploring area
			character.afraidTime = 0;

			// No matter what, if the player is closer than 1024 units, we run away even if we can't detect them!
			if (playerDistance < 1024.f)
			{
				character.state = 1;
				character.afraidTime =  (6 + rRand(8)) * 1024;
				character.phase = PAR_RUN;
				continue; // Start the loop again
			}


			if (targetDistance < 456.0f)
			{
				this->setNewTargetPlace(character, 8048.f);
				continue; // Start the loop again
			}
		}

		break; // If we reach this part of the loop we want to end it.
	}


//============================================//

    if (character.pathFindDelay)
    {
    	character.pathFindDelay--;
    }
    else
    {
        character.targetAngle.y = CorrectedAlpha(FindVectorAlpha(targetDifference.x, targetDifference.z), character.rotation.y);

        if (character.afraidTime)
        {
            character.targetAngle.y += (float)std::sin(RealTime / 1024.f) / 3.f;
            if (character.targetAngle.y < 0)
                character.targetAngle.y += 2.0f * glm::pi();
            if (character.targetAngle.y > 2.0f * glm::pi())
                character.targetAngle.y -= 2.0f * glm::pi();
        }
    }


    this->lookForAWay(character, true, true);

	if (character.pathBlockages > 8)
	{
		character.pathBlockages = 0;
		character.pathFindDelay = 44 + rRand(80);
	}

	if (character.targetAngle.y < 0)
		character.targetAngle.y += 2.0f * glm::pi();
	if (character.targetAngle.y > 2.0f * glm::pi())
		character.targetAngle.y -= 2.0f * glm::pi();

//===============================================//

    this->processPrevPhase(character);

//======== select new phase =======================//

	character.frameTime += TimeDt;
	uint32_t aniTime = character.charInfo->Animation[character.phase].AniTime;

    if (character.frameTime >= aniTime)
    {
        character.frameTime %= aniTime;
        NewPhase = true;
    }

    if (NewPhase)
	{
		if ( !character.state )
        {
            if (character.phase == PAR_IDLE1 || character.phase == PAR_IDLE2)
            {
                if (rRand(128) > 64 && character.phase == PAR_IDLE2)
                    character.phase = PAR_WALK;
                else
                    character.phase = PAR_IDLE1 + rRand(1);
            }
            else
			{
				if (rRand(128) > 120)
					character.phase = PAR_IDLE1;
				else
					character.phase = PAR_WALK;
			}
        }
        else
		{
			if (character.afraidTime)
				character.phase = PAR_RUN;
			else
				character.phase = PAR_WALK;
		}
	}

//====== process phase changing ===========//

    if ( (_Phase != character.phase) || NewPhase)
	{
		this->activateCharacterFx(character);
	}

    if ( _Phase != character.phase )
    {
        if ( _Phase <= 2 && character.phase <= 2 )
            character.frameTime = _FTime * chInfo->Animation[character.phase].AniTime / chInfo->Animation[_Phase].AniTime + 64;
        else if (!NewPhase)
            character.frameTime = 0;

        if (character.phasePrevMorphTime > 128)
        {
            character.phasePrev				= _Phase;
            character.phasePrevFrameTime	= _FTime;
            character.phasePrevMorphTime	= 0;
        }
    }

    character.frameTime %= chInfo->Animation[character.phase].AniTime;



    //========== rotation to tgalpha ===================//

    float rspd, currspeed, tgbend;
    float dalpha = (float)std::fabs(cptr->tgalpha - cptr->alpha);
    float drspd = dalpha;
    if (drspd>pi)
        drspd = 2*pi - drspd;


    if (character.phase != PAR_IDLE1 && character.phase != PAR_IDLE2)
    {
		if (drspd > 0.02)
			if (cptr->tgalpha > cptr->alpha)
				currspeed = 0.2f + drspd*1.0f;
			else
				currspeed =-0.2f - drspd*1.0f;
		else
			currspeed = 0;

		if (character.m_afraidTime)
			currspeed*=1.5;
		if (dalpha > pi)
			currspeed*=-1;
		if ((cptr->State & csONWATER) || character.m_phase==PAR_WALK)
			currspeed/=1.4f;

		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

		tgbend = drspd/3.0f;
		if (tgbend>pi/2.f)
			tgbend = pi/2.f;

		tgbend*= SGN(currspeed);
		if (fabs(tgbend) > fabs(cptr->bend))
			DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
		else
			DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);


		rspd=cptr->rspeed * TimeDt / 612.f;
		if (drspd < fabs(rspd))
			cptr->alpha = cptr->tgalpha;
		else
			cptr->alpha+=rspd;


		if (cptr->alpha > pi * 2)
			cptr->alpha-= pi * 2;
		if (cptr->alpha < 0     )
			cptr->alpha+= pi * 2;
    }

//========== movement ==============================//
    cptr->lookx = (float)cos(cptr->alpha);
    cptr->lookz = (float)sin(cptr->alpha);

    float curspeed = 0;
    if (character.m_phase == PAR_RUN )
        curspeed = 1.6f;
    if (character.m_phase == PAR_WALK)
        curspeed = 0.40f;

    if (drspd > pi / 2.f)
        curspeed*=2.f - 2.f*drspd / pi;

//========== process speed =============//
    curspeed*=cptr->scale;
    if (curspeed > cptr->vspeed)
        DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
    else
        DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);

    MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
                  cptr->lookz * cptr->vspeed * TimeDt, TRUE, TRUE);

    ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);
    if (character.m_phase==PAR_WALK)
        cptr->tggamma+= cptr->rspeed / 12.0f;
    else
        cptr->tggamma+= cptr->rspeed / 8.0f;
    DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}

void AIParasaur::animateDead(const Character& character)
{
	uint32_t phase = character.getPhase();

    if (phase != PAR_DIE && phase != PAR_SLP)
    {
        if (cptr->PPMorphTime>128)
        {
			character.setPhasePrev(phase);
            cptr->PrevPFTime  = character.m_frameTime;
            character.setMorphTime(0);
        }

        character.m_frameTime = 0;
        character.m_phase = PAR_DIE;
        ActivateCharacterFx(cptr);
    }
    else
    {
        ProcessPrevPhase(cptr);

        character.m_frameTime+=TimeDt;
        if (character.m_frameTime >= cptr->pinfo->Animation[character.m_phase].AniTime)
            if (Tranq)
            {
                character.m_frameTime=0;
                character.m_phase = PAR_SLP;
                ActivateCharacterFx(cptr);
            }
            else
                character.m_frameTime = cptr->pinfo->Animation[character.m_phase].AniTime-1;
    }

//======= movement ===========//
    DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
    cptr->pos.x+=cptr->lookx * cptr->vspeed * TimeDt;
    cptr->pos.z+=cptr->lookz * cptr->vspeed * TimeDt;

    ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

    DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}
