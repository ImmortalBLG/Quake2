#ifdef DYNAMIC_REF

#define Cmd_AddCommand	ri._Cmd_AddCommand
#define Cmd_RemoveCommand	ri._Cmd_RemoveCommand
#define Cmd_Argc	ri._Cmd_Argc
#define Cmd_Argv	ri._Cmd_Argv
#define Cmd_Args	ri._Cmd_Args
#define Cbuf_ExecuteText	ri._Cbuf_ExecuteText
#define Cvar_Get	ri._Cvar_Get
#define Cvar_GetVars	ri._Cvar_GetVars
#define Cvar_Set	ri._Cvar_Set
#define Cvar_SetValue	ri._Cvar_SetValue
#define Cvar_SetInteger	ri._Cvar_SetInteger
#define Cvar_VariableString	ri._Cvar_VariableString
#define Cvar_VariableValue	ri._Cvar_VariableValue
#define Cvar_VariableInt	ri._Cvar_VariableInt
#define Cvar_Clamp	ri._Cvar_Clamp
#define Cvar_ClampName	ri._Cvar_ClampName
#define Z_Malloc	ri._Z_Malloc
#define Z_Free	ri._Z_Free
#define CreateMemoryChain	ri._CreateMemoryChain
#define AllocChainBlock	ri._AllocChainBlock
#define FreeMemoryChain	ri._FreeMemoryChain
#define Hunk_Begin	ri._Hunk_Begin
#define Hunk_Alloc	ri._Hunk_Alloc
#define Hunk_End	ri._Hunk_End
#define Hunk_Free	ri._Hunk_Free
#define Sys_Milliseconds	ri._Sys_Milliseconds
#define Sys_Mkdir	ri._Sys_Mkdir
#define Com_Printf	ri._Com_Printf
#define Com_DPrintf	ri._Com_DPrintf
#define Com_WPrintf	ri._Com_WPrintf
#define Com_Error	ri._Com_Error
#define FS_FileExists	ri._FS_FileExists
#define FS_ListFiles	ri._FS_ListFiles
#define FS_LoadFile	ri._FS_LoadFile
#define FS_FreeFile	ri._FS_FreeFile
#define FS_Gamedir	ri._FS_Gamedir
#define MatchWildcard	ri._MatchWildcard
#define Vid_GetModeInfo	ri._Vid_GetModeInfo
#define Vid_MenuInit	ri._Vid_MenuInit
#define Vid_NewWindow	ri._Vid_NewWindow
#define ImageExists	ri._ImageExists
#define LoadPCX	ri._LoadPCX
#define LoadTGA	ri._LoadTGA
#define LoadJPG	ri._LoadJPG
#define WriteTGA	ri._WriteTGA
#define WriteJPG	ri._WriteJPG
#define LoadBspFile	ri._LoadBspFile

#else

void	Cmd_AddCommand (char *name, void(*func)(void));
void	Cmd_RemoveCommand (char *name);
int	Cmd_Argc (void);
char*	Cmd_Argv (int i);
char*	Cmd_Args (void);
void	Cbuf_ExecuteText (int exec_when, char *text);
cvar_t*	Cvar_Get (char *name, char *value, int flags);
void	Cvar_GetVars (cvarInfo_t *vars, int count);
cvar_t*	Cvar_Set (char *name, char *value);
cvar_t*	Cvar_SetValue (char *name, float value);
cvar_t*	Cvar_SetInteger (char *name, int value);
char*	Cvar_VariableString (char *name);
float	Cvar_VariableValue (char *name);
int	Cvar_VariableInt (char *name);
float	Cvar_Clamp (cvar_t *cvar, float low, float high);
float	Cvar_ClampName (char *name, float low, float high);
void*	Z_Malloc (int size);
void	Z_Free (void *ptr);
void*	CreateMemoryChain (void);
void*	AllocChainBlock (void *chain, int size);
void	FreeMemoryChain (void *chain);
void*	Hunk_Begin (int maxsize);
void*	Hunk_Alloc (int size);
int	Hunk_End (void);
void	Hunk_Free (void *buf);
int	Sys_Milliseconds (void);
void	Sys_Mkdir (char *path);
void	Com_Printf (char *str, ...);
void	Com_DPrintf (char *str, ...);
void	Com_WPrintf (char *str, ...);
void	Com_Error (int err_level, char *str, ...);
qboolean	FS_FileExists (char *filename);
basenamed_t*	FS_ListFiles (char *name, int *numfiles, unsigned musthave, unsigned canthave);
int	FS_LoadFile (char *name, void **buf);
void	FS_FreeFile (void *buf);
char*	FS_Gamedir (void);
qboolean	MatchWildcard (char *name, char *mask);
qboolean	Vid_GetModeInfo (int *width, int *height, int mode);
void	Vid_MenuInit (void);
void	Vid_NewWindow (int width, int height);
int	ImageExists (char *name, int stop_mask);
void	LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
void	LoadTGA (char *name, byte **pic, int *width, int *height);
void	LoadJPG (char *name, byte **pic, int *width, int *height);
qboolean	WriteTGA (char *name, byte *pic, int width, int height);
qboolean	WriteJPG (char *name, byte *pic, int width, int height, qboolean highQuality);
bspfile_t*	LoadBspFile (char *filename, qboolean clientload, unsigned *checksum);

#endif
