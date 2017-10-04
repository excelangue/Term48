#include "preferences.h"
#include "types.h"

int default_metamode_keys_len = 2;
keymap_t default_metamode_keys[] = {{'e', "\x1b"}, {'t', "\x09"}};

int default_metamode_sticky_keys_len = 4;
keymap_t default_metamode_sticky_keys[] = {
	{'k', "kcuu1"},
	{'j', "kcud1"},
	{'l', "kcuf1"},
	{'h', "kcub1"}
};

int default_metamode_func_keys_len = 4;
keymap_t default_metamode_func_keys[] = {
	{'a', ALT_DOWN_ACTION},
	{'c', CTRL_DOWN_ACTION},
	{'s', RESCREEN_ACTION},
	{'v', PASTE_CLIPBOARD_ACTION}
};

int default_symmenu_num_rows = 2;
int default_symmenu_row_lens[] = {10, 9};
keymap_t default_symmenu_entries[] = {
	{'q', "~"}, {'w', "`"}, {'e', "{"},  {'r', "}"}, {'t', "["}, {'y', "]"}, {'u', "<"}, {'i', ">"}, {'o', "^"}, {'p', "%"},
	{'a', "="}, {'s', "-"}, {'d', "*"},  {'f', "/"}, {'g', "\\"},{'h', "|"}, {'j', "&"}, {'k', "'"}, {'l', "\""}
};

int default_keyhold_actions_exempt_len = 2;
int default_keyhold_actions_exempt[] = {KEYCODE_BACKSPACE, KEYCODE_RETURN};

int default_altsym_entries_len = 27;
keymap_t default_altsym_entries[] = {
    {'q', "#"}, {'w', "1"}, {'e', "2"}, {'r', "3"}, {'t', "("}, {'y', ")"}, {'u', "_"}, {'i', "-"}, {'o', "+"}, {'p', "@"},
                {'a', "*"}, {'s', "4"}, {'d', "5"}, {'f', "6"}, {'g', "/"}, {'h', ":"}, {'j', ";"}, {'k', "'"}, {'l', "\""},
                {'z', "7"}, {'x', "8"}, {'c', "9"}, {'v', "?"}, {'b', "!"}, {'n', ","}, {'m', "."},
                            {'0', "0"}
};

/* passport-specific */

int passportvkb_num_rows = 1;
int passportvkb_row_lens[] = {10};
keymap_t passportvkb_entries[] = {
	{'\0', PASSPORT_SHIFT_GLYPH}, {'\0', NULL}, {'\0', "_"}, {'\0', "-"},
	{'\0', ":"}, {'\0', "/"}, {'\0', "?"}, {'\0', "."}, {'\0', PASSPORT_SYMMENU_OPEN_GLYPH}, {'\0', NULL},
};

int passportsym1_num_rows = 4;
int passportsym1_row_lens[] = {10, 10, 10, 10};
keymap_t passportsym1_entries[] = {
	{'q', "0"}, {'w', "1"}, {'e', "2"},  {'r', "3"}, {'t', "("}, {'y', ")"}, {'u', "_"}, {'i', "-"}, {'o', "+"}, {'p', "="},
	{'a', "#"}, {'s', "4"}, {'d', "5"},  {'f', "6"}, {'g', "/"}, {'h', ":"}, {'j', ";"}, {'k', "'"}, {'l', "\""},
	{'\0', PASSPORT_SYMMENU_ACTIVE_PAGE_GLYPH},
	{'z', "*"}, {'x', "7"}, {'c', "8"},  {'v', "9"}, {' ', " "}, {' ', " "}, {'b', "@"}, {'n', "&"}, {'m', "$"},
	{'\0', PASSPORT_SYMMENU_INACTIVE_PAGE_GLYPH},
	{'\0', PASSPORT_SHIFT_GLYPH}, {'\0', NULL}, {'\0', "_"}, {'\0', "-"},
	{'\0', ":"}, {'\0', "/"}, {'\0', "?"}, {'\0', "."}, {'\0', PASSPORT_SYMMENU_CLOSE_GLYPH}, {'\0', NULL},
};

int passportsym2_num_rows = 4;
int passportsym2_row_lens[] = {10, 10, 10, 10};
keymap_t passportsym2_entries[] = {
	{'q', "~"}, {'w', "`"}, {'e', "{"},  {'r', "}"}, {'t', "["}, {'y', "]"}, {'u', "<"}, {'i', ">"}, {'o', "^"}, {'p', "%"},
	{'a', "_"}, {'s', "\u00F7"}, {'d', "\u00B1"},  {'f', "\u00B7"}, {'g', "\\"},{'h', "|"}, {'j', "\u00A4"}, {'k', "\u00AB"}, {'l', "\u00BB"},
	{'\0', PASSPORT_SYMMENU_INACTIVE_PAGE_GLYPH},
	{'z', "\u00A5"}, {'x', "\u20AC"}, {'c', "\u00A3"},  {'v', "$"}, {' ', " "}, {' ', " "}, {'b', "\u00A1"},{'n', "\u00BF"}, {'m', "."},
	{'\0', PASSPORT_SYMMENU_ACTIVE_PAGE_GLYPH},
	{'\0', PASSPORT_SHIFT_GLYPH}, {'\0', NULL}, {'\0', "_"}, {'\0', "-"},
	{'\0', ":"}, {'\0', "/"}, {'\0', "?"}, {'\0', "."}, {'\0', PASSPORT_SYMMENU_CLOSE_GLYPH}, {'\0', NULL},
};
