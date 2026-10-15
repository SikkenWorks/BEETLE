/* Punch-in event queue, bounded / no dynamic allocation. Native RAM only.
 * Not a replacement for validated persistent flash storage.
 */
#ifndef TRACK_PUNCH_EVENTS_H
#define TRACK_PUNCH_EVENTS_H
#include <stdint.h>
#define TRACK_MAX_PUNCH_EVENTS 128u
struct track_punch_event {uint8_t row,fx,down;};
struct track_punch_events {struct track_punch_event events[TRACK_MAX_PUNCH_EVENTS];uint8_t count,overflow;};
static inline void track_punch_clear(struct track_punch_events *p){if(p){p->count=0;p->overflow=0;}}
static inline int track_punch_add(struct track_punch_events *p,uint8_t row,uint8_t fx,uint8_t down){
 if(!p||row>=16u||fx>=8u||down>1u)return -1;
 if(p->count>=TRACK_MAX_PUNCH_EVENTS){p->overflow=1u;return -2;}
 p->events[p->count].row=row;p->events[p->count].fx=fx;p->events[p->count].down=down;p->count++;return 0;
}
#endif
