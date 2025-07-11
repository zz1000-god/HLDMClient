#include <algorithm>
#include <cstring>
#include <vector>
#include <string>

#include "hud.h"
#include "cl_util.h"
#include "com_model.h"
#include "r_studioint.h"
#include "forcemodel.h"

extern engine_studio_api_t IEngineStudio;
extern hud_player_info_t   g_PlayerInfoList [MAX_PLAYERS + 1];
extern extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS + 1];

namespace force_model
{
	namespace
	{
		// Overrides for the teammates and enemies.
		model_t* teammate_model_override = nullptr;
		model_t* enemy_model_override = nullptr;

		// Cache for fast lookup.
		model_t* teammate_enemy_model_overrides_cache[MAX_PLAYERS];
		bool cache_initialized = false;

		// Safety check for player index
		bool is_valid_player_index(int player_index)
		{
			return player_index >= 0 && player_index < MAX_PLAYERS;
		}

		// Initialize cache if not already done
		void ensure_cache_initialized()
		{
			if (!cache_initialized)
			{
				memset(teammate_enemy_model_overrides_cache, 0, sizeof(teammate_enemy_model_overrides_cache));
				cache_initialized = true;
			}
		}

		// Safe string copy with null termination guarantee
		void safe_strncpy(char* dest, const char* src, size_t dest_size)
		{
			if (!dest || !src || dest_size == 0)
				return;
			
			strncpy(dest, src, dest_size - 1);
			dest[dest_size - 1] = '\0';
		}

		// Extract model name from path safely
		bool extract_model_name(const char* model_path, char* model_name, size_t model_name_size)
		{
			if (!model_path || !model_name || model_name_size == 0)
				return false;

			const char* models_player = strstr(model_path, "models/player/");
			if (!models_player)
			{
				safe_strncpy(model_name, model_path, model_name_size);
				return true;
			}

			const char* model_start = models_player + strlen("models/player/");
			const char* slash = strchr(model_start, '/');
			
			if (slash)
			{
				size_t model_name_len = slash - model_start;
				if (model_name_len >= model_name_size)
					model_name_len = model_name_size - 1;
				
				memcpy(model_name, model_start, model_name_len);
				model_name[model_name_len] = '\0';
			}
			else
			{
				safe_strncpy(model_name, model_start, model_name_size);
			}
			
			return true;
		}

		model_t* load_model(const char* name)
		{
			if (!name || !name[0])
			{
				gEngfuncs.Con_Printf("Error: Invalid model name.\n");
				return nullptr;
			}

			// Check for valid model name (basic validation)
			size_t name_len = strlen(name);
			if (name_len > 32) // More conservative limit for model names
			{
				gEngfuncs.Con_Printf("Error: Model name too long.\n");
				return nullptr;
			}

			// Check for invalid characters
			for (size_t i = 0; i < name_len; ++i)
			{
				char c = name[i];
				if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
					  (c >= '0' && c <= '9') || c == '_' || c == '-'))
				{
					gEngfuncs.Con_Printf("Error: Invalid character in model name.\n");
					return nullptr;
				}
			}

			char model_path[128]; // Smaller, safer buffer
			int result = std::snprintf(model_path, sizeof(model_path), "models/player/%s/%s.mdl", name, name);

			if (result >= static_cast<int>(sizeof(model_path)) || result < 0)
			{
				gEngfuncs.Con_Printf("Error: Model path too long.\n");
				return nullptr;
			}

			model_t* model = IEngineStudio.Mod_ForName(model_path, 0);
			if (!model)
			{
				gEngfuncs.Con_Printf("Error: Could not load model '%s'.\n", name);
			}

			return model;
		}

		void show_current_model_value(const char* command_name, model_t* current_override)
		{
			if (current_override && current_override->name)
			{
				char model_name[64];
				if (extract_model_name(current_override->name, model_name, sizeof(model_name)))
				{
					gEngfuncs.Con_Printf("%s is \"%s\"\n", command_name, model_name);
				}
				else
				{
					gEngfuncs.Con_Printf("%s is \"%s\"\n", command_name, current_override->name);
				}
			}
			else
			{
				gEngfuncs.Con_Printf("%s is \"\" (disabled)\n", command_name);
			}
		}

		void callback_cl_forceteammatemodel()
		{
			if (gEngfuncs.Cmd_Argc() == 1)
			{
				show_current_model_value("cl_forceteammatemodel", teammate_model_override);
				return;
			}

			if (gEngfuncs.Cmd_Argc() != 2)
			{
				gEngfuncs.Con_Printf("Usage: cl_forceteammatemodel <model name>\n");
				gEngfuncs.Con_Printf("       cl_forceteammatemodel \"\" (to disable)\n");
				gEngfuncs.Con_Printf("       cl_forceteammatemodel (to show current value)\n");
				return;
			}

			const char* model_name = gEngfuncs.Cmd_Argv(1);
			if (!model_name)
			{
				gEngfuncs.Con_Printf("Error: Invalid command argument.\n");
				return;
			}

			if (model_name[0])
			{
				model_t* model = load_model(model_name);
				if (!model)
				{
					return; // Error already printed in load_model
				}

				teammate_model_override = model;
				gEngfuncs.Con_Printf("Teammate model override set to: %s\n", model_name);
			}
			else
			{
				teammate_model_override = nullptr;
				gEngfuncs.Con_Printf("Teammate model override disabled.\n");
			}

			update_player_teams();
		}

		void callback_cl_forceenemymodel()
		{
			if (gEngfuncs.Cmd_Argc() == 1)
			{
				show_current_model_value("cl_forceenemymodel", enemy_model_override);
				return;
			}

			if (gEngfuncs.Cmd_Argc() != 2)
			{
				gEngfuncs.Con_Printf("Usage: cl_forceenemymodel <model name>\n");
				gEngfuncs.Con_Printf("       cl_forceenemymodel \"\" (to disable)\n");
				gEngfuncs.Con_Printf("       cl_forceenemymodel (to show current value)\n");
				return;
			}

			const char* model_name = gEngfuncs.Cmd_Argv(1);
			if (!model_name)
			{
				gEngfuncs.Con_Printf("Error: Invalid command argument.\n");
				return;
			}

			if (model_name[0])
			{
				model_t* model = load_model(model_name);
				if (!model)
				{
					return; // Error already printed in load_model
				}

				enemy_model_override = model;
				gEngfuncs.Con_Printf("Enemy model override set to: %s\n", model_name);
			}
			else
			{
				enemy_model_override = nullptr;
				gEngfuncs.Con_Printf("Enemy model override disabled.\n");
			}

			update_player_teams();
		}
	}

	void hook_commands()
	{
		ensure_cache_initialized();
		gEngfuncs.pfnAddCommand("cl_forceteammatemodel", callback_cl_forceteammatemodel);
		gEngfuncs.pfnAddCommand("cl_forceenemymodel", callback_cl_forceenemymodel);
	}

	void update_player_team(int player_index)
	{
		if (!is_valid_player_index(player_index))
		{
			return;
		}

		ensure_cache_initialized();

		// GetLocalPlayer() returns an undefined pointer if we aren't in-game.
		if (!gHUD.m_Teamplay || !gEngfuncs.pfnGetLevelName() || !gEngfuncs.pfnGetLevelName()[0])
		{
			teammate_enemy_model_overrides_cache[player_index] = enemy_model_override;
			return;
		}

		cl_entity_t* local_player = gEngfuncs.GetLocalPlayer();
		if (!local_player)
		{
			teammate_enemy_model_overrides_cache[player_index] = enemy_model_override;
			return;
		}

		const int local_player_index = local_player->index;
		if (local_player_index < 1 || local_player_index > MAX_PLAYERS) // Engine indices are 1-based
		{
			teammate_enemy_model_overrides_cache[player_index] = enemy_model_override;
			return;
		}

		if (g_IsSpectator[local_player_index])
		{
			const char* model_cvar = CVAR_GET_STRING("model");
			if (!model_cvar || !model_cvar[0])
			{
				teammate_enemy_model_overrides_cache[player_index] = enemy_model_override;
				return;
			}

			// Safe string operations
			char model_name_lower[64];
			safe_strncpy(model_name_lower, model_cvar, sizeof(model_name_lower));
			
			// Convert to lowercase safely
			for (int i = 0; model_name_lower[i] && i < static_cast<int>(sizeof(model_name_lower)) - 1; ++i)
			{
				if (model_name_lower[i] >= 'A' && model_name_lower[i] <= 'Z')
					model_name_lower[i] = model_name_lower[i] - 'A' + 'a';
			}

			// Bounds check for g_PlayerExtraInfo access
			int target_index = player_index + 1;
			if (target_index >= 1 && target_index <= MAX_PLAYERS && 
				g_PlayerExtraInfo[target_index].teamname[0] != '\0')
			{
				if (!strcmp(model_name_lower, g_PlayerExtraInfo[target_index].teamname))
					teammate_enemy_model_overrides_cache[player_index] = teammate_model_override;
				else
					teammate_enemy_model_overrides_cache[player_index] = enemy_model_override;
			}
			else
			{
				teammate_enemy_model_overrides_cache[player_index] = enemy_model_override;
			}
			return;
		}

		// Bounds check for g_PlayerExtraInfo access
		int target_index = player_index + 1;
		if (local_player_index >= 1 && local_player_index <= MAX_PLAYERS &&
			target_index >= 1 && target_index <= MAX_PLAYERS)
		{
			const char* local_team = g_PlayerExtraInfo[local_player_index].teamname;
			const char* player_team = g_PlayerExtraInfo[target_index].teamname;

			if (local_team && player_team && local_team[0] && player_team[0] && 
				!strcmp(local_team, player_team))
			{
				teammate_enemy_model_overrides_cache[player_index] = teammate_model_override;
			}
			else
			{
				teammate_enemy_model_overrides_cache[player_index] = enemy_model_override;
			}
		}
		else
		{
			teammate_enemy_model_overrides_cache[player_index] = enemy_model_override;
		}
	}

	void update_player_teams()
	{
		ensure_cache_initialized();
		
		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			update_player_team(i);
		}
	}

	model_t* get_model_override(int player_index)
	{
		if (!is_valid_player_index(player_index))
		{
			return nullptr;
		}

		ensure_cache_initialized();
		return teammate_enemy_model_overrides_cache[player_index];
	}

	// Cleanup function for proper resource management
	void cleanup()
	{
		teammate_model_override = nullptr;
		enemy_model_override = nullptr;
		
		if (cache_initialized)
		{
			memset(teammate_enemy_model_overrides_cache, 0, sizeof(teammate_enemy_model_overrides_cache));
			cache_initialized = false;
		}
	}
}