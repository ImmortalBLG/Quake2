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
#include <ctype.h>
#include "client.h"
#include "../client/qmenu.h"

// dmflags->integer flags
//!! NOTE: all DF_XXX... values used in engine for dmflags edit menu only
// should remove this consts (place into a separate script for dmflags editor !!)
#define	DF_NO_HEALTH		0x00000001	// 1
#define	DF_NO_ITEMS			0x00000002	// 2
#define	DF_WEAPONS_STAY		0x00000004	// 4
#define	DF_NO_FALLING		0x00000008	// 8
#define	DF_INSTANT_ITEMS	0x00000010	// 16
#define	DF_SAME_LEVEL		0x00000020	// 32
#define DF_SKINTEAMS		0x00000040	// 64
#define DF_MODELTEAMS		0x00000080	// 128
#define DF_NO_FRIENDLY_FIRE	0x00000100	// 256
#define	DF_SPAWN_FARTHEST	0x00000200	// 512
#define DF_FORCE_RESPAWN	0x00000400	// 1024
#define DF_NO_ARMOR			0x00000800	// 2048
#define DF_ALLOW_EXIT		0x00001000	// 4096
#define DF_INFINITE_AMMO	0x00002000	// 8192
#define DF_QUAD_DROP		0x00004000	// 16384
#define DF_FIXED_FOV		0x00008000	// 32768

// XATRIX
#define	DF_QUADFIRE_DROP	0x00010000	// 65536

// ROGUE
#define DF_NO_MINES			0x00020000
#define DF_NO_STACK_DOUBLE	0x00040000
#define DF_NO_NUKES			0x00080000
#define DF_NO_SPHERES		0x00100000


static int	m_main_cursor;

#define NUM_CURSOR_FRAMES 15

#define MENU_CHECK	if (*re.flags & REF_CONSOLE_ONLY) return;

static char *menu_in_sound		= "misc/menu1.wav";
static char *menu_move_sound	= "misc/menu2.wav";
static char *menu_out_sound		= "misc/menu3.wav";

void M_Menu_Main_f (void);
	void M_Menu_Game_f (void);
		void M_Menu_LoadGame_f (void);
		void M_Menu_SaveGame_f (void);
		void M_Menu_PlayerConfig_f (void);
			void M_Menu_DownloadOptions_f (void);
		void M_Menu_Credits_f (void);
	void M_Menu_Multiplayer_f (void);
		void M_Menu_JoinServer_f (void);
			void M_Menu_AddressBook_f (void);
		void M_Menu_StartServer_f (void);
			void M_Menu_DMOptions_f (void);
			void M_Menu_DMBrowse_f (void);
	void M_Menu_Video_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
	void M_Menu_Quit_f (void);

	void M_Menu_Credits (void);

static bool m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound

static void	(*m_drawfunc) (void);
static const char *(*m_keyfunc) (int key);


/*---------- Support Routines ------------*/

#define	MAX_MENU_DEPTH	8


struct
{
	void	(*draw) (void);
	const char *(*key) (int k);
} m_layers[MAX_MENU_DEPTH];

int		m_menudepth;

static void M_Banner (char *name)
{
	int w, h;

	re.DrawGetPicSize (&w, &h, name);
	re.DrawPic ((viddef.width - w) / 2, viddef.height / 2 - 110, name);
}


void M_PushMenu (void (*draw) (void), const char *(*key) (int k))
{
	int		i;

	CL_Pause (true);

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for (i = 0; i < m_menudepth; i++)
		if (m_layers[i].draw == draw &&
			m_layers[i].key == key)
		{
			m_menudepth = i;	//!! if we use dynamic menus - need to free i+1..m_menudepth (check all m_menudepth changes !!)
			//?? add SetMenuDepth() function?
			break;
		}

	if (i == m_menudepth)
	{	// menu was not pushed
		if (m_menudepth >= MAX_MENU_DEPTH)
			Com_FatalError ("M_PushMenu: MAX_MENU_DEPTH");
		m_layers[m_menudepth].draw = m_drawfunc;
		m_layers[m_menudepth].key = m_keyfunc;
		m_menudepth++;
	}

	m_drawfunc = draw;
	m_keyfunc = key;

	m_entersound = true;

	cls.key_dest = key_menu;
}

void M_ForceMenuOff (void)
{
	if (DEDICATED) return;
	m_drawfunc = 0;
	m_keyfunc = 0;
	cls.key_dest = key_game;
	m_menudepth = 0;
	Key_ClearStates ();
	CL_Pause (false);
}

void M_ForceMenuOn (void)
{
	MENU_CHECK
	if (!m_menudepth) M_Menu_Main_f ();
}


void M_PopMenu (void)
{
	S_StartLocalSound (menu_out_sound);
	if (m_menudepth < 1)
		Com_FatalError ("M_PopMenu: depth < 1");
	m_menudepth--;

	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;

	if (!m_menudepth)
		M_ForceMenuOff ();
}


const char *Default_MenuKey (menuFramework_t *m, int key)
{
	const char *sound = NULL;

	if (m)
	{
		menuCommon_t *item;

		if ((item = Menu_ItemAtCursor (m)) != 0)
		{
			if (item->type == MTYPE_FIELD)
			{
				if (Field_Key ((menuField_t*) item, key))
					return NULL;
			}
		}
	}

	switch (key)
	{
	case K_ESCAPE:
	case K_MOUSE2:
		M_PopMenu ();
		return menu_out_sound;
	case K_KP_UPARROW:
	case K_UPARROW:
	case K_MWHEELUP:
		if (m)
		{
			m->cursor--;
			Menu_AdjustCursor (m, -1);
			sound = menu_move_sound;
		}
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
	case K_MWHEELDOWN:
		if (m)
		{
			m->cursor++;
			Menu_AdjustCursor (m, 1);
			sound = menu_move_sound;
		}
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		if (m)
		{
			Menu_SlideItem (m, -1);
			sound = menu_move_sound;
		}
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		if (m)
		{
			Menu_SlideItem (m, 1);
			sound = menu_move_sound;
		}
		break;
	case K_HOME:
		if (m)
		{
			m->cursor = 0;
			Menu_AdjustCursor (m, 1);
			sound = menu_move_sound;
		}
		break;
	case K_END:
		if (m)
		{
			m->cursor = m->nitems - 1;
			Menu_AdjustCursor (m, -1);
			sound = menu_move_sound;
		}
		break;

	case K_MOUSE1:
	case K_KP_ENTER:
	case K_ENTER:
		if (m)
			Menu_SelectItem (m);
		sound = menu_move_sound;
		break;
	}

	return sound;
}

//=============================================================================

/*
================
M_DrawCharacter

Draws one solid graphics character
cx and cy are in 320*240 coordinates, and will be centered on
higher res screens.
================
*/
void M_DrawCharacter (int cx, int cy, int num)
{
	re.DrawChar (cx + ((viddef.width - 320)>>1), cy + ((viddef.height - 240)>>1), num);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		re.DrawChar (cx + ((viddef.width - 320)>>1), cy + ((viddef.height - 240)>>1), *str++, C_RED);
		cx += 8;
	}
}

void M_DrawPic (int x, int y, char *pic)
{
	re.DrawPic (x + ((viddef.width - 320)>>1), y + ((viddef.height - 240)>>1), pic);
}


/*
=============
M_DrawCursor

Draws an animating cursor with the point at
x,y.  The pic will extend to the left of x,
and both above and below y.
=============
*/
void M_DrawCursor (int x, int y, int f)
{
/* -- disabled (will not work after vid_restart anyway)
	static bool cached;

	if (!cached)
	{
		int		i;

		for (i = 0; i < NUM_CURSOR_FRAMES; i++)
			re.RegisterPic (va("m_cursor%d", i));
		cached = true;
	}
*/
	re.DrawPic (x, y, va("m_cursor%d", f));
}

void M_DrawTextBox (int x, int y, int width, int lines)
{
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	M_DrawCharacter (cx, cy, 1);
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawCharacter (cx, cy, 4);
	}
	M_DrawCharacter (cx, cy+8, 7);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		M_DrawCharacter (cx, cy, 2);
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			M_DrawCharacter (cx, cy, 5);
		}
		M_DrawCharacter (cx, cy+8, 8);
		width -= 1;
		cx += 8;
	}

	// draw right side
	cy = y;
	M_DrawCharacter (cx, cy, 3);
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawCharacter (cx, cy, 6);
	}
	M_DrawCharacter (cx, cy+8, 9);
}


/*
=======================================================================

MAIN MENU

=======================================================================
*/
#define	MAIN_ITEMS	5


void M_Main_Draw (void)
{
	int		i, w, h;
	int		ystart, xoffset;
	int		widest = -1;
	int		totalheight = 0;
	char	litname[80];
	static char	*names[] =
	{
		"m_main_game",
		"m_main_multiplayer",
		"m_main_options",
		"m_main_video",
		"m_main_quit",
		NULL
	};

	// compute menu size
	for (i = 0; names[i]; i++)
	{
		re.DrawGetPicSize (&w, &h, names[i]);

		if (w > widest)
			widest = w;
		totalheight += (h + 12);
	}

	ystart = viddef.height / 2 - 110;
	xoffset = (viddef.width - widest + 70) / 2;

	// draw non-current items
	for (i = 0; names[i]; i++)
	{
		if (i != m_main_cursor)
			re.DrawPic (xoffset, ystart + i * 40 + 13, names[i]);
	}
	// draw current item
	strcpy (litname, names[m_main_cursor]);
	strcat (litname, "_sel");
	re.DrawPic (xoffset, ystart + m_main_cursor * 40 + 13, litname);

	M_DrawCursor (xoffset - 25, ystart + m_main_cursor * 40 + 11, (unsigned)appRound (cls.realtime / 100) % NUM_CURSOR_FRAMES);

	re.DrawGetPicSize (&w, &h, "m_main_plaque");
	re.DrawPic (xoffset - 30 - w, ystart, "m_main_plaque");

	re.DrawPic (xoffset - 30 - w, ystart + h + 5, "m_main_logo");
}


const char *M_Main_Key (int key)
{
	const char *sound = menu_move_sound;

	switch (key)
	{
	case K_ESCAPE:
	case K_MOUSE2:
		M_PopMenu ();
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
	case K_MWHEELDOWN:
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		return sound;

	case K_KP_UPARROW:
	case K_UPARROW:
	case K_MWHEELUP:
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		return sound;

	case K_HOME:
		m_main_cursor = 0;
		return sound;

	case K_END:
		m_main_cursor = MAIN_ITEMS - 1;
		return sound;

	case K_KP_ENTER:
	case K_ENTER:
	case K_MOUSE1:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_Game_f ();
			break;
		case 1:
			M_Menu_Multiplayer_f ();
			break;
		case 2:
			M_Menu_Options_f ();
			break;
		case 3:
			M_Menu_Video_f ();
			break;
		case 4:
			M_Menu_Quit_f ();
			break;
		}
	}

	return NULL;
}


void M_Menu_Main_f (void)
{
	MENU_CHECK
	M_ForceMenuOff ();
	M_PushMenu (M_Main_Draw, M_Main_Key);
}

/*
=======================================================================

MULTIPLAYER MENU

=======================================================================
*/
static menuFramework_t	s_multiplayer_menu;
static menuAction_t		s_join_network_server_action;
static menuAction_t		s_start_network_server_action;
static menuAction_t		s_player_setup_action;
static menuAction_t		s_multiplayer_disconnect_action;

static void Multiplayer_MenuDraw (void)
{
	M_Banner ("m_banner_multiplayer");

	Menu_AdjustCursor (&s_multiplayer_menu, 1);
	Menu_Draw (&s_multiplayer_menu);
}

static void Multiplayer_Disconnect (void)
{
	if (Com_ServerState())
		Cbuf_AddText ("disconnect\n");
	M_ForceMenuOff ();
}

static void Multiplayer_MenuInit (void)
{
	s_multiplayer_menu.x = viddef.width * 0.50 - 64;
	s_multiplayer_menu.nitems = 0;

	MENU_ACTION(s_join_network_server_action,0,"join network server",M_Menu_JoinServer_f)
	s_join_network_server_action.generic.flags  = QMF_LEFT_JUSTIFY;

	MENU_ACTION(s_start_network_server_action,10,"start network server",M_Menu_StartServer_f)
	s_start_network_server_action.generic.flags  = QMF_LEFT_JUSTIFY;

	MENU_ACTION(s_multiplayer_disconnect_action,20,"disconnect",Multiplayer_Disconnect)
	s_multiplayer_disconnect_action.generic.flags  = QMF_LEFT_JUSTIFY;

	MENU_ACTION(s_player_setup_action,30,"player setup",M_Menu_PlayerConfig_f)
	s_player_setup_action.generic.flags  = QMF_LEFT_JUSTIFY;

	Menu_AddItem (&s_multiplayer_menu, (void *) &s_join_network_server_action);
	Menu_AddItem (&s_multiplayer_menu, (void *) &s_start_network_server_action);
	Menu_AddItem (&s_multiplayer_menu, (void *) &s_multiplayer_disconnect_action);
	Menu_AddItem (&s_multiplayer_menu, (void *) &s_player_setup_action);

	Menu_SetStatusBar (&s_multiplayer_menu, NULL);

	Menu_Center (&s_multiplayer_menu);
}

static const char *Multiplayer_MenuKey (int key)
{
	return Default_MenuKey (&s_multiplayer_menu, key);
}

void M_Menu_Multiplayer_f (void)
{
	MENU_CHECK
	Multiplayer_MenuInit ();
	M_PushMenu (Multiplayer_MenuDraw, Multiplayer_MenuKey);
}

/*
=======================================================================

KEYS MENU

=======================================================================
*/

static int	keys_cursor;

static menuFramework_t	s_keys_menu;

#define MAX_KEYS_MENU 128
#define MAX_KEYS_BUF  16384

#define BIND_LINE_HEIGHT	10

static menuAction_t s_keys_item[MAX_KEYS_MENU];
static char *bindnames[MAX_KEYS_MENU][2];
static char bindmenubuf[MAX_KEYS_BUF];

static void M_UnbindCommand (char *command)
{
	int		i, numKeys, keys[256];

	numKeys = Key_FindBinding (command, ARRAY_ARG(keys));
	for (i = 0; i < numKeys; i++)
	{
		Key_SetBinding (keys[i], NULL);
//		Com_DPrintf ("unbind %s\n",Key_KeynumToString(keys[i]));
	}
}


static void KeyCursorDrawFunc (menuFramework_t *menu)
{
	re.DrawChar (menu->x, menu->y + menu->cursor * BIND_LINE_HEIGHT,
		cls.key_dest == key_bindingMenu ? '=' : 12 + (Sys_Milliseconds() / 250 & 1));
}

static void DrawKeyBindingFunc (void *self)
{
	int		keys[2], numKeys;
	menuAction_t *a = (menuAction_t *) self;
	char	text[256];

//	M_FindKeysForCommand (bindnames[a->generic.localdata[0]][0], keys); !!! remove
	numKeys = Key_FindBinding (bindnames[a->generic.localdata[0]][0], ARRAY_ARG(keys));

	if (!numKeys)
		strcpy (text, S_RED"(not bound)");
	else
	{
		if (numKeys == 1)
			strcpy (text, Key_KeynumToString (keys[0]));
		else
			Com_sprintf (ARRAY_ARG(text), "%s or %s%s",
				Key_KeynumToString (keys[0]), Key_KeynumToString (keys[1]), numKeys > 2 ? " ..." : "");
	}
	Menu_DrawString (a->generic.x + a->generic.parent->x + 16, a->generic.y + a->generic.parent->y, text);
}

static void SeekLine (char **s)
{
	char c1, c2;
	c1 = **s;								// remember line delimiter (CR or LF)
	if (c1 != '\n' && c1 != '\r') return;	// it was not line delimiter!
	while (1)
	{
		c2 = *(++(*s));
		if (c1 == c2) return;				// next end of line
		if (c2 != '\n' && c2 != '\r') return; // next line text
	}
}

static void AppendToken (char **dest, char **src)
{
	char *tok;
	tok = COM_Parse (src);
	strcpy (*dest, tok);
	*dest += strlen (tok) + 1;
}

static void Keys_MenuInit (void)
{
	int		i, numbinds, y;
	unsigned length;
	char	*buffer, *s, *d, c;

	//?? change parser
	// load "binds.lst" (file always present - have inline file)
	buffer = (char*) FS_LoadFile ("binds.lst", &length);
	// parse this file
	s = buffer;
	numbinds = 0;
	for (i = 0; i < length; i++)
		if (s[i] == '\n') numbinds++;
	d = bindmenubuf;
	s = buffer;
	for (i = 0; i < numbinds; i++)
	{
		bindnames[i][0] = "";
		bindnames[i][1] = "";
		while ((c = *s) == ' ') s++;		// skip whitespace
		if (c == '\r' || c == '\n')
		{
			SeekLine (&s);
			continue;	// empty line
		}
		bindnames[i][0] = d;				// remember place for command name
		AppendToken (&d, &s);
		while ((c = *s) == ' ') s++;
		if (c == '\r' || c == '\n')
		{	// EOLN => command = menu string
			bindnames[i][1] = bindnames[i][0];
			SeekLine (&s);
			continue;
		}
		bindnames[i][1] = d;
		AppendToken (&d, &s);
		while (*s != '\n') s++;				// was '\r'
		SeekLine (&s);
	}

	// unload "binds.lst"
	FS_FreeFile (buffer);

	// build menu
	y = 0;

	s_keys_menu.x = viddef.width / 2;
	s_keys_menu.nitems = 0;
	s_keys_menu.cursordraw = KeyCursorDrawFunc;

	for (i = 0; i < numbinds; i++)
	{
		if (*bindnames[i][0])
		{
			s_keys_item[i].generic.type = MTYPE_ACTION;
			s_keys_item[i].generic.flags = QMF_GRAYED;
		}
		else
		{
			s_keys_item[i].generic.type = MTYPE_SEPARATOR;
			s_keys_item[i].generic.flags = QMF_CENTER;
		}
		s_keys_item[i].generic.x = 0;
		s_keys_item[i].generic.y = y; y += BIND_LINE_HEIGHT;
		s_keys_item[i].generic.ownerdraw = DrawKeyBindingFunc;
		s_keys_item[i].generic.localdata[0] = i;
		s_keys_item[i].generic.name = bindnames[i][1];
		Menu_AddItem (&s_keys_menu, &s_keys_item[i]);
	}

	Menu_SetStatusBar (&s_keys_menu, "enter to change, backspace to clear");
	Menu_Center (&s_keys_menu);
}

static void Keys_MenuDraw (void)
{
	Menu_AdjustCursor (&s_keys_menu, 1);
	Menu_Draw (&s_keys_menu);
}

static void KeyBindingFunc (void *self)
{
	menuAction_t *a = (menuAction_t *) self;

	if (Key_FindBinding (bindnames[a->generic.localdata[0]][0], NULL, 0) > 1)
		M_UnbindCommand (bindnames[a->generic.localdata[0]][0]);

	cls.key_dest = key_bindingMenu;		// hook keyboard

	Menu_SetStatusBar (&s_keys_menu, "press a key or button for this action");
}

static const char *Keys_MenuKey (int key)
{
	menuAction_t *item;

	item = (menuAction_t *) Menu_ItemAtCursor (&s_keys_menu);
	if (cls.key_dest == key_bindingMenu)
	{
		if (key != K_ESCAPE && key != '`')
			Key_SetBinding (key, COM_QuoteString (bindnames[item->generic.localdata[0]][0], true));
//			Cbuf_InsertText (va("bind %s %s\n", Key_KeynumToString(key), COM_QuoteString (bindnames[item->generic.localdata[0]][0], true)));

		Menu_SetStatusBar (&s_keys_menu, "enter to change, backspace to clear");
		cls.key_dest = key_menu;		// unhook keyboard
		return menu_out_sound;
	}

	switch (key)
	{
	case K_KP_ENTER:
	case K_ENTER:
	case K_MOUSE1:
		KeyBindingFunc (item);
		return menu_in_sound;
	case K_BACKSPACE:		// delete bindings
	case K_DEL:
	case K_KP_DEL:
		M_UnbindCommand (bindnames[item->generic.localdata[0]][0]);
		return menu_out_sound;
	default:
		return Default_MenuKey (&s_keys_menu, key);
	}
}

void M_Menu_Keys_f (void)
{
	MENU_CHECK
	Keys_MenuInit ();
	M_PushMenu (Keys_MenuDraw, Keys_MenuKey);
}


/*
=======================================================================

CONTROLS MENU

=======================================================================
*/
//static cvar_t *win_noalttab;
extern cvar_t *in_joystick;

static menuFramework_t	s_options_menu;
static menuAction_t		s_options_defaults_action;
static menuAction_t		s_options_customize_options_action;
static menuSlider_t		s_options_sensitivity_slider;
static menuList_t		s_options_freelook_box;
//static menuList_t		s_options_noalttab_box;
static menuList_t		s_options_alwaysrun_box;
static menuList_t		s_options_invertmouse_box;
static menuList_t		s_options_lookspring_box;
static menuList_t		s_options_lookstrafe_box;
static menuList_t		s_options_crosshair_box;
static menuList_t		s_options_crosshair_color_box;
static menuSlider_t		s_options_sfxvolume_slider;
static menuList_t		s_options_joystick_box;
static menuList_t		s_options_cdvolume_box;
static menuList_t		s_options_s_khz;
static menuList_t		s_options_s_8bit;
static menuList_t		s_options_s_reverse;
static menuList_t		s_options_compatibility_list;

static int crosshairs_count;

static void CrosshairFunc( void *unused )
{
	Cvar_SetInteger ("crosshair", s_options_crosshair_box.curvalue);
	Cvar_SetInteger ("crosshaircolor", s_options_crosshair_color_box.curvalue);
}

static void JoystickFunc (void *unused)
{
	Cvar_SetInteger ("in_joystick", s_options_joystick_box.curvalue);
}

static void CustomizeControlsFunc (void *unused)
{
	M_Menu_Keys_f ();
}

static void AlwaysRunFunc (void *unused)
{
	Cvar_SetInteger ("cl_run", s_options_alwaysrun_box.curvalue);
}

static void FreeLookFunc (void *unused)
{
	Cvar_SetInteger ("freelook", s_options_freelook_box.curvalue);
}

static void MouseSpeedFunc (void *unused)
{
	Cvar_SetValue ("sensitivity", s_options_sensitivity_slider.curvalue / 2.0f);
}

/*
static void NoAltTabFunc( void *unused )
{
	Cvar_SetInteger ("win_noalttab", s_options_noalttab_box.curvalue);
}
*/

static void ControlsSetMenuItemValues (void)
{
	int		i;

	s_options_sfxvolume_slider.curvalue		= Cvar_VariableValue ("s_volume") * 10;
	s_options_cdvolume_box.curvalue 		= !Cvar_VariableInt ("cd_nocd");
	s_options_s_8bit.curvalue				= Cvar_VariableInt ("s_loadas8bit");
	i = Cvar_VariableInt ("s_khz");
	s_options_s_khz.curvalue				= i == 11 ? 0 : i == 22 ? 1 : 2;
	s_options_s_reverse.curvalue			= Cvar_VariableInt ("s_reverse_stereo");

	s_options_sensitivity_slider.curvalue	= sensitivity->value * 2;

	Cvar_Clamp (cl_run, 0, 1);
	s_options_alwaysrun_box.curvalue		= cl_run->integer;

	s_options_invertmouse_box.curvalue		= m_pitch->value < 0;

	Cvar_Clamp (lookspring, 0, 1);
	s_options_lookspring_box.curvalue		= lookspring->integer;

	Cvar_Clamp (lookstrafe, 0, 1);
	s_options_lookstrafe_box.curvalue		= lookstrafe->integer;

	Cvar_Clamp (freelook, 0, 1);
	s_options_freelook_box.curvalue			= freelook->integer;

	Cvar_Clamp (crosshair, 0, crosshairs_count);
	s_options_crosshair_box.curvalue		= crosshair->integer;

	Cvar_Clamp (crosshaircolor, 0, 7);
	s_options_crosshair_color_box.curvalue	= crosshaircolor->integer;

	Cvar_Clamp (in_joystick, 0, 1);
	s_options_joystick_box.curvalue			= in_joystick->integer;

//	s_options_noalttab_box.curvalue			= win_noalttab->integer;
}

static void ControlsResetDefaultsFunc( void *unused )
{
	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_Execute ();

	ControlsSetMenuItemValues ();
}

static void InvertMouseFunc (void *unused)
{
	Cvar_SetValue ("m_pitch", -m_pitch->value);
}

static void LookspringFunc (void *unused)
{
	Cvar_SetInteger ("lookspring", !lookspring->integer);
}

static void LookstrafeFunc (void *unused)
{
	Cvar_SetInteger ("lookstrafe", !lookstrafe->integer);
}

static void UpdateVolumeFunc (void *unused)
{
	Cvar_SetValue ("s_volume", s_options_sfxvolume_slider.curvalue / 10);
}

static void UpdateCDVolumeFunc (void *unused)
{
	Cvar_SetInteger ("cd_nocd", !s_options_cdvolume_box.curvalue);
}

static void UpdateSoundFunc (void *unused)
{
	int		khz[] = {11, 22, 44};

	Cvar_SetInteger ("s_loadas8bit", s_options_s_8bit.curvalue);
	Cvar_SetInteger ("s_khz", khz[s_options_s_khz.curvalue]);
	Cvar_SetInteger ("s_reverse_stereo", s_options_s_reverse.curvalue);

	Cvar_SetInteger ("s_primary", s_options_compatibility_list.curvalue);

	M_DrawTextBox (8, 120 - 48, 36, 3 );
	M_Print (16 + 16, 120 - 48 + 8,  "Restarting the sound system. This");
	M_Print (16 + 16, 120 - 48 + 16, "could take up to a minute, so");
	M_Print (16 + 16, 120 - 48 + 24, "please be patient.");

	// the text box won't show up unless we do a buffer swap
	re.EndFrame ();

	CL_Snd_Restart_f ();
}


#define MAX_CROSSHAIRS 256
static const char *crosshair_names[MAX_CROSSHAIRS + 1] = {S_RED"(none)"};	// reserve last item for NULL
static const char *crosshair_color_names[9] = {"", "", "", "", "", "", "", "", NULL};

static void Options_ScanCrosshairs (void)
{
	int		i;

	for (i = 1; i <= MAX_CROSSHAIRS; i++)	// item [0] is "none"
	{
		if (!ImageExists (va("pics/ch%d", i), IMAGE_ANY))
		{
			crosshair_names[i] = NULL;
			break;
		}
		crosshair_names[i] = "";
	}
	crosshair_names[i] = NULL;
	crosshairs_count = i - 1;
}

static void Options_MenuInit( void )
{
	int		y;

	static const char *cd_music_items[] = {
		"disabled", "enabled", NULL
	};

	static const char *s_khz_items[] = {
		"11 khz", "22 khz", "44 khz", NULL
	};

	static const char *compatibility_items[] = {
		"max compatibility", "max performance", NULL
	};

	static const char *yesno_names[] = {
		"no", "yes", NULL
	};

//	win_noalttab = Cvar_Get( "win_noalttab", "0", CVAR_ARCHIVE );

	/*------- configure controls menu and menu items -------*/

	s_options_menu.x = viddef.width / 2;
	s_options_menu.y = viddef.height / 2 - 58;
	s_options_menu.nitems = 0;
	y = 0;

	MENU_SLIDER(s_options_sfxvolume_slider,y,"effects volume",UpdateVolumeFunc,0,10)
	MENU_SPIN(s_options_cdvolume_box,y+=10,"CD music",UpdateCDVolumeFunc,cd_music_items)
	MENU_SPIN(s_options_s_khz,y+=10,"sound quality",UpdateSoundFunc,s_khz_items)
	MENU_SPIN(s_options_s_8bit,y+=10,"8 bit sound",UpdateSoundFunc,yesno_names)
	MENU_SPIN(s_options_s_reverse,y+=10,"reverse stereo",UpdateSoundFunc,yesno_names)
	MENU_SPIN(s_options_compatibility_list,y+=10,"sound compatibility",UpdateSoundFunc,compatibility_items)
	s_options_compatibility_list.curvalue = Cvar_VariableInt ("s_primary");
	y += 10;
	MENU_SLIDER(s_options_sensitivity_slider,y+=10,"mouse speed",MouseSpeedFunc,2,22)
	MENU_SPIN(s_options_alwaysrun_box,y+=10,"always run",AlwaysRunFunc,yesno_names)
	MENU_SPIN(s_options_invertmouse_box,y+=10,"invert mouse",InvertMouseFunc,yesno_names)
	MENU_SPIN(s_options_lookspring_box,y+=10,"lookspring",LookspringFunc,yesno_names)
	MENU_SPIN(s_options_lookstrafe_box,y+=10,"lookstrafe",LookstrafeFunc,yesno_names)
	MENU_SPIN(s_options_freelook_box,y+=10,"free look",FreeLookFunc,yesno_names)
	y += 10;
	MENU_SPIN(s_options_crosshair_box,y+=10,"crosshair shape",CrosshairFunc,crosshair_names)
	MENU_SPIN(s_options_crosshair_color_box,y+=10,"crosshair color",CrosshairFunc,crosshair_color_names)
	y += 10;
//	MENU_SPIN(s_options_noalttab_box,y+=10,"disable alt-tab",NoAltTabFun,yesno+names)
	MENU_SPIN(s_options_joystick_box,y+=10,"use joystick",JoystickFunc,yesno_names)
	y += 10;
	MENU_ACTION(s_options_customize_options_action,y+=10,"customize controls",CustomizeControlsFunc)
	MENU_ACTION(s_options_defaults_action,y+=10,S_RED"reset defaults",ControlsResetDefaultsFunc)

	Options_ScanCrosshairs ();
	ControlsSetMenuItemValues ();

	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sfxvolume_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_cdvolume_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_s_khz );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_s_8bit );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_s_reverse );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_compatibility_list );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sensitivity_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_alwaysrun_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_invertmouse_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_lookspring_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_lookstrafe_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_freelook_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_crosshair_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_crosshair_color_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_joystick_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_customize_options_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_defaults_action );
}

static void Options_CrosshairDraw (void)
{
	int		w, h;
	char	name[32];

	if (!crosshair->integer)
		return;

	Com_sprintf (ARRAY_ARG(name), "ch%d", crosshair->integer);
	re.DrawGetPicSize (&w, &h, name);
	re.DrawPic ((viddef.width - w) / 2 + 32, s_options_crosshair_box.generic.y + s_options_menu.y + 10 - h / 2,
		name, crosshaircolor->integer);
}

static void Options_MenuDraw (void)
{
	M_Banner ("m_banner_options");
	Options_CrosshairDraw ();
	Menu_AdjustCursor (&s_options_menu, 1);
	Menu_Draw (&s_options_menu);
}

static const char *Options_MenuKey( int key )
{
	return Default_MenuKey( &s_options_menu, key );
}

void M_Menu_Options_f (void)
{
	MENU_CHECK
	Options_MenuInit();
	M_PushMenu ( Options_MenuDraw, Options_MenuKey );
}

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

void M_Menu_Video_f (void)
{
	MENU_CHECK
	Vid_MenuInit ();
	M_PushMenu (Vid_MenuDraw, Vid_MenuKey);
}

/*
=============================================================================

END GAME MENU

=============================================================================
*/
static int credits_start_time;
static char *credits[256];
static char *creditsBuffer;


static void M_Credits_MenuDraw (void)
{
	int		i, y;

	y = viddef.height - (cls.realtime - credits_start_time) / 40;
	for (i = 0; credits[i] && y < viddef.height; y += 10, i++)
	{
		int		j, stringoffset;
		bool	bold;

		if (y <= -8)
			continue;

		if (credits[i][0] == '+')
		{
			bold = true;
			stringoffset = 1;
		}
		else
		{
			bold = false;
			stringoffset = 0;
		}

		for (j = 0; credits[i][j+stringoffset]; j++)
		{
			int		x;

			x = (viddef.width - strlen (credits[i]) * 8 - stringoffset * 8) / 2 + (j + stringoffset) * 8;
			re.DrawChar (x, y, credits[i][j+stringoffset], bold ? C_GREEN : C_WHITE);
		}
	}

	if (y < 0)
		credits_start_time = cls.realtime;
}

static const char *M_Credits_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_MOUSE1:
	case K_MOUSE2:
		if (creditsBuffer)
			FS_FreeFile (creditsBuffer);
		M_PopMenu ();
		break;
	}

	return menu_out_sound;

}


void M_Menu_Credits_f (void)
{
	int		n;
	unsigned count;
	char	*p, *filename;

	MENU_CHECK

	filename = "credits";
	if (!FS_FileExists (filename))
	{
		// official mission packe are not provided own credits file - display defaults
		if (!stricmp (fs_gamedirvar->string, "xatrix"))
			filename = "xcredits";
		else if (!stricmp (fs_gamedirvar->string, "rogue"))
			filename = "rcredits";
	}

	creditsBuffer = (char*) FS_LoadFile (filename, &count);
	// file always present - have inline file
	p = creditsBuffer;
	for (n = 0; n < 255; n++)
	{
		credits[n] = p;
		while (*p != '\r' && *p != '\n')
		{
			p++;
			if (--count == 0)
				break;
		}
		if (*p == '\r')
		{
			*p++ = 0;
			if (--count == 0)
				break;
		}
		*p++ = 0;
		if (--count == 0)
			break;
	}
	credits[++n] = NULL;

	credits_start_time = cls.realtime;
	M_PushMenu (M_Credits_MenuDraw, M_Credits_Key);
}

/*
=============================================================================

GAME MENU

=============================================================================
*/

static int		m_game_cursor;

static menuFramework_t	s_game_menu;
static menuAction_t		s_easy_game_action;
static menuAction_t		s_medium_game_action;
static menuAction_t		s_hard_game_action;
static menuAction_t		s_load_game_action;
static menuAction_t		s_save_game_action;
static menuAction_t		s_credits_action;

static void StartGame (void)
{
	// disable updates and start the cinematic going
	cl.servercount = -1;
	M_ForceMenuOff ();
	Cvar_SetInteger ("deathmatch", 0);
	Cvar_SetInteger ("coop", 0);

	Cvar_SetInteger ("gamerules", 0);		//PGM

	Cbuf_AddText ("killserver\nloading\nwait\nnewgame\n");
	cls.key_dest = key_game;
}

static void EasyGameFunc (void *data)
{
	Cvar_ForceSet ("skill", "0");
	StartGame ();
}

static void MediumGameFunc (void *data)
{
	Cvar_ForceSet ("skill", "1");
	StartGame ();
}

static void HardGameFunc (void *data)
{
	Cvar_ForceSet ("skill", "2");
	StartGame ();
}

static void Game_MenuInit( void )
{
	static const char *difficulty_names[] = {
		"easy", "medium", "hard", NULL
	};

	s_game_menu.x = viddef.width * 0.50;
	s_game_menu.nitems = 0;

	MENU_ACTION(s_easy_game_action,0,"easy",EasyGameFunc)
	s_easy_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	MENU_ACTION(s_medium_game_action,10,"medium",MediumGameFunc)
	s_medium_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	MENU_ACTION(s_hard_game_action,20,"hard",HardGameFunc)
	s_hard_game_action.generic.flags  = QMF_LEFT_JUSTIFY;

	MENU_ACTION(s_load_game_action,40,"load game",M_Menu_LoadGame_f)
	s_load_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	MENU_ACTION(s_save_game_action,50,"save game",M_Menu_SaveGame_f)
	s_save_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	MENU_ACTION(s_credits_action,60,"credits",M_Menu_Credits_f)
	s_credits_action.generic.flags  = QMF_LEFT_JUSTIFY;

	Menu_AddItem( &s_game_menu, ( void * ) &s_easy_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_medium_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_hard_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_load_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_save_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_credits_action );

	Menu_Center( &s_game_menu );
}

static void Game_MenuDraw (void)
{
	M_Banner ("m_banner_game");
	Menu_AdjustCursor (&s_game_menu, 1);
	Menu_Draw (&s_game_menu);
}

static const char *Game_MenuKey (int key)
{
	return Default_MenuKey (&s_game_menu, key);
}

void M_Menu_Game_f (void)
{
	MENU_CHECK
	Game_MenuInit ();
	M_PushMenu (Game_MenuDraw, Game_MenuKey);
	m_game_cursor = 1;
}

/*
=============================================================================

LOADGAME MENU

=============================================================================
*/

#define	MAX_SAVEGAMES	15

static menuFramework_t	s_savegame_menu;			// used later

static menuFramework_t	s_loadgame_menu;
static menuAction_t		s_loadgame_actions[MAX_SAVEGAMES];

static char		m_savestrings[MAX_SAVEGAMES][32+1];	// reserve 1 byte for 0 (for overflowed names)
static bool		m_savevalid[MAX_SAVEGAMES];
static bool		m_shotvalid[MAX_SAVEGAMES];

static void Create_Savestrings (void)
{
	int		i;
	FILE	*f;

	memset (m_savevalid, 0, sizeof(m_savevalid));
	memset (m_shotvalid, 0, sizeof(m_shotvalid));
	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		f = fopen (va("%s/" SAVEGAME_DIRECTORY "/save%d/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir (), i), "rb");
		if (!f)
			strcpy (m_savestrings[i], "<EMPTY>");
		else
		{
			char	*name;
			int		width;

			fread (m_savestrings[i], sizeof(m_savestrings[i])-1, 1, f);
			fclose (f);
			m_savevalid[i] = true;
			name = va("/" SAVEGAME_DIRECTORY "/save%d/shot.pcx", i);
			re.ReloadImage (name);		// force renderer to refresh image
			re.DrawGetPicSize (&width, NULL, name);
			if (width) m_shotvalid[i] = true;
		}
	}
}

static void DrawSavegameShot (int index, int y)
{
	char	*name;
	int		x, w, h;

	name = va("/" SAVEGAME_DIRECTORY "/save%d/shot.pcx", index);
	if (viddef.width >= 320)
	{
		x = viddef.width / 2 + 20;
		w = viddef.width / 4 + 20;
		h = w * 3 / 4;
	}
	else
	{
		x = viddef.width / 10;
		w = viddef.width * 8 / 10;
		h = w * 3 / 4;
		y = (viddef.height - h) / 2;
	}
	re.DrawFill2 (x - 3, y - 3, w + 6, h + 6, RGB(0,0,0));

	if (!m_shotvalid[index]) return;
	re.DrawStretchPic (x, y, w, h, name);
}


static void LoadGameCallback (void *self)
{
	menuAction_t *a = (menuAction_t *) self;

	if (m_savevalid[a->generic.localdata[0]])
		Cbuf_AddText (va("load save%d\n",  a->generic.localdata[0]));
	M_ForceMenuOff ();
}

static void LoadGame_MenuInit (void)
{
	int i;

	s_loadgame_menu.x = viddef.width / 2 - 120 - (viddef.width - 320) / 6;
	s_loadgame_menu.y = viddef.height / 2 - 58;
	s_loadgame_menu.nitems = 0;

	Create_Savestrings ();

	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		s_loadgame_actions[i].generic.name			= m_savestrings[i];
		s_loadgame_actions[i].generic.flags			= QMF_LEFT_JUSTIFY;
		s_loadgame_actions[i].generic.localdata[0]	= i;
		s_loadgame_actions[i].generic.callback		= LoadGameCallback;

		s_loadgame_actions[i].generic.x = 0;
		s_loadgame_actions[i].generic.y = i * 10;
		if (i > 0)	// separate from autosave
			s_loadgame_actions[i].generic.y += 10;

		s_loadgame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem (&s_loadgame_menu, &s_loadgame_actions[i]);
	}
}

static void LoadGame_MenuDraw( void )
{
//	Menu_AdjustCursor (&s_loadgame_menu, 1);

	DrawSavegameShot (s_loadgame_menu.cursor, s_loadgame_menu.y);
	M_Banner ("m_banner_load_game");
	Menu_Draw (&s_loadgame_menu);
}

static const char *LoadGame_MenuKey (int key)
{
	if (key == K_ESCAPE || key == K_ENTER)
	{
		s_savegame_menu.cursor = s_loadgame_menu.cursor - 1;
		if (s_savegame_menu.cursor < 0)
			s_savegame_menu.cursor = 0;
	}
	return Default_MenuKey (&s_loadgame_menu, key);
}

void M_Menu_LoadGame_f (void)
{
	MENU_CHECK
	LoadGame_MenuInit ();
	M_PushMenu (LoadGame_MenuDraw, LoadGame_MenuKey);
}


/*
=============================================================================

SAVEGAME MENU

=============================================================================
*/
//static menuFramework_t	s_savegame_menu;	-- declared with loadgame menu
static menuAction_t		s_savegame_actions[MAX_SAVEGAMES];

static void SaveGameCallback( void *self )
{
	menuAction_t *a = ( menuAction_t * ) self;

	Cbuf_AddText (va("save save%d\n", a->generic.localdata[0] ));
	M_ForceMenuOff ();
}

static void SaveGame_MenuDraw (void)
{
	Menu_AdjustCursor (&s_savegame_menu, 1);

	DrawSavegameShot (s_savegame_menu.cursor + 1, s_savegame_menu.y);
	M_Banner ("m_banner_save_game");
	Menu_Draw (&s_savegame_menu);
}

static void SaveGame_MenuInit (void)
{
	int i;

	s_savegame_menu.x = viddef.width / 2 - 120 - (viddef.width - 320) / 6;
	s_savegame_menu.y = viddef.height / 2 - 58;
	s_savegame_menu.nitems = 0;

	Create_Savestrings();

	// don't include the autosave slot
	for (i = 0; i < MAX_SAVEGAMES-1; i++)
	{
		s_savegame_actions[i].generic.name = m_savestrings[i+1];
		s_savegame_actions[i].generic.localdata[0] = i+1;
		s_savegame_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_savegame_actions[i].generic.callback = SaveGameCallback;

		s_savegame_actions[i].generic.x = 0;
		s_savegame_actions[i].generic.y = ( i ) * 10;

		s_savegame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem( &s_savegame_menu, &s_savegame_actions[i] );
	}
}

static const char *SaveGame_MenuKey( int key )
{
	if ( key == K_ENTER || key == K_ESCAPE || key == K_MOUSE1 || key == K_MOUSE2 )
	{
		s_loadgame_menu.cursor = s_savegame_menu.cursor - 1;
		if ( s_loadgame_menu.cursor < 0 )
			s_loadgame_menu.cursor = 0;
	}
	return Default_MenuKey( &s_savegame_menu, key );
}

void M_Menu_SaveGame_f (void)
{
	MENU_CHECK
	if (!Com_ServerState())
		return;		// not playing a game

	SaveGame_MenuInit();
	M_PushMenu( SaveGame_MenuDraw, SaveGame_MenuKey );
	Create_Savestrings ();
}


/*
=============================================================================

JOIN SERVER MENU

=============================================================================
*/
#define MAX_LOCAL_SERVERS 8

static menuFramework_t	s_joinserver_menu;
static menuSeparator_t	s_joinserver_server_title;
static menuAction_t		s_joinserver_search_action;
static menuAction_t		s_joinserver_address_book_action;
static menuAction_t		s_joinserver_server_actions[MAX_LOCAL_SERVERS];

static int		m_num_servers;
#define	NO_SERVER_STRING	"<no server>"

// user readable information
static char local_server_names[MAX_LOCAL_SERVERS][80];

// network address
static netadr_t local_server_netadr[MAX_LOCAL_SERVERS];

void M_AddToServerList (netadr_t adr, char *info)
{
	int		i;

	if (m_num_servers == MAX_LOCAL_SERVERS) return;

	while (*info == ' ') info++;

	// ignore if duplicated
	for (i = 0; i < m_num_servers; i++)
		if (!strcmp (info, local_server_names[i]))
			return;

	local_server_netadr[m_num_servers] = adr;
	Q_strncpyz (local_server_names[m_num_servers], info, sizeof(local_server_names[0]));
	m_num_servers++;
}


static void JoinServerFunc( void *self )
{
	int		index;

	index = ( menuAction_t * ) self - s_joinserver_server_actions;

	if (!stricmp (local_server_names[index], NO_SERVER_STRING))
		return;

	if (index >= m_num_servers)
		return;

	Cbuf_AddText (va("connect %s\n", NET_AdrToString (&local_server_netadr[index])));
	M_ForceMenuOff ();
}

static void NullCursorDraw( void *self )
{
}

static void SearchLocalGames (void)
{
	int		i;

	m_num_servers = 0;
	for (i = 0; i < MAX_LOCAL_SERVERS; i++)
		strcpy (local_server_names[i], NO_SERVER_STRING);

	M_DrawTextBox (8, 120 - 48, 36, 3);
	M_Print (16 + 16, 120 - 48 + 8,  "Searching for local servers, this");
	M_Print (16 + 16, 120 - 48 + 16, "could take up to a minute, so");
	M_Print (16 + 16, 120 - 48 + 24, "please be patient.");

	// the text box won't show up unless we do a buffer swap
	re.EndFrame ();

	// send out info packets
	CL_PingServers_f ();
}

static void JoinServer_MenuInit( void )
{
	int		i, y;

	s_joinserver_menu.x = viddef.width * 0.50 - 120;
	s_joinserver_menu.nitems = 0;

	s_joinserver_server_title.generic.type = MTYPE_SEPARATOR;
	s_joinserver_server_title.generic.name = "connect to...";
	s_joinserver_server_title.generic.x    = 80;
	s_joinserver_server_title.generic.y	   = 0;

	y = 0;
	for (i = 0; i < MAX_LOCAL_SERVERS; i++)
	{
		strcpy (local_server_names[i], NO_SERVER_STRING);
		MENU_ACTION(s_joinserver_server_actions[i],y+=10,local_server_names[i],JoinServerFunc)
		s_joinserver_server_actions[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_joinserver_server_actions[i].generic.statusbar = "press ENTER to connect";
	}
	MENU_ACTION(s_joinserver_address_book_action,y+=20,"address book",M_Menu_AddressBook_f)
	s_joinserver_address_book_action.generic.flags	= QMF_LEFT_JUSTIFY;

	MENU_ACTION(s_joinserver_search_action,y+=10,"search for servers",SearchLocalGames)
	s_joinserver_search_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_search_action.generic.statusbar = "search for servers";


	for (i = 0; i < 8; i++)
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_actions[i] );
	Menu_AddItem (&s_joinserver_menu, &s_joinserver_server_title);
	Menu_AddItem (&s_joinserver_menu, &s_joinserver_address_book_action);
	Menu_AddItem (&s_joinserver_menu, &s_joinserver_search_action);

	Menu_Center( &s_joinserver_menu );

	SearchLocalGames();
}

static void JoinServer_MenuDraw(void)
{
	M_Banner( "m_banner_join_server" );
	Menu_Draw( &s_joinserver_menu );
}


static const char *JoinServer_MenuKey( int key )
{
	return Default_MenuKey( &s_joinserver_menu, key );
}

void M_Menu_JoinServer_f (void)
{
	MENU_CHECK
	JoinServer_MenuInit();
	M_PushMenu( JoinServer_MenuDraw, JoinServer_MenuKey );
}


/*
=============================================================================

START SERVER MENU

=============================================================================
*/
static menuFramework_t s_startserver_menu;

static basenamed_t *mapNames;
static void *mapNamesChain;
static int	numMaps;

static menuAction_t	s_startserver_start_action;
static menuAction_t	s_startserver_dmoptions_action;
static menuAction_t	s_startserver_dmbrowse_action;
static menuField_t	s_timelimit_field;
static menuField_t	s_fraglimit_field;
static menuField_t	s_maxclients_field;
static menuField_t	s_hostname_field;
static menuList2_t	s_startmap_list;
static menuList_t	s_rules_box;

static void DMOptionsFunc (void *self)
{
	if (s_rules_box.curvalue == 1)
		return;
	M_Menu_DMOptions_f ();
}

static void RulesChangeFunc (void *self)
{
	if (s_rules_box.curvalue == 0)				// deathmatch
	{
		s_maxclients_field.generic.statusbar = NULL;
		s_startserver_dmoptions_action.generic.statusbar = NULL;
	}
	else if (s_rules_box.curvalue == 1)			// coop
	{
		s_maxclients_field.generic.statusbar = "4 maximum for cooperative";
		if (atoi(s_maxclients_field.buffer) > 4)
			strcpy( s_maxclients_field.buffer, "4" );
		s_startserver_dmoptions_action.generic.statusbar = "N/A for cooperative";
	}
	else if (!stricmp (fs_gamedirvar->string, "rogue"))
	{
		if (s_rules_box.curvalue == 2)			// tag
		{
			s_maxclients_field.generic.statusbar = NULL;
			s_startserver_dmoptions_action.generic.statusbar = NULL;
		}
/*
		else if(s_rules_box.curvalue == 3)		// deathball
		{
			s_maxclients_field.generic.statusbar = NULL;
			s_startserver_dmoptions_action.generic.statusbar = NULL;
		}
*/
	}
}

static void StartServerFunc (char *map)
{
	char	startmap[1024];
	int		timelimit;
	int		fraglimit;
	int		maxclients;
	char	*spot;

	strcpy (startmap, map);

	maxclients  = atoi (s_maxclients_field.buffer);
	timelimit	= atoi (s_timelimit_field.buffer);
	fraglimit	= atoi (s_fraglimit_field.buffer);

	Cvar_ClampName ("maxclients", 0, maxclients);
	Cvar_ClampName ("timelimit", 0, timelimit);
	Cvar_ClampName ("fraglimit", 0, fraglimit);
	Cvar_Set ("hostname", s_hostname_field.buffer);
//	Cvar_SetValue ("deathmatch", !s_rules_box.curvalue );
//	Cvar_SetValue ("coop", s_rules_box.curvalue );

	if (!stricmp (fs_gamedirvar->string, "rogue") && s_rules_box.curvalue >= 2)
	{
		Cvar_SetInteger ("deathmatch", 1);		// deathmatch is always true for rogue games, right?
		Cvar_SetInteger ("coop", 0);			// FIXME - this might need to depend on which game we're running
		Cvar_SetInteger ("gamerules", s_rules_box.curvalue);
	}
	else
	{
		Cvar_SetInteger ("deathmatch", !s_rules_box.curvalue);
		Cvar_SetInteger ("coop", s_rules_box.curvalue);
		Cvar_SetInteger ("gamerules", 0);
	}

	spot = NULL;
	if (s_rules_box.curvalue == 1)		// PGM
	{
		if (!stricmp (startmap, "bunk1")  ||
			!stricmp (startmap, "mintro") ||
			!stricmp (startmap, "fact1"))
			spot = "start";
		else if (!stricmp (startmap, "power1"))
			spot = "pstart";
		else if (!stricmp (startmap, "biggun"))
			spot = "bstart";
		else if (!stricmp (startmap, "hangar1") || !stricmp (startmap, "city1"))
			spot = "unitstart";
		else if (!stricmp (startmap, "boss1"))
			spot = "bosstart";
	}

	if (spot)
	{
		if (Com_ServerState())
			Cbuf_AddText ("disconnect\n");
		Cbuf_AddText (va("gamemap \"*%s$%s\"\n", startmap, spot));
	}
	else
	{
		Cbuf_AddText (va("map %s\n", startmap));
	}

	if (mapNamesChain)
	{
		FreeMemoryChain (mapNamesChain);
		mapNamesChain = NULL;
	}
	M_ForceMenuOff ();
}

static void StartServerActionFunc (void *self)
{
	basenamed_t *item;

	item = FindNamedStrucByIndex (mapNames, s_startmap_list.curvalue);
	if (item)
		StartServerFunc (strchr (item->name, '\n') + 1);
}

static void StartServer_MenuInit( void )
{
	static const char *dm_coop_names[] =
	{
		"deathmatch",
		"cooperative",
		NULL
	};
//=======
//PGM
	static const char *dm_coop_names_rogue[] =
	{
		"deathmatch",
		"cooperative",
		"tag",
//		"deathball",
		NULL
	};
//PGM
//=======
	char	*buffer;
	FILE	*fp;

	// load the list of map names
	if (!(fp = fopen (va("%s/maps.lst", FS_Gamedir()), "rb")))
	{	// if file not present in OS filesystem - try pak file
		if (!(buffer = (char*) FS_LoadFile ("maps.lst")))
		{
			Com_WPrintf ("Couldn't find maps.lst\n");
			buffer = NULL;
		}
	}
	else
	{
		int		length;

		fseek (fp, 0, SEEK_END);
		length = ftell (fp);
		fseek (fp, 0, SEEK_SET);

		buffer = (char*)appMalloc (length + 1);
		fread (buffer, length, 1, fp);
		fclose (fp);
	}

	numMaps = 0;
	if (buffer)
	{
		basenamed_t *item, *last;
		char	*s;

		mapNamesChain = CreateMemoryChain ();
		mapNames = last = NULL;

		s = buffer;
		while (s)
		{
			char	name[256];

			Q_strncpylower (name, COM_ParseExt (&s, true), sizeof(name)-1);
			if (!name[0]) break;
			item = (basenamed_t*)AllocChainBlock (mapNamesChain, sizeof(basenamed_t));
			item->name = ChainCopyString (va("%s\n%s", COM_ParseExt (&s, false), name), mapNamesChain);
			// add new string to the end of list
			if (last) last->next = item;
			last = item;
			if (!mapNames) mapNames = item;

			numMaps++;
		}

		if (fp)
			appFree (buffer);
		else
			FS_FreeFile (buffer);
	}

	// initialize the menu stuff
	s_startserver_menu.x = viddef.width * 0.50;
	s_startserver_menu.nitems = 0;

	s_startmap_list.generic.type = MTYPE_SPINCONTROL2;
	s_startmap_list.generic.x	= 0;
	s_startmap_list.generic.y	= 0;
	s_startmap_list.generic.name = "initial map";
	s_startmap_list.itemnames = mapNames;

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.x	= 0;
	s_rules_box.generic.y	= 20;
	s_rules_box.generic.name = "rules";

	if (!stricmp (fs_gamedirvar->string, "rogue"))
		s_rules_box.itemnames = dm_coop_names_rogue;
	else
		s_rules_box.itemnames = dm_coop_names;

	if (Cvar_VariableInt ("coop"))
		s_rules_box.curvalue = 1;
	else
		s_rules_box.curvalue = 0;
	s_rules_box.generic.callback = RulesChangeFunc;

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "time limit";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.x	= 0;
	s_timelimit_field.generic.y	= 36;
	s_timelimit_field.generic.statusbar = "0 = no limit";
	s_timelimit_field.length = 3;
	s_timelimit_field.visible_length = 3;
	strcpy (s_timelimit_field.buffer, Cvar_VariableString ("timelimit"));

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "frag limit";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.x	= 0;
	s_fraglimit_field.generic.y	= 54;
	s_fraglimit_field.generic.statusbar = "0 = no limit";
	s_fraglimit_field.length = 3;
	s_fraglimit_field.visible_length = 3;
	strcpy (s_fraglimit_field.buffer, Cvar_VariableString ("fraglimit"));

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is.
	** Clamping will be done when the server is actually started.
	*/
	s_maxclients_field.generic.type = MTYPE_FIELD;
	s_maxclients_field.generic.name = "max players";
	s_maxclients_field.generic.flags = QMF_NUMBERSONLY;
	s_maxclients_field.generic.x	= 0;
	s_maxclients_field.generic.y	= 72;
	s_maxclients_field.generic.statusbar = NULL;
	s_maxclients_field.length = 3;
	s_maxclients_field.visible_length = 3;
	if (Cvar_VariableInt ("maxclients") == 1)
		strcpy (s_maxclients_field.buffer, "8");
	else
		strcpy (s_maxclients_field.buffer, Cvar_VariableString ("maxclients"));

	s_hostname_field.generic.type = MTYPE_FIELD;
	s_hostname_field.generic.name = "hostname";
	s_hostname_field.generic.flags = 0;
	s_hostname_field.generic.x	= 0;
	s_hostname_field.generic.y	= 90;
	s_hostname_field.generic.statusbar = NULL;
	s_hostname_field.length = 12;
	s_hostname_field.visible_length = 12;
	strcpy (s_hostname_field.buffer, Cvar_VariableString("hostname"));

	MENU_ACTION(s_startserver_dmoptions_action,108,"deathmatch flags",DMOptionsFunc)
	s_startserver_dmoptions_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_dmoptions_action.generic.x	= 24;
	s_startserver_dmoptions_action.generic.statusbar = NULL;

	MENU_ACTION(s_startserver_dmbrowse_action,118,"browse maps",M_Menu_DMBrowse_f)
	s_startserver_dmbrowse_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_dmbrowse_action.generic.x	= 24;
	s_startserver_dmbrowse_action.generic.statusbar = NULL;

	MENU_ACTION(s_startserver_start_action,138,"begin",StartServerActionFunc)
	s_startserver_start_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_start_action.generic.x	= 24;

	Menu_AddItem (&s_startserver_menu, &s_startmap_list);
	Menu_AddItem (&s_startserver_menu, &s_rules_box);
	Menu_AddItem (&s_startserver_menu, &s_timelimit_field);
	Menu_AddItem (&s_startserver_menu, &s_fraglimit_field);
	Menu_AddItem (&s_startserver_menu, &s_maxclients_field);
	Menu_AddItem (&s_startserver_menu, &s_hostname_field);
	Menu_AddItem (&s_startserver_menu, &s_startserver_dmoptions_action);
	Menu_AddItem (&s_startserver_menu, &s_startserver_dmbrowse_action);
	Menu_AddItem (&s_startserver_menu, &s_startserver_start_action);

	Menu_Center (&s_startserver_menu);

	// call this now to set proper inital state
	RulesChangeFunc (NULL);
}

static void StartServer_MenuDraw (void)
{
	Menu_Draw (&s_startserver_menu);
}

static const char *StartServer_MenuKey (int key)
{
	if (key == K_ESCAPE || key == K_MOUSE2)
	{
		if (mapNamesChain)
		{
			FreeMemoryChain (mapNamesChain);
			mapNamesChain = NULL;
		}
		numMaps = 0;
	}

	return Default_MenuKey (&s_startserver_menu, key);
}

void M_Menu_StartServer_f (void)
{
	MENU_CHECK
	StartServer_MenuInit();
	M_PushMenu (StartServer_MenuDraw, StartServer_MenuKey);
}

/*
=============================================================================

DMOPTIONS BOOK MENU

=============================================================================
*/
static char dmoptions_statusbar[128];

static menuFramework_t s_dmoptions_menu;

static menuList_t	s_friendlyfire_box;
static menuList_t	s_falls_box;
static menuList_t	s_weapons_stay_box;
static menuList_t	s_instant_powerups_box;
static menuList_t	s_powerups_box;
static menuList_t	s_health_box;
static menuList_t	s_spawn_farthest_box;
static menuList_t	s_teamplay_box;
static menuList_t	s_samelevel_box;
static menuList_t	s_force_respawn_box;
static menuList_t	s_armor_box;
static menuList_t	s_allow_exit_box;
static menuList_t	s_infinite_ammo_box;
static menuList_t	s_fixed_fov_box;
static menuList_t	s_quad_drop_box;

//ROGUE
static menuList_t	s_no_mines_box;
static menuList_t	s_no_nukes_box;
static menuList_t	s_stack_double_box;
static menuList_t	s_no_spheres_box;
//ROGUE

static void DMFlagCallback (void *self)
{
	menuList_t *f;
	int		flags, bit, nobit;

	f = (menuList_t *)self;
	flags = Cvar_VariableInt ("dmflags");
	bit = nobit = 0;

	if (f == &s_friendlyfire_box)
		nobit = DF_NO_FRIENDLY_FIRE;
	else if (f == &s_falls_box)
		nobit = DF_NO_FALLING;
	else if (f == &s_weapons_stay_box)
		bit = DF_WEAPONS_STAY;
	else if (f == &s_instant_powerups_box)
		bit = DF_INSTANT_ITEMS;
	else if (f == &s_allow_exit_box)
		bit = DF_ALLOW_EXIT;
	else if (f == &s_powerups_box)
		nobit = DF_NO_ITEMS;
	else if (f == &s_health_box)
		nobit = DF_NO_HEALTH;
	else if (f == &s_spawn_farthest_box)
		bit = DF_SPAWN_FARTHEST;
	else if (f == &s_teamplay_box)
	{
		if (f->curvalue == 1)
		{
			flags = flags | DF_SKINTEAMS & ~DF_MODELTEAMS;
		}
		else if (f->curvalue == 2)
		{
			flags = flags | DF_MODELTEAMS & ~DF_SKINTEAMS;
		}
		else
		{
			flags &= ~(DF_MODELTEAMS|DF_SKINTEAMS);
		}

	}
	else if (f == &s_samelevel_box)
		bit = DF_SAME_LEVEL;
	else if (f == &s_force_respawn_box)
		bit = DF_FORCE_RESPAWN;
	else if (f == &s_armor_box)
		nobit = DF_NO_ARMOR;
	else if (f == &s_infinite_ammo_box)
		bit = DF_INFINITE_AMMO;
	else if (f == &s_fixed_fov_box)
		bit = DF_FIXED_FOV;
	else if (f == &s_quad_drop_box)
		bit = DF_QUAD_DROP;
	else if (!stricmp (fs_gamedirvar->string, "rogue"))
	{
		if (f == &s_no_mines_box)
			bit = DF_NO_MINES;
		else if (f == &s_no_nukes_box)
			bit = DF_NO_NUKES;
		else if (f == &s_stack_double_box)
			bit = DF_NO_STACK_DOUBLE;
		else if (f == &s_no_spheres_box)
			bit = DF_NO_SPHERES;
	}

	if (f)
	{
		if (f->curvalue == 0)
			flags = flags & ~bit | nobit;
		else
			flags = flags & ~nobit | bit;
	}

	Cvar_SetInteger ("dmflags", flags);

	Com_sprintf (ARRAY_ARG(dmoptions_statusbar), "dmflags = %d", flags);
}

static void DMOptions_MenuInit (void)
{
	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};
	static const char *teamplay_names[] =
	{
		"disabled", "by skin", "by model", 0
	};
	int dmflags = Cvar_VariableInt ("dmflags");
	int y = 0;

	s_dmoptions_menu.x = viddef.width * 0.50;
	s_dmoptions_menu.nitems = 0;

	MENU_SPIN(s_falls_box,y,"falling damage",DMFlagCallback,yes_no_names)
	s_falls_box.curvalue = ( dmflags & DF_NO_FALLING ) == 0;
	MENU_SPIN(s_weapons_stay_box,y+=10,"weapons stay",DMFlagCallback,yes_no_names)
	s_weapons_stay_box.curvalue = ( dmflags & DF_WEAPONS_STAY ) != 0;
	MENU_SPIN(s_instant_powerups_box,y+=10,"instant powerups",DMFlagCallback,yes_no_names)
	s_instant_powerups_box.curvalue = ( dmflags & DF_INSTANT_ITEMS ) != 0;
	MENU_SPIN(s_powerups_box,y+=10,"allow powerups",DMFlagCallback,yes_no_names)
	s_powerups_box.curvalue = ( dmflags & DF_NO_ITEMS ) == 0;
	MENU_SPIN(s_health_box,y+=10,"allow health",DMFlagCallback,yes_no_names)
	s_health_box.curvalue = ( dmflags & DF_NO_HEALTH ) == 0;
	MENU_SPIN(s_armor_box,y+=10,"allow armor",DMFlagCallback,yes_no_names)
	s_armor_box.curvalue = ( dmflags & DF_NO_ARMOR ) == 0;
	MENU_SPIN(s_spawn_farthest_box,y+=10,"spawn farthest",DMFlagCallback,yes_no_names)
	s_spawn_farthest_box.curvalue = ( dmflags & DF_SPAWN_FARTHEST ) != 0;
	MENU_SPIN(s_samelevel_box,y+=10,"same map",DMFlagCallback,yes_no_names)
	s_samelevel_box.curvalue = ( dmflags & DF_SAME_LEVEL ) != 0;
	MENU_SPIN(s_force_respawn_box,y+=10,"force respawn",DMFlagCallback,yes_no_names)
	s_force_respawn_box.curvalue = ( dmflags & DF_FORCE_RESPAWN ) != 0;
	MENU_SPIN(s_teamplay_box,y+=10,"teamplay",DMFlagCallback,yes_no_names)
	s_teamplay_box.itemnames = teamplay_names;
	MENU_SPIN(s_allow_exit_box,y+=10,"allow exit",DMFlagCallback,yes_no_names)
	s_allow_exit_box.curvalue = ( dmflags & DF_ALLOW_EXIT ) != 0;
	MENU_SPIN(s_infinite_ammo_box,y+=10,"infinite ammo",DMFlagCallback,yes_no_names)
	s_infinite_ammo_box.curvalue = ( dmflags & DF_INFINITE_AMMO ) != 0;
	MENU_SPIN(s_fixed_fov_box,y+=10,"fixed FOV",DMFlagCallback,yes_no_names)
	s_fixed_fov_box.curvalue = ( dmflags & DF_FIXED_FOV ) != 0;
	MENU_SPIN(s_quad_drop_box,y+=10,"quad drop",DMFlagCallback,yes_no_names)
	s_quad_drop_box.curvalue = ( dmflags & DF_QUAD_DROP ) != 0;
	MENU_SPIN(s_friendlyfire_box,y+=10,"friendly fire",DMFlagCallback,yes_no_names)
	s_friendlyfire_box.curvalue = ( dmflags & DF_NO_FRIENDLY_FIRE ) == 0;

	if (!stricmp (fs_gamedirvar->string, "rogue"))
	{
		MENU_SPIN(s_no_mines_box,y+=10,"remove mines",DMFlagCallback,yes_no_names)
		s_no_mines_box.curvalue = ( dmflags & DF_NO_MINES ) != 0;
		MENU_SPIN(s_no_nukes_box,y+=10,"remove nukes",DMFlagCallback,yes_no_names)
		s_no_nukes_box.curvalue = ( dmflags & DF_NO_NUKES ) != 0;
		MENU_SPIN(s_stack_double_box,y+=10,"2x/4x stacking off",DMFlagCallback,yes_no_names)
		s_stack_double_box.curvalue = ( dmflags & DF_NO_STACK_DOUBLE ) != 0;
		MENU_SPIN(s_no_spheres_box,y+=10,"remove spheres",DMFlagCallback,yes_no_names)
		s_no_spheres_box.curvalue = ( dmflags & DF_NO_SPHERES ) != 0;
	}

	Menu_AddItem( &s_dmoptions_menu, &s_falls_box );
	Menu_AddItem( &s_dmoptions_menu, &s_weapons_stay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_instant_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_health_box );
	Menu_AddItem( &s_dmoptions_menu, &s_armor_box );
	Menu_AddItem( &s_dmoptions_menu, &s_spawn_farthest_box );
	Menu_AddItem( &s_dmoptions_menu, &s_samelevel_box );
	Menu_AddItem( &s_dmoptions_menu, &s_force_respawn_box );
	Menu_AddItem( &s_dmoptions_menu, &s_teamplay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_allow_exit_box );
	Menu_AddItem( &s_dmoptions_menu, &s_infinite_ammo_box );
	Menu_AddItem( &s_dmoptions_menu, &s_fixed_fov_box );
	Menu_AddItem( &s_dmoptions_menu, &s_quad_drop_box );
	Menu_AddItem( &s_dmoptions_menu, &s_friendlyfire_box );

	if (!stricmp (fs_gamedirvar->string, "rogue"))
	{
		Menu_AddItem( &s_dmoptions_menu, &s_no_mines_box );
		Menu_AddItem( &s_dmoptions_menu, &s_no_nukes_box );
		Menu_AddItem( &s_dmoptions_menu, &s_stack_double_box );
		Menu_AddItem( &s_dmoptions_menu, &s_no_spheres_box );
	}

	Menu_Center( &s_dmoptions_menu );

	// set the original dmflags statusbar
	DMFlagCallback( 0 );
	Menu_SetStatusBar( &s_dmoptions_menu, dmoptions_statusbar );
}

static void DMOptions_MenuDraw(void)
{
	Menu_Draw( &s_dmoptions_menu );
}

static const char *DMOptions_MenuKey( int key )
{
	return Default_MenuKey( &s_dmoptions_menu, key );
}

void M_Menu_DMOptions_f (void)
{
	MENU_CHECK
	DMOptions_MenuInit();
	M_PushMenu( DMOptions_MenuDraw, DMOptions_MenuKey );
}

/*
=============================================================================

DOWNLOADOPTIONS BOOK MENU

=============================================================================
*/
static menuFramework_t s_downloadoptions_menu;

static menuSeparator_t	s_download_title;
static menuList_t	s_allow_download_box;
static menuList_t	s_allow_download_maps_box;
static menuList_t	s_allow_download_models_box;
static menuList_t	s_allow_download_players_box;
static menuList_t	s_allow_download_sounds_box;

static void DownloadCallback (void *self)
{
	char	*name;
	menuList_t *f = (menuList_t *) self;

	if (f == &s_allow_download_box)
		name = "allow_download";
	else if (f == &s_allow_download_maps_box)
		name = "allow_download_maps";
	else if (f == &s_allow_download_models_box)
		name = "allow_download_models";
	else if (f == &s_allow_download_players_box)
		name = "allow_download_players";
	else if (f == &s_allow_download_sounds_box)
		name = "allow_download_sounds";
	else
		return;
	Cvar_SetInteger (name, f->curvalue);
}

static void DownloadOptions_MenuInit( void )
{
	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};
	int y = 0;

	s_downloadoptions_menu.x = viddef.width * 0.50;
	s_downloadoptions_menu.nitems = 0;

	s_download_title.generic.type = MTYPE_SEPARATOR;
	s_download_title.generic.name = "Download Options";
	s_download_title.generic.x	= 48;
	s_download_title.generic.y	= y;

	MENU_SPIN(s_allow_download_box, y+=20,"allow downloading", DownloadCallback, yes_no_names);
	s_allow_download_box.curvalue = Cvar_VariableInt ("allow_download");
	MENU_SPIN(s_allow_download_maps_box, y+=20,"maps", DownloadCallback, yes_no_names);
	s_allow_download_maps_box.curvalue = Cvar_VariableInt ("allow_download_maps");
	MENU_SPIN(s_allow_download_players_box, y+=20,"player models/skins",DownloadCallback, yes_no_names);
	s_allow_download_players_box.curvalue = Cvar_VariableInt ("allow_download_players");
	MENU_SPIN(s_allow_download_models_box, y+=20,"models", DownloadCallback, yes_no_names)
	s_allow_download_models_box.curvalue = Cvar_VariableInt("allow_download_models");
	MENU_SPIN(s_allow_download_sounds_box, y+=20,"sounds", DownloadCallback, yes_no_names)
	s_allow_download_sounds_box.curvalue = Cvar_VariableInt ("allow_download_sounds");

	Menu_AddItem (&s_downloadoptions_menu, &s_download_title);
	Menu_AddItem (&s_downloadoptions_menu, &s_allow_download_box);
	Menu_AddItem (&s_downloadoptions_menu, &s_allow_download_maps_box);
	Menu_AddItem (&s_downloadoptions_menu, &s_allow_download_players_box);
	Menu_AddItem (&s_downloadoptions_menu, &s_allow_download_models_box);
	Menu_AddItem (&s_downloadoptions_menu, &s_allow_download_sounds_box);

	Menu_Center (&s_downloadoptions_menu);

	// skip over title
	if (s_downloadoptions_menu.cursor == 0)
		s_downloadoptions_menu.cursor = 1;
}

static void DownloadOptions_MenuDraw(void)
{
	Menu_Draw( &s_downloadoptions_menu );
}

static const char *DownloadOptions_MenuKey( int key )
{
	return Default_MenuKey( &s_downloadoptions_menu, key );
}

void M_Menu_DownloadOptions_f (void)
{
	MENU_CHECK
	DownloadOptions_MenuInit();
	M_PushMenu( DownloadOptions_MenuDraw, DownloadOptions_MenuKey );
}
/*
=============================================================================

ADDRESS BOOK MENU

=============================================================================
*/
#define NUM_ADDRESSBOOK_ENTRIES 9

static menuFramework_t	s_addressbook_menu;
static menuField_t		s_addressbook_fields[NUM_ADDRESSBOOK_ENTRIES];

static void AddressBook_MenuInit( void )
{
	int i;

	s_addressbook_menu.x = viddef.width / 2 - 142;
	s_addressbook_menu.y = viddef.height / 2 - 58;
	s_addressbook_menu.nitems = 0;

	for (i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++)
	{
		cvar_t *adr;

		adr = Cvar_Get (va("adr%d", i), "", CVAR_ARCHIVE);

		s_addressbook_fields[i].generic.type = MTYPE_FIELD;
		s_addressbook_fields[i].generic.name = 0;
		s_addressbook_fields[i].generic.callback = 0;
		s_addressbook_fields[i].generic.x	= 0;
		s_addressbook_fields[i].generic.y	= i * 18 + 0;
		s_addressbook_fields[i].generic.localdata[0] = i;
		s_addressbook_fields[i].cursor		= 0;
		s_addressbook_fields[i].length		= 60;
		s_addressbook_fields[i].visible_length = 30;

		strcpy (s_addressbook_fields[i].buffer, adr->string);

		Menu_AddItem (&s_addressbook_menu, &s_addressbook_fields[i]);
	}
}

static const char *AddressBook_MenuKey( int key )
{
	if ( key == K_ESCAPE || key == K_MOUSE2 )
	{
		int i;

		for (i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++)
			Cvar_Set (va("adr%d", i), s_addressbook_fields[i].buffer );
	}
	return Default_MenuKey( &s_addressbook_menu, key );
}

static void AddressBook_MenuDraw(void)
{
	M_Banner( "m_banner_addressbook" );
	Menu_Draw( &s_addressbook_menu );
}

void M_Menu_AddressBook_f(void)
{
	MENU_CHECK
	AddressBook_MenuInit();
	M_PushMenu( AddressBook_MenuDraw, AddressBook_MenuKey );
}

/*
=============================================================================

PLAYER CONFIG MENU

=============================================================================
*/
static menuFramework_t	s_player_config_menu;
static menuField_t		s_player_name_field;
static menuList2_t		s_player_model_box;
static menuList2_t		s_player_skin_box;
static menuList_t		s_player_rtype_box;
static menuList_t		s_player_rcolor_box;
static menuList_t		s_player_handedness_box;
static menuList_t		s_player_rate_box;
static menuSeparator_t	s_player_skin_title;
static menuSeparator_t	s_player_model_title;
static menuSeparator_t	s_player_rtype_title;
static menuSeparator_t	s_player_rcolor_title;
static menuSeparator_t	s_player_hand_title;
static menuSeparator_t	s_player_rate_title;
static menuAction_t		s_player_download_action;

#define MAX_DISPLAYNAME 16

typedef struct playerModelInfo_s
{	// derived from basenamed_t
	char	*name;
	struct playerModelInfo_s *next;
	int		numSkins;
	basenamed_t *skins;
} playerModelInfo_t;

static playerModelInfo_t *pmi;
static int numPlayerModels;
static void *pmiChain;

static int modelChangeTime;

static int rate_tbl[] = {
	2500, 3200, 5000, 10000, 25000, 0
};

static const char *rate_names[] = {
	"28.8 Modem", "33.6 Modem", "Single ISDN", "Dual ISDN/Cable", "T1/LAN", "User defined", NULL
};

static void HandednessCallback (void *unused)
{
	Cvar_SetInteger ("hand", s_player_handedness_box.curvalue);
}

static void RTypeCallback (void *unused)
{
	Cvar_SetInteger ("railtype", s_player_rtype_box.curvalue);
}

static void RColorCallback (void *unused)
{
	Cvar_SetInteger ("railcolor", s_player_rcolor_box.curvalue);
}

static void RateCallback (void *unused)
{
	if (s_player_rate_box.curvalue != ARRAY_COUNT(rate_tbl) - 1)
		Cvar_SetInteger ("rate", rate_tbl[s_player_rate_box.curvalue]);
}

static void ModelCallback (void *unused)
{
	playerModelInfo_t *info;

	info = (playerModelInfo_t*) FindNamedStrucByIndex ((basenamed_t*)pmi, s_player_model_box.curvalue);
	if (info)
		s_player_skin_box.itemnames = info->skins;
	else
		Com_FatalError ("ModelCallback: bad index");	//?? debug
	s_player_skin_box.curvalue = 0;
	modelChangeTime = Sys_Milliseconds ();
}

//!! place skin functions to a separate unit
static bool IsValidSkin (char *skin, basenamed_t *pcxfiles)
{
	char	scratch[MAX_OSPATH], *ext, *name;
	basenamed_t *item;

	ext = strrchr (skin, '.');
	if (!ext) return false;

	if (stricmp (ext, ".pcx") && stricmp (ext, ".tga") && stricmp (ext, ".jpg"))
		return false;		// not image

	// modelname/skn_* have no icon
	name = strrchr (skin, '/');
	if (!name) return false;
	else name++;			// skip '/'

	if (!memcmp (name, "skn_", 4))
		return true;
	strcpy (scratch, skin);
	*strrchr (scratch, '.') = 0;
	strcat (scratch, "_i.pcx");

	for (item = pcxfiles; item; item = item->next)
		if (!strcmp (item->name, scratch)) return true;
	return false;
}

static bool PlayerConfig_ScanDirectories (void)
{
	char	*path;
	int		numModelDirs, i;
	basenamed_t *dirnames, *diritem;

	numPlayerModels = 0;
	pmi = NULL;

	/*----- get a list of directories -----*/
	path = NULL;
	while (path = FS_NextPath (path))
	{
		if (dirnames = FS_ListFiles (va("%s/players/*.*", path), &numModelDirs, LIST_DIRS)) break;
	}
	if (!dirnames) return false;

	/*--- go through the subdirectories ---*/
	for (i = 0, diritem = dirnames; i < numModelDirs; i++, diritem = diritem->next)
	{
		basenamed_t *pcxnames, *pcxitem;
		int		numSkins;
		playerModelInfo_t *info, *infoIns;
		basenamed_t *skins;

		// verify the existence of tris.md2
		if (!FS_FileExists (va("%s/tris.md2", diritem->name))) continue;

		// verify the existence of at least one pcx skin
		// "/*.pcx" -> pcx,tga,jpg (see IsValidSkin())
		pcxnames = FS_ListFiles (va("%s/*.*", diritem->name), NULL, LIST_FILES);
		if (!pcxnames) continue;

		// count valid skins, which consist of a skin with a matching "_i" icon
		numSkins = 0;
		skins = NULL;
		for (pcxitem = pcxnames; pcxitem; pcxitem = pcxitem->next)
			if (IsValidSkin (pcxitem->name, pcxnames))
			{
				char	*str, *ext;

				str = strrchr (pcxitem->name, '/') + 1;
				ext = strrchr (str, '.');
				ext[0] = 0;
				skins = ChainAddToNamedList (str, skins, pmiChain);
				numSkins++;
			}
		FreeNamedList (pcxnames);
		if (!numSkins) continue;

		// create model info
		info = (playerModelInfo_t*) ChainAllocNamedStruc (sizeof(playerModelInfo_t), strrchr (diritem->name, '/')+1, pmiChain);
		info->numSkins = numSkins;
		info->skins = skins;

		// add model info to pmi
		FindNamedStruc (info->name, (basenamed_t*)pmi, (basenamed_t**)&infoIns);
		if (infoIns)
		{	// place after infoIns
			info->next = infoIns->next;
			infoIns->next = info;
		}
		else
		{	// place first
			info->next = pmi;
			pmi = info;
		}

		numPlayerModels++;
	}
	if (dirnames) FreeNamedList (dirnames);
	return true;
}


static bool PlayerConfig_MenuInit (void)
{
	extern cvar_t *name, *team, *skin;
	char	currentModel[MAX_QPATH], currentSkin[MAX_QPATH], *path;
	int		i, y, currentModelIndex, currentSkinIndex;
	playerModelInfo_t *info;

	cvar_t *hand = Cvar_Get ("hand", "0", CVAR_USERINFO|CVAR_ARCHIVE);

	static const char *handedness[] = {"right", "left", "center", NULL};
	static const char *colorNames[] = {"default", "red", "green", "yellow", "blue", "magenta", "cyan", "white", NULL};
	static const char *railTypes[]  = {"original", "spiral", "rings", "beam", NULL};

	PlayerConfig_ScanDirectories ();

	if (!numPlayerModels) return false;

	Cvar_Clamp (hand, 0, 2);

	strcpy (currentModel, skin->string);
	if (path = strchr (currentModel, '/'))
	{
		strcpy (currentSkin, path + 1);
		path[0] = 0;
	}
	else
	{	// bad skin info - set default
		strcpy (currentModel, "male");
		strcpy (currentSkin, "grunt");
	}

	for (info = pmi, currentModelIndex = 0; info; info = info->next, currentModelIndex++)
		if (!stricmp (info->name, currentModel))
		{	// we have found model - find skin
			currentSkinIndex = IndexOfNamedStruc (currentSkin, info->skins);
			if (currentSkinIndex < 0)	// not found
				currentSkinIndex = 0;
			break;
		}
	if (!info)
	{	// model not found
		info = pmi;
		currentModelIndex = 0;
		currentSkinIndex = 0;
	}

	s_player_config_menu.x = viddef.width * 65 / 320;
	s_player_config_menu.y = viddef.height * 23 / 240;
	s_player_config_menu.nitems = 0;

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "name";
	s_player_name_field.generic.x	= 0;
	s_player_name_field.generic.y	= viddef.height * 20 / 240 - 20;
	s_player_name_field.length		= 20;
	s_player_name_field.visible_length = 20;
	strcpy (s_player_name_field.buffer, name->string);
	s_player_name_field.cursor = strlen (name->string);

	y = viddef.height * 68 / 240;

	// model
	s_player_model_title.generic.type = MTYPE_SEPARATOR;
	s_player_model_title.generic.name = "model";
	s_player_model_title.generic.x	= -8;
	s_player_model_title.generic.y	= y+=10;

	s_player_model_box.generic.type = MTYPE_SPINCONTROL2;
	s_player_model_box.generic.x	= -56;
	s_player_model_box.generic.y	= y+=10;
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.generic.cursor_offset = -48;
	s_player_model_box.curvalue = currentModelIndex;
	s_player_model_box.itemnames = (basenamed_t*)pmi;

	// skin
	s_player_skin_title.generic.type = MTYPE_SEPARATOR;
	s_player_skin_title.generic.name = "skin";
	s_player_skin_title.generic.x   = -16;
	s_player_skin_title.generic.y	= y+=14;

	s_player_skin_box.generic.type	= MTYPE_SPINCONTROL2;
	s_player_skin_box.generic.x		= -56;
	s_player_skin_box.generic.y		= y+=10;
	s_player_skin_box.generic.cursor_offset = -48;
	s_player_skin_box.curvalue = currentSkinIndex;
	s_player_skin_box.itemnames = info->skins;

	// rail type
	s_player_rtype_title.generic.type = MTYPE_SEPARATOR;
	s_player_rtype_title.generic.name = "rail type";
	s_player_rtype_title.generic.x	= 24;
	s_player_rtype_title.generic.y	= y+=14;

	s_player_rtype_box.generic.type = MTYPE_SPINCONTROL;
	s_player_rtype_box.generic.x = -56;
	s_player_rtype_box.generic.y = y+=10;
	s_player_rtype_box.generic.cursor_offset = -48;
	s_player_rtype_box.generic.callback = RTypeCallback;
	s_player_rtype_box.curvalue = Cvar_VariableInt ("railtype");
	s_player_rtype_box.itemnames = railTypes;

	// rail color
	s_player_rcolor_title.generic.type = MTYPE_SEPARATOR;
	s_player_rcolor_title.generic.name = "rail color";
	s_player_rcolor_title.generic.x	= 32;
	s_player_rcolor_title.generic.y	= y+=14;

	s_player_rcolor_box.generic.type = MTYPE_SPINCONTROL;
	s_player_rcolor_box.generic.x = -56;
	s_player_rcolor_box.generic.y = y+=10;
	s_player_rcolor_box.generic.cursor_offset = -48;
	s_player_rcolor_box.generic.callback = RColorCallback;
	s_player_rcolor_box.curvalue = Cvar_VariableInt ("railcolor");
	s_player_rcolor_box.itemnames = colorNames;

	// handedness
	s_player_hand_title.generic.type = MTYPE_SEPARATOR;
	s_player_hand_title.generic.name = "handedness";
	s_player_hand_title.generic.x	= 32;
	s_player_hand_title.generic.y	= y+=14;

	s_player_handedness_box.generic.type = MTYPE_SPINCONTROL;
	s_player_handedness_box.generic.x = -56;
	s_player_handedness_box.generic.y = y+=10;
	s_player_handedness_box.generic.cursor_offset = -48;
	s_player_handedness_box.generic.callback = HandednessCallback;
	s_player_handedness_box.curvalue = Cvar_VariableInt ("hand");
	s_player_handedness_box.itemnames = handedness;

	for (i = 0; i < ARRAY_COUNT(rate_tbl) - 1; i++)
		if (Cvar_VariableInt ("rate") == rate_tbl[i])
			break;

	s_player_rate_title.generic.type = MTYPE_SEPARATOR;
	s_player_rate_title.generic.name = "connect speed";
	s_player_rate_title.generic.x	= 56;
	s_player_rate_title.generic.y	= y+=14;

	s_player_rate_box.generic.type	= MTYPE_SPINCONTROL;
	s_player_rate_box.generic.x		= -56;
	s_player_rate_box.generic.y		= y+=10;
	s_player_rate_box.generic.cursor_offset = -48;
	s_player_rate_box.generic.callback = RateCallback;
	s_player_rate_box.curvalue = i;
	s_player_rate_box.itemnames = rate_names;

	s_player_download_action.generic.type = MTYPE_ACTION;
	s_player_download_action.generic.name = "download options";
	s_player_download_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_player_download_action.generic.x = -24;
	s_player_download_action.generic.y = y+=14;
	s_player_download_action.generic.callback = (void (*)(void*))M_Menu_DownloadOptions_f;

	Menu_AddItem (&s_player_config_menu, &s_player_name_field);
	Menu_AddItem (&s_player_config_menu, &s_player_model_title);
	Menu_AddItem (&s_player_config_menu, &s_player_model_box);
	if (s_player_skin_box.itemnames)
	{
		Menu_AddItem (&s_player_config_menu, &s_player_skin_title);
		Menu_AddItem (&s_player_config_menu, &s_player_skin_box);
	}
	Menu_AddItem (&s_player_config_menu, &s_player_rtype_title);
	Menu_AddItem (&s_player_config_menu, &s_player_rtype_box);
	Menu_AddItem (&s_player_config_menu, &s_player_rcolor_title);
	Menu_AddItem (&s_player_config_menu, &s_player_rcolor_box);
	Menu_AddItem (&s_player_config_menu, &s_player_hand_title);
	Menu_AddItem (&s_player_config_menu, &s_player_handedness_box);
	Menu_AddItem (&s_player_config_menu, &s_player_rate_title);
	Menu_AddItem (&s_player_config_menu, &s_player_rate_box);
	Menu_AddItem (&s_player_config_menu, &s_player_download_action);

	return true;
}

// stand: 0-39; run: 40-45
#define FIRST_FRAME	0			//!! replace with animation_type (to work correctly with ANY model type)
#define LAST_FRAME	39
#define FRAME_COUNT (LAST_FRAME-FIRST_FRAME+1)

#define MODEL_DELAY	300

static void PlayerConfig_MenuDraw (void)
{
	extern float CalcFov (float fov_x, float w, float h);
	refdef_t	refdef;
	entity_t	e[2];
	playerModelInfo_t *model;
	basenamed_t	*skin;
	static dlight_t	dl[] = {
		{{30, 100, 100}, {1, 1, 1}, 400},
		{{90, -100, 10}, {0.4, 0.2, 0.2}, 200}
	};
	bool	showModels;
	char	*icon;

//	sscanf(Cvar_VariableString("dl0"), "%g %g %g %g", VECTOR_ARG(&dl[0].origin), &dl[0].intensity);
//	sscanf(Cvar_VariableString("dl1"), "%g %g %g %g", VECTOR_ARG(&dl[1].origin), &dl[1].intensity);

	showModels = Sys_Milliseconds () - modelChangeTime > MODEL_DELAY;

	model = (playerModelInfo_t*)FindNamedStrucByIndex ((basenamed_t*)pmi, s_player_model_box.curvalue);
	if (!model) return;
	skin = FindNamedStrucByIndex (model->skins, s_player_skin_box.curvalue);
	if (!skin) return;

	memset (&refdef, 0, sizeof (refdef));

	refdef.x = viddef.width / 2;
	refdef.y = viddef.height * 6 / 25;
	refdef.width = viddef.width * 4 / 10 + 16;
	refdef.height = viddef.width * 5 / 10;
	refdef.fov_x = refdef.width * 90 / viddef.width;
	refdef.fov_y = CalcFov (refdef.fov_x, refdef.width, refdef.height);
	refdef.time = cls.realtime * 0.001;

	memset (&e, 0, sizeof(e));

	if (showModels)
	{
		// add player model
		e[0].model = re.RegisterModel (va("players/%s/tris.md2", model->name));
		e[0].skin = re.RegisterSkin (va("players/%s/%s.pcx", model->name, skin->name));
//		e[0].flags = 0;
		e[0].origin[0] = 90;
//		e[0].origin[1] = 0;
//		e[0].origin[2] = 0;
		VectorCopy (e[0].origin, e[0].oldorigin);

		e[0].frame = (cls.realtime + 99) / 100 % FRAME_COUNT + FIRST_FRAME;
		e[0].oldframe = e[0].frame - 1;
		if (e[0].oldframe < FIRST_FRAME)
			e[0].oldframe = LAST_FRAME;
		e[0].backlerp = 1.0 - (cls.realtime % 100) / 100.0;
		if (e[0].backlerp == 1.0)
			e[0].backlerp = 0.0;
		e[0].angles[1] = cls.realtime / 20 % 360;

		// add weapon model
		memcpy (&e[1], &e[0], sizeof (e[0]));		// fill angles, lerp and more ...
		e[1].model = re.RegisterModel (va("players/%s/weapon.md2", model->name));
		e[1].skin = NULL;

//		refdef.areabits = NULL;
		refdef.num_entities = 2;
		refdef.entities = e;
//		refdef.lightstyles = NULL;
		refdef.dlights = dl;
		refdef.num_dlights = ARRAY_COUNT(dl);
	}
	refdef.rdflags = RDF_NOWORLDMODEL;

	Menu_Draw (&s_player_config_menu);

	re.RenderFrame (&refdef);

	if (!memcmp (skin->name, "skn_", 4))
		icon = "/pics/default_icon.pcx";
	else
		icon = va("/players/%s/%s_i.pcx", model->name, skin->name);
	re.DrawStretchPic (s_player_config_menu.x - 40, refdef.y,
		viddef.height * 32 / 240, viddef.height * 32 / 240, icon);
}

static const char *PlayerConfig_MenuKey (int key)
{
	if (key == K_ESCAPE || key == K_MOUSE2)
	{
		char	*s;
		int		color, c;
		playerModelInfo_t *model;
		basenamed_t *skin;

		// check colorizing of player name
		color = C_WHITE;
		s = s_player_name_field.buffer;
		while (c = *s++)
		{
			int		col;

			if (c == COLOR_ESCAPE)
			{
				col = *s - '0';
				if (col >= 0 && col <= 7)
				{
					color = col;
					s++;
				}
			}
		}
		// final color must be white; "i" points to string end
		if (color != C_WHITE)
			Cvar_Set ("name", va("%s"S_WHITE, s_player_name_field.buffer));
		else
			Cvar_Set ("name", s_player_name_field.buffer);

		model = (playerModelInfo_t*)FindNamedStrucByIndex ((basenamed_t*)pmi, s_player_model_box.curvalue);
		skin = FindNamedStrucByIndex (model->skins, s_player_skin_box.curvalue);
		// set "skin" variable
		Cvar_Set ("skin", va("%s/%s", model->name, skin->name));

		// free directory scan data
		FreeMemoryChain (pmiChain);
	}
	return Default_MenuKey( &s_player_config_menu, key );
}


static void M_Menu_PlayerConfig_f (void)
{
	MENU_CHECK
	pmiChain = CreateMemoryChain ();
	if (!PlayerConfig_MenuInit ())
	{
		FreeMemoryChain (pmiChain);
		Menu_SetStatusBar (&s_multiplayer_menu, S_RED"No valid player models found");
		return;
	}
	Menu_SetStatusBar (&s_multiplayer_menu, NULL);
	M_PushMenu (PlayerConfig_MenuDraw, PlayerConfig_MenuKey);
}


/*
=======================================================================

BROWSE FOR MAP

=======================================================================
*/

#define MAX_BROWSE_MAPS		1024
#define THUMBNAIL_BORDER	4
#define THUMBNAIL_TEXT		10

typedef struct
{
	int		w, h;			// thumbnail sizes
	int		cx, cy;			// number of thumbnail on screen
	int		x0, y0, dx, dy;	// delta for coputing thumbnail coordinates on screen (Xo+Ix*Dx, Yo+Iy*Dy)
	int		count;			// number of maps
	int		top;
	int		current;
} thumbParams_t;

typedef struct
{
	int		width;			// max screen width
	int		cx, cy;
	int		w, h;
} thumbLayout_t;


static char			*browse_map_names[MAX_BROWSE_MAPS];
static basenamed_t	*browse_list;
static thumbParams_t thumbs;

static thumbLayout_t thumbLayout[] =
{
	{320, 2, 1, 128, 96},
	{512, 2, 2, 128, 96},
	{640, 2, 2, 192, 144},
	{800, 3, 3, 192, 144},
	{1024, 3, 3, 256, 192},
	{BIG_NUMBER, 4, 4, 256, 192}
};

/*
 * Read screenshots list and refine it with maps that doesn't exists. Do not bother
 * about freeing screenshots from imagelist - this will be done automatically when a
 * new level started (NOT IMPLEMENTED NOW !!) (registration sequence is less than new level sequence by 1 ...)
 */
static bool DMBrowse_MenuInit ()
{
	basenamed_t *item;
	char	*name, *ext, *path;
	int		i, x, y, oldcount;

	// free previous levelshots list
	if (browse_list)
		FreeNamedList (browse_list);

	oldcount = thumbs.count;
	thumbs.count = 0;

	x = viddef.width;
	y = viddef.height;
	i = 0;
	while (1)
	{
		if (x <= thumbLayout[i].width)
		{
			thumbs.cx = thumbLayout[i].cx;
			thumbs.cy = thumbLayout[i].cy;
			thumbs.w  = thumbLayout[i].w;
			thumbs.h  = thumbLayout[i].h;
			break;
		}
		i++;
	}

	thumbs.dx = thumbs.w + THUMBNAIL_BORDER * 4;
	thumbs.dy = thumbs.h + THUMBNAIL_BORDER * 4 + THUMBNAIL_TEXT;
	thumbs.x0 = (x - thumbs.cx * thumbs.dx + thumbs.dx - thumbs.w) / 2;	// (x-cx*dx)/2 + (dx-w)/2
	thumbs.y0 = (y - thumbs.cy * thumbs.dy + thumbs.dy - thumbs.h) / 2 - 8;

	path = NULL;
	while (path = FS_NextPath (path))
	{
		if (browse_list = FS_ListFiles (va("%s/levelshots/*.*", path), NULL, LIST_FILES)) break;
	}
	for (item = browse_list; item; item = item->next)
	{
		name = strrchr (item->name, '/');
		if (!name)
			name = item->name;
		else
			name++;
		ext = strrchr (name, '.');
		if (ext && (!strcmp (ext, ".jpg") || !strcmp (ext, ".tga") || !strcmp (ext, ".pcx")))
		{	// screenshot found - check map presence
			*ext = 0;	// cut extension
			if (!FS_FileExists (va("maps/%s.bsp", name)))
				continue;

			browse_map_names[thumbs.count++] = name;
			if (thumbs.count == MAX_BROWSE_MAPS)
			{
				Com_WPrintf ("Maximum number of levelshots reached\n");
				break;
			}
		}
	}

	if (thumbs.count != oldcount)
		thumbs.current = thumbs.top = 0;

	if (thumbs.count)
		return true;

	// no maps found - free list
	FreeNamedList (browse_list);
	browse_list = NULL;
	return false;
}

static const char *DMBrowse_MenuKey (int key)
{
	int	page;

	page = thumbs.cx * thumbs.cy;

	switch (key)
	{
	case K_ESCAPE:
	case K_MOUSE2:
		M_PopMenu ();
		return menu_out_sound;

	case K_DOWNARROW:
	case K_KP_DOWNARROW:
		thumbs.current += thumbs.cx;
		break;

	case K_UPARROW:
	case K_KP_UPARROW:
		thumbs.current -= thumbs.cx;
		break;

	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
		thumbs.current++;
		break;

	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		thumbs.current--;
		break;

	case K_PGUP:
	case K_KP_PGUP:
	case K_MWHEELUP:
		thumbs.current -= page;
		thumbs.top -= page;
		break;

	case K_PGDN:
	case K_KP_PGDN:
	case K_MWHEELDOWN:
		thumbs.current += page;
		thumbs.top += page;
		break;

	case K_HOME:
		thumbs.current = 0;
		break;

	case K_END:
		thumbs.current = MAX_BROWSE_MAPS;
		break;

	case K_KP_ENTER:
	case K_ENTER:
	case K_MOUSE1:
        StartServerFunc (browse_map_names[thumbs.current]);
		return NULL;
	}

	// clump thumbs.current
	if (thumbs.current >= thumbs.count)
		thumbs.current = thumbs.count - 1;
	else if (thumbs.current < 0)
		thumbs.current = 0;

	// clump thumbs.top
	if (thumbs.top + page > thumbs.count)
		thumbs.top = ((thumbs.count + thumbs.cx - 1) / thumbs.cx - thumbs.cy) * thumbs.cx;
	if (thumbs.top < 0)
		thumbs.top = 0;

	// scroll thumbnails
	if (thumbs.current < thumbs.top)
		thumbs.top = thumbs.current - thumbs.current % thumbs.cx;
	else if (thumbs.current >= thumbs.top + page)
		thumbs.top = (thumbs.current / thumbs.cx - thumbs.cy + 1) * thumbs.cx;

	return menu_move_sound;
}

static void DrawThumbnail (int x, int y, int w, int h, char *name, bool selected)
{
	char	name2[256];
	int		max_width, text_width;

	re.DrawFill2 (x - THUMBNAIL_BORDER, y - THUMBNAIL_BORDER, w + THUMBNAIL_BORDER * 2,
			h + THUMBNAIL_BORDER * 2 + THUMBNAIL_TEXT, selected ? RGB(0,0.1,0.2) : RGB(0,0,0) /* blue/black */);

	re.DrawStretchPic (x, y, w, h, va("/levelshots/%s.pcx", name));
	max_width = w / 8;
	strcpy (name2, name);
	text_width = strlen (name2);
	if (text_width > max_width)
	{
		strcpy (&name2[max_width - 3], "...");		// make long names ends with "..."
		text_width = max_width;
	}
	Menu_DrawString (x + (w-8*text_width) / 2, y + h + (THUMBNAIL_TEXT + THUMBNAIL_BORDER - 8) / 2, name2);
}

static void DMBrowse_DrawScroller (int x0, int y0, int w)
{
	int x;

	for (x = x0; x < x0 + w; x += 8)
		re.DrawChar (x, y0, 0, C_GREEN);
}

static void DMBrowse_MenuDraw (void)
{
	int		x, y, w;

	for (y = 0; y < thumbs.cy; y++)
		for (x = 0; x < thumbs.cx; x++)
		{
			int	i;

			i = thumbs.top + y * thumbs.cx + x;
			if (i >= thumbs.count)
				continue;

			DrawThumbnail (thumbs.x0 + x * thumbs.dx,
						   thumbs.y0 + y * thumbs.dy,
						   thumbs.w, thumbs.h, browse_map_names[i], i == thumbs.current);
		}
	// draw scrollers
	w = thumbs.w * thumbs.cx;
	x = (viddef.width - w) / 2;
	if (thumbs.top > 0)
		DMBrowse_DrawScroller (x, thumbs.y0 - 8 - THUMBNAIL_BORDER, w);
	if (thumbs.top + thumbs.cx * thumbs.cy < thumbs.count)
		DMBrowse_DrawScroller (x, thumbs.y0 + thumbs.cy * thumbs.dy - THUMBNAIL_BORDER * 2, w);
}

static void M_Menu_DMBrowse_f (void)
{
	MENU_CHECK
	if (!DMBrowse_MenuInit ())
	{
		Menu_SetStatusBar (&s_startserver_menu, "No levelshots found");
		return;
	}
	Menu_SetStatusBar (&s_startserver_menu, NULL);	//?? "ESC to quit, ENTER to select ..."
	M_PushMenu (DMBrowse_MenuDraw, DMBrowse_MenuKey);
}


/*
=======================================================================

QUIT MENU

=======================================================================
*/

static const char *M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_MOUSE2:
	case 'n':
	case 'N':
		M_PopMenu ();
		break;

	case 'Y':
	case 'y':
	case K_MOUSE1:
		cls.key_dest = key_console;		//??
		CL_Quit_f ();
		break;

	default:
		break;
	}

	return NULL;
}


static void M_Quit_Draw (void)
{
	int		w, h;

	re.DrawGetPicSize (&w, &h, "quit");
	if (w >= 320 && w * 3 / 4 == h)		// this pic is for fullscreen in mode 320x240
		re.DrawStretchPic (0, 0, viddef.width, viddef.height, "quit");
	else
		re.DrawPic ((viddef.width-w)/2, (viddef.height-h)/2, "quit");
}


static void M_Menu_Quit_f (void)
{
	MENU_CHECK
	M_PushMenu (M_Quit_Draw, M_Quit_Key);
}



//=============================================================================
/* Menu Subsystem */


/*
=================
M_Init
=================
*/
void M_Init (void)
{
	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_game", M_Menu_Game_f);
		Cmd_AddCommand ("menu_loadgame", M_Menu_LoadGame_f);
		Cmd_AddCommand ("menu_savegame", M_Menu_SaveGame_f);
		Cmd_AddCommand ("menu_joinserver", M_Menu_JoinServer_f);
			Cmd_AddCommand ("menu_addressbook", M_Menu_AddressBook_f);
		Cmd_AddCommand ("menu_startserver", M_Menu_StartServer_f);
			Cmd_AddCommand ("menu_dmoptions", M_Menu_DMOptions_f);
			Cmd_AddCommand ("menu_dmbrowse", M_Menu_DMBrowse_f);
		Cmd_AddCommand ("menu_playerconfig", M_Menu_PlayerConfig_f);
			Cmd_AddCommand ("menu_downloadoptions", M_Menu_DownloadOptions_f);
		Cmd_AddCommand ("menu_credits", M_Menu_Credits_f );
	Cmd_AddCommand ("menu_multiplayer", M_Menu_Multiplayer_f );
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
		Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
}


/*
=================
M_Draw
=================
*/
void M_Draw (void)
{
	if (!m_menudepth) return;

	if (!cls.keep_console)	// do not blend whole screen when small menu painted
		re.DrawFill2 (0, 0, viddef.width, viddef.height, RGBA(0,0,0,0.5));

	m_drawfunc ();

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		S_StartLocalSound (menu_in_sound);
		m_entersound = false;
	}
}


/*
=================
M_Keydown
=================
*/
void M_Keydown (int key)
{
	const char *s;

	if (m_keyfunc)
		if ((s = m_keyfunc (key)) != NULL)
			S_StartLocalSound ((char *) s);
}