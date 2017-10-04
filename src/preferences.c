/*
 * Copyright (c) 2013 Todd Mortimer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unicode/utf.h>

#include "terminal.h"
#include "symmenu.h"
#include "preferences.h"

pref_t *prefs = NULL;

static void first_run(pref_t *prefs) {
	char* home = getenv("HOME");
	if(home != NULL){ chdir(home); }
	
	char* readme_path = (atoi(getenv("WIDTH")) <= 720) ? README45_FILE_PATH : README_FILE_PATH;
	fprintf(stderr, "Updating README\n");
	if (access(readme_path, F_OK) != -1) {
		// stat success!
		if (symlink(readme_path, "./README") == -1){
			if (errno != EEXIST){
				fprintf(stderr, "Error linking README from app to PWD\n");
			}
		}
	}

	save_preferences(prefs, PREFS_FILE_PATH);
}

int preferences_guess_best_font_size(pref_t *prefs, int target_cols){
	/* font widths in pixels for sizes 0-250, indexed by font size */
	int screen_width, screen_height, target_width;
	if((getenv("WIDTH") == NULL) || (getenv("HEIGHT") == NULL)){
		/* no width or height in env, just return the default */
		if (prefs == NULL) {
			PRINT(stderr, "Preferences not initalized!\n");
			return 10;
		}
		return prefs->font_size;
	}
	screen_width = atoi(getenv("WIDTH"));
	screen_height = atoi(getenv("HEIGHT"));
	target_width = screen_width < screen_height ? screen_width : screen_height;
	int num_px = 0;
	for (int i = 0; i < NUM_SIZES; ++i){
		num_px = target_cols * font_widths[i];
		if(num_px > target_width){
			/* if we are too big, return the last one. */
			PRINT(stderr, "Autodetected font size %d for screen width %d\n", (i - 1), target_width);
			return (i - 1);
		}
	}
	/* if we get here, then just return the largest font */
	return font_widths[NUM_SIZES-1];
}

static void upgrade_config_v8(config_t *dst, config_t *src) {
	config_setting_t *dst_root = config_root_setting(dst);

	/* upgrade metamode hitbox */

	config_setting_t *old_hitbox_s = config_lookup(src, "metamode_hitbox");
	if (old_hitbox_s != NULL) {
		config_setting_t *x_s = config_setting_get_member(old_hitbox_s, "x");
		config_setting_t *y_s = config_setting_get_member(old_hitbox_s, "y");
		config_setting_t *w_s = config_setting_get_member(old_hitbox_s, "w");
		config_setting_t *h_s = config_setting_get_member(old_hitbox_s, "h");

		if (x_s && y_s && w_s && h_s) {
			int x = config_setting_get_int(x_s);
			int y = config_setting_get_int(y_s);
			int w = config_setting_get_int(w_s);
			int h = config_setting_get_int(h_s);

			config_setting_remove(dst_root, "metamode_hitbox");
			config_setting_t *new_hitbox_s = config_setting_add(dst_root, "metamode_hitbox", CONFIG_TYPE_ARRAY);
			config_setting_set_int_elem(new_hitbox_s, -1, x);
			config_setting_set_int_elem(new_hitbox_s, -1, y);
			config_setting_set_int_elem(new_hitbox_s, -1, w);
			config_setting_set_int_elem(new_hitbox_s, -1, h);
		}
	}

	/* upgrade various keymaps */
	char const *keymap_keys[] = {
		"metamode_keys",
		"metamode_sticky_keys",
		"metamode_func_keys",
		NULL
	};

	for (char const **key_ptr = keymap_keys; *key_ptr != NULL; ++key_ptr) {
		char const *key = *key_ptr;
		config_setting_t *old_map_s = config_lookup(src, key);
		if (old_map_s == NULL) {
			continue;
		}

		config_setting_remove(dst_root, key);
		config_setting_t *new_map_s = config_setting_add(dst_root, key, CONFIG_TYPE_LIST);
		
		for (size_t i = 0; i < config_setting_length(old_map_s); ++i) {
			config_setting_t *old_key_s = config_setting_get_elem(old_map_s, i);
			
			char const *from = config_setting_name(old_key_s);
			char const *to = config_setting_get_string(old_key_s);
			
			config_setting_t *new_key_s = config_setting_add(new_map_s, NULL, CONFIG_TYPE_LIST);
			config_setting_set_string_elem(new_key_s, -1, from);
			config_setting_set_string_elem(new_key_s, -1, to);
		}
	}

	/* upgrade symmenu */
	config_setting_t *old_map_s = config_lookup(src, "sym_keys");
	if (old_map_s == NULL) {
		return;
	}
	
	config_setting_t *new_map_s = config_setting_add(dst_root, "main_symmenu", CONFIG_TYPE_LIST);

	for (int row = config_setting_length(old_map_s) - 1; row >= 0; --row) {
		config_setting_t *old_row_s = config_setting_get_elem(old_map_s, row);
		config_setting_t *new_row_s = config_setting_add(new_map_s, NULL, CONFIG_TYPE_LIST);
			
		for (size_t col = 0; col < config_setting_length(old_row_s); ++col) {
			config_setting_t *old_key_s = config_setting_get_elem(old_row_s, col);
			
			char const *from = config_setting_name(old_key_s);
			char const *to = config_setting_get_string(old_key_s);
			
			config_setting_t *new_key_s = config_setting_add(new_row_s, NULL, CONFIG_TYPE_LIST);
			config_setting_set_string_elem(new_key_s, -1, from);
			config_setting_set_string_elem(new_key_s, -1, to);
		}
	}
}

static config_t *upgrade_config(char const *path, int old_version) {
	config_t src_data;
	config_t *src = &src_data;
	config_t *dst = (config_t*)malloc(sizeof(config_t));
	
	config_init(src);
	config_init(dst);
	config_read_file(src, path);
	config_read_file(dst, path);
	
	switch (old_version) {
	case 8:
		fprintf(stderr, "Upgrading from prefs. v8. Old prefs in %s\n", PREFS_FILE_BACKUP);
		upgrade_config_v8(dst, src);
		break;
	default:
		fprintf(stderr, "Preferences version not supported!\n");
		break;
	}

	config_destroy(src);
	return dst;
}

static int* create_int_array(config_t const *config, char const *path, size_t def_len, int const *def, int dynamic) {
	config_setting_t *setting = config_lookup(config, path);
	int use_default = 0;
	size_t source_len = 0;

	if (!setting || (config_setting_type(setting) != CONFIG_TYPE_ARRAY)) {
		fprintf(stderr, "invalid array %s, using default\n", path);
		source_len = def_len;
		use_default = 1;
	} else {
		source_len = config_setting_length(setting);
	}
	
	int *result = calloc(source_len + 1, sizeof(int));
	result[source_len] = -1;  // sentinel for end of array, only useful for positive arrays

	if (use_default) {
		for (int i = 0; i < source_len; i++) {
			result[i] = def[i];
		}
	} else {
		for (int i = 0; i < source_len; i++) {
			result[i] = config_setting_get_int_elem(setting, i);
		}
	}

	return result;
}

static keymap_t* create_keymap_array(config_t const *config, char const *path, size_t def_len, keymap_t const *def) {
	config_setting_t *setting = config_lookup(config, path);
	int use_default = 0;
	size_t source_len = 0;

	if (!setting || (config_setting_type(setting) != CONFIG_TYPE_LIST)) {
		fprintf(stderr, "invalid keymap list %s, using default\n", path);
		source_len = def_len;
		use_default = 1;
	} else {
		source_len = config_setting_length(setting);
	}
	
	keymap_t *result = calloc(source_len + 1, sizeof(keymap_t));
	result[source_len] = (keymap_t){0, NULL}; // sentinel for end of array
	
	if (use_default) {
		for (int i = 0; i < source_len; i++) {
			result[i].from = def[i].from;
			result[i].to = strdup(def[i].to);
		}
	} else {
		for (int i = 0; i < source_len; i++) {
			config_setting_t *m = config_setting_get_elem(setting, i);
			char const *from_str = config_setting_get_string_elem(m, 0);
			result[i].from = from_str[0];
			result[i].to = strdup(config_setting_get_string_elem(m, 1));
		}
	}

	return result;
}

static symmenu_t* create_symmenu(config_t const *config, char const *path, int def_num_rows, int const *def_row_lens, keymap_t const *def_entries) {
	config_setting_t *rows_s = config_lookup(config, path);
	int use_default = 0;

	symmenu_t *menu = calloc(1, sizeof(symmenu_t));
	
	/* TODO: more robust checking on config syntax */
	if (!rows_s || (config_setting_type(rows_s) != CONFIG_TYPE_LIST)) {
		fprintf(stderr, "invalid symmenu %s, using default\n", path);

		/* calculate the length of the keymap entry array */
		int def_num_keys = 0;
		for (int i = 0; i < def_num_rows; ++i) {
			def_num_keys += def_row_lens[i];
		}
		
		/* allocate the keymap entry and symkey arrays */
		menu->entries = calloc(def_num_keys + 1, sizeof(keymap_t));
		menu->entries[def_num_keys] = (keymap_t){0, NULL}; // sentinel for end of array
		
		menu->keys = calloc(def_num_rows + 1, sizeof(symkey_t*));
		menu->keys[def_num_rows] = NULL;
		
		/* fill in the keymap entry array */
		int entry_idx = 0;
		for (int row = 0; row < def_num_rows; ++row) {
			/* allocate the symkey row */
			menu->keys[row] = calloc(def_row_lens[row] + 1, sizeof(symkey_t));
			menu->keys[row][def_row_lens[row]].map = NULL;
			
			/* fill in the symkey row (rest done during render) */
			for (int col = 0; col < def_row_lens[row]; ++col) {
				menu->entries[entry_idx].from = def_entries[entry_idx].from;
				menu->entries[entry_idx].to = def_entries[entry_idx].to;
				
				menu->keys[row][col].flash = '\0';
				menu->keys[row][col].map = &menu->entries[entry_idx];
				
				++entry_idx;
			}
		}

	} else {
		/* calculate the length of the keymap entry array */
		int num_keys = 0;
		for (int row = 0; row < config_setting_length(rows_s); ++row) {
			config_setting_t *col_s = config_setting_get_elem(rows_s, row);
			num_keys += config_setting_length(col_s);
		}
	
		/* allocate the keymap entry and symkey arrays */
		menu->entries = calloc(num_keys + 1, sizeof(keymap_t));
		menu->entries[num_keys] = (keymap_t){0, NULL}; // sentinel for end of array
		
		menu->keys = calloc(config_setting_length(rows_s) + 1, sizeof(symkey_t*));
		menu->keys[config_setting_length(rows_s)] = NULL;

		/* fill in the keymap entry array */
		int entry_idx = 0;
		for (int row = 0; row < config_setting_length(rows_s); ++row) {
			config_setting_t *col_s = config_setting_get_elem(rows_s, row);
			int col_len = config_setting_length(col_s);
			
			/* allocate the symkey row */
			menu->keys[row] = calloc(col_len + 1, sizeof(symkey_t));
			menu->keys[row][col_len].map = NULL;
			
			/* fill in the symkey row (rest done during render) */
			for (int col = 0; col < col_len; ++col) {
				config_setting_t *m = config_setting_get_elem(col_s, col);
				char const *from_str = config_setting_get_string_elem(m, 0);
				menu->entries[entry_idx].from = from_str[0];
				menu->entries[entry_idx].to = strdup(config_setting_get_string_elem(m, 1));
			
				menu->keys[row][col].flash = '\0';
				menu->keys[row][col].map = &menu->entries[entry_idx];

				++entry_idx;
			}
		}
	}

	/* call the rendering function later after SDL init*/
	menu->surface = NULL;

	return menu;
}

static hitbox_t* create_hitbox(config_t const *config, char const *path, hitbox_t def) {
	config_setting_t *setting = config_lookup(config, path);
	int use_default = 0;

	if (!setting || (config_setting_type(setting) != CONFIG_TYPE_ARRAY)) {
		fprintf(stderr, "invalid array %s, using default\n", path);
		use_default = 1;
	}
	
	hitbox_t *result = calloc(1, sizeof(hitbox_t));

	if (use_default) {
		result->x = def.x;
		result->y = def.y;
		result->w = def.w;
		result->h = def.h;
	} else {
		result->x = config_setting_get_int_elem(setting, 0);
		result->y = config_setting_get_int_elem(setting, 1);
		result->w = config_setting_get_int_elem(setting, 2);
		result->h = config_setting_get_int_elem(setting, 3);
	}

	return result;
}

void destroy_preferences(pref_t *pref) {
	free(pref->font_path);
	free(pref->tty_encoding);

	free(pref->text_color);
	free(pref->background_color);
	free(pref->metamode_hitbox);
	
	keymap_t *m = pref->metamode_keys;
	while (m->to != NULL) { free(m->to); ++m; }
	free(pref->metamode_keys);
	
	m = pref->metamode_sticky_keys;
	while (m->to != NULL) { free(m->to); ++m; }
	free(pref->metamode_sticky_keys);
	
	m = pref->metamode_func_keys;
	while (m->to != NULL) { free(m->to); ++m; }
	free(pref->metamode_func_keys);

	destroy_symmenu(pref->main_symmenu);
	if (pref->is_passport) {
		destroy_symmenu(pref->passport_bar);
		destroy_symmenu(pref->passport_sym1);
		destroy_symmenu(pref->passport_sym2);
	}
	
	free(pref->keyhold_actions_exempt);

	free(pref);
}

#define DEFAULT_LOOKUP(type, conf, path, target, defval)	  \
	do { if (CONFIG_TRUE != config_lookup_##type(conf, path, &(target))) { target = defval; } } while(0)
pref_t *read_preferences(const char* filename, int is_passport) {
	pref_t *prefs = calloc(1, sizeof(pref_t)); // our internal data structure
	if (prefs == NULL) {
		fprintf(stderr, "fatal error: failed to calloc prefs structure\n");
		exit(1);
	}

	int is_first_run = 0; int upgraded = 0;
	
	config_t config_data; // what libconfig parses out of the file
	config_t *config = &config_data;
	config_init(config);
	
	if (access(filename, F_OK) == -1) {
		PRINT(stderr, "Preferences file not found, assuming first run\n");
		is_first_run = 1;
	} else {
		if(config_read_file(config, filename) != CONFIG_TRUE){
			fprintf(stderr, "%s:%d - %s\n", config_error_file(config),
			        config_error_line(config), config_error_text(config));
		}
	}

	DEFAULT_LOOKUP(int, config, "prefs_version", prefs->prefs_version, PREFS_VERSION);
	if(prefs->prefs_version != PREFS_VERSION) {
		config = upgrade_config(filename, prefs->prefs_version);
		upgraded = 1;
	}
	
	int default_font_columns = (atoi(getenv("WIDTH")) <= 720) ? 45 : 60;

	DEFAULT_LOOKUP(string, config, "font_path", prefs->font_path, DEFAULT_FONT_PATH);
	prefs->font_path = strdup(prefs->font_path);
	DEFAULT_LOOKUP(int, config, "font_size", prefs->font_size, DEFAULT_FONT_SIZE);
	prefs->text_color = create_int_array(config, "text_color", PREFS_COLOR_NUM_ELEMENTS, DEFAULT_TEXT_COLOR, 0);
	prefs->background_color = create_int_array(config, "background_color", PREFS_COLOR_NUM_ELEMENTS, DEFAULT_BACKGROUND_COLOR, 0);
	DEFAULT_LOOKUP(bool, config, "screen_idle_awake", prefs->screen_idle_awake, DEFAULT_SCREEN_IDLE_AWAKE);
	
	prefs->is_passport = is_passport;
	
	DEFAULT_LOOKUP(int, config, "metamode_doubletap_key", prefs->metamode_doubletap_key, DEFAULT_METAMODE_DOUBLETAP_KEY);
	DEFAULT_LOOKUP(int, config, "metamode_doubletap_delay", prefs->metamode_doubletap_delay, DEFAULT_METAMODE_DOUBLETAP_DELAY);
	DEFAULT_LOOKUP(bool, config, "keyhold_actions", prefs->keyhold_actions, DEFAULT_KEYHOLD_ACTIONS);
	DEFAULT_LOOKUP(int, config, "metamode_hold_key", prefs->metamode_hold_key, DEFAULT_METAMODE_HOLD_KEY);
	DEFAULT_LOOKUP(bool, config, "allow_resize_columns", prefs->allow_resize_columns, DEFAULT_ALLOW_RESIZE_COLUMNS);
	prefs->metamode_hitbox = create_hitbox(config, "metamode_hitbox", DEFAULT_METAMODE_HITBOX);
	DEFAULT_LOOKUP(string, config, "tty_encoding", prefs->tty_encoding, DEFAULT_TTY_ENCODING);
	prefs->tty_encoding = strdup(prefs->tty_encoding);
	
	prefs->metamode_keys = create_keymap_array(config, "metamode_keys", default_metamode_keys_len, default_metamode_keys);
	
	prefs->metamode_sticky_keys = create_keymap_array(config, "metamode_sticky_keys", default_metamode_sticky_keys_len, default_metamode_sticky_keys);
	
	prefs->metamode_func_keys = create_keymap_array(config, "metamode_func_keys", default_metamode_func_keys_len, default_metamode_func_keys);
	
	DEFAULT_LOOKUP(bool, config, "sticky_sym_key", prefs->sticky_sym_key, DEFAULT_STICKY_SYM_KEY);
	DEFAULT_LOOKUP(bool, config, "sticky_shift_key", prefs->sticky_shift_key, DEFAULT_STICKY_SHIFT_KEY);
	DEFAULT_LOOKUP(bool, config, "sticky_alt_key", prefs->sticky_alt_key, DEFAULT_STICKY_ALT_KEY);
	
	prefs->keyhold_actions_exempt = create_int_array(config, "keyhold_actions_exempt", default_keyhold_actions_exempt_len, default_keyhold_actions_exempt, 1);
	
	DEFAULT_LOOKUP(bool, config, "rescreen_for_symmenu", prefs->rescreen_for_symmenu, DEFAULT_RESCREEN_FOR_SYMMENU);
	DEFAULT_LOOKUP(bool, config, "keyhold_accents", prefs->keyhold_accents, DEFAULT_KEYHOLD_ACCENTS);

	prefs->main_symmenu = create_symmenu(config, "main_symmenu", default_symmenu_num_rows, default_symmenu_row_lens, default_symmenu_entries);
	prefs->altsym_entries = create_keymap_array(config, "altsym_entries", default_altsym_entries_len, default_altsym_entries);

	if (prefs->is_passport) {
		prefs->passport_bar = create_symmenu(config, "passport_bar", passportvkb_num_rows, passportvkb_row_lens, passportvkb_entries);
		prefs->passport_sym1 = create_symmenu(config, "passport_sym1", passportsym1_num_rows, passportsym1_row_lens, passportsym1_entries);
		prefs->passport_sym2 = create_symmenu(config, "passport_sym2", passportsym2_num_rows, passportsym2_row_lens, passportsym2_entries);
	} else {
		prefs->passport_bar = NULL;
		prefs->passport_sym1 = NULL;
		prefs->passport_sym2 = NULL;
	}

	/* the accent menus are configurable, but we won't include them in the default config */
	char am_name[] = {' ', '_', 'a', 'c', 'c', 'e', 'n', 't', 's', '\0'};
	for (char c = 'a'; c <= 'z'; ++c) {
		size_t idx = (size_t)(c - 'a');
		am_name[0] = c;
		prefs->accent_menus[idx][0] = create_symmenu(config, am_name, 1, &accent_row_lens[idx], lowercase_accent_entries[idx]);
		prefs->accent_menus[idx][1] = create_symmenu(config, am_name, 1, &accent_row_lens[idx], uppercase_accent_entries[idx]);
	}

	if (is_first_run) {
		first_run(prefs);
	}

	if (upgraded) {
		if (rename(PREFS_FILE_PATH, PREFS_FILE_BACKUP)) {
			fprintf(stderr, "Failed to back up old prefs! Won't overwrite %s\n", PREFS_FILE_PATH);
		} else {
			save_preferences(prefs, PREFS_FILE_PATH);
		}
	}

	return prefs;
}

void set_int_array(config_setting_t *root, char const *key, size_t num_elems, int const *source) {
	config_setting_t *setting = config_setting_add(root, key, CONFIG_TYPE_ARRAY);
	for (size_t i = 0; i < num_elems; ++i) {
		config_setting_t *elem = config_setting_add(setting, NULL, CONFIG_TYPE_INT);
		config_setting_set_int(elem, source[i]);
	}
}

void set_keymap_array(config_setting_t *root, char const *key, keymap_t const *source) {
	config_setting_t *setting = config_setting_add(root, key, CONFIG_TYPE_LIST);
	for (; source->to != NULL; ++source) {
		config_setting_t *group = config_setting_add(setting, NULL, CONFIG_TYPE_LIST);
		
		config_setting_t *from_s = config_setting_add(group, NULL, CONFIG_TYPE_STRING);
		char from_str[2] = {source->from, '\0'};
		config_setting_set_string(from_s, from_str);

		config_setting_t *to_s = config_setting_add(group, NULL, CONFIG_TYPE_STRING);
		config_setting_set_string(to_s, source->to);
	}
}

void set_symmenu(config_setting_t *root, char const *key, symmenu_t const *source) {
	config_setting_t *rows_s = config_setting_add(root, key, CONFIG_TYPE_LIST);
	config_setting_t *col_s = config_setting_add(rows_s, NULL, CONFIG_TYPE_LIST);

	int row = 0; int col = 0;
	while (1) {
		if (source->keys[row][col].map == NULL) {
			++row;
			if (source->keys[row] == NULL) {
				return;
			}
			col = 0;
			col_s = config_setting_add(rows_s, NULL, CONFIG_TYPE_LIST);
			continue;
		}
		
		config_setting_t *group = config_setting_add(col_s, NULL, CONFIG_TYPE_LIST);
		
		config_setting_t *from_s = config_setting_add(group, NULL, CONFIG_TYPE_STRING);
		char from_str[2] = {source->keys[row][col].map->from, '\0'};
		config_setting_set_string(from_s, from_str);

		config_setting_t *to_s = config_setting_add(group, NULL, CONFIG_TYPE_STRING);
		config_setting_set_string(to_s, source->keys[row][col].map->to);

		++col;
	}
}

#define PREF_SET(root, setptr, key, type, configtype, source)	  \
	do { setptr = config_setting_add(root, key, CONFIG_TYPE_##configtype); \
		config_setting_set_##type(setptr, source); } while(0)
void save_preferences(pref_t const* prefs, char const* filename) {
	config_t config;
	config_init(&config);
	config_setting_t *root = config_root_setting(&config);
	config_setting_t *setting = NULL;

	PREF_SET(root, setting, "font_path", string, STRING, prefs->font_path);
	PREF_SET(root, setting, "font_size", int, INT, prefs->font_size);
	set_int_array(root, "text_color", PREFS_COLOR_NUM_ELEMENTS, prefs->text_color);
	set_int_array(root, "background_color", PREFS_COLOR_NUM_ELEMENTS, prefs->background_color);
	PREF_SET(root, setting, "screen_idle_awake", bool, BOOL, prefs->screen_idle_awake);
	PREF_SET(root, setting, "metamode_doubletap_key", int, INT, prefs->metamode_doubletap_key);
	PREF_SET(root, setting, "metamode_doubletap_delay", int, INT, prefs->metamode_doubletap_delay);
	PREF_SET(root, setting, "keyhold_actions", bool, BOOL, prefs->keyhold_actions);
	PREF_SET(root, setting, "metamode_hold_key", bool, BOOL, prefs->metamode_hold_key);
	PREF_SET(root, setting, "allow_resize_columns", bool, BOOL, prefs->allow_resize_columns);
	set_int_array(root, "metamode_hitbox", 4, prefs->metamode_hitbox);
	PREF_SET(root, setting, "tty_encoding", string, STRING, prefs->tty_encoding);
	set_keymap_array(root, "metamode_keys", prefs->metamode_keys);
	set_keymap_array(root, "metamode_sticky_keys", prefs->metamode_sticky_keys);
	set_keymap_array(root, "metamode_func_keys", prefs->metamode_func_keys);
	set_symmenu(root, "main_symmenu", prefs->main_symmenu);
	PREF_SET(root, setting, "sticky_sym_key", bool, BOOL, prefs->sticky_sym_key);
	PREF_SET(root, setting, "sticky_shift_key", bool, BOOL, prefs->sticky_shift_key);
	PREF_SET(root, setting, "sticky_alt_key", bool, BOOL, prefs->sticky_alt_key);
	PREF_SET(root, setting, "rescreen_for_symmenu", bool, BOOL, prefs->rescreen_for_symmenu);
	
	int num_exempt = 0;
	for (; prefs->keyhold_actions_exempt[num_exempt] > 0; ++num_exempt) { }
	set_int_array(root, "keyhold_actions_exempt", num_exempt, prefs->keyhold_actions_exempt);
	
	config_write_file(&config, filename);
	return;
}

int is_int_member(int const* list, int target) {
	for (int i = 0; list[i] != -1; ++i) {
		if (list[i] == target) {
			return 1;
		}
	}
	
	return 0;
}

const char* keystroke_lookup(char keystroke, keymap_t *keymap_head) {
	while (keymap_head->to != NULL) {
		if (keymap_head->from == keystroke) {
			return keymap_head->to;
		}
		++keymap_head;
	}
	return NULL;
}
