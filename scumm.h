/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Change Log:
 * $Log$
 * Revision 1.12  2001/10/23 19:51:50  strigeus
 * recompile not needed when switching games
 * debugger skeleton implemented
 *
 * Revision 1.11  2001/10/17 10:07:40  strigeus
 * fixed verbs not saved in non dott games,
 * implemented a screen effect
 *
 * Revision 1.10  2001/10/17 07:12:37  strigeus
 * fixed nasty signed/unsigned bug
 *
 * Revision 1.9  2001/10/16 20:31:27  strigeus
 * misc fixes
 *
 * Revision 1.8  2001/10/16 10:01:47  strigeus
 * preliminary DOTT support
 *
 * Revision 1.7  2001/10/11 12:07:35  strigeus
 * Determine caption from file name.
 *
 * Revision 1.6  2001/10/11 08:00:42  strigeus
 * Dump scripts by using DUMP_SCRIPTS as a compile option instead.
 *
 * Revision 1.5  2001/10/10 10:02:33  strigeus
 * alternative mouse cursor
 * basic save&load
 *
 * Revision 1.4  2001/10/09 19:02:28  strigeus
 * command line parameter support
 *
 * Revision 1.3  2001/10/09 18:35:02  strigeus
 * fixed object parent bug
 * fixed some signed/unsigned comparisons
 *
 * Revision 1.2  2001/10/09 17:38:20  strigeus
 * Autodetection of endianness.
 *
 * Revision 1.1.1.1  2001/10/09 14:30:12  strigeus
 * initial revision
 *
 */

#include "scummsys.h"

#define SWAP(a,b) do{int tmp=a; a=b; b=tmp; } while(0)

#define BYPASS_COPY_PROT

struct Scumm;
struct Actor;

typedef void (Scumm::*OpcodeProc)();

#pragma START_PACK_STRUCTS
	
struct Point {
	int x,y;
};

struct AdjustBoxResult {
	int16 x,y;
	uint16 dist;
};

#define SIZEOF_BOX 20
struct Box { /* file format */
	int16 ulx,uly;
	int16 urx,ury;
	int16 llx,lly;
	int16 lrx,lry;
	byte mask;
	byte flags;
	uint16 scale;
};

struct VerbSlot {
	int16 x,y;
	int16 right, bottom;
	int16 oldleft, oldtop, oldright,oldbottom;
	uint8 verbid;
	uint8 color,hicolor,dimcolor,bkcolor,type;
	uint8 charset_nr,curmode;
	uint8 saveid;
	uint8 key,center;
	uint8 field_1B;
	uint16 imgindex;
};

struct VirtScreen {
	uint16 unk1;
	uint16 topline;
	uint16 width,height;
	uint16 size;
	byte alloctwobuffers;
	byte scrollable;
	uint16 xstart;
	byte tdirty[40];
	byte bdirty[40];
};

struct ActorWalkData {
	int16 destx,desty;
	byte destbox;
	byte destdir;
	byte curbox;
	byte field_7;
	int16 x,y,newx,newy;
	int32 XYFactor, YXFactor;
	uint16 xfrac,yfrac;
};

struct CostumeData {
	uint16 hdr;
	uint16 animCounter1;
	byte animCounter2;
	byte x_1;
	uint16 a[16], b[16], c[16], d[16];
};

struct MouseCursor {
	int8 hotspot_x, hotspot_y;
	byte colors[4];
	byte data[32];
};

struct ScriptSlot {
	uint32 offs;
	int32 delay;
	uint16 number;
	uint16 newfield;
	byte status;
	byte type;
	byte unk1,unk2,freezeCount,didexec;
	byte cutsceneOverride;
	byte unk5;
};

struct NestedScript {
	uint16 number;
	uint8 type;
	uint8 slot;
};

struct ResHeader {
	uint32 size;
};

class ObjectData {
public:
	uint32 offs_obim_to_room;
	uint32 offs_obcd_to_room;
	uint16 cdhd_10, cdhd_12;
	uint16 obj_nr;
	int16 x_pos;
	int16 y_pos;
	uint16 numstrips;
	uint16 height;
	byte actordir;
	byte parent;
	byte parentstate;
	byte ownerstate;
	byte fl_object_index;
	byte unk_3;
};

struct RoomHeader {
	uint32 tag, size;
	uint16 width,height;
	uint16 numObjects;
};

struct CodeHeader {
	uint32 id;
	uint32 size;
	uint16 obj_id;

	union {
		struct {
			byte x,y,w,h;
			byte flags;
			byte parent;
			uint16 unk2;
			uint16 unk3;
			byte actordir;
		} v5;

		struct {
			int16 x, y;
			uint16 w,h;
			byte flags, parent;
			uint16 unk2;
			uint16 unk3;
			byte actordir;
		} v6;
	};
};

struct ImageHeader { /* file format */
	uint32 id;
	uint32 size;
	uint16 obj_id;
	uint16 unk[5];
	uint16 img_w;
	uint16 img_h;
	uint16 unk_2;
	struct {
		int16 x,y;
	} hotspot[15];
};

#pragma END_PACK_STRUCTS

struct PathNode {
	uint index;
	struct PathNode *left, *right;
};

struct PathVertex {
	PathNode *left;
	PathNode *right;
};

struct SaveLoadEntry {
	uint16 offs;
	uint8 type;
	uint8 size;
};

enum {
	sleByte = 1,
	sleUint8 = 1,
	sleInt16 = 2,
	sleUint16 = 3,
	sleInt32 = 4,
	sleUint32 = 5
};

enum ScummVars {
	VAR_UNK_ACTOR = 1,
	VAR_WALKTO_OBJ = 38,
	VAR_OVERRIDE = 5,
	VAR_NUM_ACTOR = 8,
	VAR_OBJECT_LO = 15,
	VAR_OBJECT_HI = 16,
	VAR_CURRENTDRIVE = 10,
	VAR_TALK_ACTOR = 25,
	VAR_DEBUGMODE = 39,
	VAR_VERSION = 75,
	VAR_FIXEDDISK = 51,
	VAR_CURSORSTATE = 52,
	VAR_USERPUT = 53,
	VAR_SOUNDCARD = 48,
	VAR_VIDEOMODE = 49,
	VAR_HEAPSPACE = 40,
	VAR_MOUSEPRESENT = 67,
	VAR_SOUNDPARAM = 64,
	VAR_SOUNDPARAM2 = 65,
	VAR_SOUNDPARAM3 = 66,
	VAR_GAME_LOADED = 71,
	VAR_VIRT_MOUSE_X = 20,
	VAR_VIRT_MOUSE_Y = 21,
	VAR_PERFORMANCE_1 = 68,
	VAR_PERFORMANCE_2 = 69,
	VAR_ROOM_FLAG = 70,
	VAR_HAVE_MSG = 3,
	VAR_ENTRY_SCRIPT = 28,
	VAR_ENTRY_SCRIPT2 = 29,
	VAR_EXIT_SCRIPT = 30,
	VAR_EXIT_SCRIPT2 = 31,
	VAR_VERB_SCRIPT = 32,
	VAR_SENTENCE_SCRIPT = 33,
	VAR_LAST_SOUND = 23,
	VAR_HOOK_SCRIPT = 34,
	VAR_CUTSCENE_START_SCRIPT = 35,
	VAR_CUTSCENE_END_SCRIPT = 36,
	VAR_SCROLL_SCRIPT = 27,
	VAR_CAMERA_MIN = 17,
	VAR_CAMERA_MAX = 18,
	VAR_CAMERA_FAST = 26,
	VAR_CAMERA_CUR_POS = 2,
	VAR_NEW_ROOM = 72,
	VAR_ROOM = 4,
	VAR_ROOM_RESOURCE = 22,

	VAR_MOUSE_X = 44,
	VAR_MOUSE_Y = 45,
	
	VAR_TIMER = 46,
	VAR_TIMER_NEXT = 19,

	VAR_TMR_1 = 11,
	VAR_TMR_2 = 12,
	VAR_TMR_3 = 13,
	VAR_TMR_4 = 47,

	VAR_DRAWFLAGS = 9,

	VAR_SOUNDRESULT = 56,

	VAR_PLAYBACKTIMER = 19,

	VAR_TALK_STRING_Y = 54,
	VAR_CHARFLAG = 60,
	VAR_CHARINC = 37,

	VAR_RESTART_KEY = 42,
	VAR_PAUSE_KEY = 43,
	VAR_CUTSCENEEXIT_KEY = 24,
	VAR_TALKSTOP_KEY = 57,
	VAR_SAVELOADDIALOG_KEY = 50,

	VAR_V6_RANDOM_NR = 118,
};

#define _maxRooms res.num[1]
#define _maxScripts res.num[2]
#define _maxCostumes res.num[3]
#define _maxInventoryItems res.num[5]
#define _maxCharsets res.num[6]
#define _maxStrings res.num[7]
#define _maxVerbs res.num[8]
#define _maxActorNames res.num[9]
#define _maxBuffer res.num[10]
#define _maxScaleTable res.num[11]
#define _maxTemp res.num[12]
#define _maxFLObject res.num[13]
#define _maxMatrixes res.num[14]
#define _maxBoxes res.num[15]

#define _baseRooms res.address[1]
#define _baseScripts res.address[2]
#define _baseInventoryItems res.address[5]
#define _baseFLObject res.address[13]
#define _baseArrays res.address[7]

#define _roomFileOffsets res.roomoffs[1]

struct CharsetRenderer {
	Scumm *_vm;
	int _top;
	int _drawTop;
	int _left, _left2;
	byte _center;
	int _right;
	byte _color;
	bool _hasMask;
	
	int _strLeft, _strRight, _strTop, _strBottom;
//	int _mask_bottom, _mask_right, _mask_top, _mask_left;
	byte _curId;
	
	byte _bufPos;
	byte _unk12,_disableOffsX;
	byte *_ptr;
	byte _unk2, _bpp;
	byte _invNumBits;
	uint32 _charOffs;
	byte *_charPtr;
	int _width, _height;
	int _offsX,_offsY;
	byte _bitMask, _revBitMask;
	int _bottom;
	int _virtScreenHeight;

	byte _ignoreCharsetMask;

	byte *_bg_ptr, *_where_to_draw_ptr;
	byte *_mask_ptr;
		
	byte _colorMap[16];
	byte _buffer[256];

	void drawBits();
	void printChar(int chr);
	int getStringWidth(int a, byte *str, int pos);
	void addLinebreaks(int a, byte *str, int pos, int maxwidth);
};

struct CostumeRenderer {
	Scumm *_vm;
	byte *_ptr;
	byte _numColors;
	byte *_dataptr;
	byte *_frameptr;
	byte *_srcptr;
	byte *_where_to_draw_ptr, *_bg_ptr, *_mask_ptr, *_mask_ptr_dest;
	int _actorX, _actorY;
	byte _zbuf;
	uint _scaleX, _scaleY;
	int _xmove, _ymove;
	bool _mirror;
	byte _maskval;
	byte _shrval;
	byte _width2;
	int _width;
	byte _height2;
	int _height;
	int _xpos, _ypos;
	
	int _scaleIndexXStep;
	int _scaleIndexYStep;
	byte _scaleIndexX; /* must wrap at 256*/
	byte _scaleIndexY, _scaleIndexYTop;
	int _left,_right;
	int _dir2;
	int _top,_bottom;
	int _ypostop;
	uint _vscreenheight;
	int _ypitch;
	byte _docontinue;
	int _imgbufoffs;
	byte _repcolor;
	byte _replen;
	byte _palette[32];
	byte _transEffect[0x100];

	void proc6();
	void proc5();
	void proc4();
	void proc3();
	void proc2();
	void proc1();
	byte mainRoutine(Actor *a, int slot, int frame);
	void ignorePakCols(int num);

	void loadCostume(int id);
	byte drawOneSlot(Actor *a, int slot);
	byte drawCostume(Actor *a);

	byte animateOneSlot(CostumeData *cd, int slot);
	byte animate(CostumeData *cd);
};

struct Actor {
	int x,y,top,bottom;
	int elevation;
	uint width;
	byte number;
	byte facing;
	uint16 costume;
	byte room;
	byte talkColor;
	byte scalex,scaley;
	byte charset;
	byte newDirection;
	byte moving;
	byte ignoreBoxes;
	byte neverZClip;
	byte initFrame,walkFrame,standFrame,talkFrame1,talkFrame2;
	bool needRedraw, needBgReset,costumeNeedsInit,visible;
	uint speedx,speedy;
	byte data8; /* unused */
	byte animIndex;
	byte walkbox;
	byte mask;
	byte animProgress, animSpeed;
	int16 new_1,new_2;
	byte new_3;
	byte sound[8];
	ActorWalkData walkdata;
	CostumeData cost;
	byte palette[32];
};

struct CameraData {
	int16 _destPos, _curPos, _lastPos;
	int16 _leftTrigger, _rightTrigger;
	byte _follows, _mode;
	uint16 _movingToActor;
};

#define ARRAY_HDR_SIZE 6
struct ArrayHeader {
	int16 dim1_size;
	int16 type;
	int16 dim2_size;
	byte data[1];
};

struct SentenceTab {
		byte unk5;
		byte unk2;
		uint16 unk4;
		uint16 unk3;
		byte unk;
		byte pad;
};

struct StringTab {
	int16 t_xpos, t_ypos, t_center, t_overhead;
	int16 t_new3, t_right, t_color, t_charset;
	int16 xpos, ypos;
	int16 xpos2,ypos2;
	int16 center, overhead;
	int16 new_3, right;
	int16 color,charset;
	int16 mask_top, mask_bottom, mask_right, mask_left;
};

enum GameId {
	GID_TENTACLE = 1,
	GID_MONKEY2 = 2,
	GID_INDY4 = 3,
	GID_MONKEY = 4,
};

struct ScummDebugger;


struct Scumm {
	const char *_gameText;
	byte _gameId;
	byte _majorScummVersion;
	byte _middleScummVersion;
	byte _minorScummVersion;

	ScummDebugger *_debugger;
	
	int _lastLoadedRoom;
	int _roomResource;
	byte _encbyte;
	void *_fileHandle;
	char *_exe_name;

	int _saveLoadSlot;
	
	bool _dynamicRoomOffsets;
	byte _resFilePathId;
	
	char *_resFilePrefix;
	char *_resFilePath;

	int _keyPressed;

	uint16 *_inventory;
	byte *_arrays;
	VerbSlot *_verbs;
	ObjectData *_objs;
	uint16 *_newNames;
	int16 *_vars;
	byte *_bitVars;

	const OpcodeProc *_opcodes;

	byte _curActor;
	int _curVerb;
	int _curVerbSlot;

	int _curPalIndex;

	int _numVariables;
	int _numBitVariables;
	int _numLocalObjects;
	int _numGlobalObjects;
	int _numArray;
	int _numVerbs;
	int _numFlObject;
	int _numInventory;
	int _numRooms;
	int _numScripts;
	int _numSounds;
	int _numCharsets;
	int _numCostumes;
	
	uint8 *_roomFileIndexes;
	byte *_objectFlagTable;
	uint32 *_classData;

	byte _numGlobalScripts;
	
	uint16 _numZBuffer;
	
	uint32 _randSeed1;
	uint32 _randSeed2;

	uint16 _screenB, _screenH;

	uint16 dseg_3A76;
	uint16 _defaultTalkDelay;
	uint16 _lightsValueA,_lightsValueB;
	byte _haveMsg;
	byte _newEffect;
	uint16 _fullRedraw;
	uint16 dseg_3DB6;
	uint16 dseg_2456; /* lastDrawnRoom */
	uint16 dseg_4E8A;
	uint16 _soundParam,_soundParam2,_soundParam3;
	uint16 dseg_4F8C;
	
	byte _switchRoomEffect2, _switchRoomEffect;
	uint16 dseg_4AC2;

	bool doEffect;
	
	uint16 _drawBmpX;
	uint16 dseg_719E;
	uint16 _drawBmpY;
	uint16 dseg_4174;
	byte dseg_4E3C;
	uint16 _lastXstart;
	uint16 dseg_4EA0;
	
	bool _keepText;
	
	int16 _talkDelay;
	int16 _shakeMode;

	int16 _virtual_mouse_x, _virtual_mouse_y;

	byte _charsetColor;

	uint32 _localScriptList[0x39];

	uint16 _debugMode;

	byte *_messagePtr;

	byte _numNestedScripts;
	byte _unkTabIndex;
	byte _currentScript;

	byte _currentRoom;
	byte _numObjectsInRoom;
	byte _actorToPrintStrFor;

	int16 _screenStartStrip;
	int16 _screenEndStrip;

	int16 _scummTimer;

	byte _playBackFile;
	byte _fastMode;
	
	uint16 _completeScreenRedraw;

	byte _saveLoadFlag;

	int8 _userPut;
	int8 _cursorState;

	uint16 _mouseButStat;
	byte _leftBtnPressed, _rightBtnPressed;

	int _numInMsgStack;

//	VerbSlot verbs[102];
	VirtScreen virtscr[4];

	uint32 _ENCD_offs, _EXCD_offs;
	uint32 _CLUT_offs, _EPAL_offs;
	uint32 _IM00_offs;
	uint32 _PALS_offs;

	int _drawObjectQueNr;
	byte _drawObjectQue[0xC8];

	uint16 _currentDrive;
	uint16 _soundCardType;
	uint16 _videoMode;
	byte _mousePresent;

	int16 _palManipStart;
	int16 _palManipEnd;
	int16 _palManipCounter;

	struct {
		byte mode[17];
		uint16 num[17];
		uint32 tags[17];
		const char *name[17];
		byte **address[17];
		byte *flags[17];
		byte *roomno[17];
		uint32 *roomoffs[17];
	} res;

	struct {
		uint32 cutScenePtr[5];
		byte cutSceneScript[5];
		int16 cutSceneData[5];
		int16 cutSceneScriptIndex;
		byte cutSceneStackPointer;
		ScriptSlot slot[20];
		NestedScript nest[15];
		int16 localvar[20*17];
	} vm;

	struct {
		int16 x,y;
	} mouse;

	struct {
		byte *readPtr;
		uint16 readOffs;
		uint16 drawY;
		uint16 drawHeight;
		uint16 drawWidth;
		uint16 draw8xPos;
		int16 virtScreen;
		uint16 drawBottom;
		uint16 drawTop;

		int8 unk4;

		byte numLinesToProcess;
		byte tempNumLines;
		byte currentX;
		byte hotspot_x;
		byte hotspot_y;
		int16 drawMouseX;
		int16 drawMouseY;
		byte currentCursor;
		byte mouseColors[4];
		byte mouseColor;
		byte mouseClipMask1, mouseClipMask2, mouseClipMask3;
		byte mouseColorIndex;
		byte mouseMask[0x200];
		byte *mouseMaskPtr;
		byte *smap_ptr;
		byte *bg_ptr;
		byte *where_to_draw_ptr;
		byte *mask_ptr;
		byte *mask_ptr_dest;
		byte *z_plane_ptr;

		byte decomp_shr, decomp_mask;
		byte transparency;
		uint16 vertStripNextInc;
		byte *backupIsWhere;
//		byte mouseBackup[16*24];
	} gdi;

	Actor actor[13];

	uint16 actorDrawBits[200];

	struct {
		int upperLeftX;
		int upperRightX;
		int lowerLeftX;
		int lowerRightX;
		int upperLeftY;
		int upperRightY;
		int lowerLeftY;
		int lowerRightY;
	} box;

	CharsetRenderer charset;
	
	byte _charsetData[10][16];

	byte _resourceMapper[128];

	byte **_lastCodePtr;
	
	int _numSoundTags;
	byte *_soundTagTable;

	int16 _bootParam;

	uint32 _fileOffset;

	byte _fileReadFailed;
	byte _fileMode;

	uint32 _whereInResToRead;

	byte *_scriptPointer, *_scriptOrgPointer;
	byte *_scriptPointerStart;
	byte _opcode;

	int _xPos, _yPos;
	byte _dir;

	CameraData camera;

	int _resultVarNumber;

	uint16 _imgBufOffs[4];

	byte _sentenceIndex;
	SentenceTab sentence[6];


#if 0	
	byte _sentenceTab[6];
	byte _sentenceTab2[6];
	uint16 _sentenceTab3[6];
	uint16 _sentenceTab4[6];
	byte _sentenceTab5[6];
#endif

	StringTab string[6];

#if 0
//	int _stringXPos[4], _stringYPos[4];
	uint16 _stringOverhead[6];
	uint16 _stringCenter[6];
	uint16 _stringRight[6];
	uint16 _stringColor[6];

	int16 _stringXpos[6];
	int16 _stringYpos[6];
	uint16 _stringCharset[6];

	int16 _stringXpos2[6];
	int16 _stringYpos2[6];
#endif

	CostumeRenderer cost;

//	ObjectData objs[184];

	int16 _soundQuePos;
	int16 _soundQue[0x100];

	byte _soundQue2Pos;
	byte _soundQue2[10];

	int16 _vararg_temp_pos[16];

	uint16 _curExecScript;

	int _scrWidthIn8Unit;
	int _scrHeight;

	byte _currentPalette[0x300];

	int _palDirtyMin, _palDirtyMax;

	byte _saveLoadData;

	uint16 _colorCycleDelays[17];
	uint16 _colorCycleCounter[17];
	uint16 _colorCycleFlags[17];
	byte _colorCycleStart[17];
	byte _colorCycleEnd[17];

	byte dseg_4E3B;

	uint32 _findResSize, _findResHeaderSize;
	byte *_findResPos;

	uint32 _findResSize2, _findResHeaderSize2;
	byte *_findResPos2;

	bool _BgNeedsRedraw;

	int16 _localParamList[16];

	uint16 _verbMouseOver;

	int16 _foundPathX;
	int16 _foundPathY;

	uint16 _lastKeyHit;

	int _scummStackPos;
	int16 _scummStack[100];

	int _maxBoxVertexHeap;
	byte *_boxMatrixPtr4, *_boxMatrixPtr1, *_boxMatrixPtr3;
	int _boxPathVertexHeapIndex;
	int _boxMatrixItem;
	
	byte *_msgPtrToAdd;

	OpcodeProc getOpcode(int i) { return _opcodes[i]; }

	void openRoom(int room);
	void deleteRoomOffsets();
	void readRoomsOffsets();
	void askForDisk(const char *filename);

	
	bool openResourceFile(const char *filename);
	
	void fileClose(void *file);
	void *fileOpen(const char *filename, int mode);
	void fileSeek(void *file, long offs, int whence);
	void fileRead(void *handle, void *ptr, uint32 size);
	bool fileEof(void *handle);

	int fileReadByte();
	uint32 fileReadDwordLE();
	uint32 fileReadDwordBE();
#if defined(SCUMM_LITTLE_ENDIAN)
	uint32 fileReadDword() { return fileReadDwordLE(); }
#elif defined(SCUMM_BIG_ENDIAN)
	uint32 fileReadDword() { return fileReadDwordBE(); }
#endif
	uint fileReadWordLE();
	uint fileReadWordBE();

	byte *alloc(int size);
	void free(void *mem);

	void readResTypeList(int id, uint32 tag, const char *name);
	void allocResTypeData(int id, uint32 tag, int num, const char *name, int mode);

	void initThingsV5();
	void initThingsV6();

	void initRandSeeds();

	uint getRandomNumber(uint max);

	void loadCharset(int i);
	void nukeCharset(int i);
	void initScreens(int a, int b, int w, int h);

	void setShake(int mode);
	void setCursor(int cursor);

	void clearDrawObjectQueue();

	byte *createResource(int type, int index, uint32 size);

	void initScummVars();
	void getGraphicsPerformance();

	void nukeResource(int type, int i);
	byte *getResourceAddress(int type, int i);
	void ensureResourceLoaded(int type, int i);
	int loadResource(int type, int i);
	int getResourceRoomNr(int type, int index);
	int readSoundResource(int type, int index);
	void setResourceFlags(int type, int index, byte flag);
	void validateResource(const char *str, int type, int index);
	
	void initVirtScreen(int slot, int top, int height, bool twobufs, bool fourextra);
	void setDirtyRange(int slot, int a, int height);
	void unkVirtScreen2();
	void updateDirtyScreen(int slot);
	void unkVirtScreen4(int a);
	

	void drawStripToScreen();
	void restoreMouse();
	void initActor(Actor *a, int mode);
	bool checkFixedDisk();

	void setActorWalkSpeed(Actor *a, uint speed1, uint speed2);
	int calcMovementFactor(Actor *a, int newx, int newy);
	int actorWalkStep(Actor *a);
	int getProgrDirChange(Actor *a, int mode);

	bool checkXYInBoxBounds(int box, int x, int y);
	void setupActorScale(Actor *a);
	
	void checkRange(int max, int min, int no, const char *str);

	bool fileReadFailed(void *handle);
	void clearFileReadFailed(void *handle);

	bool getClass(int obj, int cls);
	void putClass(int obj, int cls, bool set);
	int getState(int obj);
	void putState(int obj, int state);
	int getOwner(int obj);
	void putOwner(int obj, int owner);

	void main();

	uint distanceFromPt(int x, int y, int ptx, int pty);
	Point closestPtOnLine(int ulx, int uly, int llx, int lly, int x, int y);
	bool getSideOfLine(int x1,int y1, int x2, int y2, int x, int y, int box);
	void getBoxCoordinates(int box);
	byte getMaskFromBox(int box);
	Box *getBoxBaseAddr(int box);
	byte getBoxFlags(int box);
	int getBoxScale(int box);
	byte getNumBoxes();
	byte *getBoxMatrixBaseAddr();

	void startAnimActor(Actor *a, int frame, byte direction);
	void initActorCostumeData(Actor *a);
	void fixActorDirection(Actor *a, byte direction);
	void decodeCostData(Actor *a, int frame, uint mask);

	void scummInit();
	void scummMain(int argc, char **argv);

	void runScript(int script, int a, int b, int16 *lvarptr);
	void stopScriptNr(int script);
	int getScriptSlot();
	void runScriptNested(int script);
	void updateScriptPtr();
	void getScriptBaseAddress();
	void getScriptEntryPoint();
	void executeScript();
	byte fetchScriptByte();
	int fetchScriptWord();
	void ignoreScriptWord();
	void ignoreScriptByte();
	int getVarOrDirectWord(byte mask);
	int getVarOrDirectByte(byte mask);
	int readVar(uint var);
	void writeVar(uint var, int value);
	void getResultPos();
	void setResult(int result);
	
	int getObjectIndex(int object);

	void o5_actorFollowCamera();
	void o5_actorFromPos();
	void o5_actorSet();
	void o5_actorSetClass();
	void o5_add();
	void o5_and();
	void o5_animateActor();
	void o5_badOpcode();
	void o5_breakHere();
	void o5_chainScript();
	void o5_cursorCommand();
	void o5_cutscene();
	void o5_debug();
	void o5_decrement();
	void o5_delay();
	void o5_delayVariable();
	void o5_divide();
	void o5_doSentence();
	void o5_drawBox();
	void o5_drawObject();
	void o5_dummy();
	void o5_endCutscene();
	void o5_equalZero();
	void o5_expression();
	void o5_faceActor();
	void o5_findInventory();
	void o5_findObject();
	void o5_freezeScripts();
	void o5_getActorCostume();
	void o5_getActorElevation();
	void o5_getActorFacing();
	void o5_getActorMoving();
	void o5_getActorRoom();
	void o5_getActorScale();
	void o5_getActorWalkBox();
	void o5_getActorWidth();
	void o5_getActorX();
	void o5_getActorY();
	void o5_getAnimCounter();
	void o5_getClosestObjActor();
	void o5_getDist();
	void o5_getInventoryCount();
	void o5_getObjectOwner();
	void o5_getObjectState();
	void o5_getRandomNr();
	void o5_getScriptRunning();
	void o5_getVerbEntrypoint();
	void o5_ifClassOfIs();
	void o5_increment();
	void o5_isActorInBox();
	void o5_isEqual();
	void o5_isGreater();
	void o5_isGreaterEqual();
	void o5_isLess();
	void o5_isNotEqual();
	void o5_isSoundRunning();
	void o5_jumpRelative();
	void o5_lessOrEqual();
	void o5_lights();
	void o5_loadRoom();
	void o5_loadRoomWithEgo();
	void o5_matrixOps();
	void o5_move();
	void o5_multiply();
	void o5_notEqualZero();
	void o5_or();
	void o5_overRide();
	void o5_panCameraTo();
	void o5_pickupObject();
	void o5_print();
	void o5_printEgo();
	void o5_pseudoRoom();
	void o5_putActor();
	void o5_putActorAtObject();
	void o5_putActorInRoom();
	void o5_quitPauseRestart();
	void o5_resourceRoutines();
	void o5_roomOps();
	void o5_saveRestoreVerbs();
	void o5_setCameraAt();
	void o5_setObjectName();
	void o5_setOwnerOf();
	void o5_setState();
	void o5_setVarRange();
	void o5_soundKludge();
	void o5_startMusic();
	void o5_startObject();
	void o5_startScript();
	void o5_startSound();
	void o5_stopMusic();
	void o5_stopObjectCode();
	void o5_stopObjectScript();
	void o5_stopScript();
	void o5_stopSound();
	void o5_stringOps();
	void o5_subtract();
	void o5_verbOps();
	void o5_wait();
	void o5_walkActorTo();
	void o5_walkActorToActor();
	void o5_walkActorToObject();

	void o6_pushByte();
	void o6_pushWord();
	void o6_pushByteVar();
	void o6_pushWordVar();
	void o6_invalid();
	void o6_byteArrayRead();
	void o6_wordArrayRead();
	void o6_byteArrayIndexedRead();
	void o6_wordArrayIndexedRead();
	void o6_dup();
	void o6_zero();
	void o6_eq();
	void o6_neq();
	void o6_gt();
	void o6_lt();
	void o6_le();
	void o6_ge();
	void o6_add();
	void o6_sub();
	void o6_mul();
	void o6_div();
	void o6_land();
	void o6_lor();
	void o6_kill();
	void o6_writeByteVar();
	void o6_writeWordVar();
	void o6_byteArrayWrite();
	void o6_wordArrayWrite();
	void o6_byteArrayIndexedWrite();
	void o6_wordArrayIndexedWrite();
	void o6_byteVarInc();
	void o6_wordVarInc();
	void o6_byteArrayInc();
	void o6_wordArrayInc();
	void o6_byteVarDec();
	void o6_wordVarDec();
	void o6_byteArrayDec();
	void o6_wordArrayDec();
	void o6_jumpTrue();
	void o6_jumpFalse();
	void o6_jump();
	void o6_startScriptEx();
	void o6_startScript();
	void o6_startObject();
	void o6_setObjectState();
	void o6_setObjectXY();
	void o6_stopObjectCode();
	void o6_endCutscene();
	void o6_cutScene();
	void o6_stopMusic();
	void o6_freezeUnfreeze();
	void o6_cursorCommand();
	void o6_breakHere();
	void o6_ifClassOfIs();
	void o6_setClass();
	void o6_getState();
	void o6_setState();
	void o6_setOwner();
	void o6_getOwner();
	void o6_startSound();
	void o6_stopSound();
	void o6_startMusic();
	void o6_stopObjectScript();
	void o6_panCameraTo();
	void o6_actorFollowCamera();
	void o6_setCameraAt();
	void o6_loadRoom();
	void o6_stopScript();
	void o6_walkActorToObj();
	void o6_walkActorTo();
	void o6_putActorInRoom();
	void o6_putActorAtObject();
	void o6_faceActor();
	void o6_animateActor();
	void o6_doSentence();
	void o6_pickupObject();
	void o6_loadRoomWithEgo();
	void o6_getRandomNumber();
	void o6_getRandomNumberRange();
	void o6_getActorMoving();
	void o6_getScriptRunning();
	void o6_getActorRoom();
	void o6_getObjectX();
	void o6_getObjectY();
	void o6_getObjectDir();
	void o6_getActorWalkBox();
	void o6_getActorCostume();
	void o6_findInventory();
	void o6_getInventoryCount();
	void o6_getVerbFromXY();
	void o6_beginOverride();
	void o6_endOverride();
	void o6_setObjectName();
	void o6_isSoundRunning();
	void o6_setBoxFlags();
	void o6_createBoxMatrix();
	void o6_resourceRoutines();
	void o6_roomOps();
	void o6_actorSet();
	void o6_verbOps();
	void o6_getActorFromXY();
	void o6_findObject();
	void o6_pseudoRoom();
	void o6_getActorElevation();
	void o6_getVerbEntrypoint();
	void o6_arrayOps();
	void o6_saveRestoreVerbs();
	void o6_drawBox();
	void o6_getActorWidth();
	void o6_wait();
	void o6_getActorScaleX();
	void o6_getActorAnimCounter1();
	void o6_soundKludge();
	void o6_isAnyOf();
	void o6_quitPauseRestart();
	void o6_isActorInBox();
	void o6_delay();
	void o6_delayLonger();
	void o6_delayVeryLong();
	void o6_stopSentence();
	void o6_print_0();
	void o6_print_1();
	void o6_print_2();
	void o6_print_3();
	void o6_printActor();
	void o6_printEgo();
	void o6_talkActor();
	void o6_talkEgo();
	void o6_dim();
	void o6_runVerbCodeQuick();
	void o6_runScriptQuick();
	void o6_dim2();
	void o6_abs();
	void o6_distObjectObject();
	void o6_distObjectPt();
	void o6_distPtPt();
	void o6_dummy_stacklist();
	void o6_miscOps();
	void o6_breakMaybe();
	void o6_pickOneOf();
	void o6_pickOneOfDefault();

	void soundKludge(int16 *list);

	void stopObjectCode();
	void stopObjectScript(int script);
	void putActor(Actor *a, int x, int y, byte room);
	void clearMsgQueue();
	void adjustActorPos(Actor *a);
	
	void hideActor(Actor *a);
	void showActor(Actor *a);
	void showActors();
	void turnToDirection(Actor *a, int newdir);

	int whereIsObject(int object);
	int getObjectOrActorXY(int object);
	void addSoundToQueue(int sound);
	void addSoundToQueue2(int sound);
	bool isScriptLoaded(int script);
	int getActorXYPos(Actor *a);
	void getObjectXYPos(int object);
	AdjustBoxResult adjustXYToBeInBox(Actor *a, int x, int y);

	int getWordVararg(int16 *ptr);

	int getObjActToObjActDist(int a, int b);
	void unkSoundProc22();
	bool inBoxQuickReject(int box, int x, int y, int threshold);
	AdjustBoxResult getClosestPtOnBox(int box, int x, int y);

	void setCameraAt(int dest);
	void stopTalk();
	void restoreCharsetBg();

	void setCameraFollows(Actor *a);
	void runHook(int i);
	void startScene(int room, Actor *a, int b);

	void freezeScripts(int scr);
	void unfreezeScripts();

	void runAllScripts();

	int findObject(int x, int y);
	void stopCycle(int i);
	void killScriptsAndResources();
	void runExitScript();
	void runEntryScript();

	void unkResourceProc();
	void initRoomSubBlocks();
	void loadRoomObjects();

	void setPaletteFromRes();
	void initCycl(byte *ptr);
	void initBGBuffers();
	void setDirtyColors(int min, int max);

	byte *findResource(uint32 tag, byte *searchin);
	byte *findResource2(uint32 tag, byte *searchin);

	void setScaleItem(int slot, int a, int b, int c, int d);

	void cyclePalette();

	void moveMemInPalRes(int start, int end, byte direction);

	void redrawBGAreas();
	void drawRoomObjects(int arg);
	void redrawBGStrip(int start, int num);
	void drawObject(int obj, int arg);

	void drawBmp(byte *ptr, int a, int b, int c, const char *str, int objnr);
	void decompressBitmap();
	int hasCharsetMask(int x, int y, int x2, int y2);
	void draw8ColWithMasking();
	void clear8ColWithMasking();
	void clear8Col();
	void decompressMaskImgOr();
	void decompressMaskImg();

	void GDI_UnkDecode1();
	void GDI_UnkDecode2();
	void GDI_UnkDecode3();
	void GDI_UnkDecode4();
	void GDI_UnkDecode5();
	void GDI_UnkDecode6();
	void GDI_UnkDecode7();

	void restoreBG(int left, int top, int right, int bottom);
	void updateDirtyRect(int virt, int left, int right, int top, int bottom, uint16 dirtybits);
	int findVirtScreen(int y);

	void unkScreenEffect1();
	void unkScreenEffect2();
	void unkScreenEffect3();
	void unkScreenEffect4();
	void unkScreenEffect5(int a);
	void unkScreenEffect6();
	void unkScreenEffect7(int a);
	
	void playSound(int sound);

	void decreaseScriptDelay(int amount);
	void processKbd();

	void clearUpperMask();
	void redrawVerbs();
	void checkExecVerbs();
	void checkAndRunVar33();
	void CHARSET_1();
	void walkActors();
	void moveCamera();
	void fixObjectFlags();
	void clear_fullRedraw();
	void palManipulate();
	void screenEffect(int effect);
	void clearClickedStatus();
	void verbMouseOver(int verb);
	int checkMouseOver(int x, int y);
	void playActorSounds();
	void processDrawQue();
	void setActorRedrawFlags();
	void resetActorBgs();
	void processActors();
	void drawVerb(int verb, int mode);

	void runInputScript(int a, int cmd, int mode);

	void cameraMoved();

	void walkActor(Actor *a);

	int checkKeyHit();

	int getPathToDestBox(int from, int to);
	int findPathTowards(Actor *a, int box, int box2, int box3);

	void setActorCostPalette(Actor *a);
	void drawActorCostume(Actor *a);
	void actorAnimate(Actor *a);

	int getActorFromPos(int x, int y);

	void restoreVerbBG(int verb);

	void drawString(int a);
	void drawVerbBitmap(int vrb, int x, int y);

	void setActorCostume(Actor *a, int c);
	void loadPtrToResource(int type, int i, byte *ptr);

	void push(int a);
	int pop();

	void walkActorTo(Actor *a, int x, int y, int direction);

	void setCursorImg(int cursor, int img);
	void setCursorHotspot(int cursor, int x, int y);
	void initCharset(int charset);
	void addObjectToDrawQue(int object);
	int getVerbEntrypoint(int obj, int entry);
	int unkSoundProc23(int a);
	void startWalkActor(Actor *a, int x, int y, int dir);
	void setBoxFlags(int box, int val);
	void setBoxScale(int box, int b);
	void createBoxMatrix();
	void addObjectToInventory(int obj, int room);
	void removeObjectFromRoom(int obj);
	void decodeParseString();
	void pauseGame(int i);
	void shutDown(int i);
	void lock(int type, int i);
	void unlock(int type, int i);
	void heapClear(int mode);
	void unkHeapProc2(int a, int b);
	void unkResProc(int a, int b);
	void setPalColor(int index, int r, int g, int b);
	void unkRoomFunc2(int a, int b, int c, int d, int e);
	void unkRoomFunc3(int a, int b, int c, int d, int e);
	void unkRoomFunc4(int a, int b, int c, int d, int e);
	int getVerbSlot(int id, int mode);
	void killVerb(int slot);
	byte *getObjectAddress(int obj);
	byte *getObjOrActorName(int obj);
	void clearOwnerOf(int obj);
	void runVerbCode(int script, int entry, int a, int b, int16 *vars);
	void unkSoundProc1(int a);
	void setVerbObject(int room, int object, int verb);
	void unkMessage1();
	void unkMessage2();
	void actorTalk();

	byte *addMessageToStack(byte *msg);

	void unkAddMsgToStack2(int var);
	void unkAddMsgToStack3(int var);
	void unkAddMsgToStack4(int var);
	void unkAddMsgToStack5(int var);

	byte *getActorName(Actor *a);
	uint32 getOBCDOffs(int object);

	byte isMaskActiveAt(int l, int t, int r, int b, byte *mem);

	int getInventorySlot();


	int getKeyInput(int a);
	void convertKeysToClicks();

	void drawBox(int x, int y, int x2, int y2, int color);

	void drawMouse();

	void GDI_drawMouse();
	void GDI_removeMouse();

	void dumpResource(char *tag, int index, byte *ptr);

	FILE *_saveLoadStream;
	bool _saveOrLoad;
	bool saveState(const char *filename);
	bool loadState(const char *filename);
	void saveOrLoad(FILE *inout, bool mode);
	void saveLoadBytes(void *b, int len);
	void saveLoadResource(int type, int index);
	bool isResourceLoaded(int type, int index);
	void saveLoadArrayOf(void *b, int len, int datasize, byte filetype);

	void saveLoadEntries(void *d, const SaveLoadEntry *sle);

	void saveUint32(uint32 d);
	void saveWord(uint16 d);
	void saveByte(byte b);

	byte loadByte();
	uint16 loadWord();
	uint32 loadUint32();


	Actor *derefActor(int id) { return &actor[id]; }
	Actor *derefActorSafe(int id, const char *errmsg);
	Actor *getFirstActor() { return actor; } 

	void setupCostumeRenderer(CostumeRenderer *c, Actor *a);

	PathVertex *unkMatrixProc1(PathVertex *vtx, PathNode *node);
	PathNode *unkMatrixProc2(PathVertex *vtx, int i);
	bool areBoxesNeighbours(int i, int j);
	void addToBoxMatrix(byte b);
	PathVertex *addPathVertex();
	void *addToBoxVertexHeap(int size);

	void parseCommandLine(int argc, char **argv);

	void showHelpAndExit();
	
	char *getGameName();
	bool detectGame();

	void setupOpcodes();
	void setupOpcodes2();
	void endCutscene();
	void cutscene(int16 *args);

	void setOwnerOf(int obj, int owner);
	void panCameraTo(int x);
	void actorFollowCamera(int act);
	void setCameraAtEx(int at);

	void setCursorHotspot2(int x,int y);

	void new_unk_1(int a);

	void faceActorToObj(int act, int obj);
	void animateActor(int act, int anim);
	int getScriptRunning(int script);
	int getObjX(int obj);
	int getObjY(int obj);
	int getObjDir(int obj);
	int findInventory(int owner, int index);
	int getInventoryCount(int owner);

	void beginOverride();
	void endOverride();

	void setPalette(int pal);
	void setPaletteFromPtr(byte *ptr);
	byte *findPalInPals(byte *pal, int index);

	int getStringLen(byte *ptr);

	void readArrayFromIndexFile();
	void readMAXS();
	void readIndexFileV6();

	int readArray(int array, int index, int base);
	void writeArray(int array, int index, int base, int value);
	
	int getStackList(int16 *args, uint maxnum);
	void setObjectState(int obj, int state, int x, int y);

	void setStringVars(int i);
	void decodeParseString2(int a, int b);

	void arrayop_1(int a, byte *ptr);
	
	void copyString(byte *dst, byte *src, int len);
	int getArrayId();

	void nukeArray(int a);
	int defineArray(int a, int b, int c, int d);
	int getDistanceBetween(bool is_obj_1, int b, int c, bool is_obj_2, int e, int f);
	void unkMiscOp4(int a, int b, int c, int d);
	void unkMiscOp9();
	void startManiac();

	void readIndexFileV5(int i);
};

struct ScummDebugger {
	Scumm *_s;
	byte _command;
	char *_parameters;

	bool _welcome;

	int _go_amount;
	
	char _cmd_buffer[256];
	
	void on_frame();
	bool do_command();
	void enter();
	int get_command();
	void attach(Scumm *s);
	void detach();
};



void waitForTimer(Scumm *s);
void outputdisplay2(Scumm *s, int disp);
extern const byte revBitMask[8];
void blitToScreen(Scumm *s, byte *src, int x, int y, int w, int h);

void NORETURN CDECL error(const char *s, ...);
void CDECL warning(const char *s, ...);
void CDECL debug(int level, const char *s, ...);
void checkHeap();
void initGraphics(Scumm *s);
void updateScreen(Scumm *s);

void drawMouse(Scumm *s, int x, int y, int color, byte *mask, bool visible);
