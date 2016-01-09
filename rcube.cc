// rcube.cc
// Copyright (C) 2003 Matthias Kestenholz, mk@webinterface.ch
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// http://www.gnu.org/licenses/gpl.html
//

#include "SDL.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cfloat>

#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))
#define abs(a) (((a)<0)?-(a):(a))
#define sign(a) (((a)<0)?-1:(a)>0?1:0)

static const int width=600;
static const int height=600;

// Graphics stuff

static void
line16(SDL_Surface * s, int x1, int y1, int x2, int y2, Uint32 color)
{
	int d;
	int x;
	int y;
	int ax;
	int ay;
	int sx;
	int sy;
	int dx;
	int dy;

	Uint8 *lineAddr;
	Sint32 yOffset;

	dx = x2 - x1;
	ax = abs(dx) << 1;
	sx = sign(dx);

	dy = y2 - y1;
	ay = abs(dy) << 1;
	sy = sign(dy);
	yOffset = sy * s->pitch;

	x = x1;
	y = y1;

	lineAddr = ((Uint8 *) s->pixels) + (y * s->pitch);
	if (ax > ay) {		/* x dominant */
		d = ay - (ax >> 1);
		for (;;) {
			*((Uint16 *) (lineAddr + (x << 1))) = (Uint16) color;

			if (x == x2) {
				return;
			}
			if (d >= 0) {
				y += sy;
				lineAddr += yOffset;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	} else {		/* y dominant */
		d = ax - (ay >> 1);
		for (;;) {
			*((Uint16 *) (lineAddr + (x << 1))) = (Uint16) color;

			if (y == y2) {
				return;
			}
			if (d >= 0) {
				x += sx;
				d -= ay;
			}
			y += sy;
			lineAddr += yOffset;
			d += ax;
		}
	}
}

// Hypercube stuff

// a location in the 4dimensional Space
struct V3 {
	union {
		struct {
			double x,y,z;
		};
		double d[3];
	};

	V3() {}
	V3(double x, double y, double z) {
		this->x=x;
		this->y=y;
		this->z=z;
	}

	~V3() {}
};

// screen coordinate
struct S2 {
	int x,y;

	S2() {}
	S2(int x, int y) {
		this->x=x;
		this->y=y;
	}
};

void init_points(V3 *points)
{
	points[ 0]=V3( 1., -1.,  1.);
	points[ 1]=V3( 1.,  1.,  1.);
	points[ 2]=V3(-1.,  1.,  1.);
	points[ 3]=V3(-1., -1.,  1.);

	points[ 4]=V3( 1., -1., -1.);
	points[ 5]=V3( 1.,  1., -1.);
	points[ 6]=V3(-1.,  1., -1.);
	points[ 7]=V3(-1., -1., -1.);
}

// init_lines builds a table of indexes for the two endpoints of the line
// (the indices refer to the vectors in the points[] array)
void init_lines(int *lines)
{
	lines[ 0]=0;
	lines[ 1]=1;
	lines[ 2]=1;
	lines[ 3]=2;
	lines[ 4]=2;
	lines[ 5]=3;
	lines[ 6]=3;
	lines[ 7]=0;

	for(int i=0; i<8; i++) {
		lines[i+8]=lines[i]+4;
	}

	for(int i=0; i<4; i++) {
		lines[(i<<1)+16]=i;
		lines[(i<<1)+17]=i+4;
	}
}

void init_angles(double *angles)
{
	for(int i=0; i<3; i++)
		angles[i]=0;
}

// linear algebra...
inline void rotateaxis(V3 *v, int from, int to, double angle)
{
	double c=cos(angle);
	double s=sin(angle);

	double tmp=v->d[from];
	v->d[from]=c*tmp-s*v->d[to];
	v->d[to]=s*tmp+c*v->d[to];
}

void rotate(V3 *v, double *angles)
{
	rotateaxis(v, 0, 1, angles[0]);
	rotateaxis(v, 0, 2, angles[1]);
	rotateaxis(v, 1, 2, angles[2]);
}

void project(V3 *v, double eyeZ)
{
	double k3=(eyeZ+v->z)/eyeZ;
	v->x*=k3;
	v->y*=k3;
}

// ... transformation to screen coordinates
S2 transform(V3 v)
{
	return S2((int)((width>>1)+v.x*(width/6.5)),
		  (int)((height>>1)+v.y*(height/6.5)));
}

SDL_Surface *screen = NULL;
Uint32 black, red, green, blue;
V3 points[8];
S2 spoints[8];
int lines[24];
double angles[3];
double eyeZ=4;
SDL_Rect blah;

#define TICK_INTERVAL    30

Uint32 TimeLeft(void)
{
    static Uint32 next_time = 0;
    Uint32 now;

    now = SDL_GetTicks();
    if ( next_time <= now ) {
        next_time = now+TICK_INTERVAL;
        return(0);
    }
    return(next_time-now);
}

void do_it()
{
	angles[0]+=0.07;
	angles[1]-=0.04;
	angles[2]+=0.01;
	init_points(points);

	for(int i=0; i<8; i++) {
		rotate(&points[i], angles);
		project(&points[i], eyeZ);
		spoints[i]=transform(points[i]);
	}

	SDL_FillRect(screen, &blah, black);

	// use different colors for different parts of the cube:
	for(int i=0; i<4; i++) {
		int l1=lines[i*2];
		int l2=lines[i*2+1];
		line16(screen,
		     spoints[l1].x, spoints[l1].y,
		     spoints[l2].x, spoints[l2].y,
		     red);
	}

	for(int i=4; i<8; i++) {
		int l1=lines[i*2];
		int l2=lines[i*2+1];
		line16(screen,
		     spoints[l1].x, spoints[l1].y,
		     spoints[l2].x, spoints[l2].y,
		     green);
	}

	for(int i=8; i<12; i++) {
		int l1=lines[i*2];
		int l2=lines[i*2+1];
		line16(screen,
		     spoints[l1].x, spoints[l1].y,
		     spoints[l2].x, spoints[l2].y,
		     blue);
	}

	// Update screen
	SDL_UpdateRect(screen, 0, 0, width, height);
}

int main(int argc, char *argv[])
{
	// Graphics init
	if(SDL_Init(SDL_INIT_VIDEO)==-1) {
		printf("Everything is fucked\n");
		exit(1);
	}

	atexit(SDL_Quit);

	if((screen=SDL_SetVideoMode(width, height, 16, 0))==NULL) {
		printf("Everything is fucked #2\n");
		exit(1);
	}

	// Allocate Colors
	black=SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	red=SDL_MapRGB(screen->format, 0xff, 0x00, 0x00);
	green=SDL_MapRGB(screen->format, 0x00, 0xff, 0x00);
	blue=SDL_MapRGB(screen->format, 0x00, 0x00, 0xff);

	// Keyboard
	SDL_EnableKeyRepeat(200, 50);


	// Hypercube init:
	init_angles(angles);
	init_lines(lines);

	// Initialize blah (used to clear the screen)
	blah.x=0;
	blah.y=0;
	blah.w=width;
	blah.h=height;

	while(true) {
		do_it();
		SDL_Delay(TimeLeft());
		SDL_Event event;
		SDL_PollEvent(&event);

		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE) {
			SDL_Quit();
			exit(0);
		} else if(event.type==SDL_QUIT) {
			SDL_Quit();
			exit(0);
		}
	}

	return 0;
}

