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
// client.h -- primary header for client

#include "qcommon.h"
#include "protocol.h"

#include "ref.h"

#include "vid.h"
#include "cl_scrn.h"
#include "sound.h"
#include "cl_input.h"
#include "keys.h"
#include "console.h"
#include "cdaudio.h"
#include "cl_playermodel.h"
#include "cl_fx.h"

//=============================================================================

//#define GUN_DEBUG		1		// uncomment to enable gun_xxx debugging commands

struct frame_t					//?? rename
{
	bool	valid;				// cleared if delta parsing was invalid
	int		serverframe;
	int		servertime;			// server time the message is valid for (in msec)
	int		deltaframe;
	byte	zonebits[MAX_MAP_ZONES/8]; // portalzone visibility bits
	player_state_t playerstate;
	int		num_entities;
	int		parse_entities;		// non-masked index into cl_parse_entities array
};


struct clEntityState_t : public entityStateEx_t
{
	bool	valid;				// when "false", additional fields are not initialized
	CAxis	axis;
	CBox	bounds;
	CVec3	center;
	float	radius;
};


struct centity_t
{
	unsigned clientInfoId;		// id from linked clientInfo_t (for detection of animation restart)
	clEntityState_t baseline;	// delta from this if not from a previous frame
	clEntityState_t current;
	clEntityState_t prev;		// will always be valid, but might just be a copy of current
	animState_t legsAnim;		// animation state for Quake3 player model
	animState_t torsoAnim;
	animState_t headAnim;		// used only angles

	int		serverframe;		// if not current, this ent isn't in the frame
	bool	teleported;			// when true, do not lerp entity state with previous state
	// particle effects
	CVec3	prevTrail;
	float	trailLen;
	int		fly_stoptime;

	inline void Teleported()
	{
		clientInfoId = 0xFFFFFFFF; // force restarting entity animations
	}
};

#define NEW_FX	(cl_newfx->integer)


extern char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
extern int num_cl_weaponmodels;

#define	CMD_BACKUP		64		// allow a lot of command backups for very fast systems

// the client_state_t structure is wiped completely at every server map change
struct client_state_t
{
	int			timeoutcount;

	int			timedemoFrames;
	int			timedemoStart;
	int			timedemoLongestFrame;

	bool		rendererReady;	// false if on new level or restarting renderer
	bool		sound_ambient;	// ambient sounds can start
	bool		forceViewFrame;	// should draw 1 frame of world scene from network data

	int			parse_entities;	// index (not anded off) into cl_parse_entities[]

	usercmd_t	cmds[CMD_BACKUP]; // each mesage will send several old cmds
	int			cmd_time[CMD_BACKUP]; // time sent, for calculating pings
	short		predicted_origins[CMD_BACKUP][3]; // for debug comparing against server

	float		predicted_step;	// for stair up smoothing
	unsigned 	predicted_step_time;

	CVec3		predicted_origin; // generated by CL_PredictMovement
	CVec3		predicted_angles;
	CVec3		prediction_error;

	frame_t		frame;			// received from server
	int			surpressCount;	// number of messages rate supressed
	frame_t		frames[UPDATE_BACKUP];
	frame_t		*oldFrame;		// points to previous frame; used for interpolation

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	CVec3		viewangles;

	int			time;			// this is the time value that the client
								// is rendering at, in msec; always <= cls.realtime
	double		ftime;			// same as "time/1000", in sec; more precisious than "time"
	int			overtime;		// amount of time clamped (used for detection of hang server); ms
	float		lerpfrac;		// between oldframe and frame

	refdef_t	refdef;

	CVec3		modelorg;		// center of client entity (used prediction for 1st person and server data for 3rd person)
	//?? replace with CAxis (forward=[0], right=-[1], up=[2])
	CVec3		v_forward, v_right, v_up; // set when refdef.angles is set

	// transient data from server
	char		layout[1024];	// HUD info
	int			inventory[MAX_ITEMS];

	bool		cinematicActive;

	// server state information
	bool		attractloop;	// running the attract loop, any key will menu
	int			servercount;	// server identification for prespawns
	char		gamedir[MAX_QPATH];	// used for recording demos only
	int			playernum;

	char		configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];

	// locally derived information from server state
	CRenderModel *model_draw[MAX_MODELS];
	CBspModel	*model_clip[MAX_MODELS];

	sfx_t		*sound_precache[MAX_SOUNDS];
	CBasicImage	*image_precache[MAX_IMAGES];		//?? used for write only

	clientInfo_t clientInfo[MAX_CLIENTS];
};

extern	client_state_t	cl;


enum connstate_t
{
	ca_uninitialized,
	ca_disconnected, 			// not talking to a server
	ca_connecting,				// sending request packets to the server
	ca_connected,				// netchan_t established, waiting for svc_serverdata
	ca_active					// game views should be displayed
};

enum keydest_t
{
	key_game, key_console, key_message, key_menu, key_bindingMenu
};

// the client_static_t structure is persistant through an arbitrary number
// of server connections
struct client_static_t
{
	connstate_t	state;
	keydest_t	key_dest;
	bool		keep_console;	// do not hide console even if menu active

	int			realtime;		// always increasing, no clamping, etc
	float		frametime;		// seconds since last frame
	bool		netFrameDropped;// true, when cl_inFps rejects input frame; used for correct prediction

	/*----- screen rendering information -----*/
	int			disable_servercount; // when we receive a frame and cl.servercount
								// > cls.disable_servercount, end loading plaque
								//????? check requirement of this var
	bool		loading;

	/*-------- connection information --------*/
	char		serverName[MAX_OSPATH];	// name of server from original connect
	netadr_t	serverAddr;
	float		connect_time;	// for connection retransmits

	netchan_t	netchan;
	int			serverProtocol;	// in case we are doing some kind of version hack

	int			challenge;		// from the server to use for connecting

	FILE		*download;		// file transfer from server
	TString<MAX_OSPATH> DownloadName, DownloadTempName;
	int			downloadPercent;

	// demo recording info must be here, so it isn't cleared on level change
	bool		demorecording;
	bool		demowaiting;	// don't record until a non-delta message is received
	FILE		*demofile;

	bool		newprotocol;
	bool		cheatsEnabled;
};

extern client_static_t	cls;

//=============================================================================

//
// cvars
//
extern	cvar_t	*cl_gun;
extern	cvar_t	*cl_add_blend;
extern	cvar_t	*cl_add_lights;
extern	cvar_t	*cl_add_particles;
extern	cvar_t	*cl_add_entities;
extern	cvar_t	*cl_predict;
extern	cvar_t	*cl_footsteps;
extern	cvar_t	*cl_noskins;

extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showmiss;
extern	cvar_t	*cl_showclamp;

extern	cvar_t	*cl_paused;

extern	cvar_t	*cl_vwep;
extern	cvar_t	*hand;
extern	cvar_t	*cl_3rd_person;
extern	cvar_t	*cl_cameraDist, *cl_cameraHeight, *cl_cameraAngle;

extern  cvar_t  *cl_extProtocol;

extern	cvar_t	*cl_newfx;
extern	cvar_t	*cl_showbboxes;
extern	cvar_t	*r_sfx_pause;

extern	cvar_t	*gender, *gender_auto, *skin;

extern	centity_t *cl_entities;	// [MAX_EDICTS]

// the cl_parse_entities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
#define	MAX_PARSE_ENTITIES	1024
extern	clEntityState_t	cl_parse_entities[MAX_PARSE_ENTITIES];

//
// cl_ents.cpp
//
void CL_ParseDelta(clEntityState_t *from, clEntityState_t *to, int number, unsigned bits, bool baseline);
void CL_AddEntityBox(clEntityState_t *st, unsigned rgba);
void CL_AddEntities();
void CL_ParseFrame();

//
// cl_main.cpp
//
//void CL_Init(); -- declared in qcommon.h -- init from Com_Init()

void CL_Pause(bool enable);
void CL_Disconnect();
void CL_Disconnect_f();
void CL_GetChallengePacket();

#define NUM_ADDRESSBOOK_ENTRIES 9
void CL_PingServers_f();
void CL_Snd_Restart_f();
void CL_RegisterSounds();

void CL_WriteDemoMessage();

void CL_ClearState();


//
// cl_parse.cpp
//
extern const char *svc_strings[svc_last];

void CL_ParseServerMessage();

#define SHOWNET(s)	\
	if (cl_shownet->integer >= 2) appPrintf("%3d:%s\n", net_message.readcount-1, s);

void CL_ParseClientinfo(int player);
void CL_UpdatePlayerClientInfo();

//
// cl_view.cpp
//
extern CBasicImage *railSpiralShader, *railRingsShader, *railBeamShader;
#if GUN_DEBUG
extern int gun_frame;
extern CRenderModel *gun_model;
#endif
extern float r_blend[4];

void V_Init();
void V_InitRenderer();
bool V_RenderView();

void V_AddEntity(entity_t *ent);
void V_AddEntity2(entity_t *ent);
void AddEntityWithEffects(entity_t *ent, unsigned fx);
void AddEntityWithEffects2(entity_t *ent, unsigned fx);

void V_AddLight(const CVec3 &org, float intensity, float r, float g, float b);
float CalcFov(float fov_x, float width, float height);

//
// cl_tent.cpp
//
//!! MOVE to cl_fx.h
void CL_RegisterTEntSounds();
void CL_RegisterTEntModels();
void CL_SmokeAndFlash(const CVec3 &origin);
void CL_ClearTEnts();
void CL_ParseTEnt();
void CL_AddTEnts();

//
// menu.cpp & qmenu.cpp
//
void M_Init();
void M_Keydown(int key);
void M_Draw();
void M_Menu_Main_f();
void M_ForceMenuOff();
void M_ForceMenuOn();
void M_AddToServerList(netadr_t adr, const char *info);

struct menuFramework_t;
extern menuFramework_t *m_current;


//
// cl_pred.cpp
//
void CL_EntityTrace(trace_t &tr, const CVec3 &start, const CVec3 &end, const CBox &bounds, int contents);
void CL_Trace(trace_t &tr, const CVec3 &start, const CVec3 &end, const CBox &bounds, int contents);
void CL_PredictMovement();
void CL_CheckPredictionError();

//
// cl_download.cpp
//
void CL_ParseDownload();
void CL_Download_f(bool usage, int argc, char **argv);
void CL_Precache_f(int argc, char **argv);
