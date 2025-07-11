#include "hud.h"
#include <iostream>
#include "cl_util.h"
#include <string.h>

int CHudEgg::Init() {
	cl_egg = CVAR_CREATE("cl_egg", "0", FCVAR_ARCHIVE);
	song_path = CVAR_CREATE ("song_path", "media/portal_radio.wav", FCVAR_ARCHIVE);
	return 0;
}

void CHudEgg::CheckEggTriggerInConsole()
{
	if (std::cin >> "Kv4sMaN") {
		PlaySound(song_path->string, cl_egg->value);
	}
	else {
		gEngfuncs.pfnClientCmd("stopsound\n");
		gEngfuncs.Con_Printf("Thank you for using my client :)\n");
		gEngfuncs.Con_Printf("Created by: Kv4sMaN\n");
	}
	return;
}
