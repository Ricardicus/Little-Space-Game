 /*
  * Simple Xlib game. Not super fun but you got to start somewhere :) 
  * gcc input.c -o output -lX11
  */
 
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
#include "pop.h"

/* 
*   Window variables
*/

static XColor red,blue,yellow,white;
Display *display;
Window window;
XEvent event;
int s;
int screen_num;
Colormap colormap;
Arg args[16];
int width;
int height;
GC gc;
unsigned int killed;
unsigned int lives;
enum shot_type_e player_fire;

/* Application related variables */
int alive=1;
int is_initialised=0;
int maxheight = 2000;
int maxwidth = 2000;
int enemy_spawn_rate_start = 400;
int basic_enemy_shoot_freq = 40;
int there_is_lightning = 0;
int escaped = 0;
game_session_t levels;

/* used for checking collisions, more specificly its a matrix describing where dangerous stuff is */
char collision_grid[2000][2000];

/* the world */  
world_t * world;

/* whenever the user presses 'f' the player will fire and a shot_request will be generated */
shot_request_t shot_req;

/* returns a pointer to a world */
world_t * init_world(int max){
    world_t * wd = (world_t*) malloc(sizeof(world_t));
    wd->size=0;
    wd->max_nbr_of_objects=max;
    wd->drawables = (drawable_t**) malloc(sizeof(drawable_t*)*wd->max_nbr_of_objects);

    for(int i = 0; i < maxwidth; i++){
        for(int n = 0; n < maxheight; n++){
            collision_grid[i][n] = 0;
        }
    }

    killed = 0;

    shot_req.active = 0;

    return wd;
}

/* In space there are stars, so this function creates beautiful moving stars in the background */ 
void create_star(void){

    int i = world->size;
    if(i >= world->max_nbr_of_objects){
        return;
    }

    drawable_t * drawable = (drawable_t*) malloc(sizeof(drawable_t));

    drawable->star.x = width;
    drawable->star.y = rand()%height;
    drawable->star.speed = 1 + rand()%4;
    drawable->star.side = 3 + rand()%6;
    drawable->star.draw_f = draw_star;
    drawable->star.move_f = move_star;
    drawable->type = star;

    world->drawables[i] = drawable;
    world->size++;
}


/* In space there are moons, so this function creates beautiful moving moons in the background */ 
void create_moon(void){

    int i = world->size;
    if(i >= world->max_nbr_of_objects){
        return;
    }

    drawable_t * drawable = (drawable_t*) malloc(sizeof(drawable_t));

    drawable->moon.x = width;
    drawable->moon.y = rand()%height;
    drawable->moon.speed = 1 + rand()%4;
    drawable->moon.side = 1 + rand()%6;
    drawable->moon.draw_f = draw_moon;
    drawable->moon.move_f = move_moon;
    drawable->type = moon;

    world->drawables[i] = drawable;
    world->size++;
}

/* to generate fire balls */
void create_fire_shot(int x, int y, int direction, int fire_dmg_i){
    int i = world->size;
    if(i >= world->max_nbr_of_objects){
        return;
    }
    drawable_t * drawable = (drawable_t*) malloc(sizeof(drawable_t));
    /* one dot to start with */ 
    drawable->shot.direction = direction;
    drawable->shot.fire_dmg = fire_dmg_i;
    drawable->shot.x = x;
    drawable->shot.y = y;
    drawable->shot.draw_f = draw_fire_shot;
    drawable->shot.move_f = move_shot;
    drawable->type = shot;

    world->drawables[i] = drawable;
    world->size++;
}

void free_thunder_shot_grid(void * vb){
    struct thunder_shot * ts = (struct thunder_shot*) vb;

    for(int x = 0; x<maxwidth;x++){
        free(ts->thunder_grid[x]);
    }

    free(ts->thunder_grid);

    printf("thunder shot grid freed!\n");
}

void create_thunder_shot(int x, int y, int direction, int dmg){

    int i = world->size;
    if(i >= world->max_nbr_of_objects){
        return;
    }
    drawable_t * drawable = (drawable_t*) malloc(sizeof(drawable_t));
    /* one dot to start with */ 
    drawable->thunder.direction = direction;
    drawable->thunder.xstart = x;
    drawable->thunder.ystart = y;
    drawable->thunder.state = 5;
    drawable->thunder.dmg = dmg;
    drawable->thunder.draw_f = draw_thunder_shot;
    drawable->thunder.move_f = move_thunder;

    int ** thunder_grid = (int**) malloc(sizeof(int*)*maxwidth);
    for(int i = 0; i<maxwidth;i++){
        thunder_grid[i] = (int*) malloc(sizeof(int)*maxheight);
    }

    drawable->thunder.thunder_grid = thunder_grid;

    drawable->type = thunder;

    world->drawables[i] = drawable;
    world->size++;

    printf("thunder shot created\n");

}

/* to generate laser shots */
void create_laser_shot(int x, int y, int direction){
    int i = world->size;

    if(i >= world->max_nbr_of_objects){
        return;
    }

    drawable_t * drawable = (drawable_t*) malloc(sizeof(drawable_t));

    drawable->type = laser;
    drawable->laser.xstart = x;
    drawable->laser.y = y;
    drawable->laser.direction = direction;
    drawable->laser.draw_f = draw_laser_shot;
    drawable->laser.move_f = move_laser;

    world->drawables[i] = drawable;
    world->size++;
}

/* checks if there are collisions in the world */
void collision_check_world(){
    for(int i = 0;i<world->size;i++){
        switch(world->drawables[i]->type){
            case enemy:
                if(world->drawables[i]->enemy.collide_f(world->drawables[i]->enemy.x,world->drawables[i]->enemy.y)){
                    world->drawables[i]->enemy.state = 3;
                    world->drawables[i]->enemy.substate = 0;
                }
                break;
            case dot:
                if(world->drawables[i]->dot.collide_f(world->drawables[i]->dot.x,world->drawables[i]->dot.y)){
                    world->drawables[i]->dot.state = 2;
                    world->drawables[i]->dot.substate = 0;
                    if(!lives) alive=0;
                }
                break;
            default:
                break;
        }
    }
}

void spawn_stuff(void){

    int enemy_spawn_rate = enemy_spawn_rate_start;

    enemy_spawn_rate = enemy_spawn_rate_start - killed;
    if(enemy_spawn_rate <= 20){
        enemy_spawn_rate = 20;
    }


/* background generation */
    int r = rand()%20;
    if(r == 14){
        create_star();
    }
    r = rand()%200;
    if(r == 14){
        create_moon();
    }

/* the number of enemys generated depends on the level */

    char * msg;

    switch(levels.level){
        case 1:
            if(!levels.active && levels.count <= 100){
                msg = "Get ready for the first wave...";
                XDrawString(display, window, DefaultGC(display, s), width/2 - 40, height/2, msg,strlen(msg));
                levels.count++;                
            } else {
                if(levels.count > 100){
                    levels.count = 0;
                    levels.active = 1;
                    for(int i = 0; i<20;i++){
                        create_enemy();
                    }
                    break;
                }
                if(levels.active){
                    r = rand()%enemy_spawn_rate;
                    if(!r){
                        create_enemy();
                    }
                }
            }
            if(killed >= 20){
                levels.level = 2;
                levels.active = 0;
            }
            break;
        case 2: 
            if(!levels.active && levels.count <= 100){
                msg = "You made it!! New gun: Laser gun";
                XDrawString(display, window, DefaultGC(display, s), width/2 - 40, height/2, msg,strlen(msg));
                levels.count++;                
            } else {
                if(levels.count > 100){
                    levels.count = 0;
                    levels.active = 1;
                    for(int i = 0; i<40;i++){
                        create_enemy();
                    }
                    break;
                }
                if(levels.active){
                    r = rand()%enemy_spawn_rate;
                    if(!r){
                        create_enemy();
                    }
                }
            }
            if(killed >= 40){
                levels.level = 3;
                levels.active = 0;
            }
            break;
        default:
            r = rand()%enemy_spawn_rate;
            if(!r){
                create_enemy();
            }
            break;

    }
}

/* There variables stores information that will be used to write strings onto the screen */
char display_msg[256];
int len;

void draw_standard_txt(){
    sprintf(display_msg,"Units killed: %d", killed);
    len = strlen(display_msg);

    XDrawString(display, window, DefaultGC(display, s), width - len*8, height - 20, display_msg, len);

    sprintf(display_msg,"Enemys Escaped: %d", escaped);
    len = strlen(display_msg);

    XDrawString(display, window, DefaultGC(display, s), width - len*8, height - 40, display_msg, len);


    memset(display_msg,'\0',256);    

    sprintf(display_msg,"Current weapon: ");
    len = strlen(display_msg);
    XDrawString(display, window, DefaultGC(display, s), len*3, height - 40, display_msg, len);

    switch(player_fire){
        case fire_shot:
            XDrawString(display, window, DefaultGC(display, s), len*4 + 80, height - 40, "Fireball", 8);
            break;
        case laser_shot:
            XDrawString(display, window, DefaultGC(display, s), len*4 + 80, height - 40, "Laser shot", 10);
            break;
        case thunder_shot:
            XDrawString(display, window, DefaultGC(display, s), len*4 + 80, height - 40, "Thunder shot", 12);
            break;
        default:
            break;
    }

    memset(display_msg,'\0',256);    
    sprintf(display_msg,"Lives left: ");
    len = strlen(display_msg);
    XDrawString(display, window, DefaultGC(display, s), len*4, height - 20, display_msg, len);

    for(int i = 0; i<lives; i++){
        XSetForeground(display, gc, red.pixel);
        XFillRectangle(display, window, gc, len*4 + 70 + i*15, height - 27, 10,10);
        XSetForeground(display, gc, BlackPixel(display,screen_num));
    }

    XFlush(display);
}

/* structurally calling functions to draw the content of each world-element onto the XWindow */
void draw_world(){
    for(int i = 0;i<world->size;i++){
        switch(world->drawables[i]->type){
            case monster:
                world->drawables[i]->monster.draw_f(&world->drawables[i]->monster);
                break;
            case dot:
                world->drawables[i]->dot.draw_f(&world->drawables[i]->dot);
                break;
            case shot:
                world->drawables[i]->shot.draw_f(&world->drawables[i]->shot);
                break;
            case star:
                world->drawables[i]->star.draw_f(&world->drawables[i]->star);
                break;
            case enemy:
                world->drawables[i]->enemy.draw_f(&world->drawables[i]->enemy);
                break;
            case laser:
                world->drawables[i]->laser.draw_f(&world->drawables[i]->laser);
                break;
            case thunder:
                world->drawables[i]->thunder.draw_f(&world->drawables[i]->thunder);
                break;
            case moon:
                world->drawables[i]->moon.draw_f(&world->drawables[i]->moon);
                break;                
            default:
                break;
        }
    }
}

/* A thread will run this function and another function, that is passed in as function parameter 'tf', will be called 
* at regular intervals within the forever-true-while-loop. The reason for that is to make the game logic faster that the display, 
* and for that matter, also slow down the game, which is super necessary because it runs super fast otherwise.. */
static void * timer_loop(void * tf)
{
    unsigned long event_trigger = 10000000;
    unsigned long event_counter = 0;

    timer_func tf_f = (timer_func) tf;

    while(1)
    {
        event_counter++;
        if(event_counter==event_trigger)
        {
            event_counter=0;
            tf_f();
        }
    }
    pthread_exit(NULL);
}

/* A thread will run this function that is used to get user input in the game.
* It listens to keys being pressed and acts accordingly. */
static void* event_loop(void*data)
{
    /* event loop */
    static int x = 0;
    static int y = 0;

    KeySym key;
    char text[255];

    for (;;)
    {
        XNextEvent(display, &event);
 
        /* draw or redraw the window */
        if (event.type == Expose)
        {
        }
        else if (event.type == ConfigureNotify) {
            XConfigureEvent xce = event.xconfigure;
            /* This event type is generated for a variety of
               happenings, so check whether the window has been
               resized. */
            if (xce.width != width || xce.height != height) {
                width = xce.width;
                height = xce.height;
            }
        }

        else if (event.type == KeyPress && XLookupString(&event.xkey,text,255,&key,0)==1) {
        /* the XLookupString routine converts the invent
           KeyPress data into regular text.  Weird but necessary.
        */

            switch(text[0]){
                /* to quit the game, 'q' should be pressed */
                case 'q':
                    alive=0;
                    break;
                case 'w':
                    if(world->drawables[0]->dot.direction == down){
                        world->drawables[0]->dot.direction = still;
                        break;
                    }
                    world->drawables[0]->dot.direction = up;
                   break;
                case 'd':
                    if(world->drawables[0]->dot.direction == left){
                        world->drawables[0]->dot.direction = still;
                        break;
                    }
                    world->drawables[0]->dot.direction = right;
                    break;
                case 'a':
                    if(world->drawables[0]->dot.direction == right){
                        world->drawables[0]->dot.direction = still;
                        break;
                    }
                    world->drawables[0]->dot.direction = left;
                    break;
                case 's':
                    if(world->drawables[0]->dot.direction == up){
                        world->drawables[0]->dot.direction = still;
                        break;
                    }
                    world->drawables[0]->dot.direction = down;
                    break;
                case 'f':
                    switch(player_fire){
                        case fire_shot:
                            world->drawables[0]->dot.state=1;
                            world->drawables[0]->dot.state_count=1;
                            shot_req.shot_type = fire_shot;
                            shot_req.fire_dmg = 1;
                            shot_req.x = world->drawables[0]->dot.x;
                            shot_req.y = world->drawables[0]->dot.y;
                            shot_req.active = 1;
                            shot_req.direction = 2;
                            break;
                        case laser_shot:
                            world->drawables[0]->dot.state=1;
                            world->drawables[0]->dot.state_count=1;
                            shot_req.shot_type = laser_shot;
                            shot_req.x = world->drawables[0]->dot.x;
                            shot_req.y = world->drawables[0]->dot.y;
                            shot_req.direction = 1;
                            shot_req.active = 1;
                            break;
                        case thunder_shot:
                            world->drawables[0]->dot.state=1;
                            world->drawables[0]->dot.state_count=1;
                            shot_req.shot_type = thunder_shot;
                            shot_req.x = world->drawables[0]->dot.x;
                            shot_req.y = world->drawables[0]->dot.y;
                            shot_req.direction = 10;
                            shot_req.fire_dmg = 1;
                            shot_req.active = 1;
                        default:
                            break;                   
                    }
                    break;
                case 'e':
                    create_enemy();
                    break;
                case 'x':
                    switch(levels.level){
                        case 1:
                            break;
                        case 2:
                        switch(player_fire){
                            case fire_shot:
                                player_fire = laser_shot;
                                break;
                            case laser_shot:
                                player_fire = fire_shot;
                                break;    
                            default:
                                break;                  
                        }  
                        break;
                    }
                    // switch(player_fire){
                    //     case fire_shot:
                    //         player_fire = laser_shot;
                    //         break;
                    //     case laser_shot:
                    //         player_fire = thunder_shot;
                    //         break;    
                    //     case thunder_shot:
                    //         player_fire = fire_shot;
                    //         break;   
                    //     default:
                    //         break;                  
                    // }  
            }

        } 
    }
    pthread_exit(NULL);
    return 0;
}

/* to synchronize the game-loop thread with the input-istener thread when the player should fire
* this function is necessary */ 
void check_requests(void){
    if(shot_req.active){
        switch(shot_req.shot_type){
            case fire_shot:
                create_fire_shot(shot_req.x,shot_req.y,shot_req.direction,shot_req.fire_dmg);
                break;
            case laser_shot:
                create_laser_shot(shot_req.x,shot_req.y, shot_req.direction);
                break;
            case thunder_shot:
                create_thunder_shot(shot_req.x,shot_req.y,shot_req.direction, shot_req.fire_dmg);
            default:
                break;
        }
        shot_req.active = 0;
    }
}

/* Called at regular intervals in the game-loop, it is the function passed as input argument to 
* the 'timer_loop' */ 
void world_refresh(){
    XClearWindow(display, window);
    check_requests();
    spawn_stuff();
    draw_world();
    draw_standard_txt();
    collision_check_world();
    move_world();
    collision_check_world();
    update_level();
}

/* allocated memory is being freed */
void destroy_world(){
    int size = world->size;
    for(int i=0; i<size;i++){
        free(world->drawables[i]);
    }
    free(world->drawables);
    free(world);
    printf("world freed\n");
}

/* checks the collision grids for a collision */
int check_collision_here(int x, int y){
    if(x<0 | x>width | y<0 | y>height)
        return 0;
    return collision_grid[x][y];
}

/* checks for collision with an aircraft that is located at a specified coordinate (x,y) */
int check_collision_aircraft(int x, int y){
    int collision = 0;
    for(int c = x-5; c<x+5; c++){
        for(int r = y-5; r<y+5; r++){
            collision = check_collision_here(c,r);
            if(collision == 1){
                return collision;
            } else {
                collision = 0;
            }
        }
    }
    return collision;
}

/* checks for collision with the player that is located at a specified coordinate (x,y) */
int check_collision_player(int x, int y){
    int collision = 0;
    for(int c = x+5; c>x-5; c--){
        for(int r = y-5; r<y+5; r++){
            collision = check_collision_here(c,r);
            if(collision == 2){
                return collision;
            } else {
                collision = 0;
            }
        }
    }
    return collision;
}

/* initilises the world_t instance of the game */
void initialise_world_1(void){
    if(is_initialised){
        return;
    }

    world = init_world(100000);

    lives = 5;

    player_fire = fire_shot;

    drawable_t * drawable = (drawable_t*) malloc(sizeof(drawable_t));
    /* one dot to start with */ 
    drawable->dot.direction = still;
    drawable->dot.x = 50;
    drawable->dot.y = 50;
    drawable->dot.side = 10;
    drawable->dot.state = 0;
    drawable->dot.state_count = 0;
    drawable->dot.nbr_of_states = 1;
    drawable->dot.draw_f = draw_dot_bulb;
    drawable->dot.move_f = move_dot;
    drawable->dot.collide_f = check_collision_player;
    drawable->type = dot;

    world->drawables[0] = drawable;
    world->size = 1;

    levels.level = 1;
    levels.sublevel = 0;
    levels.active = 0;
    levels.count = 0;
}

/* main entry, calls initialisation functions for the game logic and the and X window system */
int main(int argc, char **argv)
{    /* prepating the X window client for multi-threading */
    XInitThreads();
 
    /* open connection with the server */
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        exit(1);
    }
 
    s = DefaultScreen(display);
 
    /* create window */
    width = 300;
    height = 1000;

    window = XCreateSimpleWindow(display, RootWindow(display, s), 10, 10, height, width, 1,
                           BlackPixel(display, s), WhitePixel(display, s));
 
    /* select kind of events we are interested in */
    XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
 
    /* map (show) the window */
    XMapWindow(display, window);

   // screen_num = DefaultScreen(display);
    Colormap screen_colormap = DefaultColormap(display, DefaultScreen(display));

    XGCValues values;
    gc = XCreateGC(display,window, 0, &values);
    if(gc<0){
        printf("XCreateGC error\n");
    }

    int rc;
    rc = XAllocNamedColor(display, screen_colormap, "red", &red, &red);
    if (rc == 0) {
    fprintf(stderr, "XAllocNamedColor - failed to allocated 'red' color.\n");
    exit(1);
    }

    rc = XAllocNamedColor(display, screen_colormap, "blue", &blue, &blue);
    if (rc == 0) {
    fprintf(stderr, "XAllocNamedColor - failed to allocated 'blue' color.\n");
    exit(1);
    }

    rc = XAllocNamedColor(display, screen_colormap, "yellow", &yellow, &yellow);
    if (rc == 0) {
    fprintf(stderr, "XAllocNamedColor - failed to allocated 'yellow' color.\n");
    exit(1);
    }

/* initialising the world! */
    initialise_world_1();

/* Initialising threads */
    pthread_t thread_events, thread_timer;

    rc = pthread_create(&thread_events, NULL, event_loop, NULL);
    if(rc)
    {
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    rc = pthread_create(&thread_timer, NULL, timer_loop, world_refresh);
    if(rc)
    {
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    while(alive){
        continue;
    }

    /* close connection to server */
    destroy_world();
    close_x();
    return 0;
}

void draw_aircraft(int x, int y){
    XSetForeground(display, gc, BlackPixel(display,screen_num));
 //   XDrawArc(display,window,gc,x,y-2,5,5,0,64*360);
    XDrawLine(display,window, gc, x, y, x-5, y+5);
    XDrawLine(display,window, gc, x, y, x-5, y-5);
    XDrawLine(display,window, gc, x, y, x-5,y);
    XDrawLine(display,window, gc, x-5, y, x-10, y+10);
    XDrawLine(display,window, gc, x-5, y, x-10, y-10);
}

void draw_enemy_aircraft(int x, int y){
    XSetForeground(display, gc, blue.pixel);
    XDrawLine(display,window, gc, x, y, x+5, y-5);
    XDrawLine(display,window, gc, x, y, x+5, y+5);
    XDrawLine(display,window, gc, x, y, x+5,y);
    XDrawLine(display,window, gc, x+5, y, x+10, y-10);
    XDrawLine(display,window, gc, x+5, y, x+10, y+10);
    XFlush(display); 
}

void draw_enemy(void * dt){
    
    struct enemy_aircraft* db = (struct enemy_aircraft*) dt;
    XSetForeground(display, gc, BlackPixel(display,screen_num));
    
    switch(db->state){  
        case 0:
            XDrawArc(display, window, gc, db->x, db->y, 20, 20, db->substate*20*64,db->substate*20*64+20);
            XDrawArc(display, window, gc, db->x, db->y, 10, 10, db->substate*30*64,db->substate*30*64+20);
            break;
        case 1:
            draw_enemy_aircraft(db->x, db->y);
            break;
        case 2:
            draw_enemy_aircraft(db->x, db->y);
            XSetForeground(display, gc, red.pixel);
            if(db->fire_count == 1){
                XFillRectangle(display, window, gc, db->x-2, db->y, 1,1);
                XFillRectangle(display, window, gc, db->x-3, db->y, 1,1);
                XFillRectangle(display, window, gc, db->x-2, db->y+1, 1,1);
                XFillRectangle(display, window, gc, db->x-2, db->y-1, 1,1);
                db->fire_count = 0;
            } else if(db->fire_count == 0){
                db->state=1;
            }
            XFillRectangle(display, window, gc, db->x+2, db->y, 1,1);
            break;
        case 3:
            XDrawArc(display, window, gc, db->x, db->y, 20, 20, db->substate*20*64,db->substate*20*64+20);
            XDrawArc(display, window, gc, db->x, db->y, 10, 10, db->substate*30*64,db->substate*30*64+20);
        default:
            break;
    }
}

void draw_dot_bulb(void * dt){
    struct dot_bulb* db = (struct dot_bulb*) dt;
    int current_state = db->state;
    switch(current_state){
        case 0:
            draw_aircraft(db->x, db->y);
            break;
        case 1:
            draw_aircraft(db->x, db->y);
            XSetForeground(display, gc, red.pixel);
            if(db->state_count == 1){
                XFillRectangle(display, window, gc, db->x+2, db->y, 1,1);
                XFillRectangle(display, window, gc, db->x+3, db->y, 1,1);
                XFillRectangle(display, window, gc, db->x+2, db->y+1, 1,1);
                XFillRectangle(display, window, gc, db->x+2, db->y-1, 1,1);
                db->state_count = 0;
            } else if(db->state_count == 0){
                db->state=0;
            }
            XFillRectangle(display, window, gc, db->x+2, db->y, 1,1);
            break;
        case 2:
            XDrawArc(display, window, gc, db->x, db->y, 20, 20, db->substate*20*64,db->substate*20*64+20);
            XDrawArc(display, window, gc, db->x, db->y, 10, 10, db->substate*30*64,db->substate*30*64+20);
        default:
            break;
    }
}

void create_enemy(void){

    drawable_t * drawable = (drawable_t*) malloc(sizeof(drawable_t));
    /* one dot to start with */ 
    drawable->enemy.direction = still;
    drawable->enemy.x = width-30 + rand()%15;
    drawable->enemy.y = rand()%height;
    drawable->enemy.state = 0;
    drawable->enemy.substate = 0;
    drawable->enemy.fire_count = 0;
    drawable->enemy.draw_f = draw_enemy;
    drawable->enemy.collide_f = check_collision_aircraft;
    drawable->enemy.move_f = move_enemy;
    drawable->type = enemy;

    world->drawables[world->size] = drawable;
    world->size++;
}

void erase_drawable(int id){
    if(id>=world->max_nbr_of_objects){
        return;
    } else if(id == world->max_nbr_of_objects-1){
        free(world->drawables[id]);
        world->size--;
        world->drawables[id] = NULL;
    } else {
        free(world->drawables[id]);
        world->drawables[id] = NULL;
        for(int i=id;i<world->max_nbr_of_objects-2;i++){
            world->drawables[i]=world->drawables[i+1];
        }
        world->drawables[world->max_nbr_of_objects-1] = NULL;
        world->size--;
    }

}

void draw_fire_shot(void * dt){
    struct shot_bulb* sb = (struct shot_bulb*) dt;
    XSetForeground(display, gc, red.pixel);
    XFillArc(display, window, gc, sb->x, sb->y, 4, 4, 0,64*360);
    XSetForeground(display, gc, BlackPixel(display,screen_num));
}

int thunder_mod = 12;

void draw_thunder(struct thunder_shot* ts, int x, int y, int direction){

    if(x >= width | x < 0 | y < 0 | y >= height) return;

    for(int xx = x; xx<x+direction;xx++){
        ts->thunder_grid[xx][y] = 1;
        collision_grid[xx][y] = ts->dmg;
    }
    int r = rand()%thunder_mod;
    switch(r){
        case 0:
            draw_thunder(ts,x+direction,y+2,direction);
            draw_thunder(ts,x+direction,y-2,direction);
            break;
        case 1:
            draw_thunder(ts,x+direction,y+5,direction);
            break;
        case 2:
            draw_thunder(ts,x+direction,y-5,direction);
            break;
        case 3:
            draw_thunder(ts,x+direction,y+7,direction);
            break;
        case 4:
            draw_thunder(ts,x+direction,y-7,direction);
            break;
        case 5:
            draw_thunder(ts,x+direction,y+10,direction);
            break;
        case 6:
            draw_thunder(ts,x+direction,y-10,direction);
            break; 
        default:
            draw_thunder(ts,x+direction,y,direction);
    }


}

void init_thunder_grid(struct thunder_shot* ts){
    for(int x = 0; x<maxwidth; x++){
        for(int y = 0; y<maxheight; y++){
            ts->thunder_grid[x][y] = 0;
        }
    }
    draw_thunder(ts,ts->xstart, ts->ystart, ts->direction);
}

void draw_thunder_shot(void * vb){
    struct thunder_shot * ts = (struct thunder_shot*) vb;

    if(ts->state == 5) init_thunder_grid(ts);

    XSetForeground(display, gc, yellow.pixel);

    for(int x = 0; x<width; x++){
        for(int y = 0; y<height; y++){
            if(ts->thunder_grid[x][y]) {
                XFillRectangle(display, window, gc, x, y, 2,2);
            }
        }
    }

    XSetForeground(display, gc, BlackPixel(display,screen_num));
}

void draw_laser_shot(void * dt){

    struct laser_shot* ls = (struct laser_shot*) dt;
    XSetForeground(display, gc, blue.pixel);
    if(ls->direction == 1){
        for(int n = ls->xstart; n<=width; n++){
            XFillRectangle(display, window, gc, n, ls->y, 2,2);
        }
    } else if(ls->direction == -1){
        for(int n = ls->xstart; n>=0; n--){
            XFillRectangle(display, window, gc, n, ls->y, 2,2);
        }
    }
    XSetForeground(display, gc, BlackPixel(display,screen_num));

}

void draw_star(void * dt){
    struct star_bulb* sb = (struct star_bulb*) dt;
    XSetForeground(display, gc, BlackPixel(display,screen_num));
    XDrawLine(display,window, gc, sb->x, sb->y-sb->side, sb->x-1, sb->y-1);
    XDrawLine(display,window, gc, sb->x-1, sb->y-1, sb->x-sb->side, sb->y);
    XDrawLine(display,window, gc, sb->x-sb->side, sb->y, sb->x-1, sb->y+1);
    XDrawLine(display,window, gc, sb->x-1, sb->y+1, sb->x, sb->y+sb->side);
    XDrawLine(display,window, gc, sb->x, sb->y+sb->side, sb->x+1, sb->y+1);
    XDrawLine(display,window, gc, sb->x+1, sb->y+1, sb->x+sb->side, sb->y);
    XDrawLine(display,window, gc, sb->x+sb->side, sb->y, sb->x+1, sb->y-1);
    XDrawLine(display,window, gc, sb->x+1, sb->y-1, sb->x, sb->y-sb->side);
}

void draw_moon(void * dm){
    struct moon_bulb * mb = (struct moon_bulb*) dm;
    XSetForeground(display, gc, BlackPixel(display,screen_num));
    XDrawArc(display,window,gc,mb->x,mb->y,mb->side,mb->side,0,64*360);
}

void move_world(void){

    for(int i = 0;i<world->size;i++){
    switch(world->drawables[i]->type){
        case monster:
            world->drawables[i]->monster.draw_f(&world->drawables[i]->monster);
            break;
        case dot:
            world->drawables[i]->dot.move_f(&world->drawables[i]->dot);
            break;
        case shot:
            if(world->drawables[i]->shot.move_f(&world->drawables[i]->shot))
                erase_drawable(i);
            break;
        case star:
            if(world->drawables[i]->star.move_f(&world->drawables[i]->star)){
                erase_drawable(i);
            }
            break;
        case enemy:
            if(world->drawables[i]->enemy.move_f(&world->drawables[i]->enemy))
                erase_drawable(i);
            break;
        case laser:
            if(world->drawables[i]->laser.move_f(&world->drawables[i]->laser))
                erase_drawable(i);
            break;
        case thunder:
            if(world->drawables[i]->thunder.move_f(&world->drawables[i]->thunder))
                erase_drawable(i);
            break;
        case moon:
            if(world->drawables[i]->moon.move_f(&world->drawables[i]->moon))
                erase_drawable(i);
            break;
        }
     }
}

int move_star(void * sb){
    struct star_bulb * st = (struct star_bulb *) sb;
    if(st->x <= 0){
        return 1;
    }
    st->x -= st->speed;
    return 0;
}

int move_moon(void * sb){
    struct moon_bulb * st = (struct moon_bulb *) sb;
    if(st->x <= 0){
        return 1;
    }
    st->x -= st->speed;
    return 0;
}

int move_shot(void * sb){
    struct shot_bulb * st = (struct shot_bulb *) sb;

    if(st->x >= width | st->x <= 0)
        return 1;

    collision_grid[st->x][st->y] = 0; 
    st->x+=st->direction;
    if(st->x >= 0 && st->x <= width)
        collision_grid[st->x][st->y] = st->fire_dmg;

    return 0;
}

int move_thunder(void * vb){
    struct thunder_shot * tb = (struct thunder_shot*) vb;

    if(tb->state == 0){

        for(int x = 0; x<maxwidth; x++){
            for(int y = 0; y<maxheight;y++){
                if(tb->thunder_grid[x][y]) collision_grid[x][y] = 0;
            }
        }

        free_thunder_shot_grid(tb);
        return 1;
    }

    tb->state--;
    return 0;
}

int move_laser(void * lb){
    struct laser_shot * ls = (struct laser_shot*) lb;


    if(ls->direction == 1){
        for(int x = ls->xstart; x<=width; x++){
            collision_grid[x][ls->y] = 0;
        }

        ls->xstart += 30;

        if(ls->xstart >= width) return 1; 

        for(int x = ls->xstart; x<=width; x++){
            collision_grid[x][ls->y] = 1;
        }
    } else if(ls->direction){
        for(int x = ls->xstart; x>=0; x--){
            collision_grid[x][ls->y] = 0;
        }

        ls->xstart -= 30;

        if(ls->xstart <= 0) return 1; 

        for(int x = ls->xstart; x>=0; x--){
            collision_grid[x][ls->y] = 1;
        }
    }

    return 0;
}

int move_enemy(void * eb){

    struct enemy_aircraft * ea = (struct enemy_aircraft *) eb;

    static int enemy_move_count = 0;

    switch(ea->state){
        case 0:
            if(ea->substate == 10){
                ea->state = 1;
            ea->substate = 0;
            } else {
                ea->substate += 1;
            }
            break;
        case 1:
            if(enemy_move_count == 100){
                enemy_move_count = 0;
            } else {
                if(!(enemy_move_count % 10))
                    ea->direction = rand()%3;
                int shoot = rand() % basic_enemy_shoot_freq;
                if(!shoot){
                    ea->state=2;
                    ea->fire_count=1;
                    create_fire_shot(ea->x,ea->y,-2,2);
                }
                enemy_move_count++;
            }
            switch(ea->direction){
                case 0:
                    break;
                case 1:
                    ea->y = (height+ea->y - 1)%height;
                    break;
                case 2:
                    ea->y = (ea->y + 1)%height; 
                    break;
                default:
                    break;
            }
            ea->x -= 1;
            if(ea->x<=0){
                escaped++;
                return 1;
            }
            break;
        case 3:
            if(ea->substate == 10){
                ea->substate = 0;
                killed++;
                return 1;
            } else {
                ea->substate += 1;
            }
            break;
        default:
            break; 
    }
    return 0;
}

int move_dot(void * sb){
    struct dot_bulb * dot= (struct dot_bulb*) sb;
    switch(dot->direction){
        case still:
            break;
        case up:
            dot->y = (height+dot->y - 2)%height;
            break;
        case down:
            dot->y = (dot->y + 2)%height; 
            break;
        case left:
            dot->x = (width+dot->x - 1)%width; 
            break; 
        case right:
            dot->x = (dot->x + 1)%width;  
            break;
        default:
            break;
    }
    if(dot->state == 2){
        dot->substate+=1;
    }
    if(dot->substate==20){
        dot->state=0;
        dot->substate=0;
        lives--;
    }
    return 0;
}

void update_level(){

}

/* returns system resources to the system */
void close_x() {
    XFreeColormap(display,colormap);
    XDestroyWindow(display,window);
    XCloseDisplay(display); 
    exit(1);       
    printf("close call\n");         
}
