#include "stdafx.h"

#include "activatingcharcollisiondelay.h"
#include "characterphysicssupport.h"
#include "phmovementcontrol.h"
#ifdef	DEBUG
#include "phdebug.h"
#endif
activating_character_delay::activating_character_delay(CCharacterPhysicsSupport *char_support_) : char_support(*char_support_), activate_time(Device.dwTimeGlobal + delay )
{
	VERIFY(char_support_);
	VERIFY(char_support.movement());
	VERIFY( !char_support.movement()->CharacterExist() );
}
bool activating_character_delay::active()
{
	VERIFY(char_support.movement());
	return !char_support.movement()->CharacterExist();
}
void activating_character_delay::update()
{
	if(!active())
		return;

	if( Device.dwTimeGlobal < activate_time )
		return;

	if( do_position_correct() )
		char_support.CreateCharacter();

	activate_time = Device.dwTimeGlobal + delay;
}


bool activating_character_delay::do_position_correct()
{
	CPHMovementControl *m = char_support.movement();	
	CObject *obj =	m->ParentObject();
	Fvector sv_pos = obj->Position();

	bool ret = char_support.CollisionCorrectObjPos();
	if(!ret)
		obj->Position().set(sv_pos);
 
	return ret;

}