#pragma once
class CHudEgg : public CHudBase
{
	public:
		virtual int Init();
		void CheckEggTriggerInConsole();

	private:
		cvar_t* cl_egg;
		cvar_t* song_path;
};