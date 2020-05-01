#ifndef AIPARASAUR_H_INCLUDED
#define AIPARASAUR_H_INCLUDED

#include "AI.hpp"
#include <cstdint>

class AIParasaur : public AI
{
public:

	enum AIParasaurEnum
	{
		PAR_WALK	= 0,
		PAR_RUN		= 1,
		PAR_IDLE1	= 2,
		PAR_IDLE2	= 3,
		PAR_DIE		= 4,
		PAR_SLP		= 5
	};

	AIParasaur();
	~AIParasaur();

	void animate(const Character& character);
	void animateDead(const Character& character);
};

#endif // AIPARASAUR_H_INCLUDED
