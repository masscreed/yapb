//
// Yet Another POD-Bot, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright (c) Yet Another POD-Bot Contributors <yapb@entix.io>.
//
// This software is licensed under the MIT license.
// Additional exceptions apply. For full license details, see LICENSE.txt
//

#pragma once

// line draw
CR_DECLARE_SCOPED_ENUM (DrawLine,
   Simple,
   Arrow,
   Count
)

// trace ignore
CR_DECLARE_SCOPED_ENUM (TraceIgnore,
   None = 0,
   Glass = cr::bit (0),
   Monsters = cr::bit (1),
   Everything = Glass | Monsters
)

// variable type
CR_DECLARE_SCOPED_ENUM (Var,
   Normal = 0,
   ReadOnly,
   Password,
   NoServer,
   NoRegister
)

// supported cs's
CR_DECLARE_SCOPED_ENUM (GameFlags,
   Modern = cr::bit (0), // counter-strike 1.6 and above
   Xash3D = cr::bit (1), // counter-strike 1.6 under the xash engine (additional flag)
   ConditionZero = cr::bit (2), // counter-strike: condition zero
   Legacy = cr::bit (3), // counter-strike 1.3-1.5 with/without steam
   Mobility = cr::bit (4), // additional flag that bot is running on android (additional flag)
   CSBot = cr::bit (5), // additional flag that indicates official cs bots are in game
   Metamod = cr::bit (6), // game running under meta\mod
   CSDM = cr::bit (7), // csdm mod currently in use
   FreeForAll = cr::bit (8), // csdm mod with ffa mode
   ReGameDLL = cr::bit (9), // server dll is a regamedll
   HasFakePings = cr::bit (10), // on that game version we can fake bots pings
   HasBotVoice = cr::bit (11) // on that game version we can use chatter
)

// defines map type
CR_DECLARE_SCOPED_ENUM (MapFlags,
   Assassination = cr::bit (0),
   HostageRescue = cr::bit (1),
   Demolition = cr::bit (2),
   Escape = cr::bit (3),
   KnifeArena = cr::bit (4),
   Fun = cr::bit (5),
   HasDoors = cr::bit (10), // additional flags
   HasButtons = cr::bit (11) // map has buttons
)

// recursive entity search
CR_DECLARE_SCOPED_ENUM (EntitySearchResult,
   Continue,
   Break
)

// variable reg pair
struct VarPair {
   Var type;
   cvar_t reg;
   bool missing;
   const char *regval;
   class ConVar *self;
   String info;
   float initial, min, max;
   bool bounded;
};

// entity prototype
using EntityFunction = void (*) (entvars_t *);

// rehlds has this fixed, but original hlds doesn't allocate string space  passed to precache* argument, so game will crash when unloading module using metamod
class EngineWrap final : public DenyCopying {
public:
   EngineWrap () = default;
   ~EngineWrap () = default;

private:
   const char *allocStr (const char *str) const {
      return STRING (engfuncs.pfnAllocString (str));
   }

public:
   int32 precacheModel (const char *model) const {
      return engfuncs.pfnPrecacheModel (allocStr (model));
   }

   int32 precacheSound (const char *sound) const {
      return engfuncs.pfnPrecacheSound (allocStr (sound));
   }

   void setModel (edict_t *ent, const char *model) {
      engfuncs.pfnSetModel (ent, allocStr (model));
   }
};

// provides utility functions to not call original engine (less call-cost)
class Game final : public Singleton <Game> {
public:
   using EntitySearch = Lambda <EntitySearchResult (edict_t *)>;

private:
   int m_drawModels[DrawLine::Count];
   int m_spawnCount[Team::Unassigned];

   // bot client command
   StringArray m_botArgs;

   edict_t *m_startEntity;
   edict_t *m_localEntity;

   Array <edict_t *> m_breakables;
   SmallArray <VarPair> m_cvars;
   SharedLibrary m_gameLib;
   EngineWrap m_engineWrap;

   bool m_precached;
   int m_gameFlags;
   int m_mapFlags;

   float m_slowFrame; // per second updated frame

public:
   Game ();
   ~Game () = default;

public:
   // precaches internal stuff
   void precache ();

   // initialize levels
   void levelInitialize (edict_t *entities, int max);

   // display world line
   void drawLine (edict_t *ent, const Vector &start, const Vector &end, int width, int noise, const Color &color, int brightness, int speed, int life, DrawLine type = DrawLine::Simple);

   // test line
   void testLine (const Vector &start, const Vector &end, int ignoreFlags, edict_t *ignoreEntity, TraceResult *ptr);

   // test line
   void testHull (const Vector &start, const Vector &end, int ignoreFlags, int hullNumber, edict_t *ignoreEntity, TraceResult *ptr);

   // get's the wave length
   float getWaveLen (const char *fileName);

   // we are on dedicated server ?
   bool isDedicated ();

   // get stripped down mod name
   const char *getModName ();

   // get the valid mapname
   const char *getMapName ();

   // get the "any" entity origin
   Vector getEntityWorldOrigin (edict_t *ent);

   // registers a server command
   void registerEngineCommand (const char *command, void func ());

   // play's sound to client
   void playSound (edict_t *ent, const char *sound);

   // sends bot command
   void prepareBotArgs (edict_t *ent, String str);

   // adds cvar to registration stack
   void addNewCvar (const char *name, const char *value, const char *info, bool bounded, float min, float max, Var varType, bool missingAction, const char *regval, class ConVar *self);

   // check the cvar bounds
   void checkCvarsBounds ();

   // sends local registration stack for engine registration
   void registerCvars (bool gameVars = false);

   // checks whether softwared rendering is enabled
   bool isSoftwareRenderer ();

   // load the cs binary in non metamod mode
   bool loadCSBinary ();

   // do post-load stuff
   bool postload ();

   // detects if csdm mod is in use
   void applyGameModes ();

   // executes stuff every 1 second
   void slowFrame ();

   // search entities by variable field
   void searchEntities (const String &field, const String &value, EntitySearch functor);

   // search entities in sphere
   void searchEntities (const Vector &position, const float radius, EntitySearch functor);

   // this function is checking that pointed by ent pointer obstacle, can be destroyed
   bool isShootableBreakable (edict_t *ent);

   // public inlines
public:
   // get the current time on server
   float time () const {
      return globals->time;
   }

   // get "maxplayers" limit on server
   int maxClients () const  {
      return globals->maxClients;
   }

   // get the fakeclient command interface
   bool isBotCmd () const  {
      return !m_botArgs.empty ();
   }

   // gets custom engine args for client command
   const char *botArgs () const {
      return strings.format (String::join (m_botArgs, " ", m_botArgs[0] == "say" || m_botArgs[0] == "say_team" ? 1 : 0).chars ());
   }

   // gets custom engine argv for client command
   const char *botArgv (size_t index) const {
      if (index >= m_botArgs.length ()) {
         return "";
      }
      return m_botArgs[index].chars ();
   }

   // gets custom engine argc for client command
   int botArgc () const {
      return m_botArgs.length ();
   }

   // gets edict pointer out of entity index
   edict_t *entityOfIndex (const int index) {
      return static_cast <edict_t *> (m_startEntity + index);
   };

   // gets edict pointer out of entity index (player)
   edict_t *playerOfIndex (const int index) {
      return entityOfIndex (index) + 1;
   };

   // gets edict index out of it's pointer
   int indexOfEntity (const edict_t *ent) {
      return static_cast <int> (ent - m_startEntity);
   };

   // gets edict index of it's pointer (player)
   int indexOfPlayer (const edict_t *ent) {
      return indexOfEntity (ent) - 1;
   }

   // verify entity isn't null
   bool isNullEntity (const edict_t *ent) {
      return !ent || !indexOfEntity (ent) || ent->free;
   }

   // get the wroldspawn entity
   edict_t *getStartEntity () {
      return m_startEntity;
   }

   // get spawn count for team
   int getSpawnCount (int team) const {
      return m_spawnCount[team];
   }

   // gets the player team
   int getTeam (edict_t *ent) {
      if (isNullEntity (ent)) {
         return Team::Unassigned;
      }
      return util.getClient (indexOfPlayer (ent)).team;
   }

   // sets the precache to uninitialize
   void setUnprecached () {
      m_precached = false;
   }

   // gets the local entity (host edict)
   edict_t *getLocalEntity () {
      return m_localEntity;
   }

   // sets the local entity (host edict)
   void setLocalEntity (edict_t *ent) {
      m_localEntity = ent;
   }

   // check the engine visibility wrapper
   bool checkVisibility (edict_t *ent, uint8 *set);

   // get pvs/pas visibility set
   uint8 *getVisibilitySet (Bot *bot, bool pvs);

   // what kind of game engine / game dll / mod / tool we're running ?
   bool is (const int type) const {
      return !!(m_gameFlags & type);
   }

   // adds game flag
   void addGameFlag (const int type) {
      m_gameFlags |= type;
   }

   // gets the map type
   bool mapIs (const int type) const {
      return !!(m_mapFlags & type);
   }

   // get loaded gamelib
   const SharedLibrary &lib () {
      return m_gameLib;
   }

   // get registered cvars list
   const SmallArray <VarPair> &getCvars () {
      return m_cvars;
   }

   // check if map has breakables
   const Array <edict_t *> &getBreakables () {
      return m_breakables;
   }

   // map has breakables ?
   bool hasBreakables () const {
      return !m_breakables.empty ();
   }

   // helper to sending the client message
   void sendClientMessage (bool console, edict_t *ent, const char *message);

   // send server command
   template <typename ...Args> void serverCommand (const char *fmt, Args ...args) {
      engfuncs.pfnServerCommand (strings.concat (strings.format (fmt, cr::forward <Args> (args)...), "\n", StringBuffer::StaticBufferSize));
   }

   // send a bot command
   template <typename ...Args> void botCommand (edict_t *ent, const char *fmt, Args ...args) {
      prepareBotArgs (ent, strings.format (fmt, cr::forward <Args> (args)...));
   }

   // prints data to servers console
   template <typename ...Args> void print (const char *fmt, Args ...args) {
      engfuncs.pfnServerPrint (strings.concat (strings.format (conf.translate (fmt), cr::forward <Args> (args)...), "\n", StringBuffer::StaticBufferSize));
   }

   // prints center message to specified player
   template <typename ...Args> void clientPrint (edict_t *ent, const char *fmt, Args ...args) {
      if (isNullEntity (ent)) {
         print (fmt, cr::forward <Args> (args)...);
         return;
      }
      sendClientMessage (true, ent, strings.concat (strings.format (conf.translate (fmt), cr::forward <Args> (args)...), "\n", StringBuffer::StaticBufferSize));
   }

   // prints message to client console
   template <typename ...Args> void centerPrint (edict_t *ent, const char *fmt, Args ...args) {
      if (isNullEntity (ent)) {
         print (fmt, cr::forward <Args> (args)...);
         return;
      }
      sendClientMessage (false, ent, strings.concat (strings.format (conf.translate (fmt), cr::forward <Args> (args)...), "\n", StringBuffer::StaticBufferSize));
   }
};

// simplify access for console variables
class ConVar final : public DenyCopying {
public:
   cvar_t *ptr;

public:
   ConVar () = delete;
   ~ConVar () = default;

public:
   ConVar (const char *name, const char *initval, Var type = Var::NoServer, bool regMissing = false, const char *regVal = nullptr) : ptr (nullptr) {
      Game::get ().addNewCvar (name, initval, "", false, 0.0f, 0.0f, type, regMissing, regVal, this);
   }

   ConVar (const char *name, const char *initval, const char *info, bool bounded = true, float min = 0.0f, float max = 1.0f, Var type = Var::NoServer, bool regMissing = false, const char *regVal = nullptr) : ptr (nullptr) {
      Game::get ().addNewCvar (name, initval, info, bounded, min, max, type, regMissing, regVal, this);
   }

   bool bool_ () const {
      return ptr->value > 0.0f;
   }

   int int_ () const {
      return static_cast <int> (ptr->value);
   }

   float float_ () const {
      return ptr->value;
   }

   const char *str () const {
      return ptr->string;
   }

   void set (float val) {
      engfuncs.pfnCVarSetFloat (ptr->name, val);
   }

   void set (int val) {
      set (static_cast <float> (val));
   }

   void set (const char *val) {
      engfuncs.pfnCvar_DirectSet (ptr, const_cast <char *> (val));
   }
};

class MessageWriter final {
private:
   bool m_autoDestruct { false };

public:
   MessageWriter () = default;

   MessageWriter (int dest, int type, const Vector &pos = nullptr, edict_t *to = nullptr) {
      start (dest, type, pos, to);
      m_autoDestruct = true;
   }

   ~MessageWriter () {
      if (m_autoDestruct) {
         end ();
      }
   }

public:
   MessageWriter &start (int dest, int type, const Vector &pos = nullptr, edict_t *to = nullptr) {
      engfuncs.pfnMessageBegin (dest, type, pos, to);
      return *this;
   }

   void end () {
      engfuncs.pfnMessageEnd ();
   }

   MessageWriter &writeByte (int val) {
      engfuncs.pfnWriteByte (val);
      return *this;
   }

   MessageWriter &writeLong (int val) {
      engfuncs.pfnWriteLong (val);
      return *this;
   }

   MessageWriter &writeChar (int val) {
      engfuncs.pfnWriteChar (val);
      return *this;
   }

   MessageWriter &writeShort (int val) {
      engfuncs.pfnWriteShort (val);
      return *this;
   }

   MessageWriter &writeCoord (float val) {
      engfuncs.pfnWriteCoord (val);
      return *this;
   }

   MessageWriter &writeString (const char *val) {
      engfuncs.pfnWriteString (val);
      return *this;
   }

public:
   static inline uint16 fu16 (float value, float scale) {
      return cr::clamp <uint16> (static_cast <uint16> (value * cr::bit (static_cast <short> (scale))), 0, USHRT_MAX);
   }

   static inline short fs16 (float value, float scale) {
      return cr::clamp <short> (static_cast <short> (value * cr::bit (static_cast <short> (scale))), -SHRT_MAX, SHRT_MAX);
   }
};

class LightMeasure final : public Singleton <LightMeasure> {
private:
   lightstyle_t m_lightstyle[MAX_LIGHTSTYLES];
   int m_lightstyleValue[MAX_LIGHTSTYLEVALUE];
   bool m_doAnimation = false;

   Color m_point;
   model_t *m_worldModel = nullptr;

public:
   LightMeasure () {
      initializeLightstyles ();
      m_point.reset ();
   }

public:
   void initializeLightstyles ();
   void animateLight ();
   void updateLight (int style, char *value);

   float getLightLevel (const Vector &point);
   float getSkyColor ();

private:
   template <typename S, typename M> bool recursiveLightPoint (const M *node, const Vector &start, const Vector &end);

public:
   void resetWorldModel () {
      m_worldModel = nullptr;
   }

   void setWorldModel (model_t *model) {
      if (m_worldModel) {
         return;
      }
      m_worldModel = model;
   }

   model_t *getWorldModel () const {
      return m_worldModel;
   }

   void enableAnimation (bool enable) {
      m_doAnimation = enable;
   }
};

// simple handler for parsing and rewriting queries (fake queries)
class QueryBuffer {
   SmallArray <uint8> m_buffer;
   size_t m_cursor;

public:
   QueryBuffer (const uint8 *msg, size_t length, size_t shift) : m_cursor (0) {
      m_buffer.insert (0, msg, length);
      m_cursor += shift;
   }

public:
   template <typename T> T read () {
      T result;
      auto size = sizeof (T);

      if (m_cursor + size > m_buffer.length ()) {
         return 0;
      }

      memcpy (&result, m_buffer.data () + m_cursor, size);
      m_cursor += size;

      return result;
   }

   // must be called right after read
   template <typename T> void write (T value) {
      auto size = sizeof (value);
      memcpy (m_buffer.data () + m_cursor - size, &value, size);
   }

   template <typename T> void skip () {
      auto size = sizeof (T);

      if (m_cursor + size > m_buffer.length ()) {
         return;
      }
      m_cursor += size;
   }

   void skipString () {
      if (m_buffer.length () < m_cursor) {
         return;
      }
      for (; m_cursor < m_buffer.length () && m_buffer[m_cursor] != '\0'; ++m_cursor) { }
      ++m_cursor;
   }

   void shiftToEnd () {
      m_cursor = m_buffer.length ();
   }

public:
   Twin <const uint8 *, size_t> data () {
      return { m_buffer.data (), m_buffer.length () };
   }
};

// for android
#if defined (CR_ANDROID) && defined(CR_ARCH_ARM)
   extern "C" void player (entvars_t *pev);
#endif

class DynamicEntityLink : public Singleton <DynamicEntityLink> {
private:
#if defined (CR_WINDOWS)
#  define MODULE_HANDLE HMODULE
#  define MODULE_SYMBOL GetProcAddress
#else
#  define MODULE_HANDLE void *
#  define MODULE_SYMBOL dlsym
#endif

private:
   using Handle = void *;
   using Name = const char *;

private:
   template <typename K> struct FunctionHash {
      uint32 operator () (Name key) const {
         char *str = const_cast <char *> (key);
         uint32 hash = 0;

         while (*str++) {
            hash = ((hash << 5) + hash) + *str;
         }
         return hash;
      }
   };

private:
   SharedLibrary m_self;
   SimpleHook m_dlsym;
   Dictionary <Name, Handle, FunctionHash <Name>> m_exports;

public:
   DynamicEntityLink () = default;

   ~DynamicEntityLink () {
      m_dlsym.disable ();
   }
public:
   Handle search (Handle module, Name function);

public:
   void initialize () {
      if (plat.arm) {
         return;
      }
      m_dlsym.patch (reinterpret_cast <void *> (&MODULE_SYMBOL), reinterpret_cast <void *> (&DynamicEntityLink::replacement));
      m_self.locate (&engfuncs);
   }

   EntityFunction getPlayerFunction () {
#if defined (CR_ANDROID) && defined(CR_ARCH_ARM)
      return player;
#else
      return reinterpret_cast <EntityFunction> (search (Game::get ().lib ().handle (), "player"));
#endif
   }

public:
   static Handle CR_STDCALL replacement (Handle module, Name function) {
      return DynamicEntityLink::get ().search (module, function);
   }
};

// expose globals
CR_EXPOSE_GLOBAL_SINGLETON (Game, game);
CR_EXPOSE_GLOBAL_SINGLETON (LightMeasure, illum);
CR_EXPOSE_GLOBAL_SINGLETON (DynamicEntityLink, ents);
