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

		struct player_name_override_t
		{
			char player_name[32];
			model_t* model;
			bool active;
		};

		struct model_replacement_t
		{
			char old_model[32];
			model_t* new_model;
			bool active;
		};

		player_name_override_t player_name_overrides[64];
		int name_override_count = 0;

		model_replacement_t model_replacements[64];
		int replacement_count = 0;

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

		model_t* find_player_name_override(const char* player_name)
		{
			if (!player_name || !player_name[0])
				return nullptr;

			for (int i = 0; i < name_override_count; ++i)
			{
				if (player_name_overrides[i].active &&
					strcmp(player_name_overrides[i].player_name, player_name) == 0)
				{
					return player_name_overrides[i].model;
				}
			}
			return nullptr;
		}

		bool set_player_name_override(const char* player_name, model_t* model)
		{
			if (!player_name || !player_name[0])
				return false;

			for (int i = 0; i < name_override_count; ++i)
			{
				if (strcmp(player_name_overrides[i].player_name, player_name) == 0)
				{
					player_name_overrides[i].model = model;
					player_name_overrides[i].active = (model != nullptr);
					return true;
				}
			}

			if (name_override_count < 64 && model)
			{
				safe_strncpy(player_name_overrides[name_override_count].player_name,
					player_name, sizeof(player_name_overrides[name_override_count].player_name));
				player_name_overrides[name_override_count].model = model;
				player_name_overrides[name_override_count].active = true;
				name_override_count++;
				return true;
			}

			return false;
		}

		bool remove_player_name_override(const char* player_name)
		{
			if (!player_name || !player_name[0])
				return false;

			for (int i = 0; i < name_override_count; ++i)
			{
				if (strcmp(player_name_overrides[i].player_name, player_name) == 0)
				{
					player_name_overrides[i].active = false;
					player_name_overrides[i].model = nullptr;
					return true;
				}
			}
			return false;
		}

		model_t* find_model_replacement(const char* model_name)
		{
			if (!model_name || !model_name[0])
				return nullptr;

			for (int i = 0; i < replacement_count; ++i)
			{
				if (model_replacements[i].active &&
					strcmp(model_replacements[i].old_model, model_name) == 0)
				{
					return model_replacements[i].new_model;
				}
			}
			return nullptr;
		}

		bool set_model_replacement(const char* old_model, model_t* new_model)
		{
			if (!old_model || !old_model[0])
				return false;

			for (int i = 0; i < replacement_count; ++i)
			{
				if (strcmp(model_replacements[i].old_model, old_model) == 0)
				{
					model_replacements[i].new_model = new_model;
					model_replacements[i].active = (new_model != nullptr);
					return true;
				}
			}

			if (replacement_count < 64 && new_model)
			{
				safe_strncpy(model_replacements[replacement_count].old_model,
					old_model, sizeof(model_replacements[replacement_count].old_model));
				model_replacements[replacement_count].new_model = new_model;
				model_replacements[replacement_count].active = true;
				replacement_count++;
				return true;
			}

			return false;
		}

		bool remove_model_replacement(const char* old_model)
		{
			if (!old_model || !old_model[0])
				return false;

			for (int i = 0; i < replacement_count; ++i)
			{
				if (strcmp(model_replacements[i].old_model, old_model) == 0)
				{
					model_replacements[i].active = false;
					model_replacements[i].new_model = nullptr;
					return true;
				}
			}
			return false;
		}

		void callback_cl_forceplayermodel()
		{
			if (gEngfuncs.Cmd_Argc() == 1)
			{
				bool has_active = false;
				for (int i = 0; i < name_override_count; ++i)
				{
					if (player_name_overrides[i].active && player_name_overrides[i].model)
					{
						if (!has_active)
						{
							gEngfuncs.Con_Printf("Active player name overrides:\n");
							has_active = true;
						}

						char model_name[64];
						if (extract_model_name(player_name_overrides[i].model->name,
							model_name, sizeof(model_name)))
						{
							gEngfuncs.Con_Printf("  %s -> %s\n",
								player_name_overrides[i].player_name, model_name);
						}
					}
				}

				if (!has_active)
				{
					gEngfuncs.Con_Printf("No player name overrides active.\n");
				}
				return;
			}

			if (gEngfuncs.Cmd_Argc() == 2)
			{
				const char* player_name = gEngfuncs.Cmd_Argv(1);
				if (!player_name || !player_name[0])
				{
					gEngfuncs.Con_Printf("Error: Invalid player name.\n");
					return;
				}

				if (remove_player_name_override(player_name))
				{
					gEngfuncs.Con_Printf("Model override for player '%s' removed.\n", player_name);
					update_player_teams();
				}
				else
				{
					gEngfuncs.Con_Printf("No model override found for player '%s'.\n", player_name);
				}
				return;
			}

			if (gEngfuncs.Cmd_Argc() != 3)
			{
				gEngfuncs.Con_Printf("Usage: cl_forceplayermodel <player_name> <model_name>\n");
				gEngfuncs.Con_Printf("       cl_forceplayermodel <player_name> \"\" (to remove override)\n");
				gEngfuncs.Con_Printf("       cl_forceplayermodel <player_name> (to remove override)\n");
				gEngfuncs.Con_Printf("       cl_forceplayermodel (to show all active overrides)\n");
				return;
			}

			const char* player_name = gEngfuncs.Cmd_Argv(1);
			const char* model_name = gEngfuncs.Cmd_Argv(2);

			if (!player_name || !player_name[0])
			{
				gEngfuncs.Con_Printf("Error: Invalid player name.\n");
				return;
			}

			if (!model_name)
			{
				gEngfuncs.Con_Printf("Error: Invalid model name argument.\n");
				return;
			}

			if (model_name[0] == '\0')
			{
				if (remove_player_name_override(player_name))
				{
					gEngfuncs.Con_Printf("Model override for player '%s' removed.\n", player_name);
				}
				else
				{
					gEngfuncs.Con_Printf("No model override found for player '%s'.\n", player_name);
				}
			}
			else
			{
				model_t* model = load_model(model_name);
				if (!model)
				{
					return;
				}

				if (set_player_name_override(player_name, model))
				{
					gEngfuncs.Con_Printf("Model override for player '%s' set to: %s\n", player_name, model_name);
				}
				else
				{
					gEngfuncs.Con_Printf("Error: Could not set model override (limit reached?).\n");
				}
			}

			update_player_teams();
		}

		void callback_cl_forcemodel()
		{
			if (gEngfuncs.Cmd_Argc() == 1)
			{
				bool has_active = false;
				for (int i = 0; i < replacement_count; ++i)
				{
					if (model_replacements[i].active && model_replacements[i].new_model)
					{
						if (!has_active)
						{
							gEngfuncs.Con_Printf("Active model replacements:\n");
							has_active = true;
						}

						char new_model_name[64];
						if (extract_model_name(model_replacements[i].new_model->name,
							new_model_name, sizeof(new_model_name)))
						{
							gEngfuncs.Con_Printf("  %s -> %s\n",
								model_replacements[i].old_model, new_model_name);
						}
					}
				}

				if (!has_active)
				{
					gEngfuncs.Con_Printf("No model replacements active.\n");
				}
				return;
			}

			if (gEngfuncs.Cmd_Argc() == 2)
			{
				const char* old_model = gEngfuncs.Cmd_Argv(1);
				if (!old_model || !old_model[0])
				{
					gEngfuncs.Con_Printf("Error: Invalid model name.\n");
					return;
				}

				if (remove_model_replacement(old_model))
				{
					gEngfuncs.Con_Printf("Model replacement for '%s' removed.\n", old_model);
					update_player_teams();
				}
				else
				{
					gEngfuncs.Con_Printf("No model replacement found for '%s'.\n", old_model);
				}
				return;
			}

			if (gEngfuncs.Cmd_Argc() != 3)
			{
				gEngfuncs.Con_Printf("Usage: cl_forcemodel <old_model> <new_model>\n");
				gEngfuncs.Con_Printf("       cl_forcemodel <old_model> \"\" (to remove replacement)\n");
				gEngfuncs.Con_Printf("       cl_forcemodel <old_model> (to remove replacement)\n");
				gEngfuncs.Con_Printf("       cl_forcemodel (to show all active replacements)\n");
				return;
			}

			const char* old_model = gEngfuncs.Cmd_Argv(1);
			const char* new_model = gEngfuncs.Cmd_Argv(2);

			if (!old_model || !old_model[0])
			{
				gEngfuncs.Con_Printf("Error: Invalid old model name.\n");
				return;
			}

			if (!new_model)
			{
				gEngfuncs.Con_Printf("Error: Invalid new model name argument.\n");
				return;
			}

			if (new_model[0] == '\0')
			{
				if (remove_model_replacement(old_model))
				{
					gEngfuncs.Con_Printf("Model replacement for '%s' removed.\n", old_model);
				}
				else
				{
					gEngfuncs.Con_Printf("No model replacement found for '%s'.\n", old_model);
				}
			}
			else
			{
				model_t* model = load_model(new_model);
				if (!model)
				{
					return;
				}

				if (set_model_replacement(old_model, model))
				{
					gEngfuncs.Con_Printf("Model '%s' will be replaced with '%s'\n", old_model, new_model);
				}
				else
				{
					gEngfuncs.Con_Printf("Error: Could not set model replacement (limit reached?).\n");
				}
			}

			update_player_teams();
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
		gEngfuncs.pfnAddCommand("cl_forcemodel", callback_cl_forceplayermodel);
		gEngfuncs.pfnAddCommand("cl_forcemmodel", callback_cl_forcemodel);
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

	model_t* get_player_name_model_override(int player_index)
	{
		if (!is_valid_player_index(player_index))
			return nullptr;

		int engine_index = player_index + 1;
		if (engine_index < 1 || engine_index > MAX_PLAYERS)
			return nullptr;

		if (g_PlayerInfoList[engine_index].name && g_PlayerInfoList[engine_index].name[0])
		{
			return find_player_name_override(g_PlayerInfoList[engine_index].name);
		}

		return nullptr;
	}

	model_t* get_model_replacement_override(int player_index)
	{
		if (!is_valid_player_index(player_index))
			return nullptr;

		int engine_index = player_index + 1;
		if (engine_index < 1 || engine_index > MAX_PLAYERS)
			return nullptr;

		hud_player_info_t* player_info = &g_PlayerInfoList[engine_index];
		if (!player_info || !player_info->model || !player_info->model[0])
			return nullptr;

		return find_model_replacement(player_info->model);
	}

	model_t* get_model_override(int player_index)
	{
		if (!is_valid_player_index(player_index))
		{
			return nullptr;
		}

		ensure_cache_initialized();

		model_t* name_override = get_player_name_model_override(player_index);
		if (name_override)
		{
			return name_override;
		}

		model_t* replacement_override = get_model_replacement_override(player_index);
		if (replacement_override)
		{
			return replacement_override;
		}

		return teammate_enemy_model_overrides_cache[player_index];
	}

	// Cleanup function for proper resource management
	void cleanup()
	{
		teammate_model_override = nullptr;
		enemy_model_override = nullptr;

		for (int i = 0; i < 64; ++i)
		{
			player_name_overrides[i].player_name[0] = '\0';
			player_name_overrides[i].model = nullptr;
			player_name_overrides[i].active = false;
		}
		name_override_count = 0;

		for (int i = 0; i < 64; ++i)
		{
			model_replacements[i].old_model[0] = '\0';
			model_replacements[i].new_model = nullptr;
			model_replacements[i].active = false;
		}
		replacement_count = 0;

		if (cache_initialized)
		{
			memset(teammate_enemy_model_overrides_cache, 0, sizeof(teammate_enemy_model_overrides_cache));
			cache_initialized = false;
		}
	}
}