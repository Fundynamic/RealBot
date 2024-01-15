/**
  * RealBot : Artificial Intelligence
  * Version : Work In Progress
  * Author  : Stefan Hendriks
  * Url     : http://realbot.bots-united.com
  **
  * DISCLAIMER
  *
  * History, Information & Credits: 
  * RealBot is based partially upon the HPB-Bot Template #3 by Botman
  * Thanks to Ditlew (NNBot), Pierre Marie Baty (RACCBOT), Tub (RB AI PR1/2/3)
  * Greg Slocum & Shivan (RB V1.0), Botman (HPB-Bot) and Aspirin (JOEBOT). And
  * everybody else who helped me with this project.
  * Storage of Visibility Table using BITS by Cheesemonster.
  *
  * Some portions of code are from other bots, special thanks (and credits) go
  * to (in no specific order):
  *
  * Pierre Marie Baty
  * Count-Floyd
  *  
  * !! BOTS-UNITED FOREVER !!
  *  
  * This project is open-source, it is protected under the GPL license;
  * By using this source-code you agree that you will ALWAYS release the
  * source-code with your project.
  *
  **/

#ifndef ENGINE_H
#define ENGINE_H

#include <const.h>
#include <eiface.h>

// engine prototypes (from engine\eiface.h)...
int pfnPrecacheModel(char *s);
int pfnPrecacheSound(char *s);
void pfnSetModel(edict_t * e, const char *m);
int pfnModelIndex(const char *m);
int pfnModelFrames(int modelIndex);
void pfnSetSize(edict_t * e, const float *rgflMin, const float *rgflMax);
void pfnChangeLevel(const char *s1, const char *s2);
void pfnGetSpawnParms(edict_t * ent);
void pfnSaveSpawnParms(edict_t * ent);
float pfnVecToYaw(const float *rgflVector);
void pfnVecToAngles(const float *rgflVectorIn, float *rgflVectorOut);
void pfnMoveToOrigin(edict_t * ent, const float *pflGoal, float dist,
                     int iMoveType);
void pfnChangeYaw(edict_t * ent);
void pfnChangePitch(edict_t * ent);
edict_t *pfnFindEntityByString(edict_t * pEdictStartSearchAfter,
                               const char *pszField, const char *pszValue);
int pfnGetEntityIllum(edict_t * pEnt);
edict_t *pfnFindEntityInSphere(edict_t * pEdictStartSearchAfter,
                               const float *org, float rad);
edict_t *pfnFindClientInPVS(edict_t * pEdict);
edict_t *pfnEntitiesInPVS(edict_t * pplayer);
void pfnMakeVectors(const float *rgflVector);
void pfnAngleVectors(const float *rgflVector, float *forward, float *right,
                     float *up);
edict_t *pfnCreateEntity();
void pfnRemoveEntity(edict_t * e);
edict_t *pfnCreateNamedEntity(int className);
void pfnMakeStatic(edict_t * ent);
int pfnEntIsOnFloor(edict_t * e);
int pfnDropToFloor(edict_t * e);
int pfnWalkMove(edict_t * ent, float yaw, float dist, int iMode);
void pfnSetOrigin(edict_t * e, const float *rgflOrigin);
void pfnEmitSound(edict_t * entity, int channel, const char *sample,    /*int */
                  float volume, float attenuation, int fFlags, int pitch);
void pfnEmitAmbientSound(edict_t * entity, float *pos, const char *samp,
                         float vol, float attenuation, int fFlags,
                         int pitch);
void pfnTraceLine(const float *v1, const float *v2, int fNoMonsters,
                  edict_t * pentToSkip, TraceResult * ptr);
void pfnTraceToss(edict_t * pent, edict_t * pentToIgnore,
                  TraceResult * ptr);
int pfnTraceMonsterHull(edict_t * pEdict, const float *v1, const float *v2,
                        int fNoMonsters, edict_t * pentToSkip,
                        TraceResult * ptr);
void pfnTraceHull(const float *v1, const float *v2, int fNoMonsters,
                  int hullNumber, edict_t * pentToSkip, TraceResult * ptr);
void pfnTraceModel(const float *v1, const float *v2, int hullNumber,
                   edict_t * pent, TraceResult * ptr);
const char *pfnTraceTexture(edict_t * pTextureEntity, const float *v1,
                            const float *v2);
void pfnTraceSphere(const float *v1, const float *v2, int fNoMonsters,
                    float radius, edict_t * pentToSkip, TraceResult * ptr);
void pfnGetAimVector(edict_t * ent, float speed, float *rgflReturn);
void pfnServerCommand(char *str);
void pfnServerExecute();
void pfnClientCommand(edict_t * pEdict, const char *szFmt, ...);
void pfnParticleEffect(const float *org, const float *dir, float color,
                       float count);
void pfnLightStyle(int style, char *val);
int pfnDecalIndex(const char *name);
int pfnPointContents(const float *rgflVector);
void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin,
                     edict_t * edict);
void pfnMessageEnd();
void pfnWriteByte(int iValue);
void pfnWriteChar(int iValue);
void pfnWriteShort(int iValue);
void pfnWriteLong(int iValue);
void pfnWriteAngle(float flValue);
void pfnWriteCoord(float flValue);
void pfnWriteString(const char *sz);
void pfnWriteEntity(int iValue);
void pfnCVarRegister(cvar_t * pCvar);
float pfnCVarGetFloat(const char *szVarName);
const char *pfnCVarGetString(const char *szVarName);
void pfnCVarSetFloat(const char *szVarName, float flValue);
void pfnCVarSetString(const char *szVarName, const char *szValue);
void pfnAlertMessage(ALERT_TYPE atype, char *szFmt, ...);
void pfnEngineFprintf(FILE * pfile, char *szFmt, ...);
#ifdef _WIN32
void *pfnPvAllocEntPrivateData(edict_t * pEdict, __int32 cb);
#else
void *pfnPvAllocEntPrivateData(edict_t * pEdict, int cb);
#endif
void *pfnPvEntPrivateData(edict_t * pEdict);
void pfnFreeEntPrivateData(edict_t * pEdict);
const char *pfnSzFromIndex(int iString);
int pfnAllocString(const char *szValue);
struct entvars_s *pfnGetVarsOfEnt(edict_t * pEdict);
edict_t *pfnPEntityOfEntOffset(int iEntOffset);
int pfnEntOffsetOfPEntity(const edict_t * pEdict);
int pfnIndexOfEdict(const edict_t * pEdict);
edict_t *pfnPEntityOfEntIndex(int iEntIndex);
edict_t *pfnFindEntityByVars(struct entvars_s *pvars);
void *pfnGetModelPtr(edict_t * pEdict);
int pfnRegUserMsg(const char *pszName, int iSize);
void pfnAnimationAutomove(const edict_t * pEdict, float flTime);
void pfnGetBonePosition(const edict_t * pEdict, int iBone,
                        float *rgflOrigin, float *rgflAngles);
uint32 pfnFunctionFromName(const char *pName);
const char *pfnNameForFunction(uint32 function);
void pfnClientPrintf(edict_t * pEdict, PRINT_TYPE ptype,
                     const char *szMsg);
void pfnServerPrint(const char *szMsg);
const char *pfnCmd_Args();
const char *pfnCmd_Argv(int argc);
int pfnCmd_Argc();
void pfnGetAttachment(const edict_t * pEdict, int iAttachment,
                      float *rgflOrigin, float *rgflAngles);
void pfnCRC32_Init(CRC32_t * pulCRC);
void pfnCRC32_ProcessBuffer(CRC32_t * pulCRC, void *p, int len);
void pfnCRC32_ProcessByte(CRC32_t * pulCRC, unsigned char ch);
CRC32_t pfnCRC32_Final(CRC32_t pulCRC);
int32 pfnRandomLong(int32 lLow, int32 lHigh);
float pfnRandomFloat(float flLow, float flHigh);
void pfnSetView(const edict_t * pClient, const edict_t * pViewent);
float pfnTime();
void pfnCrosshairAngle(const edict_t * pClient, float pitch, float yaw);
byte *pfnLoadFileForMe(char *filename, int *pLength);
void pfnFreeFile(void *buffer);
void pfnEndSection(const char *pszSectionName);
int pfnCompareFileTime(char *filename1, char *filename2, int *iCompare);
void pfnGetGameDir(char *szGetGameDir);
void pfnCvar_RegisterVariable(cvar_t * variable);
void pfnFadeClientVolume(const edict_t * pEdict, int fadePercent,
                         int fadeOutSeconds, int holdTime,
                         int fadeInSeconds);
void pfnSetClientMaxspeed(const edict_t * pEdict, float fNewMaxspeed);
edict_t *pfnCreateFakeClient(const char *netname);
void pfnRunPlayerMove(edict_t * fakeclient, const float *viewangles,
                      float forwardmove, float sidemove, float upmove,
                      unsigned short buttons, byte impulse, byte msec);
int pfnNumberOfEntities();
char *pfnGetInfoKeyBuffer(edict_t * e);
char *pfnInfoKeyValue(char *infobuffer, char *key);
void pfnSetKeyValue(char *infobuffer, char *key, char *value);
void pfnSetClientKeyValue(int clientIndex, char *infobuffer, char *key,
                          char *value);
int pfnIsMapValid(char *filename);
void pfnStaticDecal(const float *origin, int decalIndex, int entityIndex,
                    int modelIndex);
int pfnPrecacheGeneric(char *s);
int pfnGetPlayerUserId(edict_t * e);
void pfnBuildSoundMsg(edict_t * entity, int channel, const char *sample,
                      /*int */ float volume, float attenuation, int fFlags,
                      int pitch, int msg_dest, int msg_type,
                      const float *pOrigin, edict_t * ed);
int pfnIsDedicatedServer();
cvar_t *pfnCVarGetPointer(const char *szVarName);
unsigned int pfnGetPlayerWONId(edict_t * e);
const char *pfnGetPlayerAuthId(edict_t * e);    // new

void pfnInfo_RemoveKey(char *s, const char *key);
const char *pfnGetPhysicsKeyValue(const edict_t * pClient,
                                  const char *key);
void pfnSetPhysicsKeyValue(const edict_t * pClient, const char *key,
                           const char *value);
const char *pfnGetPhysicsInfoString(const edict_t * pClient);
unsigned short pfnPrecacheEvent(int type, const char *psz);
void pfnPlaybackEvent(int flags, const edict_t * pInvoker,
                      unsigned short eventindex, float delay,
                      float *origin, float *angles, float fparam1,
                      float fparam2, int iparam1, int iparam2, int bparam1,
                      int bparam2);
unsigned char *pfnSetFatPVS(float *org);
unsigned char *pfnSetFatPAS(float *org);
int pfnCheckVisibility(const edict_t * entity, unsigned char *pset);
void pfnDeltaSetField(struct delta_s *pFields, const char *fieldname);
void pfnDeltaUnsetField(struct delta_s *pFields, const char *fieldname);
void pfnDeltaAddEncoder(char *name,
                        void (*conditionalencode) (struct delta_s *
                                                   pFields,
                                                   const unsigned char
                                                   *from,
                                                   const unsigned char
                                                   *to));
int pfnGetCurrentPlayer();
int pfnCanSkipPlayer(const edict_t * player);
int pfnDeltaFindField(struct delta_s *pFields, const char *fieldname);
void pfnDeltaSetFieldByIndex(struct delta_s *pFields, int fieldNumber);
void pfnDeltaUnsetFieldByIndex(struct delta_s *pFields, int fieldNumber);
void pfnSetGroupMask(int mask, int op);
int pfnCreateInstancedBaseline(int classname,
                               struct entity_state_s *baseline);
void pfnCvar_DirectSet(struct cvar_s *var, char *value);
void pfnForceUnmodified(FORCE_TYPE type, float *mins, float *maxs,
                        const char *filename);
void pfnGetPlayerStats(const edict_t * pClient, int *ping,
                       int *packet_loss);

#endif                          // ENGINE_H
