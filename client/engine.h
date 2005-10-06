//--------- cvars -----------
cvar_t*	Cvar_Get (const char *name, const char *value, int flags = 0);
void	Cvar_GetVars (const cvarInfo_t *vars, int count);
cvar_t*	Cvar_Set (const char *name, const char *value);
cvar_t*	Cvar_SetValue (const char *name, float value);
cvar_t*	Cvar_SetInteger (const char *name, int value);
const char* Cvar_VariableString (const char *name);
float Cvar_VariableValue (const char *name);
int	Cvar_VariableInt (const char *name);
float Cvar_Clamp (cvar_t *cvar, float low, float high);

//--------- console ---------
void Com_DPrintf (const char *str, ...);

//---------- files ----------
const char* FS_Gamedir ();
void FS_CopyFile (const char *src, const char *dst);
void FS_CopyFiles (const char *srcMask, const char *dstDir);
void FS_RemoveFiles (const char *mask);

//------- video mode --------
void Vid_Restart (void);
bool Vid_GetModeInfo (int *width, int *height, int mode);

//--------- images ----------
int ImageExists (const char *name, int stop_mask = IMAGE_ANY);
void LoadPCX (const char *name, byte *&pic, byte *&palette, int &width, int &height);
void LoadTGA (const char *name, byte *&pic, int &width, int &height);
void LoadJPG (const char *name, byte *&pic, int &width, int &height);
bool WriteTGA (const char *name, byte *pic, int width, int height);
bool WriteJPG (const char *name, byte *pic, int width, int height, bool highQuality);

//--------- models ----------
bspfile_t* LoadBspFile (const char *filename, bool clientload, unsigned *checksum);
// mins/maxs may be NULL for {0,0,0}
void CM_BoxTrace (trace_t &tr, const CVec3 &start, const CVec3 &end, const CBox &bounds, int headnode, int brushmask);
void CM_TransformedBoxTrace (trace_t &tr, const CVec3 &start, const CVec3 &end, const CBox &bounds, int headnode, int brushmask, const CVec3 &origin, const CVec3 &angles);
void CM_TransformedBoxTrace (trace_t &tr, const CVec3 &start, const CVec3 &end, const CBox &bounds, int headnode, int brushmask, const CVec3 &origin, const CAxis &axis);
int CM_BrushTrace (const CVec3 &start, const CVec3 &end, int *brushes, int maxBrushes);
int CM_RefineBrushTrace (const CVec3 &start, const CVec3 &end, int *brushes, int numBrushes);

//-- system-dependent code --
#ifdef _WIN32
void*	Vid_CreateWindow (int width, int height, bool fullscreen);
void	Vid_DestroyWindow (bool force);
#endif
