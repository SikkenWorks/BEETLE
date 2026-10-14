/* TRACK project name editor for the actual FM-1. GPL-3.0-only.
 * Pure state machine; only the SAVE key can confirm. Never writes flash.
 */
#ifndef TRACK_NAME_H
#define TRACK_NAME_H
#include <stdint.h>
#define TRACK_NAME_MAX 12u
struct track_name_editor {
 char name[TRACK_NAME_MAX+1u];
 uint8_t cursor;             /* K1: character cell, 0..11 */
 uint8_t glyph_index;        /* K2: current index in TRACK_NAME_CHARS */
 uint8_t remembered_index;   /* starting wheel position of subsequent cells */
 uint8_t last_index[TRACK_NAME_MAX]; /* value selected at each position */
 uint8_t touched[TRACK_NAME_MAX];   /* blank is different from a selected space */
 uint8_t active;
};
void track_name_begin(struct track_name_editor *ed,const char *existing);
void track_name_move(struct track_name_editor *ed,int32_t detents);
void track_name_change(struct track_name_editor *ed,int32_t detents);
void track_name_finish(struct track_name_editor *ed,uint8_t slot,char output[TRACK_NAME_MAX+1u]);
uint8_t track_name_charset_size(void);
char track_name_charset_at(uint8_t index);
#endif
