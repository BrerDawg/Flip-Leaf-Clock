/*
Copyright (C) 2022 BrerDawg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


//flip_leaf_clock.cpp

//v1.01   2022-jul-16		//x11 screensaver
//v1.0?   20??				//added: ?

#include "flip_leaf_clock.h"


#define FALSE 0
#define TRUE 1
#define BLACK 0x000000
#define WHITE 0xFFFFFF
#define RED 0xff0000
#define GREEN 0x00ff00
#define BLUE 0x0000ff


string s_m1_dbg;						//used to write a debug file via 'm1_dbg'
mystr m1_dbg;


Window root;
Display *dpy;
XWindowAttributes gwa;
GC gc;
	
XftColor xftcolor;
XftFont *font25;
XftFont *font150;
XftDraw *xft_draw;
Pixmap pixbf0;
Pixmap pixbf1;
XImage* xshm_img0;
XImage* xshm_img1;

Visual *visual;
Colormap cmap;
XShmSegmentInfo shminfo0;
XShmSegmentInfo shminfo1;

flip_clock_wnd fc;

//user opts
bool b_app_mode = 0;
bool b_debug = 0;
float wander_period = 30.0f;				//reduces screen burn  0-->100
float factor_scale = 0.5f;
bool b_24hr = 0;							//military display
float gravity = 50;							//flip speed 2-->100
bool b_show_secs = 0;




string sdebug;



void append_dbg( string ss )
{
s_m1_dbg += ss;
m1_dbg = s_m1_dbg;
//m1_dbg.writefile( "/home/gc/zzzscnsvr_dbg.txt" );				//un-comment to dump to a txt file
}
















uint32_t make_color( unsigned char rr, unsigned char gg, unsigned char bb)
{
uint32_t col = rr;
col = col << 8;

col |= gg;
col = col << 8;

col |= bb;

return col;
}








flip_clock_wnd::flip_clock_wnd()
{
bytes_per_pixel = 3;						//set this so a minimum, will be changed once x11 is polled
filter_type = fwt_blackman_harris;
kern_size0 = 15;

theta_hrs = pi;
theta_mins = pi;
theta_secs = pi;
ihrs = 0;
imins = 0;
isecs = 57;

prev_secs = 0;
prev_mins = 0;
prev_hrs = 0;

b_24hr = 0;
b_show_secs = 0;


first_tick = 1;
last_secs = 0;
last_mins = 0;
last_hrs = 0;

last_theta_hrs = 0;
last_theta_mins = 0;
last_theta_secs = 0;
last_redraw_time = 0;

set_flip_speed( gravity );

m1_timer.time_start( m1_timer.ns_tim_start );

b_preview = 0;
}






flip_clock_wnd::~flip_clock_wnd()
{
}






void flip_clock_wnd::set_flip_speed( float grav )
{
if( grav < 2.0f ) grav = 2.0f;
if( grav > 100.0f ) grav = 100.0f;

flip_speed = 5.0f * grav / 100.0f;
}



uint32_t flip_clock_wnd::make_color( unsigned int rr, unsigned int gg, unsigned int bb ) 
{
uint32_t col = rr;
col = col << 8;

col |= gg;
col = col << 8;

col |= bb;

return col;
}








void flip_clock_wnd::set_color( int rr, int gg, int bb )
{
XSetForeground( dpy, gc, make_color( rr, gg, bb ) );
}





void flip_clock_wnd::draw_rectf( int xx, int yy, int ww, int hh )
{
XFillRectangle(dpy, pixbf0, gc, xx, yy, ww, hh );
}




void flip_clock_wnd::draw_rectf( unsigned char *bf, int xx, int yy, int ww, int hh, int linedx, unsigned char rr, unsigned char gg, unsigned char bb )
{
for( int y = yy; y < (hh + yy); y++ )
	{
	for( int x = xx; x < (ww + xx); x++ )
		{
		int ptr = y * linedx * bytes_per_pixel   +   x * bytes_per_pixel;
		if( bytes_per_pixel == 3 )
			{
			bf[ptr] = bb;
			bf[ptr + 1 ] = gg;
			bf[ptr + 2 ] = rr;
			}
		else{
			bf[ptr] = bb;
			bf[ptr + 1 ] = gg;
			bf[ptr + 2 ] = rr;
			bf[ptr + 3 ] = 0;
			}
		}
	}
}







void flip_clock_wnd::copy_area( unsigned char *bfsrc, int srcx, int srcy, int srcwid, int srchei, int linedx, unsigned char *bfdest, int destx, int desty, int destlinedx )
{
int yy = desty;

for( int y = srcy; y < (srcy + srchei); y++ )
	{
	int xx = destx;
	
	for( int x = srcx; x < (srcx + srcwid); x++ )
		{
		if( bytes_per_pixel == 3 )
			{
			unsigned char rr, gg, bb, aa;

			int ptr0 = y * linedx * bytes_per_pixel   +   x * bytes_per_pixel;
			bb = bfsrc[ptr0];
			gg = bfsrc[ptr0 + 1 ];
			rr = bfsrc[ptr0 + 2 ];

			int ptr1 = yy * destlinedx * bytes_per_pixel   +   xx * bytes_per_pixel;
			bfdest[ptr1] = bb;
			bfdest[ptr1 + 1 ] = gg;
			bfdest[ptr1 + 2 ] = rr;
			}
		else{
			unsigned char rr, gg, bb, aa;

			int ptr0 = y * linedx * bytes_per_pixel   +   x * bytes_per_pixel;
			bb = bfsrc[ptr0];
			gg = bfsrc[ptr0 + 1 ];
			rr = bfsrc[ptr0 + 2 ];
			aa = bfsrc[ptr0 + 3 ];

			int ptr1 = yy * destlinedx * bytes_per_pixel   +   xx * bytes_per_pixel;
			bfdest[ptr1] = bb;
			bfdest[ptr1 + 1 ] = gg;
			bfdest[ptr1 + 2 ] = rr;
			bfdest[ptr1 + 3 ] = aa;
			}
		xx++;
		}
	yy++;
	}
}







void flip_clock_wnd::line_style( int line_width, int line_style, int cap_style, int join_style )
{
XSetLineAttributes( dpy, gc, line_width, line_style, cap_style, join_style );
}







void flip_clock_wnd::draw_line( int x0, int y0, int x1, int y1 )
{
XDrawLine( dpy, pixbf0, gc, x0, y0, x1, y1 );
}






void flip_clock_wnd::get_pixel( unsigned char *bf, int xx, int yy, int &rr, int &gg, int &bb, int linedx )
{
int ix = nearbyint( xx );
int iy = nearbyint( yy );
int psrc = ( (iy) * (linedx)*bytes_per_pixel )  +  (ix)*bytes_per_pixel;

bb = bf[ psrc++ ];
gg = bf[ psrc++ ];
rr = bf[ psrc ];


}









void flip_clock_wnd::set_pixel( unsigned char *bf, int xx, int yy, int rr, int gg, int bb, int linedx )
{
int ix = nearbyint( xx );
int iy = nearbyint( yy );
int psrc = ( (iy) * (linedx)*bytes_per_pixel )  +  (ix)*bytes_per_pixel;

bf[ psrc++ ] = bb;
bf[ psrc++ ] = gg;
bf[ psrc++ ] = rr;

if( bytes_per_pixel == 4 ) 
	{
	bf[ psrc ] = 0;
	}

}










//draw a line in spec buf
void flip_clock_wnd::line_plot_in_buf( unsigned char *bf, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int linedx )
{
float xx1 = x1;
float yy1 = y1;

float xx2 = x2;
float yy2 = y2;


if( 1 )
	{
	float dx = xx2 - xx1;
	float dy = yy2 - yy1;

	if( fabsf( dx ) >= fabsf( dy ) )
		{
		if( xx1 > xx2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( yy2 - yy1 ) / ( xx2 - xx1 );

		float b = -(m*xx1 - yy1);

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		
		int sub = 0;
		for( int i = xx1; i < xx2; i+=1 )
			{
			int xx = i;
			int yy = m*i + b;

			set_pixel( bf, xx, yy, rr, gg, bb, linedx );
			}	
		}
	else{
		//dy > dx
		if( yy1 > yy2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( xx2 - xx1 ) / ( yy2 - yy1 );

		float b = -(m*yy1 - xx1);

		int sub = 0;
		for( int i = yy1; i < yy2; i+=1 )
			{
			int yy = i;
			int xx = m*i + b;
			
			set_pixel( bf, xx, yy, rr, gg, bb, linedx );
			}	
		}
	}


}




//rough circle, vars vals are not checked, 'theta_start' <= 'theta_end'
void flip_clock_wnd::arc_plot_in_buf( unsigned char *bf, int centrex, int centrey, int radius, float theta_start, float theta_end, float theta_step, int rr, int gg, int bb, int linedx )
{
if( theta_step < 0.0001 ) theta_step = 0.0001;

for( float theta = theta_start; theta < theta_end; theta += theta_step )
	{
	float xx = centrex + radius * cosf( theta );
	float yy = centrey + radius * sinf( theta );
	set_pixel( bf, nearbyint(xx), nearbyint(yy), rr, gg, bb, linedx );		//dest pixel
	}
}














//'px,py' is 'left,top' corner
void flip_clock_wnd::draw_text_line_black( int px, int py, int rectww, int recthh, int text_offx, int text_offy, int line_black_offx, int line_black_offy, XftFont *text_font, string stext, int line_width, int rr, int gg, int bb )
{
string s1;


int col_bkgd_r = 60;
int col_bkgd_g = 60;
int col_bkgd_b = 60;


strpf( s1, "%s", stext.c_str() );

XGlyphInfo extents;                           
XftTextExtentsUtf8( dpy, text_font, (XftChar8 *)s1.c_str(), s1.length(), &extents );
              
int textww = extents.width - extents.x;                                        
int texthh = extents.height;                                       

set_color( col_bkgd_r, col_bkgd_g, col_bkgd_b );
draw_rectf( px, py, rectww, recthh );

//fc.draw_rectf( (unsigned char*)pixbf0->image, 0, 0, xshm_img1->width, xshm_img1->height, xshm_img1->width, 0, 0, 0 );

//return;




//set_color( 130, 130, 130 );
//line_style ( line_width, LineSolid, CapRound, JoinRound );


set_color( rr, gg, bb );



XftDrawStringUtf8( xft_draw, &xftcolor, text_font, px + text_offx, py + text_offy + texthh, (const FcChar8 *) s1.c_str(), s1.length() );


int col_line_r = 40;
int col_line_g = 40;
int col_line_b = 40;

//black line
set_color( col_line_r, col_line_g, col_line_b );
line_style ( line_width, LineSolid, CapRound, JoinRound );

draw_line( px, py + recthh/2 + line_black_offy, px + 200, py + recthh/2 + line_black_offy );

}








//'px,py' is 'left,top' corner
void flip_clock_wnd::draw_text( int px, int py, string stext, XftFont *text_font, int rr, int gg, int bb, int align )
{

int col_bkgd_r = 60;
int col_bkgd_g = 60;
int col_bkgd_b = 60;

set_color( rr, gg, bb );

string s1;
strpf( s1, "%s", stext.c_str() );


XGlyphInfo extents;                           
XftTextExtentsUtf8( dpy, text_font, (XftChar8 *)s1.c_str(), s1.length(), &extents );
              
XftDrawStringUtf8( xft_draw, &xftcolor, text_font, px, py, (const FcChar8 *) s1.c_str(), s1.length() );
}














//render text numerals offscreen
void flip_clock_wnd::render_text()
{
string s1;

int px = 0;
int py = 0;

int rectww = 200;
int recthh = 200;

int text_offx = 17;
int text_offy = 45;
int line_break_offx = 0;
int line_break_offy = 0;
//int text_font = 0;
//int text_size = 150;
int break_line_width = 4;

int rr = 220;
int gg = 220;
int bb = 220;

int disp_secs = isecs;
int disp_mins = imins;
int disp_hrs = ihrs;


//b_show_secs = 1;

//if(b_show_alarm)
//	{
//	disp_secs = alarm_secs[alarm_idx];
//	disp_mins = alarm_mins[alarm_idx];
//	disp_hrs = alarm_hrs[alarm_idx];
//	}

int hrs_minus1 = disp_hrs - 1;
if (hrs_minus1 < 0 ) hrs_minus1 = 23;

int mins_minus1 = disp_mins - 1;
if (mins_minus1 < 0 ) mins_minus1 = 59;

int secs_minus1 = disp_secs - 1;
if (secs_minus1 < 0 ) secs_minus1 = 59;


hrs_minus1 = prev_hrs;
mins_minus1 = prev_mins;
secs_minus1 = prev_secs;


//----- hrs - large font
if( b_24hr )											//prev hr
	{
	strpf( s1, "%02d", hrs_minus1 );
	if( hrs_minus1 == 0 ) s1 = "00";
	}
else{
	int ii = hrs_minus1;
	if( ii > 12 ) ii -= 12;
	
	if( ii < 10 ) 
		{
		strpf( s1, " %d", ii );							//single digit
		}
	else{
		strpf( s1, "%02d", ii);
		}

	if( ii == 0 ) s1 = "12";
	}

draw_text_line_black( px, py, rectww, recthh, text_offx, text_offy, line_break_offx, line_break_offy, font150, s1, break_line_width, rr, gg, bb );

if( b_24hr )											//cur hr
	{
	strpf( s1, "%02d", disp_hrs );
	if( disp_hrs == 0 ) s1 = "00";
	}
else{
	int ii = disp_hrs;
	if( ii > 12 ) ii -= 12;
		
	if( ii < 10 ) 
		{
		strpf( s1, " %d", ii );							//single digit
		}
	else{
		strpf( s1, "%02d", ii);
		}

	if( ii == 0 ) s1 = "12";
	}
	
//draw_text_line_black( px + 250, py, 0, -4, 0, -1, font_type, font_size, s1, break_line_width, 220, 220, 220 );
draw_text_line_black( px + 250, py, rectww, recthh, text_offx, text_offy, line_break_offx, line_break_offy, font150, s1, break_line_width, rr, gg, bb );

//----- 


//----- AM/PM - small font
string smeridian_old = "AM";
if( hrs_minus1 > 11 ) smeridian_old = "PM";

string smeridian_new = "AM";
if( disp_hrs > 11 ) smeridian_new = "PM";

int text_align = 0;
if( !b_24hr ) 
	{
	draw_text( px + 5, py + 190, smeridian_old, font25, rr, gg, bb, text_align );			//AM/PM indicator
	draw_text( px + 250 + 5, py + 190, smeridian_new, font25, rr, gg, bb, text_align );
	}
//if(b_show_alarm)draw_text( px + 250+5, py + 17, "ALARM", font_type, font_size2, 220, 220, 220, text_align );
//----- 



//----- mins - large font

//if( b_show_secs ) strpf( s1, "%02d", disp_secs );
//else strpf( s1, "%02d", mins_minus1 );
strpf( s1, "%02d", mins_minus1 );
draw_text_line_black( px + 500, py, rectww, recthh, text_offx, text_offy, line_break_offx, line_break_offy, font150, s1, break_line_width, rr, gg, bb );

strpf( s1, "%02d", disp_mins );
draw_text_line_black( px + 750, py, rectww, recthh, text_offx, text_offy, line_break_offx, line_break_offy, font150, s1, break_line_width, rr, gg, bb );

//----- 

//----- secs - large font
if( b_show_secs );
	{

	strpf( s1, "%02d", secs_minus1 );
	draw_text_line_black( px + 1000, py, rectww, recthh, text_offx, text_offy, line_break_offx, line_break_offy, font150, s1, break_line_width, rr, gg, bb );

	strpf( s1, "%02d", disp_secs );
	draw_text_line_black( px + 1250, py, rectww, recthh, text_offx, text_offy, line_break_offx, line_break_offy, font150, s1, break_line_width, rr, gg, bb );

	}
//----- 

}













//render leaf flipping numerals and compose
void flip_clock_wnd::render( unsigned char *bf, int wid, int hei, int destx, int desty )
{
string s1;


int linedx = wid;

float sub_pixel_factor = 0.5f;

//theta_hrs += 0.1;



int drwx = 0;
int drwy = 0;

float drw_offset_z = 400.0f;
float drw_scle = 400.0f;
float scale_ratio = drw_scle / drw_offset_z;

st_3d_pixel_block_texture_wrap_single_axis_tag ot;



/*
int px = 0;
int py = 0;

int rectww = 200;
int recthh = 200;

int text_offx = 17;
int text_offy = 45;
int line_break_offx = 0;
int line_break_offy = 0;
int text_font = 0;
int text_size = 150;
int break_line_width = 4;

int rr = 220;
int gg = 220;
int bb = 220;
*/

/*
int disp_secs = isecs;
int disp_mins = imins;
int disp_hrs = ihrs;


b_show_secs = 1;

//if(b_show_alarm)
//	{
//	disp_secs = alarm_secs[alarm_idx];
//	disp_mins = alarm_mins[alarm_idx];
//	disp_hrs = alarm_hrs[alarm_idx];
//	}

int hrs_minus1 = disp_hrs - 1;
if (hrs_minus1 < 0 ) hrs_minus1 = 23;

int mins_minus1 = disp_mins - 1;
if (mins_minus1 < 0 ) mins_minus1 = 59;

int secs_minus1 = disp_secs - 1;
if (secs_minus1 < 0 ) secs_minus1 = 59;


hrs_minus1 = prev_hrs;
mins_minus1 = prev_mins;
secs_minus1 = prev_secs;


//----- hrs - large
if( b_24hr )											//current hr
	{
	strpf( s1, "%02d", hrs_minus1 );
	if( hrs_minus1 == 0 ) s1 = "00";
	}
else{
	int ii = hrs_minus1;
	if( ii > 12 ) ii -= 12;
	
	if( ii < 10 ) 
		{
		strpf( s1, " %d", ii );							//single digit
		}
	else{
		strpf( s1, "%02d", ii);
		}

	if( ii == 0 ) s1 = "12";
	}

s1 = "38" ;
fc.draw_text_line_black( px, py, rectww, recthh, text_offx, text_offy, line_break_offx, line_break_offy, text_font, text_size, s1, break_line_width, rr, gg, bb );


XCopyArea( dpy, double_buffer, root, gc, 0, 0, wid, hei, 0, 0);

//XCopyArea( dpy, double_buffer, root, gc, 0, 0, wid, hei, 0, 0);

//draw_text_line_black( drwx, drwy, 0, -4, 0, -1, font_type, font_size, s1, break_line_width, 220, 220, 220 );
*/


//------- hrs rotated composite large size -------

int drw_cmpst_dest_hrs_x = destx - 100;
int drw_cmpst_dest_hrs_y = desty - 200;


float theta1 = theta_hrs;

if( theta1 > 0.0f )
	{

	//----- back image (full)

	ot.bflip_vert = 0;
	ot.bf = bf;
	ot.srcx = drwx + 0;					//'left,top corner' pixel position to be used for texture wrapping
	ot.srcy = drwy;
	ot.linedx = linedx;
	ot.wid = 200;
	ot.hei = 200;


	ot.bfdest = bf;
	ot.destx = destx;
	ot.desty = desty;
	ot.destlinedx = linedx;

	ot.rotation_axis = 0;
	ot.origin3d_x = 0;
	ot.origin3d_y = -ot.hei/2;
	ot.origin3d_z = 0;
	ot.theta = 0;
	ot.offset_z = drw_offset_z;
	ot.scale = drw_scle;

	tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );



	if( theta1 > 0.0f )
		{
		//behind top half leaf (stationary - shows the next number behind)
		ot.bflip_vert = 0;
		ot.bf = bf;
		ot.srcx = drwx+250;					//'left,top corner' pixel position to be used for texture wrapping
		ot.srcy = drwy;
		ot.linedx = linedx;
		ot.wid = 200;
		ot.hei = 100 - 3;


		ot.bfdest = bf;
		ot.destx = destx;
		ot.desty = desty - 100*scale_ratio - 1*scale_ratio;
		ot.destlinedx = linedx;

		ot.rotation_axis = 0;
		ot.origin3d_x = 0;
		ot.origin3d_y = -ot.hei/2 - 3;
		ot.origin3d_z = 0;
		ot.theta = 0;
		ot.offset_z = drw_offset_z;
		ot.scale = drw_scle;


		tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );
		}

		//----- rotating image
		if( theta1 != 0.0f )
			{
			if( theta1 < pi/2 )
				{
				//----top of flip ----
				ot.bflip_vert = 0;
				ot.bf = bf;

				ot.srcx = drwx;					//'left,top corner' pixel position to be used for texture wrapping
				ot.srcy = drwy;
				ot.linedx = linedx;
				ot.wid = 200;
				ot.hei = 100;

				ot.bfdest = bf;
				ot.destx = destx;
				ot.desty = desty - 100*(scale_ratio);
				ot.destlinedx = linedx;

				ot.rotation_axis = 0;
				ot.origin3d_x = 0;
				ot.origin3d_y = -ot.hei/2;
				ot.origin3d_z = 0;
				ot.theta = theta1;
				ot.offset_z = drw_offset_z;
				ot.scale = drw_scle - 0.0f;
				tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );
				}
			else{
				//--- bottom of flip ----
				ot.bflip_vert = 1;
				ot.bf = bf;
				ot.srcx = drwx + 250;					//'left,top corner' pixel position to be used for texture wrapping
				ot.srcy = drwy + 100;
				ot.linedx = linedx;
				ot.wid = 200;
				ot.hei = 100-1;


				ot.bfdest = bf;
				ot.destx = destx;
				ot.desty = desty - 100*(scale_ratio);
				ot.destlinedx = linedx;

				ot.rotation_axis = 0;
				ot.origin3d_x = 0;
				ot.origin3d_y = -ot.hei/2;
				ot.origin3d_z = 0;
				ot.theta = theta1;
				ot.offset_z = drw_offset_z;
				ot.scale = drw_scle;
				tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );
				}

			}
		//-----
	}
//-------







int drw_cmpst_src_offx = 500;
int drw_cmpst_src_offy = 0;

int drw_cmpst_dest_offx = 300;
int drw_cmpst_dest_offy = 0;

int drw_cmpst_dest_mins_x = destx + drw_cmpst_dest_offx - 100;
int drw_cmpst_dest_mins_y = desty + drw_cmpst_dest_offy - 200;


//------- mins rotated composite large size -------
	theta1 = theta_mins;

	if( theta1 > 0.0f )
		{
		//----- back image (full)

		ot.bflip_vert = 0;
		ot.bf = bf;
		ot.srcx = drwx + drw_cmpst_src_offx;					//'left,top corner' pixel position to be used for texture wrapping
		ot.srcy = drwy + drw_cmpst_src_offy;
		ot.linedx = linedx;
		ot.wid = 200;
		ot.hei = 200;


		ot.bfdest = bf;
		ot.destx = destx + drw_cmpst_dest_offx;
		ot.desty = desty + drw_cmpst_dest_offy;
		ot.destlinedx = linedx;

		ot.rotation_axis = 0;
		ot.origin3d_x = 0;
		ot.origin3d_y = -ot.hei/2;
		ot.origin3d_z = 0;
		ot.theta = 0;
		ot.offset_z = drw_offset_z;
		ot.scale = drw_scle;

		tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );



		if( theta1 > 0.0f )
			{
			//behind top half leaf (stationary - shows the next number behind)
			ot.bflip_vert = 0;
			ot.bf = bf;
			ot.srcx = drwx + drw_cmpst_src_offx + 250;					//'left,top corner' pixel position to be used for texture wrapping
			ot.srcy = drwy + drw_cmpst_src_offy;
			ot.linedx = linedx;
			ot.wid = 200;
			ot.hei = 100 - 3;


			ot.bfdest = bf;
			ot.destx = destx + drw_cmpst_dest_offx;
			ot.desty = desty + drw_cmpst_dest_offy - 100*scale_ratio - 1*scale_ratio;
			ot.destlinedx = linedx;

			ot.rotation_axis = 0;
			ot.origin3d_x = 0;
			ot.origin3d_y = -ot.hei/2 - 3;
			ot.origin3d_z = 0;
			ot.theta = 0;
			ot.offset_z = drw_offset_z;
			ot.scale = drw_scle;


			tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );
			}



		//----- rotating image
		if( theta1 != 0.0f )
			{
			if( theta1 < pi/2 )
				{
				//----top of flip ----
				ot.bflip_vert = 0;
				ot.bf = bf;

				ot.srcx = drwx + drw_cmpst_src_offx;					//'left,top corner' pixel position to be used for texture wrapping
				ot.srcy = drwy + drw_cmpst_src_offy;
				ot.linedx = linedx;
				ot.wid = 200;
				ot.hei = 100;

				ot.bfdest = bf;
				ot.destx = destx + drw_cmpst_dest_offx;
				ot.desty = desty + drw_cmpst_dest_offy - 100*(scale_ratio);
				ot.destlinedx = linedx;

				ot.rotation_axis = 0;
				ot.origin3d_x = 0;
				ot.origin3d_y = -ot.hei/2;
				ot.origin3d_z = 0;
				ot.theta = theta1;
				ot.offset_z = drw_offset_z;
				ot.scale = drw_scle - 0.0f;
				tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );
				}
			else{
				//--- bottom of flip ----
				ot.bflip_vert = 1;
				ot.bf = bf;
				ot.srcx = drwx + drw_cmpst_src_offx + 250;					//'left,top corner' pixel position to be used for texture wrapping
				ot.srcy = drwy + drw_cmpst_src_offy + 100;
				ot.linedx = linedx;
				ot.wid = 200;
				ot.hei = 100-1;


				ot.bfdest = bf;
				ot.destx = destx + drw_cmpst_dest_offx;
				ot.desty = desty + drw_cmpst_dest_offy - 100*(scale_ratio);
				ot.destlinedx = linedx;

				ot.rotation_axis = 0;
				ot.origin3d_x = 0;
				ot.origin3d_y = -ot.hei/2;
				ot.origin3d_z = 0;
				ot.theta = theta1;
				ot.offset_z = drw_offset_z;
				ot.scale = drw_scle;
				tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );
				}

			}
		//-----
	}
//-------







drw_cmpst_src_offx = 1000;
drw_cmpst_src_offy = 0;

drw_cmpst_dest_offx = 300 + 300;
drw_cmpst_dest_offy = 0;


int drw_cmpst_dest_secs_x = destx + drw_cmpst_dest_offx - 100;
int drw_cmpst_dest_secs_y = desty + drw_cmpst_dest_offy - 200;


//------- secs rotated composite large size -------

	theta1 = theta_secs;
	
	if( ( b_show_secs ) && ( theta1 > 0.0f ) )
		{
		//----- back image (full)

		ot.bflip_vert = 0;
		ot.bf = bf;
		ot.srcx = drwx + drw_cmpst_src_offx;					//'left,top corner' pixel position to be used for texture wrapping
		ot.srcy = drwy + drw_cmpst_src_offy;
		ot.linedx = linedx;
		ot.wid = 200;
		ot.hei = 200;


		ot.bfdest = bf;
		ot.destx = destx + drw_cmpst_dest_offx;
		ot.desty = desty + drw_cmpst_dest_offy;
		ot.destlinedx = linedx;

		ot.rotation_axis = 0;
		ot.origin3d_x = 0;
		ot.origin3d_y = -ot.hei/2;
		ot.origin3d_z = 0;
		ot.theta = 0;
		ot.offset_z = drw_offset_z;
		ot.scale = drw_scle;

		tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );





		//isecs = 0;
		//float theta1 = spoke_angle;


		if( theta1 > 0.0f )
			{
			//behind top half leaf (stationary - shows the next number behind)
			ot.bflip_vert = 0;
			ot.bf = bf;
			ot.srcx = drwx + drw_cmpst_src_offx + 250;					//'left,top corner' pixel position to be used for texture wrapping
			ot.srcy = drwy + drw_cmpst_src_offy;
			ot.linedx = linedx;
			ot.wid = 200;
			ot.hei = 100 - 3;


			ot.bfdest = bf;
			ot.destx = destx + drw_cmpst_dest_offx;
			ot.desty = desty + drw_cmpst_dest_offy - 100*scale_ratio - 1*scale_ratio;
			ot.destlinedx = linedx;

			ot.rotation_axis = 0;
			ot.origin3d_x = 0;
			ot.origin3d_y = -ot.hei/2 - 3;
			ot.origin3d_z = 0;
			ot.theta = 0;
			ot.offset_z = drw_offset_z;
			ot.scale = drw_scle;

			tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );
			}



		//----- rotating image
		if( theta1 != 0.0f )
			{
			if( theta1 < pi/2 )
				{
				//----top of flip ----
				ot.bflip_vert = 0;
				ot.bf = bf;

				ot.srcx = drwx + drw_cmpst_src_offx;					//'left,top corner' pixel position to be used for texture wrapping
				ot.srcy = drwy + drw_cmpst_src_offy;
				ot.linedx = linedx;
				ot.wid = 200;
				ot.hei = 100;

				ot.bfdest = bf;
				ot.destx = destx + drw_cmpst_dest_offx;
				ot.desty = desty + drw_cmpst_dest_offy - 100*(scale_ratio);
				ot.destlinedx = linedx;

				ot.rotation_axis = 0;
				ot.origin3d_x = 0;
				ot.origin3d_y = -ot.hei/2;
				ot.origin3d_z = 0;
				ot.theta = theta1;
				ot.offset_z = drw_offset_z;
				ot.scale = drw_scle - 0.0f;
				tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );
				}
			else{
				//--- bottom of flip ----
				ot.bflip_vert = 1;
				ot.bf = bf;
				ot.srcx = drwx + drw_cmpst_src_offx + 250;					//'left,top corner' pixel position to be used for texture wrapping
				ot.srcy = drwy + drw_cmpst_src_offy + 100;
				ot.linedx = linedx;
				ot.wid = 200;
				ot.hei = 100-1;


				ot.bfdest = bf;
				ot.destx = destx + drw_cmpst_dest_offx;
				ot.desty = desty + drw_cmpst_dest_offy - 100*(scale_ratio);
				ot.destlinedx = linedx;

				ot.rotation_axis = 0;
				ot.origin3d_x = 0;
				ot.origin3d_y = -ot.hei/2;
				ot.origin3d_z = 0;
				ot.theta = theta1;
				ot.offset_z = drw_offset_z;
				ot.scale = drw_scle;
				tw.draw_texture_rot3d_single_axis( ot, sub_pixel_factor, 0 );
				}

			}
		//-----

	//-------
	}


//printf("drw_cmpst_dest_hrs_x %d\n", drw_cmpst_dest_hrs_x );


//draw downsampled images for preview mode
if( ( b_preview ) || ( b_debug ) )
	{
	float scale = 0.25;//factor_scale;
	int rendered_wid, rendred_hei;

	int margin_flipx = 38;	//this offset allows lhs of moving flap to be visible, moving flap is larger than base image due to perspective projection making closer objs appear larger
	
	// --- resize using a downsample filter ---
	filter_scale_block( bf,  drw_cmpst_dest_hrs_x-margin_flipx, drw_cmpst_dest_hrs_y, 280, 200, linedx, (unsigned char*)ucdigit[0], 0, 0, cn_fc_render_max_wid, scale, rendered_wid, rendred_hei );
	filter_scale_block( bf, drw_cmpst_dest_mins_x-margin_flipx, drw_cmpst_dest_mins_y, 280, 200, linedx, (unsigned char*)ucdigit[1], 0, 0, cn_fc_render_max_wid, scale, rendered_wid, rendred_hei );
	if( b_show_secs )filter_scale_block( bf, drw_cmpst_dest_secs_x-margin_flipx, drw_cmpst_dest_secs_y, 280, 200, linedx, (unsigned char*)ucdigit[2], 0, 0, cn_fc_render_max_wid, scale, rendered_wid, rendred_hei );


	drw_filtered_x = 10;
	drw_filtered_y = 600;


	//copy pre-rendered bitmap blocks to 
	ot.bf = (unsigned char*)ucdigit[0];
	ot.srcx = 0;							//'left,top corner' pixel position to be used for texture wrapping
	ot.srcy = 0;
	ot.linedx = cn_fc_render_max_wid;
	ot.wid = rendered_wid;
	ot.hei = rendred_hei;
	ot.bfdest = bf;
	ot.destx = drw_filtered_x;
	ot.desty = drw_filtered_y;
	ot.destlinedx = linedx;

	tw.copy_pixel_block( ot );					//copy block



	ot.bf = (unsigned char*)ucdigit[1];
	ot.srcx = 0;							//'left,top corner' pixel position to be used for texture wrapping
	ot.srcy = 0;
	ot.linedx = cn_fc_render_max_wid;
	ot.wid = rendered_wid;
	ot.hei = rendred_hei;
	ot.bfdest = bf;
	ot.destx = drw_filtered_x + 65;
	ot.desty = drw_filtered_y;
	ot.destlinedx = linedx;

	tw.copy_pixel_block( ot );					//copy block



	if( b_show_secs )
		{
		ot.bf = (unsigned char*)ucdigit[2];
		ot.srcx = 0;							//'left,top corner' pixel position to be used for texture wrapping
		ot.srcy = 0;
		ot.linedx = cn_fc_render_max_wid;
		ot.wid = rendered_wid;
		ot.hei = rendred_hei;
		ot.bfdest = bf;
		ot.destx = drw_filtered_x + 65 + 65;
		ot.desty = drw_filtered_y;
		ot.destlinedx = linedx;

		tw.copy_pixel_block( ot );					//copy block
		}
	}
}





bool flip_clock_wnd::filter_kernel_float( en_filter_window_type_tag wnd_type, float *w, int N )
{
int ilow;

for( float n = 0; n < N; n++ )
	{
	int i = (int)n;
	
    
	switch( wnd_type )
		{
		case fwt_rect:
			w[ i ] = 1.0f;                              										//rect window impulse resp
		break;

		case fwt_bartlett:
            ilow = ( N - 1.0f ) / 2.0f ;                        //work out which formula to use (what side of the triangle peak we are in)
            
            if( i <= ilow ) w[ i ] = 2.0f * n / ( N - 1.0f ) ;                                //bartlett/triangle window impulse resp
            else w[ i ] =  2.0f - 2.0f * n / ( N - 1.0f ) ;
		break;

		case fwt_hann:
           w[ i ] = 0.5f * ( 1.0f - cosf( ( twopi * n / ( N - 1.0f ) ) ) );                     //hann window impulse resp
		break;

		case fwt_bartlett_hanning:
            w[ i ] = 0.62f - 0.48f * fabsf( n / ( N - 1.0f ) - 0.5f ) + 0.38f * cosf( ( twopi * ( n / ( N - 1.0f ) - 0.5f ) ) );        //bartlett-hanning window impulse resp
		break;

		case fwt_hamming:
            w[ i ] = 0.54f - 0.46f * cosf( twopi * n / ( N - 1.0f ) );            //hamming window impulse resp, note the text in  http://www.mikroe.com.. listed above is slightly incorrect, used wikipedia version
		break;

		case fwt_blackman:
            w[ i ] = 0.42f - 0.5f * cosf( twopi * n / ( N - 1.0 ) ) + 0.08f * cosf( 2.0f * twopi * n / ( N - 1.0f ) );        //blackman window impulse resp
		break;

		case fwt_blackman_harris:
            w[ i ] = 0.35875f - 0.48829f * cosf( twopi * n / ( N - 1.0f ) ) + 0.14128f * cosf( 2.0f * twopi * n / ( N - 1.0f ) ) - 0.01168f * cosf( 3.0f * twopi * n / ( N - 1.0f ) );        //blackman-harris window impulse resp
		break;

		default:
			printf( "flip_clock_wnd::filter_kernel() - unknown filter window type(wnd): %u\n", wnd_type );
			return 0;
		break;
        }
	}
	
return 1;
}








//colour orderings below are indicative only, they are dependent hardware config 
void flip_clock_wnd::filter_scale_block( unsigned char *bfsrc, int srcx_in, int srcy_in, int wid, int hei, int linedx, unsigned char *bfdest, int destx, int desty, int destlinedx, float scale_in, int &rendered_wid, int &rendered_hei )
{
int kern_size = kern_size0;

en_filter_window_type_tag wnd_type = filter_type;//fwt_blackman_harris;

filter_kernel_float( wnd_type, kern00, kern_size );

float scale_inv = 1.0f/scale_in;
float fx = 0;
float fy = 0;
int size_srcx;
int size_srcy;
int size_destx;
int size_desty;

float renorm = kern_size;

if( 1 )
	{
	int pdest = 0;
	bool at_edge;

	bool done_whgt_sum = 0;
	float wght_sum = 0;

	size_srcx = wid - kern_size;						//don't scan rightmost and bottom most pixels so as to avoid convolution going past end of src image dimensions
	size_srcy = hei - kern_size;
	size_destx = 0;
	size_desty = 0;

	//image scaling - convolve filter and image  (note: the kernel has not been centred over src pixels, so dest would be shifted slightly to the right by 'kern_size')
	size_desty = 0;
	for( fy = 0; fy < size_srcy; fy += scale_inv )
		{
		size_destx = 0;
		for( fx = 0; fx < size_srcx; fx += scale_inv )
			{
			float sumr = 0;
			float sumg = 0;
			float sumb = 0;
			for( int ky = 0; ky < kern_size; ky++ )
				{
				int srcy = nearbyint(fy) + ky;

				if (srcy >= hei) at_edge = 1;
				else at_edge = 0;

				float fky = kern00[ ky ];

				for( int kx = 0; kx < kern_size; kx++ )
					{				
					int srcx = nearbyint(fx) + kx;
					
					if (srcx >= wid) at_edge = 1;
					else at_edge = 0;

					int psrc = ( (srcy_in + srcy)*linedx*bytes_per_pixel )  +  (srcx_in + srcx)*bytes_per_pixel;
					
					float fkx = kern00[ kx ];

					if( !done_whgt_sum ) wght_sum += fky*fkx;
					
					float fk = fky*fkx;
					
					unsigned char uc;

					uc = bfsrc[ psrc + 0 ];								
					if( at_edge ) uc = 0;				
					sumr += uc*fk;											//mac

					uc = bfsrc[ psrc + 1 ];				
					if( at_edge ) uc = 0;				
					sumg += uc*fk;
					
					uc = bfsrc[ psrc + 2 ];				
					if( at_edge ) uc = 0;				
					sumb += uc*fk;
					}
				}

			pdest = ( ( desty + size_desty ) * ( destlinedx ) * bytes_per_pixel    +    ( destx + size_destx )* bytes_per_pixel );

			bfdest[ pdest++ ] = sumr / wght_sum;							//place new pixel	
			bfdest[ pdest++ ] = sumg / wght_sum;	
			bfdest[ pdest ] = sumb / wght_sum;

			done_whgt_sum = 1;

			size_destx++;
			}

		rendered_wid = size_destx;
		
//printf( "size_destx: %d\n", size_destx );

		size_desty++;
		}
	
	rendered_hei = size_desty;
	}
}
















float flip_clock_wnd::tick()
{
mystr m1;

double dt = m1_timer.time_passed( m1_timer.ns_tim_start );
m1_timer.time_start( m1_timer.ns_tim_start );

//printf("dt: %f\n", dt);

struct tm tt;
m1.get_time_now( tt );

int iihrs, iimins, iisecs;

iihrs = tt.tm_hour;
iimins = tt.tm_min;
iisecs = tt.tm_sec;


//bool b_time_sim = 0;

//if( b_time_sim )
//	{
//	iihrs = time_sim_hrs;
//	iimins = time_sim_mins;
//	iisecs = time_sim_secs;
//	}



set_flip_speed( gravity );
//set_flip_speed( 10 );


b_flip_hrs = 0;
b_flip_mins = 0;
b_flip_secs = 0;
if( last_hrs != iihrs ) b_flip_hrs = 1;
if( last_mins != iimins ) b_flip_mins = 1;
if( last_secs != iisecs ) b_flip_secs = 1;

if( !b_show_secs ) b_flip_secs = 0;

bool need_flip = 0;
if( first_tick )
	{
	iisecs = iisecs/10 * 10;
	last_secs = iisecs - 10;								//remove units digit
	
	if( last_secs < 0 ) last_secs += 60;
	
	last_mins = iimins - 1;
	if( last_mins < 0 ) last_mins += 60;
	
	last_hrs = iihrs - 1;
	if( last_hrs < 0 ) last_hrs += 24;
	
	prev_secs = last_secs;
	prev_mins = last_mins;
	prev_hrs = last_hrs;
	
	last_redraw_time = 0;

	first_tick = 0;
	}

int delta;
if( 1 )
	{
	delta = iisecs - last_secs;
	if( delta < 0 ) delta += 60;
	}
else{
	delta = iimins - last_mins;
	if( delta < 0 ) delta += 60;
	}


bool b_5sec;
if( !(iisecs%5) ) b_5sec = 1;
else b_5sec = 0;

bool b_10sec;
if( !(iisecs%10) ) b_10sec = 1;
else b_10sec = 0;

bool b_30sec;
if( !(iisecs%30) ) b_30sec = 1;
else b_30sec = 0;

bool b_sec_flip = b_5sec;
if( flip_speed < 1.0f ) b_sec_flip = b_10sec;
if( flip_speed < 0.5f ) b_sec_flip = b_30sec;

if( ( delta >= 1 ) && (b_sec_flip) || ( need_flip ) )
	{
	last_secs = iisecs;
	last_mins = iimins;
	last_hrs = iihrs;
	if( b_flip_hrs ) theta_hrs = 0.001f;						//start flip
	if( b_flip_mins ) theta_mins = 0.001f;
	if( b_flip_secs ) theta_secs = 0.001f;

	ihrs = iihrs;
	imins = iimins;
	isecs = iisecs;
	}

//if( b_show_alarm ) 
//	{
//	theta_secs = pi;
//	theta_mins = pi;
//	theta_hrs = pi;
//	}



if( theta_secs >= pi )
	{
	prev_secs = last_secs;
	}

if( theta_mins >= pi )
	{
	prev_mins = last_mins;
	}

if( theta_hrs >= pi )
	{
	prev_hrs = last_hrs;
	}








if( ( theta_hrs >= 0.001f ) && ( theta_hrs < pi ) )				//flip progression
	{
	float dtt = dt * flip_speed;
	theta_hrs += dtt * 2.0f;
	}

if( ( theta_mins >= 0.001f ) && ( theta_mins < pi ) )			//flip progression
	{
	float dtt = dt * flip_speed;
	theta_mins += dtt * 2.0f;
	}

if( ( theta_secs >= 0.001f ) && ( theta_secs < pi ) )			//flip progression
	{
	float dtt = dt * flip_speed;
	theta_secs += dtt * 2.0f;
	}

if( theta_hrs >= pi ) 											//end of flip ?
	{
	theta_hrs = pi;
	}

if( theta_mins >= pi ) 											//end of flip ?
	{
	theta_mins = pi;
	}

if( theta_secs >= pi ) 											//end of flip ?
	{
	theta_secs = pi;
	}




if( theta_hrs != last_theta_hrs ) need_full_redraw = 1;
last_theta_hrs = theta_hrs;

if( theta_mins != last_theta_mins ) need_full_redraw = 1;
last_theta_mins = theta_mins;

if( theta_secs != last_theta_secs ) need_full_redraw = 1;
last_theta_secs = theta_secs;


last_redraw_time += dt;

if( need_full_redraw ) 
	{
	need_redraw = 1;
	last_redraw_time = 0.249;
	}

if( last_redraw_time >= 0.25f ) 				//reduce simple redraw freq
	{
	last_redraw_time = 0;
	need_redraw = 1;
	}

return dt;
}








void printf_help()
{
string s1;


printf("Usage:\n" );
printf("-a              app, open in its own window, non screensaver mode\n" );
printf("-b              burn, period in secs between moves of clock around screen, helps reduce screen burn (0-->100)\n" );
printf("-d              debug, open in its own debug window\n" );
//printf("-f 0.25         change display size by spec factor (0.25-->0.5)\n" );
printf("-g 30           gravity, affects flip speed, lower give slower flip (2-->100)\n" );
printf("-h              help, show this\n" );
printf("-s              seconds, display\n" );
printf("-t              24Hr, display\n" );
printf("e.g: ./flip_leaf_clock -a -s -b 30 -g 20\n" );

strpf( s1, "printf_help() - exiting\n" );
append_dbg( s1 );
	

exit(0);
}






int num;
bool is_beep;
float sigma;
string fname;

void process_args(int argc, char** argv)
{
string s1;

//    const char* const short_opts = "dn:bs:f:h";
//const char* const short_opts = "dr:f:st";
const char* const short_opts = "rab:dg:stw";



for( int i = 0; i < argc; i++ )
	{
	string s1 = argv[i];
	
	if( s1.find( "-root" ) != string::npos ) 			//change any '-root' as it confuses 'getopt_long()', set it to '-r'
		{
		printf( "changing '-root' to -r\n" );
		strcpy( argv[i], "-r" );
		}

	if( s1.find( "-window-id" ) != string::npos ) 		//change any '-window-id' as it confuses 'getopt_long()', set it to '-w'
		{
		printf( "changing '-window-id' to -w\n" );
		strcpy( argv[i], "-w" );
		}

	strpf( s1, "argc %d  argv '%s'\n", i, argv[i] );
	append_dbg( s1 );
	}



const option long_opts[] = 
{
	{"root", no_argument, nullptr, 'r'},
	{"app", no_argument, nullptr, 'a'},
	{"debug", no_argument, nullptr, 'd'},
//	{"factor", required_argument, nullptr, 'f'},
	{"gravity", required_argument, nullptr, 'g'},
	{"secs", no_argument, nullptr, 's'},
	{"twenty", no_argument, nullptr, 't'},
	{"wander", required_argument, nullptr, 'b'},
	{"window_id", required_argument, nullptr, "w" },
	{nullptr, no_argument, nullptr, 0}
};




int j = 0;

while( 1 )
	{	
	strpf( s1, "process_args() pre getopt_long()  j = %d\n", j );
	append_dbg( s1 );
	j++;

	const auto opt = getopt_long( argc, argv, short_opts, long_opts, nullptr);

	if(opt == -1) break;

	switch (opt)
		{
		case 'r':
			printf( "--root\n" );
			break;

		case 'a':
			b_app_mode = 1;
			strpf( s1, "b_app_mode: %d\n", b_app_mode );
			printf( s1.c_str() );
			append_dbg( s1 );
			break;

		case 'd':
			b_debug = 1;
			strpf( s1, "b_debug: %d\n", b_debug );
			printf( s1.c_str() );
			append_dbg( s1 );
			break;


//		case 'f':
//			factor_scale = std::stof(optarg);
//			strpf( s1, "factor_scale: %f\n", factor_scale );
//			printf( s1.c_str() );
//			append_dbg( s1 );
//			break;

		case 'g':
			gravity = std::stof(optarg);
			strpf( s1, "gavity: %f\n", gravity );
			printf( s1.c_str() );
			append_dbg( s1 );
			break;

		case 's':
			b_show_secs = 1;
			strpf( s1, "b_secs: %d\n", b_show_secs );
			printf( s1.c_str() );
			append_dbg( s1 );
			break;

		case 't':
			b_24hr = 1;
			strpf( s1, "b_24hr: %d\n", b_24hr );
			printf( s1.c_str() );
			append_dbg( s1 );
			break;

		case 'b':
			wander_period = std::stof(optarg);
			if( wander_period < 0.0f ) wander_period = 0.0f;
			if( wander_period > 100.0f ) wander_period = 100.0f;
			
			strpf( s1, "wander_period: %f\n", wander_period );
			printf( s1.c_str() );
			append_dbg( s1 );
			break;

		case 'w':
			strpf( s1, "window-id\n" );
			printf( s1.c_str() );
			append_dbg( s1 );
			break;

//		case 'f':
//			fname = std::string(optarg);
//			printf( "fname: %s\n", fname.c_str() );
//			break;

		case 'h': // -h or --help
		case '?': // unrecognized option

		default:
			printf_help();

		break;
		}
    }


printf( "---params---\n" );
//printf( "factor: %f\n", factor_scale );
printf( "gravity: %f\n", gravity );
printf( "secs: %d\n", b_show_secs );
printf( "twenty: %d\n", b_24hr );
printf( "wander: %f\n", wander_period );
printf( "------------\n" );
}









static void exit_with_mesg(const char *msg)
{
fprintf(stderr, "%s", msg);
exit(1);
}





int check_for_xshm( Display *display )
{
int major, minor, ignore;
Bool pixmaps;

if (XQueryExtension( display, "MIT-SHM", &ignore, &ignore, &ignore ))
	{
	if (XShmQueryVersion( display, &major, &minor, &pixmaps )==True)
		{
		return (pixmaps==True) ? 2 : 1;
		}
	else{
		return 0;
		}
	}
else
   {
   return 0;
   }
}







//generates a rnd num between -1.0f -> 1.0f
float rnd()
{
float frnd =  (float)( RAND_MAX / 2 - rand() ) / (float)( RAND_MAX / 2 );

return frnd;
}










bool mem_alloc( int cx, int cy )
{
//graphics context
gc = XCreateGC( dpy, root, 0, NULL );

//create bufs
pixbf0 = XCreatePixmap( dpy, root, cx, cy, gwa.depth);
pixbf1 = XCreatePixmap( dpy, root, cx, cy, gwa.depth);


xft_draw = XftDrawCreate( dpy, pixbf0, visual, cmap );


//use a shared mem image for fast bitmap manipulation, won't work with some x11 setup
//refer: https://www.xfree86.org/current/mit-shm.html
//------------
xshm_img0 = XShmCreateImage( dpy, visual, gwa.depth, ZPixmap, NULL, &shminfo0, cx, cy );
if (xshm_img0 == 0) 
   {
   printf("XShmCreateImage() shared mem failed\n");
   exit( EXIT_FAILURE );
   }

printf("XShmCreateImage() succeeded\n");


shminfo0.shmid = shmget( IPC_PRIVATE, xshm_img0->bytes_per_line * xshm_img0->height, IPC_CREAT|0777 );

if (shminfo0.shmid < 0)
	{
    printf("shmget() - failed to alloc shared mem back buffer: 'shminfo0'");
    XDestroyImage( xshm_img0 );
    xshm_img0 = NULL;
	exit( EXIT_FAILURE );
	}

shminfo0.shmaddr = xshm_img0->data = (char*)shmat( shminfo0.shmid, 0, 0 );
if (shminfo0.shmaddr == (char *) -1) 
	{
    printf("shmat() - shared mem failed: 'shminfo0'");
    XDestroyImage( xshm_img0 );
    xshm_img0 = NULL;
    exit( EXIT_FAILURE );
    }

shminfo0.readOnly = False;

bool result = XShmAttach ( dpy, &shminfo0 );
//------------



//use a shared mem image for fast bitmap manipulation, won't work with some x11 setup
//------------
xshm_img1 = XShmCreateImage( dpy, visual, gwa.depth, ZPixmap, NULL, &shminfo1, cx, cy );
if (xshm_img1 == 0) 
   {
   printf("XShmCreateImage() shared mem failed\n");
   exit( EXIT_FAILURE );
   }

printf("XShmCreateImage() succeeded\n");


shminfo1.shmid = shmget( IPC_PRIVATE, xshm_img1->bytes_per_line * xshm_img1->height, IPC_CREAT|0777 );

if (shminfo1.shmid < 0)
	{
    printf("shmget() - failed to alloc shared mem back buffer: 'shminfo1'");
    XDestroyImage( xshm_img1 );
    xshm_img1 = NULL;
	exit( EXIT_FAILURE );
	}

shminfo1.shmaddr = xshm_img1->data = (char*)shmat( shminfo1.shmid, 0, 0 );
if (shminfo1.shmaddr == (char *) -1) 
	{
    printf("shmat() - shared mem failed: 'shminfo1'");
    XDestroyImage( xshm_img1 );
    xshm_img1 = NULL;
    exit( EXIT_FAILURE );
    }

shminfo1.readOnly = False;

result = XShmAttach ( dpy, &shminfo1 );
//------------

return 0;
}




//free up
void mem_free()
{
XShmDetach ( dpy, &shminfo0);
shmdt(&shminfo0.shmaddr);
shmctl(shminfo0.shmid, IPC_RMID, 0);

XShmDetach ( dpy, &shminfo1);
shmdt(&shminfo1.shmaddr);
shmctl(shminfo1.shmid, IPC_RMID, 0);

XDestroyImage(xshm_img0);
XDestroyImage(xshm_img1);
XftDrawDestroy( xft_draw );

XFreePixmap( dpy, pixbf0 );
XFreePixmap( dpy, pixbf1 );
XFreeGC (dpy, gc);

}













int dbg_cnt = 0;



int main(int argc, char *argv[]) 
{
string s1;
float dt = 0.0f;
float wander_timer = 1.0f;			//set low to cause an immediate 'wander_period' calc
int wander_x = 0;
int wander_y = 0;

strpf( s1, "here0\n" );
append_dbg( s1 );

bool vb = 0;

shminfo0.shmid = -1;
shminfo1.shmid = -1;


process_args( argc, argv );

strpf( s1, "here1\n" );
append_dbg( s1 );

fc.b_show_secs = b_show_secs;
fc.b_24hr = b_24hr;


int sleep_us;



// Create our display
dpy = XOpenDisplay(getenv("DISPLAY"));


strpf( s1, "here1\n" );
append_dbg( s1 );

//test if available
if (!check_for_xshm(dpy))
	{
	printf("sorry - x11 shared memory extension is not available, exiting\n");
	exit(EXIT_FAILURE);
	}
else{
	printf("good - x11 shared memory extension is available\n");
	}

strpf( s1, "here2\n" );
append_dbg( s1 );


char *xwin = getenv ("XSCREENSAVER_WINDOW");

int root_window_id = 0;

if (xwin)
	{
	root_window_id = strtol (xwin, NULL, 0);
	}

// get root window
if ( (!b_app_mode) && (!b_debug) )
	{
	// root = DefaultRootWindow(dpy);
	if (root_window_id == 0)
		{
		// root = DefaultRootWindow(dpy);
		printf ("usage as standalone app: %s -a (specify -h  for help)\n", argv[0]);
		return EXIT_FAILURE;
		}
	else{
		root = root_window_id;
		}
	}
else{
	// debug mode, create a window
	int screen = DefaultScreen(dpy);
	root = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 24, 100, 1800, 900, 1, BlackPixel(dpy, screen), WhitePixel(dpy, screen));

	int ytext = 10;
	int htext = ytext + 4;   
	XSizeHints xsh;                                      
	xsh.flags = ( PPosition | PSize | PMinSize );
	xsh.height = htext + 10;                     
	xsh.min_height = xsh.height;

	int wtext = 30;                
	xsh.width = wtext;                           
	xsh.min_width = xsh.width;                   
	xsh.x = 50;                                  
	xsh.y = 50;                                  
	XSetStandardProperties( dpy, root, "flip_leaf_clock", "flip_leaf_clock", None, argv, argc, &xsh );

	XMapWindow(dpy, root);
	}


strpf( s1, "here3\n" );
append_dbg( s1 );



XSelectInput (dpy, root, ExposureMask | StructureNotifyMask | KeyPressMask | PointerMotionMask);


XGetWindowAttributes(dpy, root, &gwa);


int cx = gwa.width;
int cy = gwa.height;
	
if( cx < 1920 ) cx = 1920;					//min size for pixmap
if( cy < 1080 ) cy = 1080;

int ww = gwa.width;
int hh = gwa.height;



strpf( s1, "ww %d\n", ww );
append_dbg( s1 );

strpf( s1, "hh %d\n", hh );
append_dbg( s1 );

bool bpreview = 0;
if( ww < 1000 ) bpreview = 1;				//small window?, screensaver preview?


if( bpreview )
	{
	strpf( s1, "preview mode active\n" );
	printf( s1.c_str() );
	append_dbg( s1 );
	}
else{
	strpf( s1, "fullscreen mode active (not preview)\n" );
	printf( s1.c_str() );
	append_dbg( s1 );
	}


fc.midx = ww/2;
fc.midy = hh/2;

int scr = DefaultScreen(dpy);
visual = DefaultVisual(dpy, scr);
cmap =  DefaultColormap(dpy, scr);

//alloc font sizes to be used
font25 = XftFontOpen( dpy, scr, XFT_FAMILY, XftTypeString, "DejaVu", XFT_PIXEL_SIZE, XftTypeDouble, 25.0, XFT_WEIGHT, XftTypeInteger, FC_WEIGHT_BOLD, NULL );
font150 = XftFontOpen( dpy, scr, XFT_FAMILY, XftTypeString, "DejaVu", XFT_PIXEL_SIZE, XftTypeDouble, 150.0, XFT_WEIGHT, XftTypeInteger, FC_WEIGHT_BOLD, NULL );



mem_alloc( cx, cy );


fc.bytes_per_pixel = xshm_img0->bits_per_pixel / 8;

printf("xshm_img0 has %d bytes_per_pixel \n", fc.bytes_per_pixel );


int ShmCompletionType = -1;


ShmCompletionType = XShmGetEventBase(dpy) + ShmCompletion;				//will send x11 event.type == 'ShmCompletionType' when XShmPutImage() call is displayed



//window close notice
Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
XSetWMProtocols(dpy, root, &wm_delete_window, 1);

//try to load the given font
XFontStruct* font_info;       /* Font structure, used for drawing text.    */
char *font_name = "fixed";

char* text_string = " --- Flip Leaf Clock Preview ---";
int font_height;

font_info = XLoadQueryFont(dpy, font_name);
if (!font_info) 
  {
  fprintf(stderr, "XLoadQueryFont: failed loading font '%s'\n", font_name);
  exit(EXIT_FAILURE);
  }


//create xft color used for font rending
if (!XftColorAllocName( dpy, visual, cmap, "#d0d0d0", &xftcolor )) 
	{
	printf( "failed to alloc XftColorAllocName() color\n");

    XDestroyImage( xshm_img0 );
    xshm_img0 = NULL;
	exit( EXIT_FAILURE );
	}





int compose_x;
int compose_y;


bool bfirst_draw = 1;



bool xshm_is_safe_to_render = 1;
bool quit = 0;
while(!quit) 
		{
        while ( XPending(dpy) > 0) 
			{
            XEvent event = {0};
            XNextEvent(dpy, &event);
            switch (event.type) 
				{
				case ConfigureNotify: 
					{
					XConfigureEvent xce = event.xconfigure;

					if (xce.width != gwa.width || xce.height != gwa.height) 		//window resized ?
						{
						gwa.width = xce.width;
						gwa.height = xce.height;
						printf("resize occurred  ww, hh:  %d, %d\n", gwa.width, gwa.height );

						mem_free();

						cx = gwa.width;
						cy = gwa.height;
	
						if( cx < 1920 ) cx = 1920;								//min size for pixmap
						if( cy < 1080 ) cy = 1080;

						mem_alloc( cx, cy );

						ww = gwa.width;
						hh = gwa.height;

						fc.midx = ww/2;
						fc.midy = hh/2;

						bfirst_draw = 1;
						fc.need_full_redraw = 1;
						continue;
						}
					}
				break;

				case KeyPress: 
					{
					switch (XLookupKeysym(&event.xkey, 0))
						{
						case 'q':
							quit = 1;
						break;
						}
					}
				break;

            case MotionNotify:
				{
				int mousex = event.xmotion.x;
				int mousey = event.xmotion.y;
				}
            break;

            case ClientMessage: 
				{
                if ((Atom) event.xclient.data.l[0] == wm_delete_window )
					{
					quit = 1;
					}
				}
            break;

            default: 
				{
                if (event.type == ShmCompletionType )
					{
					xshm_is_safe_to_render = 1;
//                    printf("'event.type == CompletionType'  xshm\n" );
					}
				}
			}
		}


	if( quit ) goto finish_up;
	
	













/*
	XEvent e;
	if(XCheckWindowEvent(dpy, root, ExposureMask, &e) )
		{
		if( e.type == Expose)
			{
			printf("expose event\n" );
			}
		}


	if(XCheckWindowEvent(dpy, root, 0xffffffff, &e) )
		{
		if( e.type == ShmCompletionType)
			{
			printf("xshm completion event\n" );
			}
		}
*/

	if( ( fc.need_full_redraw ) && ( xshm_is_safe_to_render ) )
		{
		//clear buf
		XSetBackground(dpy, gc, BLACK);
		XSetForeground(dpy, gc, BLACK);
		XFillRectangle(dpy, pixbf0, gc, 0, 0, cx, cy );


//-----------



//	XSetBackground(dpy, gc, BLACK);
//	XSetForeground(dpy, gc, BLACK);

//	XSetForeground(dpy, gc, make_color( 80, 255, 80));
	//XDrawLine(dpy, double_buffer, gc, 10, 10, 300, 300 );


//	fc.set_color( 255, 160, 9 );
//if( !bpreview )	fc.draw_line( 10, 10, 300, 300 );



	//string s1 = "Test";
	//XftDrawStringUtf8( draw, &color, font, 10, 250, (const FcChar8 *) s1.c_str(), s1.length() );


	int px = 0;
	int py = 0;

	int rectww = 200;
	int recthh = 200;

	int text_offx = 17;
	int text_offy = 45;
	int line_break_offx = 0;
	int line_break_offy = 0;
	int text_font = 0;
	int text_size = 150;
	int line_width = 4;

	int rr = 220;
	int gg = 220;
	int bb = 220;

	//position and size of top window (XSizeHints)                   


//	string stext = "38";

	//fc.draw_text_line_black( px, py, rectww, recthh, text_offx, text_offy, line_break_offx, line_break_offy, text_font, text_size, stext, line_width, rr, gg, bb );

	//XCopyArea(dpy, double_buffer, root, gc, 0, 0, wa.width, wa.height, 0, 0);

	//  char* font_name = "*-helvetica-*-12-*"; /* font to use for drawing text.   */



	//find the height of the characters drawn using this font
	font_height = font_info->ascent + font_info->descent;

	int x = 0;
	int y = font_height;
//	XDrawString(dpy, root, gc, x, y, text_string, strlen(text_string));



//	XFontStruct *font;
//	char* name = "-*-dejavu sans-bold-r-*-*-*-220-100-100-*-*-iso8859-1";
//	font = XLoadQueryFont( dpy, name );

	if( bpreview )						//screen saver preview?
		{
//		XSetForeground(dpy, gc, make_color( 255, 160, 0 ));
//		XSetFont(dpy, gc, font_info->fid);
//		XDrawString(dpy, root, gc, x, y, text_string, strlen(text_string));
		}
	
	int dir;
	int ascent;
	int descent;
	XCharStruct overall;
	int width = 10;
	//XTextExtents(font_info, text_string, strlen(text_string), &dir, &ascent, &descent, &overall);
//	XDrawString(dpy,root,gc, (width-XTextWidth(font_info, text_string, strlen(text_string)))/2, ascent, text_string, strlen(text_string));

	//-----------

	//-----------

	//XPutPixel( image, 0, 0, 0xffffffff);


	//	XCopyArea(dpy, double_buffer, root, gc, 0, 0, wa.width, wa.height, 0, 0);

	//	XImage *image = XGetImage( dpy, double_buffer, 0, 0, ww, hh, AllPlanes, ZPixmap );


//set_color( col_bkgd_r, col_bkgd_g, col_bkgd_b );
//fc.draw_rectf( 0, 0, cx, cy );
				

		bool test_pattern = 0;
		
		if( !test_pattern )
			{
			fc.render_text();											//draw numbers and text
//			fc.draw_rectf( (unsigned char*)xshm_img0->data, 0, 0, xshm_img0->width, xshm_img0->height, xshm_img0->width, 0, 0, 0 );
			XShmGetImage( dpy, pixbf0, xshm_img0, 0, 0, AllPlanes );	//make an image from rendered text
			}
		else{
			fc.draw_rectf( (unsigned char*)xshm_img0->data, 0, 0, xshm_img0->width, xshm_img0->height, xshm_img0->width, 0, 0, 0 );
			fc.draw_rectf( (unsigned char*)xshm_img0->data, 0, 0, 200, 200, xshm_img0->width, 100, 100, 100 );
			fc.draw_rectf( (unsigned char*)xshm_img0->data, 250, 0, 200, 200, xshm_img0->width, 150, 150, 150 );

			fc.draw_rectf( (unsigned char*)xshm_img0->data, 500, 0, 200, 200, xshm_img0->width, 100, 100, 100 );
			fc.draw_rectf( (unsigned char*)xshm_img0->data, 750, 0, 200, 200, xshm_img0->width, 150, 150, 150 );

			fc.draw_rectf( (unsigned char*)xshm_img0->data, 1000, 0, 200, 200, xshm_img0->width, 100, 100, 100 );
			fc.draw_rectf( (unsigned char*)xshm_img0->data, 1250, 0, 200, 200, xshm_img0->width, 150, 150, 150 );

			fc.arc_plot_in_buf( (unsigned char*)xshm_img0->data, 100,       100, 90, 0.0f, pi2, 0.001f, 255, 0, 0, xshm_img0->width );
			fc.arc_plot_in_buf( (unsigned char*)xshm_img0->data, 100 + 250, 100, 70, 0.0f, pi2, 0.001f, 0, 255, 0, xshm_img0->width );

			fc.arc_plot_in_buf( (unsigned char*)xshm_img0->data, 100 + 500, 100, 90, 0.0f, pi2, 0.001f, 255, 0, 0, xshm_img0->width );
			fc.arc_plot_in_buf( (unsigned char*)xshm_img0->data, 100 + 750, 100, 70, 0.0f, pi2, 0.001f, 0, 255, 0, xshm_img0->width );

			fc.line_plot_in_buf( (unsigned char*)xshm_img0->data, 1000, 0, 1000 + 200, 200, 255, 255, 255,  xshm_img0->width );
			fc.arc_plot_in_buf( (unsigned char*)xshm_img0->data, 100 + 1000, 100, 90, 0.0f, pi2, 0.001f, 255, 0, 0, xshm_img0->width );
			fc.arc_plot_in_buf( (unsigned char*)xshm_img0->data, 100 + 1250, 100, 70, 0.0f, pi2, 0.001f, 0, 255, 0, xshm_img0->width );
			}

//		fc.arc_plot_in_buf( (unsigned char*)xshm_img0->data, 150, 150, 50, 0.0f, pi2, 0.001f, 255, 255, 255, xshm_img0->width );
	/*
		printf("image->width %d\n", image->width );
		printf("image->height %d\n", image->height );
		printf("image->xoffset %d\n", image->xoffset );
		printf("image->xoffset %d\n", image->xoffset );
		printf("image->byte_order %d\n", image->byte_order );
		printf("image->bitmap_unit %d\n", image->bitmap_unit );
		printf("image->bitmap_bit_order %d\n", image->bitmap_bit_order );
		printf("image->bitmap_pad %d\n", image->bitmap_pad );
		printf("image->depth %d\n", image->depth );
		printf("image->bytes_per_line %d\n", image->bytes_per_line );
		printf("image->bits_per_pixel %d\n", image->bits_per_pixel );
		printf("\n" );
	*/
	//	printf("byte_per_pixel %d\n", bytes_per_pixel );

		fc.tw.bytes_per_pixel = fc.bytes_per_pixel;

		compose_x = 100;							//where composite image will be built
		compose_y = 450;
		
		//do flip perpective projection
		fc.b_preview = bpreview;					//flag to generate downsampled clock render as well
		fc.render( (unsigned char*)xshm_img0->data, xshm_img0->width, xshm_img0->height, compose_x, compose_y );

		
//if( dbg_cnt & 1 ) fc.draw_rectf( (unsigned char*)xshm_img0->data, 100, 600, 200, 200, xshm_img0->width, 0, 180, 0 );
//else fc.draw_rectf( (unsigned char*)xshm_img0->data, 200, 400, 200, 200, xshm_img0->width, 0, 180, 0 );
//dbg_cnt++;

		//XCopyArea(dpy, double_buffer, root, gc, 0, 0, image->width, image->height, 0, 0);

//		XPutImage( dpy, root, gc, xshm_img, 500, 300, 0, 0, ww - 400, hh - 200 );
//		XShmPutImage(dpy, root, gc, xshm_img, 0, 0, 0, 0, ww - 0, hh - 0, 0);


//	XSetForeground(dpy, gc, BLACK);
//	XFillRectangle(dpy, root, gc, 0, 0, gwa.width, gwa.height);


	//draw the black background
//	XShmPutImage(dpy, root, gc, xshm_img1, 0, 0, 0, 0, ww, hh, 0);

	
//XSetForeground(dpy, gc, BLACK);
//XFillRectangle(dpy, root, gc, 0, 0, gwa.width, gwa.height);
//	XFlush(dpy);



	//'xshm_img1' will be used as the final image, clear it first, clock image will then be added
	fc.draw_rectf( (unsigned char*)xshm_img1->data, 0, 0, xshm_img1->width, xshm_img1->height, xshm_img1->width, 0, 0, 0 );


	int margin_flipx = 35;	//this offset allows lhs/rhs of moving flap to be visible, moving flap is larger than base image due to perspective projection making closer objs appear larger
	
	int clock_wid = 300 + 300 + 200 + 2*margin_flipx;
	int clock_hei = 200;
	
	if( !b_show_secs ) clock_wid = 300 + 200 + 2*margin_flipx;
	
//bpreview = 1;
	if( bpreview )
		{
		//roughly quarter sized clock image via downsample filter
		clock_wid = 65 + 65 + 65;
		if( !b_show_secs ) clock_wid = 65 + 65;
		
		clock_hei *= 0.25f;
		}


	if( bfirst_draw )
		{
		wander_x = ww / 2 - clock_wid / 2;		//center display
		wander_y = hh / 2 - clock_hei/2;
		bfirst_draw = 0;
		}

	if( ( wander_period > 0.01f ) )				//wander mode specified?
		{
		//roam to avoid screen burn
		if( wander_timer > ( wander_period ) )
			{
			float rangex = ww - clock_wid;
			float rangey = hh - clock_hei - 200;
			
			wander_x = rangex * (rnd() + 1.0f)/2;
			wander_y = rangey * (rnd() + 1.0f)/2;
			
			wander_timer = 0;
if(vb)printf("wander_x %f  pos %d %d\n", wander_timer, wander_x, wander_y );
			}
		}
	else{
		//centred 
		wander_x = ww / 2 - clock_wid / 2;
		wander_y = hh / 2 - clock_hei / 2;
		}


//b_debug = 1;
	if( !bpreview )	
		{
		if( !b_debug ) 
//		if( 1 )
			{
			//wandering full screen display
			//add cropped clock image
			fc.copy_area( (unsigned char*)xshm_img0->data, compose_x - (100 + margin_flipx), compose_y - 200, ww - compose_x, hh - (compose_y - 200), xshm_img0->width, (unsigned char*)xshm_img1->data, wander_x, wander_y, xshm_img1->width );
			}
		else{
			//full screen debug display
			fc.copy_area( (unsigned char*)xshm_img0->data, 0, 0, ww, hh, xshm_img0->width, (unsigned char*)xshm_img1->data, 0, 0, xshm_img1->width );
			}
		}
	else{
		//preview mode
		
		float destx = ww/2 - clock_wid / 2;
		if( !b_show_secs ) destx = hh/2 - 50;

		float desty = hh/2 - 25;

//XSetForeground(dpy, gc, BLACK);
//XFillRectangle(dpy, root, gc, 0, 0, gwa.width, gwa.height);

		//add cropped clock image
		if( b_show_secs ) 
			{
			fc.copy_area( (unsigned char*)xshm_img0->data, fc.drw_filtered_x, fc.drw_filtered_y, clock_wid, clock_hei, xshm_img0->width, (unsigned char*)xshm_img1->data, destx, desty, xshm_img1->width );
			}
		else{
			fc.copy_area( (unsigned char*)xshm_img0->data, fc.drw_filtered_x, fc.drw_filtered_y, clock_wid, clock_hei, xshm_img0->width, (unsigned char*)xshm_img1->data, destx, desty, xshm_img1->width );
			}
		}


		if(vb)printf("need_full_redraw\n" );
		


		if(vb)printf("need_redraw   xshm_is_safe_to_render %d\n", xshm_is_safe_to_render );
		XShmPutImage(dpy, pixbf1, gc, xshm_img1, 0, 0, 0, 0, gwa.width, gwa.height, 1);

//		XCopyArea(dpy, pixbf0, pixbf1, gc, 0, 0, ww, hh, 0, 0);
		XCopyArea(dpy, pixbf1, root, gc, 0, 0, ww, hh, 0, 0);			//make visible 
		xshm_is_safe_to_render = 0;
//			
		fc.need_redraw = 0;
		fc.need_full_redraw = 0;
		}



	//	XDestroyImage(xshm_img);
	//-----------

//	XFlush(dpy);

	sleep_us = 10000;
	usleep( sleep_us );							//don't hog processor

	dt = fc.tick();
	wander_timer += dt;
	
	if( ( wander_period > 0.01f ) )		//wander mode specified?
		{
		if( wander_timer >  wander_period ) fc.need_full_redraw = 1;
		}
	
if(vb)printf("wander_x %f \n", wander_timer );
	}


finish_up:

//free up
mem_free();

/*
XShmDetach ( dpy, &shminfo0);
shmdt(&shminfo0.shmaddr);
shmctl(shminfo0.shmid, IPC_RMID, 0);

XShmDetach ( dpy, &shminfo1);
shmdt(&shminfo1.shmaddr);
shmctl(shminfo1.shmid, IPC_RMID, 0);

XDestroyImage(xshm_img0);
XDestroyImage(xshm_img1);
XftDrawDestroy( xft_draw );


XFreePixmap( dpy, pixbf0 );
XFreePixmap( dpy, pixbf1 );
XFreeGC (dpy, gc);
*/

XDestroyWindow(dpy, root);
XCloseDisplay (dpy);

return EXIT_SUCCESS;
}






