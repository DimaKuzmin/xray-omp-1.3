#pragma once

#include "game_sv_freemp.h"

class game_sv_coop : public game_sv_freemp
{
	typedef game_sv_freemp inherited;
 
	
public:
										game_sv_coop();
	virtual								~game_sv_coop();

	virtual LPCSTR						type_name() const { return "coop"; };
 
};

