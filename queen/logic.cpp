/* ScummVM - Scumm Interpreter
 * Copyright (C) 2003 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "queen/logic.h"
#include "queen/defs.h"
#include "queen/display.h"
#include "queen/graphics.h"
#include "queen/walk.h"


namespace Queen {

Logic::Logic(Resource *resource, Graphics *graphics, Display *theDisplay)
	: _resource(resource), _graphics(graphics), _display(theDisplay), _talkSpeed(DEFAULT_TALK_SPEED) {
	_jas = _resource->loadFile("QUEEN.JAS", 20);
	_joe.x = _joe.y = 0;
	_joe.scale = 100;
	_walk = new Walk(this, _graphics);
	memset(_gameState, 0, sizeof(_gameState));
	initialise();
}

Logic::~Logic() {
	delete[] _jas;
	delete _walk;
}

void Logic::initialise() {
	int16 i, j;
	uint8 *ptr = _jas;
	
	_numRooms = READ_BE_UINT16(ptr); ptr += 2;
	_numNames = READ_BE_UINT16(ptr); ptr += 2;
	_numObjects = READ_BE_UINT16(ptr); ptr += 2;
	_numDescriptions = READ_BE_UINT16(ptr); ptr += 2;
	
	// Object data
	_objectData = new ObjectData[_numObjects + 1];
	memset(&_objectData[0], 0, sizeof(ObjectData));
	for (i = 1; i <= _numObjects; i++) {
		_objectData[i].readFrom(ptr);
	}
	
	// Room data
	_roomData = new uint16[_numRooms + 2];
	_roomData[0] = 0;
	for (i = 1; i <= (_numRooms + 1); i++) {
		_roomData[i] = READ_BE_UINT16(ptr);	ptr += 2;
	}
	_roomData[_numRooms + 1] = _numObjects;

	// SFX Name
	// the following table isn't available in demo version
	if (_resource->isDemo()) {
		_sfxName = NULL;
	}
	else {
		_sfxName = new uint16[_numRooms + 1];
		_sfxName[0] = 0;
		for (i = 1; i <= _numRooms; i++) {
			_sfxName[i] = READ_BE_UINT16(ptr); ptr += 2;
		}	
	}

	// Item information
	_numItems = READ_BE_UINT16(ptr); ptr += 2;

	_itemData = new ItemData[_numItems + 1];
	memset(&_itemData[0], 0, sizeof(ItemData));
	for (i = 1; i <= _numItems; i++) {
		_itemData[i].readFrom(ptr);
	}
		
	// Graphic Image Data
	_numGraphics = READ_BE_UINT16(ptr); ptr += 2;

	_graphicData = new GraphicData[_numGraphics + 1];
	memset(&_graphicData[0], 0, sizeof(GraphicData));
	for (i = 1; i <= _numGraphics; i++) {
		_graphicData[i].readFrom(ptr);
	}
	
	_objMax   = new int16[_numRooms + 1];
	_areaMax  = new int16[_numRooms + 1];
	_area     = new Area[_numRooms + 1][MAX_AREAS_NUMBER];

	_objMax[0] = 0;
	_areaMax[0] = 0;
	memset(&_area[0], 0, sizeof(Area) * MAX_AREAS_NUMBER);
	for (i = 1; i <= _numRooms; i++) {
		_objMax[i] = (int16)READ_BE_UINT16(ptr); ptr += 2;
		_areaMax[i] = (int16)READ_BE_UINT16(ptr); ptr += 2;
		memset(&_area[i][0], 0, sizeof(Area));
		for (j = 1; j <= _areaMax[i]; j++) {
			assert(j < MAX_AREAS_NUMBER);
			_area[i][j].readFrom(ptr);
		}
	}

	_objectBox = new Box[_numObjects + 1];
	memset(&_objectBox[0], 0, sizeof(Box));
	for (i = 1; i <= _numObjects; i++) {
		_objectBox[i].readFrom(ptr);
	}

	// Walk OFF Data
	_numWalkOffs = READ_BE_UINT16(ptr);	ptr += 2;

	_walkOffData = new WalkOffData[_numWalkOffs + 1];
	memset(&_walkOffData[0], 0, sizeof(WalkOffData));
	for (i = 1; i <= _numWalkOffs; i++) {
		_walkOffData[i].readFrom(ptr);
	}

	// Special Object Descriptions
	_numObjDesc = READ_BE_UINT16(ptr); ptr += 2;

	_objectDescription = new ObjectDescription[_numObjDesc + 1];
	memset(&_objectDescription[0], 0, sizeof(ObjectDescription));
	for (i = 1; i <= _numObjDesc; i++) {
		_objectDescription[i].readFrom(ptr);
	}

	// Command List Data
	_numCmdList = READ_BE_UINT16(ptr); ptr += 2;

	_cmdList = new CmdListData[_numCmdList + 1];
	memset(&_cmdList[0], 0, sizeof(CmdListData));
	for (i = 1; i <= _numCmdList; i++) {
		_cmdList[i].readFrom(ptr);
	}
	
	// Command AREA
	_numCmdArea = READ_BE_UINT16(ptr); ptr += 2;

	_cmdArea = new CmdArea[_numCmdArea + 1];
	memset(&_cmdArea[0], 0, sizeof(CmdArea));
	for (i = 1; i <= _numCmdArea; i++) {
		_cmdArea[i].readFrom(ptr);
	}
	
	// Command OBJECT
	_numCmdObject = READ_BE_UINT16(ptr); ptr += 2;

	_cmdObject = new CmdObject[_numCmdObject + 1];
	memset(&_cmdObject[0], 0, sizeof(CmdObject));
	for (i = 1; i <= _numCmdObject; i++) {
		_cmdObject[i].readFrom(ptr);
	}

	// Command INVENTORY
	_numCmdInventory = READ_BE_UINT16(ptr);	ptr += 2;

	_cmdInventory = new CmdInventory[_numCmdInventory + 1];
	memset(&_cmdInventory[0], 0, sizeof(CmdInventory));
	for (i = 1; i <= _numCmdInventory; i++) {
		_cmdInventory[i].readFrom(ptr);
	}
	
	// Command GAMESTATE
	_numCmdGameState = READ_BE_UINT16(ptr);	ptr += 2;
	_cmdGameState = new CmdGameState[_numCmdGameState + 1];
	memset(&_cmdGameState[0], 0, sizeof(CmdGameState));
	for (i = 1; i <= _numCmdGameState; i++) {
		_cmdGameState[i].readFrom(ptr);
	}

	_entryObj = READ_BE_UINT16(ptr); ptr += 2;

	// Furniture DATA
	_numFurniture = READ_BE_UINT16(ptr); ptr += 2;

	_furnitureData = new FurnitureData[_numFurniture + 1];
	memset(&_furnitureData[0], 0, sizeof(_furnitureData));
	for (i = 1; i <= _numFurniture; i++) {
		_furnitureData[i].readFrom(ptr);
	}

	// Actors
	_numActors = READ_BE_UINT16(ptr); ptr += 2;
	_numAAnim = READ_BE_UINT16(ptr); ptr += 2;
	_numAName = READ_BE_UINT16(ptr); ptr += 2;
	_numAFile = READ_BE_UINT16(ptr); ptr += 2;

	_actorData = new ActorData[_numActors + 1];
	memset(&_actorData[0], 0, sizeof(ActorData));
	for (i = 1; i <= _numActors; i++) {
		_actorData[i].readFrom(ptr);
	}

	_numGraphicAnim = READ_BE_UINT16(ptr); ptr += 2;
	
	_graphicAnim = new GraphicAnim[_numGraphicAnim + 1];
	memset(&_graphicAnim[0], 0, sizeof(GraphicAnim));
	for (i = 1; i <= _numGraphicAnim; i++) {
		_graphicAnim[i].readFrom(ptr);
	}

	_currentRoom = _objectData[_entryObj].room;
	_entryObj = 0;

	if(memcmp(ptr, _resource->JASVersion(), 5) != 0) {
		warning("Unexpected queen.jas file format");
	}
	
	/*
		switch (_resource->getLanguage()) {
			case	ENGLISH:
					_keyLanguage = _keyCommands[KEYS_ENGLISH];
					break;
			case	GERMAN:
					_keyLanguage = _keyCommands[KEYS_GERMAN];
					break;
			case	FRENCH:
					_keyLanguage = _keyCommands[KEYS_FRENCH];
					break;
			case	ITALIAN:
					_keyLanguage = _keyCommands[KEYS_ITALIAN];
					break;
			default:
					_keyLanguage = _keyCommands[KEYS_ENGLISH];
					break;
		}
	*/

	_objDescription = new char*[_numDescriptions + 1];
	_objDescription[0] = 0;
	for (i = 1; i <= _numDescriptions; i++)
		_objDescription[i] = _resource->getJAS2Line();

	//Patch for German text bug
	if (_resource->getLanguage() == GERMAN) {
		char *txt = new char[48];
		strcpy(txt, "Es bringt nicht viel, das festzubinden.");
		_objDescription[296] = txt;
	}
	
	_objName = new char*[_numNames + 1];
	_objName[0] = 0;
	for (i = 1; i <= _numNames; i++)
		_objName[i] = _resource->getJAS2Line();

	_roomName = new char*[_numRooms + 1];
	_roomName[0] = 0;
	for (i = 1; i <= _numRooms; i++)
		_roomName[i] = _resource->getJAS2Line();

	_verbName[0] = 0;
	for (i = 1; i <= 12; i++)
		_verbName[i] = _resource->getJAS2Line();

	_joeResponse[0] = 0;
	for (i = 1; i <= JOE_RESPONSE_MAX; i++)
		_joeResponse[i] = _resource->getJAS2Line();

	_aAnim = new char*[_numAAnim + 1];
	_aAnim[0] = 0;
	for (i = 1; i <= _numAAnim; i++)
		_aAnim[i] = _resource->getJAS2Line();

	_aName = new char*[_numAName + 1];
	_aName[0] = 0;
	for (i = 1; i <= _numAName; i++)
		_aName[i] = _resource->getJAS2Line();
	
	_aFile = new char*[_numAFile + 1];
	_aFile[0] = 0;
	for (i = 1; i <= _numAFile; i++)
		_aFile[i] = _resource->getJAS2Line();

	_textToggle = true;	
	if (_resource->isFloppy())
		_speechToggle = false;
	else
		_speechToggle = true;

	_graphics->loadPanel();
	_graphics->bobSetupControl();
	joeSetup();
	zoneSetupPanel();

	memset(_zones, 0, sizeof(_zones));
	_oldRoom = 0;
}

uint16 Logic::currentRoom() {
	return _currentRoom;
}

void Logic::currentRoom(uint16 room) {
	_currentRoom = room;
}

void Logic::oldRoom(uint16 room) {
	_oldRoom = room;
}

ObjectData* Logic::objectData(int index) {
  return &_objectData[index];
}

uint16 Logic::roomData(int room) {
	return _roomData[room];
}

uint16 Logic::objMax(int room) {
	return _objMax[room];
}

Area *Logic::area(int room, int num) {
	return &_area[room][num];
}

Area *Logic::currentRoomArea(int num) {
	if (num == 0 || num > _areaMax[_currentRoom]) {
		error("Logic::currentRoomArea() - Bad area number = %d (max = %d), currentRoom = %d", num, _areaMax[_currentRoom], _currentRoom);
	}
	return &_area[_currentRoom][num];
}

uint16 Logic::areaMax(int room) {
	return _areaMax[room];
}

uint16 Logic::currentRoomAreaMax() {
	return _areaMax[_currentRoom];
}

uint16 Logic::walkOffCount() {
	return _numWalkOffs;
}

WalkOffData *Logic::walkOffData(int index) {
	return &_walkOffData[index];
}

GraphicData *Logic::graphicData(int index) {
	return &_graphicData[index];
}

uint16 Logic::findBob(uint16 obj) {

	uint16 i;
	uint16 bobnum = 0;
	uint16 bobtype = 0; // 1 for animated, 0 for static

	if (obj > _numObjects)
		error("Object index (%i) > _numObjects (%i)", obj, _numObjects);

	uint16 room = _objectData[obj].room;

	if (room >= _numRooms) {
		warning("room (%i) > _numRooms (%i)", room, _numRooms);
	}
	
	int16 img = _objectData[obj].image;
	if(img != 0) {
		if(img == -3 || img == -4) {
			// a person object
			for(i = _roomData[room] + 1; i <= obj; ++i) {
				img = _objectData[i].image;
				if(img == -3 || img == -4) {
					++bobnum;
				}
			}
		}
		else {
			if(img <= -10) {
				// object has been turned off, but the image order hasn't been updated
				if(_graphicData[-(img + 10)].lastFrame != 0) {
					bobtype = 1;
				}
			}
			else if(img == -2) {
				// -1 static, -2 animated
				bobtype = 1;
			}
			else if(img > 0) {
				if(_graphicData[img].lastFrame != 0) {
					bobtype = 1;
				}
			}

			uint16 idxAnimated = 0;
			uint16 idxStatic = 0;
			for(i = _roomData[room] + 1; i <= obj; ++i) {
				img = _objectData[i].image;
				if(img <= -10) {
					if(_graphicData[-(img + 10)].lastFrame != 0) {
						++idxAnimated;
					}
					else {
						++idxStatic;
					}
				}
				else if(img > 0) {
					if(img > 5000) {
						img -= 5000;
					}

					if (img >= _numGraphics)
						warning("img (%i) >= _numGraphics (%i)", img, _numGraphics);
					
					if(_graphicData[img].lastFrame != 0) {
						++idxAnimated;
					}
					else {
						++idxStatic;
					}
				}
				else if(img == -1) {
					++idxStatic;
				}
				else if(img == -2) {
					++idxAnimated;
				}
			}
			if(bobtype == 0) {
				// static bob
				if(idxStatic > 0) {
					bobnum = 19 + _numFurnitureStatic + idxStatic;
				}
			}
			else {
				// animated bob
				if(idxAnimated > 0) {
					bobnum = 4 + _numFurnitureAnimated + idxAnimated;
				}
			}
		}
	}
	return bobnum;
}


uint16 Logic::findFrame(uint16 obj) {

	uint16 i;
	uint16 framenum = 0;

	uint16 room = _objectData[obj].room;
	int16 img = _objectData[obj].image;
	if(img == -3 || img == -4) {
		uint16 bobnum = 0;
		for(i = _roomData[room] + 1; i <= obj; ++i) {
			img = _objectData[i].image;
			if(img == -3 || img == -4) {
				++bobnum;
			}
		}
		if(bobnum <= 3) {
			framenum = 29 + FRAMES_JOE_XTRA + bobnum;
		}
	}
	else {
		uint16 idx = 0;
		for(i = _roomData[room] + 1; i < obj; ++i) {
			img = _objectData[i].image;
			if(img <= -10) {
				GraphicData* pgd = &_graphicData[-(img + 10)];
				if(pgd->lastFrame != 0) {
					// skip all the frames of the animation
					idx += ABS(pgd->lastFrame) - pgd->firstFrame + 1;
				}
				else {
					// static bob, skip one frame
					++idx;
				}
			}
			else if(img == -1) {
				++idx;
			}
			else if(img > 0) {
				if(img > 5000) {
					img -= 5000;
				}
				GraphicData* pgd = &_graphicData[img];
				uint16 lastFrame = ABS(pgd->lastFrame);
				if(pgd->firstFrame < 0) {
					idx += lastFrame;
				}
				else if(lastFrame != 0) {
					idx += (lastFrame - pgd->firstFrame) + 1;
				}
				else {
					++idx;
				}
			}
		}

		img = _objectData[obj].image;
		if(img <= -10) {
			GraphicData* pgd = &_graphicData[-(img + 10)];
			if(pgd->lastFrame != 0) {
				idx += ABS(pgd->lastFrame) - pgd->firstFrame + 1;
			}
			else {
				++idx;
			}
		}
		else if(img == -1 || img > 0) {
			++idx;
		}

		// calculate only if there are person frames
		if(idx > 0) {
			framenum = 36 + FRAMES_JOE_XTRA + _numFurnitureStatic + _numFurnitureAnimatedLen + idx;
		}
	}
	return framenum;
}


uint16 Logic::objectForPerson(uint16 bobNum) {

	uint16 bobcur = 0;
	// first object number in the room
	uint16 cur = _roomData[_currentRoom] + 1;
	// last object number in the room
	uint16 last = _roomData[_currentRoom + 1];
	while (cur <= last) {
		int16 image = _objectData[cur].image;
		if (image == -3 || image == -4) {
			// the object is a bob
			++bobcur;
		}
		if (bobcur == bobNum) {
			return cur;
		}
		++cur;
	}
	return 0;
}


WalkOffData *Logic::walkOffPointForObject(uint16 obj) {
	
	uint16 i;
	for (i = 1; i <= _numWalkOffs; ++i) {
		if (_walkOffData[i].entryObj == obj) {
			return &_walkOffData[i];
		}
	}
	return NULL;
}


void Logic::joeFacing(uint16 dir) {
	_joe.facing = dir;
}

void Logic::joeX(uint16 x) {
	_joe.x = x;
}

void Logic::joeY(uint16 y) {
	_joe.y = y;
}

void Logic::joeWalk(uint16 walking) {
	_joe.walk = walking;
}

void Logic::joeScale(uint16 scale) {
	_joe.scale = scale;
}

void Logic::joePrevFacing(uint16 dir) {
	_joe.prevFacing = dir;
}

int16 Logic::gameState(int index) {
	if (index >= 0 && index < GAME_STATE_COUNT)
		return _gameState[index];
	else
		error("[QueenLogic::gameState] invalid index: %i", index);
}

void Logic::gameState(int index, int16 newValue) {
	if (index >= 0 && index < GAME_STATE_COUNT)
		 _gameState[index] = newValue;
	else
		error("[QueenLogic::gameState] invalid index: %i", index);
}


void Logic::zoneSet(uint16 screen, uint16 zone, uint16 x1, uint16 y1, uint16 x2, uint16 y2) {

	debug(9, "Logic::zoneSet(%d, %d, (%d,%d), (%d,%d))", screen, zone, x1, y1, x2, y2);
	ZoneSlot *pzs = &_zones[screen][zone];
	pzs->valid = true;
	pzs->box.x1 = x1;
	pzs->box.y1 = y1;
	pzs->box.x2 = x2;
	pzs->box.y2 = y2;
}


void Logic::zoneSet(uint16 screen, uint16 zone, const Box& box) {
	
	debug(9, "Logic::zoneSet(%d, %d, (%d,%d), (%d,%d))", screen, zone, box.x1, box.y1, box.x2, box.y2);
	ZoneSlot *pzs = &_zones[screen][zone];
	pzs->valid = true;
	pzs->box = box;
}


uint16 Logic::zoneIn(uint16 screen, uint16 x, uint16 y) {

	int i;
	if (screen == ZONE_PANEL) {
		y -= ROOM_ZONE_HEIGHT;
	}
	for(i = 1; i < MAX_ZONES_NUMBER; ++i) {
		ZoneSlot *pzs = &_zones[screen][i];
		if (pzs->valid && pzs->box.contains(x, y)) {
			return i;
		}
	}
	return 0;
}


uint16 Logic::zoneInArea(uint16 screen, uint16 x, uint16 y) {
	uint16 zone = zoneIn(screen, x, y);
	if (zone <= _objMax[_currentRoom]) {
		zone = 0;
	}
	else {
		zone -= _objMax[_currentRoom];
	}
	return zone;
}


void Logic::zoneClearAll(uint16 screen) {

	int i;
	for(i = 1; i < MAX_ZONES_NUMBER; ++i) {
		_zones[screen][i].valid = false;
	}
}


void Logic::zoneSetup() {

	debug(9, "Logic::zoneSetup()");
	zoneClearAll(ZONE_ROOM);

	int i;
	int zoneNum;

	// setup objects zones
	uint16 maxObjRoom = _objMax[_currentRoom];
	uint16 objRoomNum = _roomData[_currentRoom];
	zoneNum = 1;
	for (i = objRoomNum + 1; i <= objRoomNum + maxObjRoom; ++i) {
		if (_objectData[i].name != 0) {
			zoneSet(ZONE_ROOM, zoneNum, _objectBox[i]);
		}
		++zoneNum;
	}

	// setup room zones (areas)
	uint16 maxAreaRoom = _areaMax[_currentRoom];
	for (zoneNum = 1; zoneNum <= maxAreaRoom; ++zoneNum) {
		zoneSet(ZONE_ROOM, maxObjRoom + zoneNum, _area[_currentRoom][zoneNum].box);
	}
}


void Logic::zoneSetupPanel() {

	int i;

	// verbs 
	for (i = 0; i <= 7; ++i) {
		int x = i * 20;
		zoneSet(ZONE_PANEL, i + 1, x, 10, x + 19, 49);
	}

	// inventory scrolls
	zoneSet(ZONE_PANEL, PANEL_AREA_INV_UP,   160, 10, 179, 29);
	zoneSet(ZONE_PANEL, PANEL_AREA_INV_DOWN, 160, 30, 179, 49);

	// inventory items
	zoneSet(ZONE_PANEL, PANEL_AREA_INV_1, 180, 10, 213, 49);
	zoneSet(ZONE_PANEL, PANEL_AREA_INV_2, 214, 10, 249, 49);
	zoneSet(ZONE_PANEL, PANEL_AREA_INV_3, 250, 10, 284, 49);
	zoneSet(ZONE_PANEL, PANEL_AREA_INV_4, 285, 10, 320, 49);
}


void Logic::roomErase() {

	_graphics->frameEraseAll(false);
	_graphics->bankErase(15);
	_graphics->bankErase(11);
	_graphics->bankErase(10);
	_graphics->bankErase(12);

	// TODO: TALKHEAD=0;

	if (_currentRoom >= 114) {
		_display->palFadeOut(0, 255, _currentRoom);
	}
	else {
		_display->palFadeOut(0, 223, _currentRoom);
	}
	
	// TODO: credits system

	// invalidates all persons animations
	uint16 i;
	for (i = 0; i <= 3; ++i) {
		_personFrames[i] = 0;
	}
	for (i = 1; i <= 16; ++i) {
		_newAnim[i][0].frame = 0;
	}

	uint16 cur = _roomData[_oldRoom] + 1;
	uint16 last = _roomData[_oldRoom + 1];
	while (cur <= last) {
		ObjectData *pod = &_objectData[cur];
		if (pod->name == 0) {
			// object has been deleted, invalidate image
			pod->image = 0;
		}
		else if (pod->image > -4000 && pod->image <= -10) {
			if (_graphicData[ABS(pod->image + 10)].lastFrame == 0) {
				// static Bob
				pod->image = -1;
			}
			else {
				// animated Bob
				pod->image = -2;
			}
		}
		++cur;
	}
}


void Logic::roomSetupFurniture() {
	_numFurnitureStatic = 0;
	_numFurnitureAnimated = 0;
	_numFurnitureAnimatedLen = 0;
	uint16 curImage = 36 + FRAMES_JOE_XTRA;

	// count the furniture and update gameState
	uint16 furnitureTotal = 0;
	uint16 i;
	for (i = 1; i <= _numFurniture; ++i) {
		if (_furnitureData[i].room == _currentRoom) {
			++furnitureTotal;
			_gameState[furnitureTotal] = _furnitureData[i].gameStateValue;
		}
	}
	if (furnitureTotal == 0) {
		return;
	}

	// unpack the furniture from the bank 15
	// there are 3 kinds :
	// - static (bobs), gamestate range = ]0;5000]
	// - animated (bobs), gamestate range = ]0;5000]
	// - static (paste downs), gamestate range = [5000; [

	// unpack the static bobs
	for	(i = 1; i <= furnitureTotal; ++i) {
		int16 obj = _gameState[i];
		if (obj > 0 && obj <= 5000) {
			GraphicData *pgd = &_graphicData[obj];
			if (pgd->lastFrame == 0) {
				++_numFurnitureStatic;
				++curImage;
				_graphics->bankUnpack(pgd->firstFrame, curImage, 15);
				++_numFrames;
				BobSlot *pbs = _graphics->bob(19 + _numFurnitureStatic);
				pbs->active = true;
				pbs->x = pgd->x;
				pbs->y = pgd->y;
				pbs->frameNum = curImage;
			}
		}
	}

	// unpack the animated bobs
	uint16 curBob = 0;
	for  (i = 1; i <= furnitureTotal; ++i) {
		int16 obj = _gameState[i];
		if (obj > 0 && obj <= 5000) {
			GraphicData *pgd = &_graphicData[obj];

			bool rebound = false;
			int16 lastFrame = pgd->lastFrame;
			if (lastFrame < 0) {
				rebound = true;
				lastFrame = -lastFrame;
			}

			if (lastFrame > 0) {
				_numFurnitureAnimatedLen += lastFrame - pgd->firstFrame + 1;
				++_numFurnitureAnimated;
				uint16 image = curImage + 1;
				int k;
				for (k = pgd->firstFrame; k <= pgd->lastFrame; ++k) {
					++curImage;
					_graphics->bankUnpack(k, curImage, 15);
					++_numFrames;
				}
				_graphics->bobAnimNormal(5 + curBob, image, curImage, pgd->speed / 4, rebound, false);
				BobSlot *pbs = _graphics->bob(5 + curBob);
				pbs->x = pgd->x;
				pbs->y = pgd->y;
				++curBob;
			}
		}
	}

	// unpack the paste downs
	++curImage;
	for  (i = 1; i <= furnitureTotal; ++i) {
		int16 obj = _gameState[i];
		if (obj > 5000) {
			obj -= 5000;
			GraphicData *pgd = &_graphicData[obj];
			_graphics->bankUnpack(pgd->firstFrame, curImage, 15);
			_graphics->bobPaste(curImage, pgd->x, pgd->y);
			// no need to increment curImage here, as bobPaste() destroys the 
			// unpacked frame after blitting it
		}
	}
}


void Logic::roomSetupObjects() {
	warning("Logic::roomSetupObjects() not fully implemented");

	uint16 i;
	// furniture frames are reserved in ::roomSetupFurniture(), we append objects 
	// frames after the furniture ones.
	uint16 curImage = 36 + FRAMES_JOE_XTRA + _numFurnitureStatic + _numFurnitureAnimatedLen;
	uint16 firstRoomObj = _roomData[_currentRoom] + 1;
	uint16 lastRoomObj = _roomData[_currentRoom + 1];
	uint16 numObjectStatic = 0;
	uint16 numObjectAnimated = 0;
	uint16 curBob;

	// invalidates all Bobs for persons (except Joe's one)
	for (i = 1; i <= 3; ++i) {
		_graphics->bob(i)->active = false;
	}

	// static/animated Bobs
	for (i = firstRoomObj; i <= lastRoomObj; ++i) {
		ObjectData *pod = &_objectData[i];
		// setup blanks bobs for turned off objects (in case 
		// you turn them on again)
		if (pod->image == -1) {
			// static OFF Bob
			curBob = 20 + _numFurnitureStatic + numObjectStatic;
			++numObjectStatic;
			// create a blank frame for the for the OFF object
			++_numFrames;
			++curImage;
		}
		else if(pod->image == -2) {
			// animated OFF Bob
			curBob = 5 + _numFurnitureAnimated + numObjectAnimated;
			++numObjectAnimated;
		}
		else if(pod->image > 0 && pod->image < 5000) {
			GraphicData *pgd = &_graphicData[pod->image];
			int16 lastFrame = pgd->lastFrame;
			bool rebound = false;
			if (lastFrame < 0) {
				lastFrame = -lastFrame;
				rebound = true;
			}
			if (pgd->firstFrame < 0) {
				// FIXME: if(TEMPA[1]<0) bobs[CURRBOB].xflip=1;
				curBob = 5 + _numFurnitureAnimated;
				AnimFrame *paf = NULL;
				if (pod->name > 0) {
					paf = _newAnim[curBob + numObjectAnimated];
				}
				BobSlot *pbs = _graphics->bob(curBob + numObjectAnimated);
				int16 f = animFindAll(pgd, curImage + 1, paf);
				curImage += pgd->lastFrame;
				if (paf != NULL) {
					if (f < 0) {
						pbs->xflip = true;
					}
					pbs->active = true;
					pbs->x = pgd->x;
					pbs->y = pgd->y;
					_graphics->bobAnimString(curBob + numObjectAnimated, paf);
				}
				else {
					pbs->animating = false;
				}
				++numObjectAnimated;
			}
			else if (lastFrame != 0) {
				// animated objects
				uint16 j;
				uint16 firstFrame = curImage + 1;
				for (j = pgd->firstFrame; j <= lastFrame; ++j) {
					++curImage;
					_graphics->bankUnpack(j, curImage, 15);
					++_numFrames;
				}
				curBob = 5 + _numFurnitureAnimated + numObjectAnimated;
				if (pod->name > 0) {
					BobSlot *pbs = _graphics->bob(curBob);
					pbs->active = true;
					pbs->x = pgd->x;
					pbs->y = pgd->y;
					pbs->frameNum = firstFrame;
					if (pgd->speed > 0) {
						_graphics->bobAnimNormal(curBob, firstFrame, curImage, pgd->speed / 4, rebound, false);
					}
				}
				++numObjectAnimated;
			}
			else {
				// static objects
				curBob = 20 + _numFurnitureStatic + numObjectStatic;
				++curImage;
				_graphics->bobClear(curBob);

				// FIXME: if((COMPANEL==2) && (FULLSCREEN==1)) bobs[CURRBOB].y2=199;

				_graphics->bankUnpack(pgd->firstFrame, curImage, 15);
				++_numFrames;
				if (pod->name > 0) {
					BobSlot *pbs = _graphics->bob(curBob);
					pbs->active = true;
					pbs->x = pgd->x;
					pbs->y = pgd->y;
					pbs->frameNum = curImage;
				}
				++numObjectStatic;
			}
		}
	}

	// persons Bobs
	for (i = firstRoomObj; i <= lastRoomObj; ++i) {
		ObjectData *pod = &_objectData[i];
		if (pod->image == -3 || pod->image == -4) {
			debug(9, "Logic::roomSetupObjects() - Setting up person %d", i);
			uint16 noun = i - _roomData[_currentRoom];
			if (pod->name > 0) {
				curImage = personSetup(noun, curImage);
			}
			else {
				curImage = personAllocate(noun, curImage);
			}
		}
	}

	// paste downs list
	++curImage;
	_numFrames = curImage;
	for (i = firstRoomObj; i <= lastRoomObj; ++i) {
		int16 obj = _objectData[i].image;
		if (obj > 5000) {
			obj -= 5000;
			GraphicData *pgd = &_graphicData[obj];
			_graphics->bankUnpack(pgd->firstFrame, curImage, 15);
			_graphics->bobPaste(curImage, pgd->x, pgd->y);
		}
	}
}


uint16 Logic::roomRefreshObject(uint16 obj) {
	warning("Logic::roomRefreshObject() not fully implemented");
	uint16 curImage = _numFrames;

	ObjectData *pod = &_objectData[obj];
	if (pod->image == 0) {
		return curImage;
	}

	// check the object is in the current room
	if (pod->room != _currentRoom) {
		warning("Logic::roomRefreshObject() - Trying to display an object (%i=%s) that is not in room (object room=%i, current room=%i)",
				obj, _objName[_objectData[obj].name], pod->room, _currentRoom);
		return curImage;
	}

	// find bob for the object
	uint16 curBob = findBob(obj);
	BobSlot *pbs = _graphics->bob(curBob);

	if (pod->image == -3 || pod->image == -4) {
		// a person object
		if (pod->name < 0) {
			_graphics->bobClear(curBob);
		}
		else {
			// find person number
			uint16 pNum = 1;
			uint16 i = _roomData[_currentRoom] + 1;
			while (i < obj) {
				if (_objectData[i].image == -3 || _objectData[i].image == -4) {
					++pNum;
				}
				++i;
			}
			curImage = _personFrames[pNum] - 1;
			if (_personFrames[pNum] == 0) {
				curImage = _numFrames;
				_personFrames[pNum] = curImage;
			}
			curImage = personSetup(obj - _roomData[_currentRoom], curImage);
		}
		return curImage;
	}

	if (pod->name < 0 || pod->image < 0) {
		// object is hidden or disabled
		_graphics->bobClear(curBob);
		return curImage;
	}

	// find frame used for object
	curImage = findFrame(obj);

	int image = pod->image;
	if (image > 5000) {
		image -= 5000;
	}
	
	GraphicData *pgd = &_graphicData[image];
	bool rebound = false;
	int16 lastFrame = pgd->lastFrame;
	if (lastFrame < 0) {
		lastFrame = -lastFrame;
		rebound = true;
	}
	if (pgd->firstFrame < 0) {
		AnimFrame *paf = NULL;
		if (pod->name != 0) {
			paf = _newAnim[curBob];
		}
		int16 f = animFindAll(pgd, curImage, paf);
		curImage += pgd->lastFrame - 1;
		if (f < 0) {
			pbs->xflip = true;
		}
		if (paf != NULL) {
			pbs->active = true;
			pbs->x = pgd->x;
			pbs->y = pgd->y;
			_graphics->bobAnimString(curBob, _newAnim[curBob]);
		}
		else {
			pbs->animating = false;
		}
	}
	else if (lastFrame != 0) {
		// turn on an animated bob
		_graphics->bankUnpack(pgd->firstFrame, 2, 15);
		pbs->animating = false;
		uint16 firstImage = curImage;
		--curImage;
		uint16 j;
		for (j = pgd->firstFrame; j <= lastFrame; ++j) {
			++curImage;
			_graphics->bankUnpack(j, curImage, 15);
		}
		pbs->active = true;
		pbs->x = pgd->x;
		pbs->y = pgd->y;
		pbs->frameNum = firstImage;
		if (pgd->speed > 0) {
			_graphics->bobAnimNormal(curBob, firstImage, curImage, pgd->speed / 4, rebound, false);
		}
	}
	else {
		// frame 2 is used as a buffer frame to prevent BOB flickering
		_graphics->bankUnpack(pgd->firstFrame, 2, 15);
		_graphics->bankUnpack(pgd->firstFrame, curImage, 15);
		pbs->active = true;
		pbs->x = pgd->x;
		pbs->y = pgd->y;
		pbs->frameNum = curImage;
	}

	return curImage;
}


void Logic::roomSetup(const char* room, int comPanel, bool inCutaway) {

	char filename[20];

	// loads background image
	sprintf(filename, "%s.PCX", room);
	_graphics->loadBackdrop(filename, _currentRoom);

	// custom colors
	_display->palCustomColors(_currentRoom);

	// setup graphics to enter fullscreen/panel mode
	_display->screenMode(comPanel, inCutaway);

	// reset sprites table (bounding box...)
	_graphics->bobClearAll();

	// load/setup objects associated to this room
	sprintf(filename, "%s.BBK", room);
	_graphics->bankLoad(filename, 15);

	zoneSetup();
	_numFrames = 37 + FRAMES_JOE_XTRA;
	roomSetupFurniture();
	roomSetupObjects();
}


void Logic::roomDisplay(const char* room, RoomDisplayMode mode, uint16 scale, int comPanel, bool inCutaway) {

	debug(9, "Logic::roomDisplay(%s, %d, %d, %d, %d)", room, mode, scale, comPanel, inCutaway);

	roomErase();
	// TODO: _sound->loadSFX(SFXNAME[_currentRoom]);
	roomSetup(room, comPanel, inCutaway);
	ObjectData *pod = NULL;
	if (mode != RDM_FADE_NOJOE) {
		pod = joeSetupInRoom(mode != RDM_FADE_JOE_XY, scale);
	}
	// FIXME: commented for now, to avoid color glitches when
	// switching rooms during cutaway
//	if (mode != RDM_NOFADE_JOE) {
		_graphics->update();
		if (_currentRoom >= 114) {
			_display->palFadeIn(0, 255, _currentRoom);
		}
		else {
			_display->palFadeIn(0, 223, _currentRoom);
		}
//	}
	if (pod != NULL) {
		_walk->joeMove(0, pod->x, pod->y, inCutaway);
	}
}


uint16 Logic::findScale(uint16 x, uint16 y) {
	uint16 scale = 100;
	uint16 areaNum = zoneInArea(ZONE_ROOM, x, y);
	if(areaNum != 0) {
		scale = _area[_currentRoom][areaNum].calcScale(y);
	}
	return scale;
}


void Logic::personSetData(int16 noun, const char *actorName, bool loadBank, Person *pp) {
	
	if (noun <= 0) {
		warning("Logic::personSetData() - Negative object number");
	}

	uint16 i;
	uint16 obj = _roomData[_currentRoom] + noun;
	int16 img = _objectData[obj].image;
	if (img != -3 && img != -4) {
		warning("Logic::personSetData() - Object %d is not a person", obj);
		return;
	}

	// search Bob number for the person
	uint16 bobNum = 0;
	for (i = _roomData[_currentRoom] + 1; i <= obj; ++i) {
		img = _objectData[i].image;
		if (img == -3 || img == -4) {
			++bobNum;
		}
	}

	// search for a matching actor
	uint16 actor = 0;
	for (i = 1; i <= _numActors; ++i) {
		ActorData *pad = &_actorData[i];
		if (pad->room == _currentRoom) {
			if (_gameState[pad->gameStateSlot] == pad->gameStateValue) {
				if ((bobNum > 0 && bobNum == pad->bobNum) || strcmp(_aName[pad->name], actorName) == 0) {
					actor = i;
					break;
				}
			}
		}
	}

	if (!actor)
		error ("Actor '%s' not found", actorName);

	pp->actor = &_actorData[actor];
	pp->bankNum = pp->actor->bankNum;
	pp->name = _aName[pp->actor->name];
	if (pp->actor->anim != 0) {
		pp->anim = _aAnim[pp->actor->anim];
	}
	else {
		pp->anim = NULL;
	}
	
	if (loadBank) {
		const char *actorFile = _aFile[pp->actor->actorFile];
		if (actorFile) {
			_graphics->bankLoad(actorFile, pp->bankNum);
		}
		else {
			pp->bankNum = 15;
		}		
	}
	
	if (pp->actor->bobNum >= 1 && pp->actor->bobNum <= 3) {
		pp->bobFrame = 29 + FRAMES_JOE_XTRA + pp->actor->bobNum;
	}
	else {
		warning("Logic::personSetData() - The bob number for actor is not in the [1:3] range");
	}
}


uint16 Logic::personSetup(uint16 noun, uint16 curImage) {

	Person p;
	personSetData(noun, "", true, &p);

	const ActorData *pad = p.actor;
	uint16 scale = 100;
	uint16 a = zoneInArea(ZONE_ROOM, pad->x, pad->y);
	if (a > 0) {
		// person is not standing in the area box, scale it accordingly
		scale = currentRoomArea(a)->calcScale(pad->y);
	}
	_graphics->bankUnpack(pad->bobFrameStanding, p.bobFrame, p.bankNum);
	bool xflip = false;
	uint16 person = _roomData[_currentRoom] + noun;
	if (_objectData[person].image == -3) {
		// person is facing left
		xflip = true;
	}
	BobSlot *pbs = _graphics->bob(pad->bobNum);
	pbs->active = true;
	pbs->scale = scale;
	pbs->x = pad->x;
	pbs->y = pad->y;
	pbs->frameNum = p.bobFrame;
	pbs->xflip = xflip;
	
	if (p.anim != NULL) {
		_personFrames[pad->bobNum] = curImage + 1;
		curImage = animCreate(curImage, &p);
	}
	else {
		animErase(pad->bobNum);
	}
	return curImage;
}


uint16 Logic::personAllocate(uint16 noun, uint16 curImage) {
	
	uint16 i;
	uint16 person = _roomData[_currentRoom] + noun;

	// search Bob number for the person
	uint16 bobNum = 0;
	for (i = _roomData[_currentRoom] + 1; i <= person; ++i) {
		int16 img = _objectData[i].image;
		if (img == -3 || img == -4) {
			++bobNum;
		}
	}
		
	// search for a matching actor
	uint16 actor = 0;
	for (i = 1; i <= _numActors; ++i) {
		ActorData *pad = &_actorData[i];
		if (pad->room == _currentRoom) {
			if (_gameState[pad->gameStateSlot] == pad->gameStateValue) {
				if (bobNum > 0 && bobNum == pad->bobNum) {
					actor = i;
					break;
				}
			}
		}
	}
	
	if (actor > 0) {
		const char *animStr = _aAnim[_actorData[actor].actorFile];
		if (animStr[0] != '\0') {
			bool allocatedFrames[256];
			memset(allocatedFrames, 0, sizeof(allocatedFrames));
			uint16 f1, f2;
			do {
				sscanf(animStr, "%3hu,%3hu", &f1, &f2);
				animStr += 8;
				allocatedFrames[f1] = true;
			} while(f1 != 0);
			for (i = 1; i <= 255; ++i) {
				if (allocatedFrames[i]) {
					++curImage;
				}
			}
			_personFrames[bobNum] = curImage + 1;
		}
	}
	return curImage;
}


uint16 Logic::animCreate(uint16 curImage, const Person *person) {

	AnimFrame *animFrames = _newAnim[person->actor->bobNum];

	uint16 allocatedFrames[256];
	memset(allocatedFrames, 0, sizeof(allocatedFrames));
	const char *p = person->anim;
	int frame = 0;
	uint16 f1, f2;
	do {
		sscanf(p, "%3hu,%3hu", &f1, &f2);
		animFrames[frame].frame = f1;
		animFrames[frame].speed = f2;
		
		if (f1 > 500) {
			// SFX
			allocatedFrames[f1 - 500] = 1;
		}
		else {
			allocatedFrames[f1] = 1;
		}
		
		p += 8;
		++frame;
	} while(f1 != 0);
	
	// ajust frame numbers
	uint16 n = 1;
	uint16 i;
	for (i = 1; i <= 255; ++i) {
		if (allocatedFrames[i] != 0) {
			allocatedFrames[i] = n;
			++n;
		}
	}
	for (i = 0; animFrames[i].frame != 0; ++i) {
		uint16 frameNum = animFrames[i].frame;
		if (frameNum > 500) {
			animFrames[i].frame = curImage + allocatedFrames[frameNum - 500] + 500;
		}
		else {
			animFrames[i].frame = curImage + allocatedFrames[frameNum];
		}
	}
	
	// unpack necessary frames
	for (i = 1; i <= 255; ++i) {
		if (allocatedFrames[i] != 0) {
			++curImage;
			_graphics->bankUnpack(i, curImage, person->bankNum);
		}
	}
	
	// start animation
	_graphics->bobAnimString(person->actor->bobNum, animFrames);
	
	return curImage;
}


void Logic::animErase(uint16 bobNum) {
	_newAnim[bobNum][0].frame = 0;
	BobSlot *pbs = _graphics->bob(bobNum);
	pbs->animating = false;
	pbs->anim.string.buffer = NULL;
}


int16 Logic::animFindAll(const GraphicData *gd, uint16 firstImage, AnimFrame *paf) {
	
	int16 tempFrames[20];
	memset(tempFrames, 0, sizeof(tempFrames));
	uint16 numTempFrames = 0;
	uint16 i, j;
	for (i = 1; i <= _numGraphicAnim; ++i) {
		const GraphicAnim *pga = &_graphicAnim[i];
		if (pga->keyFrame == gd->firstFrame) {
			int16 frame = pga->frame;
			if (frame > 500) { // SFX
				frame -= 500;
			}
			bool foundMatchingFrame = false;
			for (j = 0; j < numTempFrames; ++j) {
				if (tempFrames[j] == frame) {
					foundMatchingFrame = true;
					break;
				}
			}
			if (!foundMatchingFrame) {
				assert(numTempFrames < 20);
				tempFrames[numTempFrames] = frame;
				++numTempFrames;
			}
		}
	}

	// sort found frames ascending
	bool swap = true;
	while (swap) {
		swap = false;
		for (i = 0; i < numTempFrames - 1; ++i) {
			if (tempFrames[i] > tempFrames[i + 1]) {
				SWAP(tempFrames[i], tempFrames[i + 1]);
				swap = true;
			}
		}
	}

	// queen.c l.962-980 / l.1269-1294
	for (i = 0; i < gd->lastFrame; ++i) {
		_graphics->bankUnpack(ABS(tempFrames[i]), firstImage + i, 15);
	}
	if (paf != NULL) {
		uint16 frameNr = 0;
		for (i = 1; i <= _numGraphicAnim; ++i) {
			const GraphicAnim *pga = &_graphicAnim[i];
			if (pga->keyFrame == gd->firstFrame) {
				for (j = 1; j <= gd->lastFrame; ++j) {
					int16 f = pga->frame;
					if (f > 500) {
						f -= 500;
					}
					if (f == tempFrames[j - 1]) {
						frameNr = j + firstImage - 1;
					}
					if (pga->frame > 500) {
						frameNr += 500;
					}
				}
				paf->frame = frameNr;
				paf->speed = pga->speed;
				++paf;
			}
		}
		paf->frame = 0;
		paf->speed = 0;
	}
	return tempFrames[0];
}


StateDirection Logic::findStateDirection(uint16 state) {

	// FIXME: may be we should return a DIR_* constant instead
	// of a STATE_DIR_*. Some (all ?) calls to FIND_STATE(, "DIR")
	// are often followed by a DIR_* constant.

	// see queen.c l.4016-4023
	StateDirection sd = STATE_DIR_BACK;
	switch ((state >> 2) & 3) {
	case 0: 
		sd = STATE_DIR_BACK;
		break;
	case 1: 
		sd = STATE_DIR_RIGHT;
		break;
	case 2: 
		sd = STATE_DIR_LEFT;
		break;
	case 3: 
		sd = STATE_DIR_FRONT;
		break;
	}
	return sd;
}

StateTalk Logic::findStateTalk(uint16 state) {
	return (state & (1 << 9)) ? STATE_TALK_TALK : STATE_TALK_MUTE;
}

void Logic::joeSetup() {
	int i;

	_graphics->bankLoad("joe_a.BBK", 13);
	for (i = 11; i <= 28 + FRAMES_JOE_XTRA; ++i) {
		_graphics->bankUnpack(i - 10, i, 13);
	}
	_graphics->bankErase(13);

	_graphics->bankLoad("joe_b.BBK", 7);
	_graphics->bankUnpack(1, 33 + FRAMES_JOE_XTRA, 7);
	_graphics->bankUnpack(3, 34 + FRAMES_JOE_XTRA, 7);
	_graphics->bankUnpack(5, 35 + FRAMES_JOE_XTRA, 7);

	_joe.facing = DIR_FRONT;
}


ObjectData *Logic::joeSetupInRoom(bool autoPosition, uint16 scale) {
	// queen.c SETUP_HERO()

	uint16 oldx;
	uint16 oldy;
	WalkOffData *pwo = NULL;
	ObjectData *pod = &_objectData[_entryObj];
	if (pod == NULL) {
		error("Logic::joeSetupInRoom() - No object data for obj %d", _entryObj);
	}

	if (!autoPosition || _joe.x != 0 || _joe.y != 0) {
		oldx = _joe.x;
		oldy = _joe.y;
	}
	else {
		// find the walk off point for the entry object and make 
		// Joe walking to that point
		pwo = walkOffPointForObject(_entryObj);
		if (pwo != NULL) {
			oldx = pwo->x;
			oldy = pwo->y;
		}
		else {
			// no walk off point, use object position
			oldx = pod->x;
			oldy = pod->y;
		}
	}

	debug(9, "Logic::joeSetupInRoom() - oldx=%d, oldy=%d", oldx, oldy);

	if (scale > 0 && scale < 100) {
		_joe.scale = scale;
	}
	else {
		uint16 a = zoneInArea(ZONE_ROOM, oldx, oldy);
		if (a > 0) {
			_joe.scale = currentRoomArea(a)->calcScale(oldy);
		}
		else {
			_joe.scale = 100;
		}
	}

	// TODO: cutawayJoeFacing

    // check to see which way Joe entered room
	switch (findStateDirection(pod->state)) {
	case STATE_DIR_FRONT:
		_joe.facing = DIR_FRONT;
		break;
	case STATE_DIR_BACK:
		_joe.facing = DIR_BACK;
		break;
	case STATE_DIR_LEFT:
		_joe.facing = DIR_LEFT;
		break;
	case STATE_DIR_RIGHT:
		_joe.facing = DIR_RIGHT;
		break;
	}

	_joe.prevFacing = _joe.facing;
	BobSlot *pbs = _graphics->bob(0);
	pbs->scale = _joe.scale;

	// TODO: room 108 specific

	joeFace();
	pbs->active = true;
	pbs->x = oldx;
	pbs->y = oldy;
	pbs->frameNum = 29 + FRAMES_JOE_XTRA;
	_joe.x = 0;
	_joe.y = 0;

	if (pwo != NULL) {
		// entryObj has a walk off point, then walk from there to object x,y
		return pod;
	}
	return NULL;
}


uint16 Logic::joeFace() {

	debug(9, "Logic::joeFace() - curFace = %d, prevFace = %d", _joe.facing, _joe.prevFacing);
	BobSlot *pbs = _graphics->bob(0);
	uint16 frame;
	if (_currentRoom == 108) {
		frame = 1;
	}
	else {
		frame = 33;
		if (_joe.facing == DIR_FRONT) {
			if (_joe.prevFacing == DIR_BACK) {
				pbs->frameNum = 33 + FRAMES_JOE_XTRA;
				_graphics->update();
			}
			frame = 34;
		}
		else if (_joe.facing == DIR_BACK) {
			if (_joe.prevFacing == DIR_FRONT) {
				pbs->frameNum = 33 + FRAMES_JOE_XTRA;
				_graphics->update();
			}
			frame = 35;
		}
		else if ((_joe.facing == DIR_LEFT && _joe.prevFacing == DIR_RIGHT) 
			|| 	(_joe.facing == DIR_RIGHT && _joe.prevFacing == DIR_LEFT)) {
			pbs->frameNum = 34 + FRAMES_JOE_XTRA;
			_graphics->update();
		}
		pbs->frameNum = frame + FRAMES_JOE_XTRA;
		pbs->scale = _joe.scale;
		pbs->xflip = (_joe.facing == DIR_LEFT);
		_graphics->update();
		_joe.prevFacing = _joe.facing;
		switch (frame) {
		case 33: frame = 1; break;
		case 34: frame = 3; break;
		case 35: frame = 5; break;
		}
	}
	pbs->frameNum = 29 + FRAMES_JOE_XTRA;
	_graphics->bankUnpack(frame, pbs->frameNum, 7);
	return frame;
}


} // End of namespace Queen

