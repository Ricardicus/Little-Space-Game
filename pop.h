#ifdef __cplusplus
extern "C" {
#endif 

#include <X11/Xlib.h>
#include <X11/Shell.h>
#include <X11/Xfuncs.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define up      1
#define down    2
#define left    3
#define right   4
#define still   5

/* Function type declarations */
typedef void (*draw_callback)(void*);
typedef void (*timer_func)(void);

/* Struct declarations */
struct dot_bulb {
    int x;
    int y;
    int side;
    int state;
    int direction;
    int state_count;
    int nbr_of_states;
    draw_callback draw_f;
};

struct shot_bulb {
    int x;
    int y;
    draw_callback draw_f;
};

struct monster_bulb {
    int x;
    int y;
    int side;
    int move;
    int state;
    int nbr_of_states;
    draw_callback draw_f;
};

/* Type declarations */
typedef enum draw_type_e{
    monster,
    dot,
    shot
} draw_object_type_t;

typedef struct drawable_u {
    draw_object_type_t type;
    struct dot_bulb dot;
    struct monster_bulb monster;
    struct shot_bulb shot;
} drawable_t;

typedef struct world_u {
    int size;
    int max_nbr_of_objects;
    drawable_t ** drawables;
} world_t;


#ifdef __cpluspluc
}
#endif	