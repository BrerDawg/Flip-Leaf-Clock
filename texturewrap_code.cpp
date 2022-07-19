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


//texturewrap_code.cpp


//---- v1.01	2022-jul-18		//simplified for 'flip_leaf_clock'





#include "texturewrap_code.h"











cl_texture_wrap::cl_texture_wrap()
{
bytes_per_pixel = 3;

}


















//colour ordering below are indicative only, they are dependent hardware config 
void cl_texture_wrap::get_pixel( unsigned char *bf, int xx, int yy, int &rr, int &gg, int &bb, int linedx )
{
int ix = nearbyint( xx );
int iy = nearbyint( yy );
int psrc = ( (iy) * (linedx)*bytes_per_pixel )  +  (ix)*bytes_per_pixel;

rr = bf[ psrc++ ];
gg = bf[ psrc++ ];
bb = bf[ psrc++ ];

}






//colour ordering below are indicative only, they are dependent hardware config 
void cl_texture_wrap::set_pixel( unsigned char *bf, int xx, int yy, int rr, int gg, int bb, int linedx )
{
int ix = nearbyint( xx );
int iy = nearbyint( yy );
int psrc = ( (iy) * (linedx)*bytes_per_pixel )  +  (ix)*bytes_per_pixel;

bf[ psrc++ ] = rr;
bf[ psrc++ ] = gg;
bf[ psrc++ ] = bb;

if( bytes_per_pixel == 4 ) bf[ psrc ] = 0;
}









//slow piecemeal raster copy
void cl_texture_wrap::copy_pixel_block( st_3d_pixel_block_texture_wrap_single_axis_tag ot )
{

int startx = 0;
int endx = ot.wid;

int starty = 0;
int endy =  ot.hei;

int rr;
int gg;
int bb;

for( int yy = starty; yy < endy; yy++ )
	{
	for( int xx = startx; xx < endx; xx++ )
		{
		get_pixel( ot.bf, xx + ot.srcx, yy + ot.srcy, rr, gg, bb, ot.linedx );				//src pixel
		set_pixel( ot.bfdest, xx + ot.destx, yy + ot.desty, rr, gg, bb, ot.destlinedx );	//dest pixel
		}
	}
}

















//draw a line in spec buf
void cl_texture_wrap::line_plot_in_buf( unsigned char *bf, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int linedx )
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

			set_pixel( bf, xx, yy, rr, gg, bb, linedx );		//dest pixel
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

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		int sub = 0;
		for( int i = yy1; i < yy2; i+=1 )
			{
			int yy = i;
			int xx = m*i + b;
			
			set_pixel( bf, xx, yy, rr, gg, bb, linedx );		//dest pixel
			}	
		}
	}
}








































void cl_texture_wrap::draw_texture_rot3d_single_axis( st_3d_pixel_block_texture_wrap_single_axis_tag ot, float sub_pixel_factor, bool plot_raster_with_fl_points )
{

float ma0[3];				//matrices
float ma1[3][3];
float ma2[3][3];
float ma3[3][3];
float mares0[3];
float mares1[3];
float mares2[3];


//translate
ma1[0][0] = ot.origin3d_x;
ma1[0][1] = 0;
ma1[0][2] = 0;

ma1[1][0] = 0;
ma1[1][1] = ot.origin3d_y;
ma1[1][2] = 0;

ma1[2][0] = 0;
ma1[2][1] = 0;
ma1[2][2] = ot.origin3d_z;



//rot around x-axis
ma2[0][0] = 1;
ma2[0][1] = 0;
ma2[0][2] = 0;

ma2[1][0] = 0;
ma2[1][1] = cosf( ot.theta );
ma2[1][2] = -sin( ot.theta );

ma2[2][0] = 0;
ma2[2][1] = sinf( ot.theta );
ma2[2][2] = cosf( ot.theta );


//rot around y-axis
if( ot.rotation_axis == 1 )
	{
	ma2[0][0] = cosf( ot.theta );
	ma2[0][1] = 0;
	ma2[0][2] = -sin( ot.theta );

	ma2[1][0] = 0;
	ma2[1][1] = 1;
	ma2[1][2] = 0;

	ma2[2][0] = sinf(ot.theta);
	ma2[2][1] = 0;
	ma2[2][2] = cosf( ot.theta );
	}


//rot around z-axis
if( ot.rotation_axis == 2 )
	{
	ma2[0][0] = cosf( ot.theta );
	ma2[0][1] = -sin( ot.theta );
	ma2[0][2] = 0;

	ma2[1][0] = sinf(ot.theta);
	ma2[1][1] = cosf( ot.theta );
	ma2[1][2] = 0;

	ma2[2][0] = 0;
	ma2[2][1] = 0;
	ma2[2][2] = 1;
	}

//spare
ma3[0][0] = 1;
ma3[0][1] = 0;
ma3[0][2] = 0;

ma3[1][0] = 0;
ma3[1][1] = 1;
ma3[1][2] = 0;

ma3[2][0] = 0;
ma3[2][1] = 0;
ma3[2][2] = 1;




//dx = 240;						//size of src pixel rectangle
//dy = 240;

//int startx = -ot.wid/2;
//int endx = ot.wid/2;

//int starty = -ot.hei/2 + 0;
//int endy = ot.hei/2 + 0;


int startx = 0;
int endx = ot.wid;

int starty = 0;
int endy = ot.hei;




//bool get_extents = 1;
//float minx = 1e6;								//put some number that will be surley overwritten
//float miny = 1e6;

//float maxx = -1e6;
//float maxy = -1e6;

float zz = 1;




//convert src pixel coords (in a square raster format) to address dest pixel locations
float sub = sub_pixel_factor;
for( float yy = starty; yy < endy; yy += sub )
	{	
	for( float xx = startx; xx < endx; xx += sub )
		{
		ma0[0] = xx - ot.wid/2;
		ma0[1] = yy - ot.hei/2;
		ma0[2] = zz;

//--- add
		mares0[0] = ma0[0] + ma1[0][0];
//		mares0[0] += ma0[1] + ma1[0][1];
//		mares0[0] += ma0[2] + ma1[0][2];

//		mares0[1] = ma0[0] + ma1[1][0];
		mares0[1] = ma0[1] + ma1[1][1];
//		mares0[1] += ma0[2] + ma1[1][2];

//		mares0[2] = ma0[0] + ma1[2][0];
//		mares0[2] += ma0[1] + ma1[2][1];
		mares0[2] = ma0[2] + ma1[2][2];
//---


//--- multiply
		mares1[0] = mares0[0] * ma2[0][0];
		mares1[0] += mares0[1] * ma2[0][1];
		mares1[0] += mares0[2] * ma2[0][2];

		mares1[1] = mares0[0] * ma2[1][0];
		mares1[1] += mares0[1] * ma2[1][1];
		mares1[1] += mares0[2] * ma2[1][2];

		mares1[2] = mares0[0] * ma2[2][0];
		mares1[2] += mares0[1] * ma2[2][1];
		mares1[2] += mares0[2] * ma2[2][2];
//---



//--- multiply
		mares2[0] = mares1[0] * ma3[0][0];
		mares2[0] += mares1[1] * ma3[0][1];
		mares2[0] += mares1[2] * ma3[0][2];

		mares2[1] = mares1[0] * ma3[1][0];
		mares2[1] += mares1[1] * ma3[1][1];
		mares2[1] += mares1[2] * ma3[1][2];

		mares2[2] = mares1[0] * ma3[2][0];
		mares2[2] += mares1[1] * ma3[2][1];
		mares2[2] += mares1[2] * ma3[2][2];
//---

		int rr;
		int gg;
		int bb;
		
		int ix = nearbyint( mares2[0] / (mares2[2] + ot.offset_z) * ot.scale );		//perspective divide
		int iy = nearbyint( mares2[1] / (mares2[2] + ot.offset_z) * ot.scale );

//		if( get_extents )
//			{
//			if( ix < minx ) ix = minx;
//			if( iy < miny ) iy = miny;
			
//			if( ix > maxx ) ix = maxx;
//			if( iy > maxy ) iy = maxy;
//			}


		if( plot_raster_with_fl_points )
			{
//			fl_color( 255, 0, 0 );
//			fl_point( ix + ot.destx, iy + ot.desty );							//just plot coloured points to show raster's shape
			}
		else{
			if( !ot.bflip_vert ) get_pixel( ot.bf, xx + ot.srcx, yy + ot.srcy, rr, gg, bb, ot.linedx );			//src pixel
			else get_pixel( ot.bf, xx + ot.srcx, ( ot.srcy + starty ) + endy - yy, rr, gg, bb, ot.linedx );		//src pixel

			set_pixel( ot.bfdest, ix + ot.destx, iy + ot.desty, rr, gg, bb, ot.destlinedx );					//dest pixel
			}
		}
	}

}






























