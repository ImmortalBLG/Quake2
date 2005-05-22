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

#ifndef QCOMMON_H
#define QCOMMON_H

#include "Core.h"

//!!!! temp:
#define Com_FatalError	appFatalError
#define Com_DropError	appNonFatalError

//-------- declarations, required for ref_xxxx.h --------------

// forward declarations
struct cvar_t;
struct cvarInfo_t;
struct trace_t;
struct bspfile_t;
struct CAxis;

typedef float vec3_t[3];

// image flags
#define IMAGE_PCX	1
#define IMAGE_WAL	2
#define IMAGE_TGA	4
#define IMAGE_JPG	8

#define IMAGE_8BIT	(IMAGE_PCX|IMAGE_WAL)
#define IMAGE_32BIT	(IMAGE_TGA|IMAGE_JPG)
#define IMAGE_ANY	(IMAGE_8BIT|IMAGE_32BIT)

// exports for renderer
#include "../client/engine_intf.h"

// declarations for game system (!!) + math
#include "q_shared2.h"

//--------------- some constants ------------------------------

// version number

#define	VERSION		4.11

#ifdef WIN32

#define BUILDSTRING "Win32"

#ifdef _M_IX86
#define	CPUSTRING	"x86"
#elif defined _M_ALPHA
#define	CPUSTRING	"AXP"
#endif

#elif defined __linux__

#define BUILDSTRING "Linux"

#ifdef __i386__
#define CPUSTRING "i386"
#elif defined __alpha__
#define CPUSTRING "axp"
#else
#define CPUSTRING "Unknown"
#endif

#elif defined __sun__

#define BUILDSTRING "Solaris"

#ifdef __i386__
#define CPUSTRING "i386"
#else
#define CPUSTRING "sparc"
#endif

#else	// unknown platform

#define BUILDSTRING "Unknown"
#define	CPUSTRING	"Unknown"

#endif


#define VERSION_STR		STR(VERSION) " " CPUSTRING " " __DATE__ " " BUILDSTRING

// some configuration

#define APPNAME		"Quake2"
#define	BASEDIRNAME	"baseq2"
#define CONFIGNAME	"config.cfg"
// if CD_PATH is not defined, CD checking and adding its paths will not be performed
#define CD_PATH		"install/data"
#define CD_CHECK	"install/data/quake2.exe"	// file used for CD validation

#define NEW_PROTOCOL_ID "gildor"

#define SAVEGAME_DIRECTORY			"save"
#define SAVEGAME_SERVER_EXTENSION	"sv2"
#define SAVEGAME_GAME_EXTENSION		"sav"
#define SAVEGAME_VARS_EXTENSION		"ssv"


#ifndef DEDICATED_ONLY
#define DEDICATED	dedicated->integer
#else
#define DEDICATED	1
#endif


/*-----------------------------------------------------------------------------
	msg.cpp
-----------------------------------------------------------------------------*/

class sizebuf_t
{
public:
	//!! should be private:
	byte	*data;
	int		maxsize;						// size of buffer
	bool	overflowed;						// set to true if the buffer size failed
	//!! should be public:
	bool	allowoverflow;					// if false, do a error when overflowed
	int		cursize;						// writting cursor
	int		readcount;						// reading cursor

	// Functions
	void Init (void *data, int length);		// setup buffer
	inline Clear ()							// clear writting
	{
		cursize = 0;
		overflowed = false;
	}
	void Write (const void *data, int length);
	inline void Write (sizebuf_t &from)
	{
		Write (from.data, from.cursize);
	}
};

void	MSG_WriteChar (sizebuf_t *sb, int c);
void	MSG_WriteByte (sizebuf_t *sb, int c);
void	MSG_WriteShort (sizebuf_t *sb, int c);
void	MSG_WriteLong (sizebuf_t *sb, int c);
void	MSG_WriteFloat (sizebuf_t *sb, float f);
void	MSG_WriteString (sizebuf_t *sb, const char *s);
void	MSG_WritePos (sizebuf_t *sb, vec3_t pos);
void	MSG_WriteAngle (sizebuf_t *sb, float f);
void	MSG_WriteAngle16 (sizebuf_t *sb, float f);
void	MSG_WriteDir (sizebuf_t *sb, vec3_t vector);


inline void MSG_BeginReading (sizebuf_t *msg)
{
	msg->readcount = 0;
}


int		MSG_ReadChar (sizebuf_t *sb);
int		MSG_ReadByte (sizebuf_t *sb);
int		MSG_ReadShort (sizebuf_t *sb);
int		MSG_ReadLong (sizebuf_t *sb);
float	MSG_ReadFloat (sizebuf_t *sb);
char	*MSG_ReadString (sizebuf_t *sb);

void	MSG_ReadPos (sizebuf_t *sb, vec3_t pos);
float	MSG_ReadAngle (sizebuf_t *sb);
float	MSG_ReadAngle16 (sizebuf_t *sb);

void	MSG_ReadDir (sizebuf_t *sb, vec3_t vector);

void	MSG_ReadData (sizebuf_t *sb, void *buffer, int size);

/*-----------------------------------------------------------------------------
	Delta compression for quake2 protocol (entdelta.cpp)
-----------------------------------------------------------------------------*/

// entity_state_t communication

void	MSG_WriteDeltaEntity (sizebuf_t *msg, const entity_state_t *from, entity_state_t *to, bool force, bool newentity);
void	MSG_WriteRemoveEntity (sizebuf_t *msg, int num);

int		MSG_ReadEntityBits (sizebuf_t *msg, unsigned *bits, bool *remove);
void	MSG_ReadDeltaEntity (sizebuf_t *msg, const entity_state_t *from, entity_state_t *to, unsigned bits);

void	MSG_ReadDeltaUsercmd (sizebuf_t *smg, const usercmd_t *from, usercmd_t *to);
void	MSG_WriteDeltaUsercmd (sizebuf_t *msg, const usercmd_t *from, usercmd_t *to);

void	MSG_ReadDeltaPlayerstate (sizebuf_t *msg, const player_state_t *oldState, player_state_t *newState);
void	MSG_WriteDeltaPlayerstate (sizebuf_t *msg, const player_state_t *oldState, player_state_t *newState);


/*-----------------------------------------------------------------------------
	anorms.cpp
-----------------------------------------------------------------------------*/

#define NUMVERTEXNORMALS	162
// used by:
// 1. common.cpp: MSG_WriteDir(), MSG_ReadDir()
// 2. cl_fx.cpp:  CL_FlyParticles(), CL_BfgParticles()
extern	/*const*/ vec3_t bytedirs[NUMVERTEXNORMALS];

void	InitByteDirs ();


/*-----------------------------------------------------------------------------
	key/value info strings
-----------------------------------------------------------------------------*/

#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512

const char * Info_ValueForKey (const char *s, const char *key);
void	Info_SetValueForKey (char *s, const char *key, const char *value);
void	Info_Print (const char *s);


/*-----------------------------------------------------------------------------
	crc.cpp
-----------------------------------------------------------------------------*/


byte	Com_BlockSequenceCRCByte (byte *base, int length, int sequence);


/*
==============================================================

PROTOCOL

==============================================================
*/

// declaration from server.h
typedef enum {
	ss_dead,
	ss_loading,
	ss_game,
	ss_cinematic,
	ss_demo,
	ss_pic
} server_state_t;
// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)


// protocol.h -- communications protocols

#define	PROTOCOL_VERSION	34

//=========================================

#define	PORT_MASTER	27900
#define	PORT_CLIENT	27901
#define	PORT_SERVER	27910

//=========================================

#define	UPDATE_BACKUP	16	// copies of entity_state_t to keep buffered
							// must be power of two
#define	UPDATE_MASK		(UPDATE_BACKUP-1)



// the svc_strings[] array in cl_parse.cpp should mirror this
// server to client
enum
{
	svc_bad,

	// these ops are known to the game dll
	svc_muzzleflash,
	svc_muzzleflash2,
	svc_temp_entity,
	svc_layout,
	svc_inventory,

	// the rest are private to the client and server
	svc_nop,
	svc_disconnect,
	svc_reconnect,
	svc_sound,					// <see code>
	svc_print,					// [byte] id [string] null terminated string
	svc_stufftext,				// [string] stuffed into client's console buffer, should be \n terminated
	svc_serverdata,				// [long] protocol ...
	svc_configstring,			// [short] [string]
	svc_spawnbaseline,
	svc_centerprint,			// [string] to put in center of the screen
	svc_download,				// [short] size [size bytes]
	svc_playerinfo,				// variable
	svc_packetentities,			// [...]
	svc_deltapacketentities,	// [...]
	svc_frame,

	svc_last					// number of commands
};

// client to server
enum
{
	clc_bad,
	clc_nop,
	clc_move,					// [usercmd_t]
	clc_userinfo,				// [userinfo string]
	clc_stringcmd				// [string message]
};


// a sound without an ent or pos will be a local only sound
#define	SND_VOLUME			(1<<0)		// a byte
#define	SND_ATTENUATION		(1<<1)		// a byte
#define	SND_POS				(1<<2)		// three coordinates
#define	SND_ENT				(1<<3)		// a short 0-2: channel, 3-12: entity
#define	SND_OFFSET			(1<<4)		// a byte, msec offset from frame start

#define DEFAULT_SOUND_PACKET_VOLUME			1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION	1.0



/*-----------------------------------------------------------------------------
	Command text buffering and command execution (cmd.cpp)
-----------------------------------------------------------------------------*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

*/


void	Cbuf_AddText (const char *text);
// as new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.

void	Cbuf_Execute (void);
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!

void	Cbuf_CopyToDefer (void);
void	Cbuf_InsertFromDefer (void);
// These two functions are used to defer any pending commands while a map
// is being loaded (NOTE: cannot simply disable Cbuf_Execute(): while map is loading,
// there can be executed different commands)


//!! here for command completion (current version) only
class CAlias : public CStringItem
{
public:
	char	*value;
};
extern TList<CAlias> AliasList;

class CCommand : public CStringItem
{
public:
	int		flags;
	void (*func) ();
};
extern TList<CCommand> CmdList;


void	Cmd_Init (void);

//--bool	RegisterCommand (char *cmd_name, void (*func)(void), int flags);
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_stringcmd instead of executed locally
//--void	UnregisterCommand (const char *cmd_name);

// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are always safe.

bool	Cmd_ExecuteString (const char *text);
// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console; if command is unknown, will return false

void	Cmd_ForwardToServer (int argc, char **argv);
// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.
// NOTE: implemented in client

void	Cmd_WriteAliases (FILE *f);


/*-----------------------------------------------------------------------------
	cvar.cpp
-----------------------------------------------------------------------------*/

struct cvarInfo_t
{
	cvar_t	**var;				// destination, may be NULL
	const char *string;			// name[/value]
	unsigned flags;
};

// NOTES:
//	CVAR_FULL() useful for any case (especially for vars with value == "")
//	CVAR_VAR() useful for vars, which C name is the same as cvar.name
//	CVAR_VAR2() useful for vars, which C name differs from cvar.name
//	CVAR_GET() useful for retreiving var pointer without setting var parameters
//	CVAR_NULL() useful for setting var parameters without retreiving its pointer
#define CVAR_BEGIN(name)				static const cvarInfo_t name[] = {
#define CVAR_FULL(pvar,name,value,flags)	{pvar, name"="value, flags}	// any parameters, value not stringified
#define CVAR_VAR(name,value,flags)			CVAR_FULL(&name, #name, #value, flags)	// var == name, non-empty value, flags
#define CVAR_VAR2(var,name,value,flags)		CVAR_FULL(&var,  #name, #value, flags)	// var != name, non-empty value, flags
#define CVAR_GET(name)						CVAR_FULL(&name, #name, "",		CVAR_NODEFAULT)	// just set var
#define CVAR_NULL(name,value,flags)			CVAR_FULL(NULL,  #name, #value, flags)	// register cvar without saving pointer
#define CVAR_END						};


extern cvar_t *cvar_vars;
extern int	cvar_initialized;

//--cvar_t *Cvar_Get (char *var_name, char *value, int flags);
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags

cvar_t 	*Cvar_FullSet (const char *var_name, const char *value, unsigned flags);

//--cvar_t	*Cvar_SetValue (char *var_name, float value);
//--cvar_t	*Cvar_SetInteger (char *var_name, int value);
//--cvar_t 	*Cvar_Set (char *var_name, char *value);				// will create the variable if it doesn't exist
cvar_t	*Cvar_ForceSet (const char *var_name, const char *value);	// will set the variable even if NOSET or LATCH

// expands value to a string and calls Cvar_Set
//--float	Cvar_Clamp (cvar_t *cvar, float low, float high);

//--float	Cvar_VariableValue (char *var_name);
//--int		Cvar_VariableInt (char *var_name);
// returns 0 if not defined or non numeric

//--const char	*Cvar_VariableString (char *var_name);
// returns an empty string if not defined

void	Cvar_GetLatchedVars (void);
// any CVAR_LATCHED variables that have been set will now take effect

bool	Cvar_Command (int argc, char **argv);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void	Cvar_WriteVariables (FILE *f, int includeMask, int excludeMask, const char *header);
// appends lines containing "set variable value" for all variables with the archive flag set.

void	Cvar_Cheats (bool enable);
void	Cvar_Init (void);

const char *Cvar_Userinfo (void);
// returns an info string containing all the CVAR_USERINFO cvars

const char *Cvar_Serverinfo (void);
// returns an info string containing all the CVAR_SERVERINFO cvars

extern bool userinfo_modified;
// this is set each time a CVAR_USERINFO variable is changed
// so that the client knows to send it to the server


/*-----------------------------------------------------------------------------
	net_chan.cpp, net_{platform}.cpp
-----------------------------------------------------------------------------*/


#define	PORT_ANY		-1

#define	MAX_MSGLEN		16384		// max length of a message
#define MAX_MSGLEN_OLD	1400		// MAX_MSGLEN for old clients and for IPX protocol

typedef enum {NA_LOOPBACK, NA_BROADCAST, NA_IP, NA_IPX, NA_BROADCAST_IPX} netadrtype_t;

typedef enum {NS_CLIENT, NS_SERVER} netsrc_t;

typedef struct
{
	netadrtype_t type;
	byte	ip[4];
	byte	ipx[10];
	unsigned short port;
} netadr_t;

bool	IPWildcard (netadr_t *a, char *mask);

void	NET_Init (void);
void	NET_Shutdown (void);

void	NET_Config (bool multiplayer);

bool	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_msg);
void	NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to);

bool	NET_CompareAdr (netadr_t *a, netadr_t *b);
bool	NET_CompareBaseAdr (netadr_t *a, netadr_t *b);
bool	NET_IsLocalAddress (netadr_t *adr);
char	*NET_AdrToString (netadr_t *a);
bool	NET_StringToAdr (const char *s, netadr_t *a);


class netchan_t
{
public:
	bool	fatal_error;

	netsrc_t sock;

	int		dropped;			// between last packet and previous

	int		last_received;		// for timeouts
	int		last_sent;			// for retransmits

	netadr_t remote_address;
	int		port;				// qport value to write when transmitting

	// sequencing variables
	int		incoming_sequence;
	int		incoming_acknowledged;
	int		incoming_reliable_acknowledged;	// single bit

	int		incoming_reliable_sequence;		// single bit, maintained local

	int		outgoing_sequence;
	int		reliable_sequence;				// single bit
	int		last_reliable_sequence;			// sequence number of last send

	// reliable staging and holding areas
	sizebuf_t message;		// writing buffer to send to server
	byte	message_buf[MAX_MSGLEN-16];		// leave space for header

	// message is copied to this buffer when it is first transfered
	int		reliable_length;
	byte	reliable_buf[MAX_MSGLEN-16];	// unacked reliable message

	// methods

	void Setup (netsrc_t sock, netadr_t adr, int qport);
	// returns true if the last reliable message has acked; UNUSED??
	inline bool CanReliable ()			// used in net_chan.cpp only
	{
		return (reliable_length == 0);	// if != 0 -- waiting for ack
	}
	bool NeedReliable ();
	void Transmit (void *data, int length);
	bool Process (sizebuf_t *msg);
};

extern netadr_t	net_from;
extern sizebuf_t net_message;


void	Netchan_Init (void);
void	Netchan_OutOfBandPrint (netsrc_t net_socket, netadr_t adr, const char *format, ...);


/*-----------------------------------------------------------------------------
	cmodel.cpp
-----------------------------------------------------------------------------*/

// cmodel_t.flags
#define CMODEL_ALPHA	1
#define CMODEL_MOVABLE	2

struct cmodel_t;		// full declaration in cmodel.h

extern char map_name[];
extern bool map_clientLoaded;

cmodel_t *CM_LoadMap (const char *name, bool clientload, unsigned *checksum);
cmodel_t *CM_InlineModel (const char *name);	// *1, *2, etc

int		CM_NumClusters (void);
int		CM_NumInlineModels (void);
const char *CM_EntityString (void);

// creates a clipping hull for an arbitrary box
int		CM_HeadnodeForBox (vec3_t mins, vec3_t maxs);

// returns an OR'ed contents mask
int		CM_PointContents (const vec3_t p, int headnode);
int		CM_TransformedPointContents (const vec3_t p, int headnode, const vec3_t origin, const vec3_t angles);
int		CM_TransformedPointContents (const vec3_t p, int headnode, const vec3_t origin, const CAxis &axis);

//--void	CM_BoxTrace (trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask);
//--void	CM_TransformedBoxTrace (trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask, vec3_t origin, vec3_t angles);

byte	*CM_ClusterPVS (int cluster);
byte	*CM_ClusterPHS (int cluster);

int		CM_PointLeafnum (const vec3_t p, int num = 0);

// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int		CM_BoxLeafnums (const vec3_t mins, const vec3_t maxs, int *list, int listsize, int *topnode = NULL, int headnode = 0);

int		CM_LeafContents (int leafnum);
int		CM_LeafCluster (int leafnum);
int		CM_LeafArea (int leafnum);

void	CM_SetAreaPortalState (int portalnum, bool open);
bool	CM_AreasConnected (int area1, int area2);

int		CM_WriteAreaBits (byte *buffer, int area);
bool	CM_HeadnodeVisible (int headnode, byte *visbits);

void	CM_WritePortalState (FILE *f);
void	CM_ReadPortalState (FILE *f);


/*-----------------------------------------------------------------------------
	Player movement code (pmove.cpp)
	Common between server and client so prediction matches
-----------------------------------------------------------------------------*/

extern float pm_airaccelerate;

void	Pmove (pmove_t *pmove);


/*-----------------------------------------------------------------------------
	File system (files.cpp)
-----------------------------------------------------------------------------*/

extern cvar_t	*fs_gamedirvar;

#define LIST_FILES	1
#define LIST_DIRS	2

class QFILE;		// empty declaration

void	FS_InitFilesystem (void);
bool	FS_SetGamedir (const char *dir);
//--char	*FS_Gamedir (void);
char	*FS_NextPath (const char *prevpath);
void	FS_LoadGameConfig (void);

QFILE	*FS_FOpenFile (const char *filename, int *plen = NULL);
//--bool FS_FileExists (char *filename);
void	FS_FCloseFile (QFILE *f);
void	FS_Read (void *buffer, int len, QFILE *f);
// properly handles partial reads

//--void*	FS_LoadFile (const char *path, unsigned *len = NULL);
// a null buffer will just return the file length without loading
// a -1 length is not present
//--void	FS_FreeFile (void *buffer);

//--void	FS_CreatePath (char *path);

void	Sys_Mkdir (char *path);

const char	*Sys_FindFirst (const char *path, int flags);
const char	*Sys_FindNext (void);
void		Sys_FindClose (void);
bool		Sys_FileExists (const char *path, int flags);


/*------------- Miscellaneous -----------------*/

// debugging
void	DebugPrintf (const char *fmt, ...);


extern	int linewidth;		// for functions, which wants to perform advanced output formatting

void	Com_BeginRedirect (char *buffer, int buffersize, void (*flush)(char*));
void	Com_EndRedirect (void);

void	NORETURN Com_Quit (void);

server_state_t Com_ServerState (void);		// this should have just been a cvar...
void	Com_SetServerState (server_state_t state);

// md4.cpp
unsigned Com_BlockChecksum (void *buffer, int length);

#ifdef DYNAMIC_REF
// Using inline version will grow executable by ~2Kb (cl_fx.cpp uses a lots of [c|f]rand() calls)
// BUT: require this for dynamic renderer (or move to "engine.h" or to Core)
// 0 to 1
inline float frand (void)
{
	return rand() * (1.0f/RAND_MAX);
}

// -1 to 1
inline float crand (void)
{
	return rand() * (2.0f/RAND_MAX) - 1;
}
#else
float frand (void);
float crand (void);
#endif


extern cvar_t	*developer;
extern cvar_t	*dedicated;
extern cvar_t	*com_speeds;
extern cvar_t	*timedemo;
extern cvar_t	*timescale;
extern cvar_t	*sv_cheats;

// com_speeds times
extern unsigned time_before_game, time_after_game, time_before_ref, time_after_ref;

bool	Com_CheckCmdlineVar (const char *name);

void	Com_Init (const char *cmdline);
void	Com_Frame (float msec);
void	Com_Shutdown (void);

// this is in the client code, but can be used for debugging from server
void	SCR_DebugGraph (float value, int color);


/*-----------------------------------------------------------------------------
	Non-portable system services (sys_*.cpp)
-----------------------------------------------------------------------------*/

void	Sys_Init (void);

void	*Sys_GetGameAPI (void *parms);
void	Sys_UnloadGame (void);
// loads the game dll and calls the api init function

char	*Sys_ConsoleInput (void);
void	Sys_ConsoleOutput (const char *string);
void	Sys_SendKeyEvents (void);
void	NORETURN Sys_Quit (void);
void	Sys_CopyProtect (void);


/*-----------------------------------------------------------------------------
	Client / server systems
-----------------------------------------------------------------------------*/

void	CL_Init (void);
void	CL_Drop (bool fromError = false);
void	CL_Shutdown (bool error);
void	CL_Frame (float msec, float realMsec);

void	Key_Init (void);
void	Con_Print (const char *text);
void	SCR_BeginLoadingPlaque (void);

void	SV_Init (void);
void	SV_Shutdown (const char *finalmsg, bool reconnect);
void	SV_Frame (float msec);


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

// from qfiles.h -- but required more frequently (may be, move all MAX_MAP_XXX consts here?)
#define MAX_MAP_AREAS	256

// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// lower bits are stronger, and will eat weaker brushes completely

#define	CONTENTS_SOLID			0x00000001	// an eye is never valid in a solid
#define	CONTENTS_WINDOW			0x00000002	// translucent, but not watery
#define	CONTENTS_AUX			0x00000004
#define	CONTENTS_LAVA			0x00000008
#define	CONTENTS_SLIME			0x00000010
#define	CONTENTS_WATER			0x00000020
#define	CONTENTS_MIST			0x00000040
#define CONTENTS_ALPHA			0x00000080	// from Kingping - can shoot through this

// remaining contents are non-visible, and don't eat brushes

#define	CONTENTS_AREAPORTAL		0x00008000

#define	CONTENTS_PLAYERCLIP		0x00010000
#define	CONTENTS_MONSTERCLIP	0x00020000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x00040000
#define	CONTENTS_CURRENT_90		0x00080000
#define	CONTENTS_CURRENT_180	0x00100000
#define	CONTENTS_CURRENT_270	0x00200000
#define	CONTENTS_CURRENT_UP		0x00400000
#define	CONTENTS_CURRENT_DOWN	0x00800000

#define	CONTENTS_ORIGIN			0x01000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x02000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x04000000
#define	CONTENTS_DETAIL			0x08000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|	\
		CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

// surface flags
#define	SURF_LIGHT		0x0001		// value will hold the light strength

#define	SURF_SLICK		0x0002		// effects game physics

#define	SURF_SKY		0x0004		// don't draw, but add to skybox
#define	SURF_WARP		0x0008		// turbulent water warp
#define	SURF_TRANS33	0x0010
#define	SURF_TRANS66	0x0020
#define	SURF_FLOWING	0x0040		// scroll towards angle
#define	SURF_NODRAW		0x0080		// don't bother referencing the texture

// added since 4.00
// Kingpin (used for non-KP maps too)
#define SURF_ALPHA		0x1000
#define	SURF_SPECULAR	0x4000		// have a bug in KP's q_shared.h: SPECULAR and DIFFUSE consts are 0x400 and 0x800
#define	SURF_DIFFUSE	0x8000

#define SURF_AUTOFLARE	0x2000		// just free flag (should use extra struc for dtexinfo_t !!)



#endif // QCOMMON_H
