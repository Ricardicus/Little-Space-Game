/*
* This file contains all the typedefs, function declarations and structs used in the game
*/

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
typedef int (*move_callback)(void*);
typedef int (*collision_callback)(int,int);
typedef void (*timer_func)(void);

/* Struct declarations,  each struct define a certain type of element within the game */
struct dot_bulb {
    int x;
    int y;
    int side;
    int state;
    int substate;
    int direction;
    int state_count;
    int nbr_of_states;
    draw_callback draw_f;
    collision_callback collide_f;
    move_callback move_f;
};

struct shot_bulb {
    int x;
    int y;
    int direction;
    int fire_dmg;
    draw_callback draw_f;
    move_callback move_f;
};

struct laser_shot {
    int xstart;
    int y;
    int direction;
    draw_callback draw_f;
    move_callback move_f;
};

struct thunder_shot {
    int **thunder_grid;
    int xstart;
    int ystart;
    int state;
    int direction;
    int dmg;
    draw_callback draw_f;
    move_callback move_f;
};

struct monster_bulb {
    int x;
    int y;
    int side;
    int move;
    int state;
    int nbr_of_states;
    draw_callback draw_f;
    collision_callback collide_f;
};

struct star_bulb {
    int x;
    int y;
    int speed;
    int side;
    draw_callback draw_f;
    move_callback move_f;
};

struct moon_bulb {
    int x;
    int y;
    int speed;
    int side;
    draw_callback draw_f;
    move_callback move_f;
};

struct enemy_aircraft {
    int x;
    int y;
    int side;
    int state;
    int substate;
    int fire_count;
    int direction;
    draw_callback draw_f;
    collision_callback collide_f;
    move_callback move_f;
};

/* Type declarations */
typedef enum draw_type_e{
    monster,
    dot,
    shot,
    laser,
    thunder,
    star,
    moon,
    enemy
} draw_object_type_t;

typedef struct drawable_u {
    draw_object_type_t type;
    struct dot_bulb dot;
    struct monster_bulb monster;
    struct shot_bulb shot;
    struct star_bulb star;
    struct moon_bulb moon;
    struct enemy_aircraft enemy;
    struct laser_shot laser;
    struct thunder_shot thunder;
} drawable_t;

typedef struct world_u {
    int size;
    int max_nbr_of_objects;
    drawable_t ** drawables;
} world_t;

enum shot_type_e {
    fire_shot,
    laser_shot,
    thunder_shot
};

typedef struct shot_request_s {
    int active;
    enum shot_type_e shot_type;
    int direction;
    int fire_dmg;
    int x;
    int y;
} shot_request_t;

/* Function declarations */
void draw_dot_bulb(void*);
void erase_dot_bulb(void*);
void draw_fire_shot(void*);
void draw_laser_shot(void*);
void draw_thunder_shot(void*);
void draw_enemy(void*);
void draw_star(void*);
void draw_moon(void*);
void draw_aircraft(int,int);
void draw_enemy_aircraft(int,int);
void draw_world(void);
int move_star(void*);
int move_moon(void*);
int move_shot(void*);
int move_laser(void*);
int move_thunder(void*);
int move_dot(void*);
int move_enemy(void*);
void spawn_stuff(void);
void create_star(void);
void create_moon(void);
void create_enemy(void);
void create_fire_shot(int,int,int,int);
void create_laser_shot(int,int,int);
void create_thunder_shot(int,int,int,int);
void shot_request(void);
int check_collision_aircraft(int,int);
void collision_check_world(void);
void check_requests(void);
void erase_drawable(int id);
world_t* init_world(int);
void initialise_world_1(void);
void destroy_world(void);
void free_thunder_shot_grid(void*);
static void* event_loop(void*);
static void* timer_loop(void*);
void close_x(void);
void move_world(void);
void world_refresh(void);


#ifdef __cpluspluc
}
#endif	