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

//flip_leaf_clock.h

//v1.01


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string>
//#include <vector>
#include <wchar.h>
#include <algorithm>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#include <getopt.h>

#include "GCProfile.h"
#include "texturewrap_code.h"


#define pi (M_PI)
#define pi2 (2.0f*M_PI)
#define twopi  (2.0f*M_PI) 
#define cn_rad2deg (180.0f / M_PI)
#define cn_deg2rad ( M_PI / 180 )

//#define cn_draw_suface_maxx (1920 + 20)
//#define cn_draw_suface_maxy (1080 + 20)
#define cn_draw_suface_maxx (5120 + 20)
#define cn_draw_suface_maxy (2880 + 20)


#define cn_bytes_per_pixel_max 4			//only 3 bytes per pixel are actually used
#define cn_fc_render_max_wid 256
#define cn_fc_render_max_hei 256


using namespace std;




enum en_filter_window_type_tag
{
fwt_undefined,
fwt_rect,
fwt_kaiser,                     //not yet supported
fwt_bartlett,
fwt_hann,
fwt_bartlett_hanning,
fwt_hamming,
//fwt_bohman,                   //not yet supported
fwt_blackman,
fwt_blackman_harris,
};




class flip_clock_wnd
{
private:
mystr m1_timer;


public:
int midx;
int midy;
cl_texture_wrap tw;
float theta_hrs;
float theta_mins;
float theta_secs;

bool first_tick = 1;
float flip_speed;

bool b_flip_hrs;
bool b_flip_mins;
bool b_flip_secs;
int last_secs = 0;
int last_mins = 0;
int last_hrs = 0;

int ihrs, imins, isecs;
int prev_secs;
int prev_mins;
int prev_hrs;

bool b_show_secs;
bool b_24hr;

float last_theta_hrs;
float last_theta_mins;
float last_theta_secs;
float last_redraw_time;

bool need_full_redraw;
bool need_redraw;
bool b_preview;

int bytes_per_pixel;
en_filter_window_type_tag filter_type;
unsigned char ucdigit[6][ cn_fc_render_max_wid * cn_fc_render_max_hei * cn_bytes_per_pixel_max ];
int kern_size0;															//odd gives a 1.0 peak
float kern00[ 256 ];

int drw_filtered_x, drw_filtered_y;


public:
flip_clock_wnd();
~flip_clock_wnd();

uint32_t make_color( unsigned int rr, unsigned int gg, unsigned int bb );
void set_color( int rr, int gg, int bb );
void line_style( int line_width, int line_style, int cap_style, int join_style );
void draw_line( int x0, int y0, int x1, int y1 );
void draw_rectf( int xx, int yy, int ww, int hh );
void draw_rectf( unsigned char *bf, int xx, int yy, int ww, int hh, int linedx, unsigned char rr, unsigned char gg, unsigned char bb );
void copy_area( unsigned char *bfsrc, int srcx, int srcy, int srcwid, int srchei, int linedx, unsigned char *bfdest, int destx, int desty, int destlinedx );
void get_pixel( unsigned char *bf, int xx, int yy, int &rr, int &gg, int &bb, int linedx );
void set_pixel( unsigned char *bf, int xx, int yy, int rr, int gg, int bb, int linedx );
void line_plot_in_buf( unsigned char *bf, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int linedx );
void arc_plot_in_buf( unsigned char *bf, int centrex, int centrey, int radius, float theta_start, float theta_end, float theta_step, int rr, int gg, int bb, int linedx );

void draw_text_line_black( int px, int py, int rectww, int recthh, int text_offx, int text_offy, int line_black_offx, int line_black_offy, XftFont *text_font, string stext, int line_width, int rr, int gg, int bb );
void draw_text( int px, int py, string stext, XftFont *text_font, int rr, int gg, int bb, int align );
void render_text();
void render( unsigned char *bf, int wid, int hei, int destx, int desty );
float tick();
void filter_scale_block( unsigned char *bfsrc, int srcx_in, int srcy_in, int wid, int hei, int linedx, unsigned char *bfdest, int destx, int desty, int destlinedx, float scale_in, int &rendered_wid, int &rendered_hei );
bool filter_kernel_float( en_filter_window_type_tag wnd_type, float *w, int N );
void set_flip_speed( float grav );

};
