#ifndef KEYMAPS_H_
#define KEYMAPS_H_

extern int default_metamode_keys_len;
extern keymap_t default_metamode_keys[];

extern int default_metamode_sticky_keys_len;
extern keymap_t default_metamode_sticky_keys[];

extern int default_metamode_func_keys_len;
extern keymap_t default_metamode_func_keys[];

extern int default_symmenu_num_rows;
extern int default_symmenu_row_lens[];
extern keymap_t default_symmenu_entries[];

extern int default_keyhold_actions_exempt_len;
extern int default_keyhold_actions_exempt[];

extern int default_altsym_entries_len;
extern keymap_t default_altsym_entries[];

/* passport-specific */
extern int passportvkb_num_rows;
extern int passportvkb_row_lens[];
extern keymap_t passportvkb_entries[];

extern int passportsym1_num_rows;
extern int passportsym1_row_lens[];
extern keymap_t passportsym1_entries[];

extern int passportsym2_num_rows;
extern int passportsym2_row_lens[];
extern keymap_t passportsym2_entries[];

#endif
