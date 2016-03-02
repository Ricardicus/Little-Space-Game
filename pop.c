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
    collision_callback collide_f;
    move_callback move_f;
};

struct shot_bulb {
    int x;
    int y;
    int direction;
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
    star,
    enemy
} draw_object_type_t;

typedef struct drawable_u {
    draw_object_type_t type;
    struct dot_bulb dot;
    struct monster_bulb monster;
    struct shot_bulb shot;
    struct star_bulb star;
    struct enemy_aircraft enemy;
} drawable_t;

typedef struct world_u {
    int size;
    int max_nbr_of_objects;
    drawable_t ** drawables;
} world_t;

/* Function declarations */
void draw_dot_bulb(void*);
void erase_dot_bulb(void*);
void draw_fire_shot(void*);
void draw_enemy(void*);
void draw_star(void*);
void draw_aircraft(int,int);
void draw_enemy_aircraft(int,int);
int move_star(void*);
int move_shot(void*);
int move_dot(void*);
int move_enemy(void*);
void create_fire_shot(int,int,int);
int check_collision_aircraft(int,int);
void erase_drawable(int id);
void create_enemy(void);
world_t* init_world(int);
void initialise_world_1(void);
void destroy_world(void);
static void* event_loop(void*);
static void* timer_loop(void*);
void close_x(void);
void move_world(void);

/* 
*   Window variables
*/

static XColor red,blue,white;
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

/* Application related variables */
int alive=1;
int is_initialised=0;
int maxheight = 2000;
int maxwidth = 2000;
char collision_grid[2000][2000];

/* the world */  
world_t * world;

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

    return wd;
}

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

void create_fire_shot(int x, int y, int direction){
    int i = world->size;
    if(i >= world->max_nbr_of_objects){
        return;
    }
    drawable_t * drawable = (drawable_t*) malloc(sizeof(drawable_t));
    /* one dot to start with */ 
    drawable->shot.direction = direction;
    drawable->shot.x = x;
    drawable->shot.y = y;
    drawable->shot.draw_f = draw_fire_shot;
    drawable->shot.move_f = move_shot;
    drawable->type = shot;

    world->drawables[i] = drawable;
    world->size++;
}

void collision_check_world(){
    for(int i = 0;i<world->size;i++){
        switch(world->drawables[i]->type){
            case enemy:
                if(world->drawables[i]->enemy.collide_f(world->drawables[i]->enemy.x,world->drawables[i]->enemy.y)){
                    world->drawables[i]->enemy.state = 3;
                    world->drawables[i]->enemy.substate = 0;
                }
                break;
            default:
                break;
        }
    }
}

char units_killed_message[256];
int len;

void draw_world(){
    int r = rand()%20;
    if(r == 14){
        create_star();
    }
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
            default:
                break;
        }
    }

    sprintf(units_killed_message,"Units killed: %d", killed);
    len = strlen(units_killed_message);

    XDrawString(display, window, DefaultGC(display, s), width - len-100, height- 20, units_killed_message, len);

    XFlush(display);
}

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

        /* exit on key press */
//        else if (event.type == ButtonPress){

//            XDrawString(display, window, DefaultGC(display, s), event.xbutton.x,event.xbutton.y, "Click", 5);
//        }


        else if (event.type == KeyPress && XLookupString(&event.xkey,text,255,&key,0)==1) {
        /* the XLookupString routine converts the invent
           KeyPress data into regular text.  Weird but necessary.
        */

            switch(text[0]){
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
                    world->drawables[0]->dot.state=1;
                    world->drawables[0]->dot.state_count=1;
                    create_fire_shot(world->drawables[0]->dot.x,world->drawables[0]->dot.y,2);
                    break;
                case 'e':
                    create_enemy();
            }

        } 
    }
    pthread_exit(NULL);
    return 0;
}

void world_refresh(){
    XClearWindow(display, window);
    draw_world();
    collision_check_world();
    move_world();
    collision_check_world();
}

void destroy_world(){
    int size = world->size;
    for(int i=0; i<size;i++){
        free(world->drawables[i]);
    }
    free(world->drawables);
    free(world);
    printf("world freed\n");
}

int check_collision_here(int x, int y){
    if(x<0 | x>width | y<0 | y>height)
        return 0;
    return collision_grid[x][y];
}

int check_collision_aircraft(int x, int y){
    int collision = 0;
    for(int c = x+2; c<x+5; c++){
        for(int r = y-5; r<y+5; r++){
            collision+=check_collision_here(c,r);
        }
    }
    return collision;
}

void initialise_world_1(void){
    if(is_initialised){
        return;
    }

    world = init_world(100000);

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
    drawable->type = dot;

    world->drawables[0] = drawable;
    world->size = 1;
}



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
    width = 500;
    height = 500;

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
    fprintf(stderr, "XAllocNamedColor - failed to allocated 'red' color.\n");
    exit(1);
    }

/* initialising the world! */
    initialise_world_1();

/* Initialising threads */
    pthread_t thread_events_1, thread_events_2, thread_timer;

    rc = pthread_create(&thread_events_1, NULL, event_loop,NULL);
    if(rc)
    {
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    rc = pthread_create(&thread_events_2, NULL, event_loop,NULL);
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
    XFillRectangle(display, window, gc, sb->x, sb->y, 1,1);
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

int move_shot(void * sb){
    struct shot_bulb * st = (struct shot_bulb *) sb;

    if(st->x >= width | st->x <= 0)
        return 1;
    if(st->x >= 0){
        collision_grid[st->x][st->y] = 0; 
        st->x+=st->direction;
        if(st->x >= 0)
            collision_grid[st->x][st->y] = 1;
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
                int shoot = rand()%40;
                if(!shoot){
                    ea->state=2;
                    ea->fire_count=1;
                    create_fire_shot(ea->x,ea->y,-2);
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
            dot->y = (height+dot->y - 1)%height;
            break;
        case down:
            dot->y = (dot->y + 1)%height; 
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
    return 0;
}

/* returns system resources to the system */
void close_x() {
    XFreeColormap(display,colormap);
    XDestroyWindow(display,window);
    XCloseDisplay(display); 
    exit(1);       
    printf("close call\n");         
}
