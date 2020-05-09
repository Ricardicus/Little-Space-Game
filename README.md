# Little-Space-Game
A little game in Xlib :) 
# Want to run it?
Unfortunately it's not super portable.This will probably only run on a UNIX machine. 
If you are on a mac computer, please download XQuartz. Then you compile the program by writing "make" in the terminal in the right directory. Run it by hitting the executable "./pop". 
I have tried to make this work on Ubuntu but I ran into some problems. For sure though, this runs on any Mac with XQuartz installed.
Download XQuartz here: http://www.xquartz.org/

# Build on OSX 

```
# Explictly set some variables, to point
# to the X11 installation
make X11INC=/usr/X11/include X11LIB=/usr/X11/lib
```

# Build on Ubutnu

```
# No need to set any variables
make
```

# Build on Windows

Create an issue, and I will give it a go!

# Start

```
./pop
```

# Gameplay

One can play with keys on the keyboard.
Enemies spawn like this:

<img src="https://raw.githubusercontent.com/Ricardicus/Little-Space-Game/master/snapshot/SS1.png"></img>

Then they spawn more randomly like this:

<img src="https://raw.githubusercontent.com/Ricardicus/Little-Space-Game/master/snapshot/SS2.png"></img>

You unlock new weapons as you survive and time passes by.
