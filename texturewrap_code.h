/*
Copyright (C) 2022 BrerDawg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


//texturewrap_code.h

//v1.01


#ifndef texture_wrap_code_h
#define texture_wrap_code_h



#define _FILE_OFFSET_BITS 64			//large file handling, must be before all #include...
//#define _LARGE_FILES

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string>
#include <vector>
#include <wchar.h>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <cassert>

#include "GCProfile.h"

//linux code
#ifndef compile_for_windows

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
//#include <X11/Xaw/Form.h>							//64bit
//#include <X11/Xaw/Command.h>						//64bit

#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>		//MakeIniPathFilename(..) needs this
#endif


//windows code
#ifdef compile_for_windows
#include <windows.h>
#include <process.h>
#include <winnls.h>
#include <share.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <conio.h>

#define WC_ERR_INVALID_CHARS 0x0080		//missing from gcc's winnls.h
#endif


using namespace std;





//NOTE '+y' is up screen, '+z' is into screen (away from viewer) 
struct st_texture_wrap_pixel_block_3d_tag
{
unsigned char* bf;					//pixel data block in 8 bit r, g, b ordering (can be larger than block to be manipulated), this is the texture to be wrapped
int srcx;							//left most pixel offset in 'bf[]' to work from, this value will be multiplied by 3 to step over r,g,b groupings
int srcy;							//top most pixel offset in 'bf[]' to work from
int linedx;							//this defines how many pixels exist in a whole horiz line held in 'bf[]'

int wid;							//pixel size of block to be manipulated
int hei;

unsigned char* bfdest;				//pixel data block in 8 bit r, g, b ordering to receive manipulated pixels, can be same as 'bf[]'
int dest_midx, dest_midy;			//center of bfdest[] pixel block
int destx;							//left most pixel in 'bfdest[]' to receive manipulated pixels
int desty;
int destlinedx;						//this defines how many pixels exist in a whole horiz line held in 'bfdest[]'

float origin3d_x;					//this is the 3d surface the pixel map is textured with, e.g. offsets the rotation point, plus 'origin3d_y' is upscreen 
float origin3d_y;
float origin3d_z;

float scale;						//final display scaling, adj this inconjuction with 'offset_z', to change perpective distortion (try 400.0f)

bool bflip_vert;
};






//NOTE '+y' is up screen, '+z' is into screen (away from viewer) 
struct st_3d_pixel_block_texture_wrap_single_axis_tag
{
unsigned char* bf;					//pixel data block in 8 bit r, g, b ordering (can be larger than block to be manipulated), this is the texture to be wrapped
int srcx;							//left most pixel offset in 'bf[]' to work from, this value will be multiplied by 3 to step over r,g,b groupings
int srcy;							//top most pixel offset in 'bf[]' to work from
int linedx;							//this defines how many pixels exist in a whole horiz line held in 'bf[]'

int wid;							//pixel size of block to be manipulated
int hei;

unsigned char* bfdest;				//pixel data block in 8 bit r, g, b ordering to receive manipulated pixels, can be same as 'bf[]'
int destx;							//left most pixel in 'bfdest[]' to receive manipulated pixels
int desty;
int destlinedx;						//this defines how many pixels exist in a whole horiz line held in 'bfdest[]'

int rotation_axis;					//0: rot. around 'x',   1: rot. around 'y'   2: rot. around 'z'
float origin3d_x;					//this is the 3d surface the pixel map is textured with, e.g. offsets the rotation point, plus 'origin3d_y' is upscreen 
float origin3d_y;
float origin3d_z;
float theta;						//3d rotation angle about: 'origin3d_x' etc

float offset_z;						//moves rotated 3d surface's  'z' position, adj this inconjuction with 'scale', to alter perpective distortion (try 400.0f)
float scale;						//final display scaling, adj this inconjuction with 'offset_z', to change perpective distortion (try 400.0f)

bool bflip_vert;
};








class cl_texture_wrap
{
private:										//private var


public:
int bytes_per_pixel;


public:
cl_texture_wrap();

void line_plot_in_buf( unsigned char *bf, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int linedx );
void get_pixel( unsigned char *bf, int xx, int yy, int &rr, int &gg, int &bb, int linedx );
void set_pixel( unsigned char *bf, int xx, int yy, int rr, int gg, int bb, int linedx );
void copy_pixel_block( st_3d_pixel_block_texture_wrap_single_axis_tag ot );

void draw_texture_rot3d_single_axis( st_3d_pixel_block_texture_wrap_single_axis_tag ot, float sub_pixel_factor, bool plot_raster_with_fl_points );


};








#endif
