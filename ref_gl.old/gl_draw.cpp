/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// draw.c

#include "gl_local.h"

image_t		*draw_chars;

extern	bool	scrap_dirty;
void Scrap_Upload (void);

/*
Table to convert 3 bit color to 24 (3*float) color

colors are:
 -1 - original (must be handled out here)
  0 - black,
  1 - red,
  2- green,
  4 - blue,
  7 - white
*/
#define I 1
#define o 0.2
static float gl_colortable[8][3] = {
	{0, 0, 0},
	{I, o, o},
	{o, I, o},
	{I, I, o},
	{o, o, I},
	{I, o, I},
	{o, I, I},
	{I, I, I}
};
#undef I
#undef o

/*
===============
Draw_InitLocal
===============
*/
struct image_s  *Draw_FindPic (char *name);

void Draw_InitLocal (void)
{
	// load console characters (don't bilerp characters)
//	draw_chars = GL_FindImage ("pics/conchars.pcx", it_pic);
	draw_chars = Draw_FindPic ("conchars");
	GL_Bind( draw_chars->texnum );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}



/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;

	if ((num&127) == ' ')
		return;		// space is hardcoded

	if (y <= -8 || y >= vid.height)
		return;		// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	GL_Bind (draw_chars->texnum);

	glBegin (GL_QUADS);
	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + size, frow);
	glVertex2f (x+8, y);
	glTexCoord2f (fcol + size, frow + size);
	glVertex2f (x+8, y+8);
	glTexCoord2f (fcol, frow + size);
	glVertex2f (x, y+8);
	glEnd ();
}


/*
================
Draw_CharColor

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_CharColor (int x, int y, int num, int color)
{
	if ((num&127) == ' ')
		return;		// space is hardcoded

	if (y <= -8 || y >= vid.height)
		return;		// totally off screen

	if (color >= 0 && color <= 6)	// white is hardcoded
	{
		glColor3fv (gl_colortable[color]);
		GL_TexEnv (GL_MODULATE);
		Draw_Char (x, y, num);
		GL_TexEnv (GL_REPLACE);
		glColor3f (1, 1, 1);
	}
	else
		Draw_Char (x, y, num);
}


/*
=============
Draw_FindPic
=============
*/
image_t	*Draw_FindPic (char *name)
{
	image_t *gl;

	if (name[0] != '/' && name[0] != '\\')
		gl = GL_FindImage (va("pics/%s.???", name), it_pic);
	else
		gl = GL_FindImage (name+1, it_pic);

	return gl;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize (int *w, int *h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		if (w) *w = 0;
		if (h) *h = 0;
	}
	else
	{
		if (w) *w = gl->width;
		if (h) *h = gl->height;
	}
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic (int x, int y, int w, int h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		Com_Printf ("Can't find pic: %s\n", pic);
		return;
	}

	if (scrap_dirty)
		Scrap_Upload ();

	if ( ( (gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION) ) && !gl->has_alpha)
		glDisable (GL_ALPHA_TEST);

	GL_Bind (gl->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (gl->sl, gl->tl);
	glVertex2f (x, y);
	glTexCoord2f (gl->sh, gl->tl);
	glVertex2f (x+w, y);
	glTexCoord2f (gl->sh, gl->th);
	glVertex2f (x+w, y+h);
	glTexCoord2f (gl->sl, gl->th);
	glVertex2f (x, y+h);
	glEnd ();

	if ( ( (gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION) ) && !gl->has_alpha)
		glEnable (GL_ALPHA_TEST);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		Com_Printf ("Can't find pic: %s\n", pic);
		return;
	}
	if (scrap_dirty)
		Scrap_Upload ();

	if ( ( (gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION) ) && !gl->has_alpha)
		glDisable (GL_ALPHA_TEST);
	else if (gl->has_alpha)
	{
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
	}

	GL_Bind (gl->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (gl->sl, gl->tl);
	glVertex2f (x, y);
	glTexCoord2f (gl->sh, gl->tl);
	glVertex2f (x+gl->width, y);
	glTexCoord2f (gl->sh, gl->th);
	glVertex2f (x+gl->width, y+gl->height);
	glTexCoord2f (gl->sl, gl->th);
	glVertex2f (x, y+gl->height);
	glEnd ();

	if ( ( (gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION) )  && !gl->has_alpha)
		glEnable (GL_ALPHA_TEST);
	else if (gl->has_alpha)
	{
		glEnable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);
	}
}


/*
=============
Draw_PicColor
=============
*/

void Draw_PicColor (int x, int y, char *pic, int color)
{
	if (color >= 0 && color <= 6)	// white is hardcoded - no modulate
	{
		glColor3fv (gl_colortable[color]);
		GL_TexEnv (GL_MODULATE);
		Draw_Pic (x, y, pic);
		GL_TexEnv (GL_REPLACE);
		glColor3f (1, 1, 1);
	}
	else
		Draw_Pic (x, y, pic);
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h, char *pic)
{
	image_t	*image;

	image = Draw_FindPic (pic);	//!! this picture, mostly, will be placed in scrap -- buggy screen appearence! (sorry, can't fix ...)
	if (!image)
	{
		Com_Printf ("Can't find pic: %s\n", pic);
		return;
	}

	if ( ( (gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION) )  && !image->has_alpha)
		glDisable (GL_ALPHA_TEST);

	GL_Bind (image->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (x/64.0, y/64.0);
	glVertex2f (x, y);
	glTexCoord2f ( (x+w)/64.0, y/64.0);
	glVertex2f (x+w, y);
	glTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	glVertex2f (x+w, y+h);
	glTexCoord2f ( x/64.0, (y+h)/64.0 );
	glVertex2f (x, y+h);
	glEnd ();

	if ( ( (gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION) )  && !image->has_alpha)
		glEnable (GL_ALPHA_TEST);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	union
	{
		unsigned	c;
		byte		v[4];
	} color;

	if ( (unsigned)c > 255)
		Com_FatalError ("Draw_Fill: bad color");

	glDisable (GL_TEXTURE_2D);

	color.c = d_8to24table[c];
	glColor3f (color.v[0]/255.0,
		color.v[1]/255.0,
		color.v[2]/255.0);

	glBegin (GL_QUADS);

	glVertex2f (x,y);
	glVertex2f (x+w, y);
	glVertex2f (x+w, y+h);
	glVertex2f (x, y+h);

	glEnd ();
	glColor3f (1,1,1);
	glEnable (GL_TEXTURE_2D);
}

void Draw_Fill2 (int x, int y, int w, int h, unsigned rgba)
{
	color_t	c;
	c.rgba = rgba;
	if (c.c[3] < 179) c.c[3] = 179;	// 179 == 0.7*255; because of alpha-test ...

	glEnable (GL_BLEND);
	glDisable (GL_TEXTURE_2D);
	glColor4ubv (c.c);
	glBegin (GL_QUADS);

	glVertex2f (x,y);
	glVertex2f (x+w, y);
	glVertex2f (x+w, y+h);
	glVertex2f (x, y+h);

	glEnd ();
	glColor4f (1,1,1,1);
	glEnable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);
}


//====================================================================


/*
=============
Draw_StretchRaw8
=============
*/
extern unsigned	r_rawpalette[256];

void Draw_StretchRaw8 (int x, int y, int w, int h, int cols, int rows, byte *data)
{
	unsigned	image32[256*256];
	int			i, j, trows;
	byte		*source;
	int			frac, fracstep;
	float		hscale;
	int			row;
	float		t;

	GL_Bind (0);

	if (rows<=256)
	{
		hscale = 1;
		trows = rows;
	}
	else
	{
		hscale = rows/256.0;
		trows = 256;
	}
	t = rows*hscale / 256;

//	if ( !glColorTableEXT )
	{
		unsigned *dest;

		for (i=0 ; i<trows ; i++)
		{
			row = (int)(i*hscale);
			if (row > rows)
				break;
			source = data + cols*row;
			dest = &image32[i*256];
			fracstep = cols*0x10000/256;
			frac = fracstep >> 1;
			for (j=0 ; j<256 ; j++)
			{
				dest[j] = r_rawpalette[source[frac>>16]];
				frac += fracstep;
			}
		}

		glTexImage2D (GL_TEXTURE_2D, 0, gl_tex_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);
	}
/*	else
	{
		unsigned char *dest;

		for (i=0 ; i<trows ; i++)
		{
			row = (int)(i*hscale);
			if (row > rows)
				break;
			source = data + cols*row;
			dest = &image8[i*256];
			fracstep = cols*0x10000/256;
			frac = fracstep >> 1;
			for (j=0 ; j<256 ; j++)
			{
				dest[j] = source[frac>>16];
				frac += fracstep;
			}
		}

		glTexImage2D( GL_TEXTURE_2D,
			           0,
					   GL_COLOR_INDEX8_EXT,
					   256, 256,
					   0,
					   GL_COLOR_INDEX,
					   GL_UNSIGNED_BYTE,
					   image8 );
	}
*/
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ( (gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION) )
		glDisable (GL_ALPHA_TEST);

	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex2f (x, y);
	glTexCoord2f (1, 0);
	glVertex2f (x+w, y);
	glTexCoord2f (1, t);
	glVertex2f (x+w, y+h);
	glTexCoord2f (0, t);
	glVertex2f (x, y+h);
	glEnd ();

	if ( (gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION) )
		glEnable (GL_ALPHA_TEST);

	gl_speeds.numUploads++;
}