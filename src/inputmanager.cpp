/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo   *
 *   massimiliano.torromeo@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "debug.h"
#include "inputmanager.h"
#include "utilities.h"
#include "gmenu2x.h"

#include <iostream>
#include <fstream>

using namespace std;

uint8_t *keystate = SDL_GetKeyState(NULL);

InputManager::InputManager()
	: wakeUpTimer(NULL) {
}

InputManager::~InputManager() {
	for (uint32_t x = 0; x < joysticks.size(); x++)
		if (SDL_JoystickOpened(x))
			SDL_JoystickClose(joysticks[x]);
}

void InputManager::init(const string &conffile) {
	initJoysticks();

	if (!readConfFile(conffile))
		ERROR("InputManager: Could not load input.conf");
}

void InputManager::initJoysticks() {
	//SDL_JoystickEventState(SDL_IGNORE);

	int newJoy = SDL_NumJoysticks();

	if (newJoy == numJoy) return;
	numJoy = newJoy;
	INFO("%d joysticks found", newJoy);
	for (int x = 0; x < newJoy; x++) {
		SDL_Joystick *joy = SDL_JoystickOpen(x);
		if (joy) {
			INFO("Initialized joystick: '%s'", SDL_JoystickName(x));
			joysticks.push_back(joy);
		}
		else WARNING("Failed to initialize joystick: %i", x);
	}
}

bool InputManager::readConfFile(const string &conffile) {
	setActionsCount(NUM_ACTIONS);

	if (!fileExists(conffile)) {
		// ERROR("File not found: %s", conffile.c_str());
		return false;
	}

	ifstream inf(conffile.c_str(), ios_base::in);
	if (!inf.is_open()) {
		ERROR("Could not open %s", conffile.c_str());
		return false;
	}

	int action, linenum = 0;
	string line, name, value;
	string::size_type pos;
	vector<string> values;

	for (uint32_t x = UDC_CONNECT; x < NUM_ACTIONS; x++) {
		InputMap map;
		map.type = InputManager::MAPPING_TYPE_KEYPRESS;
		map.value = x - UDC_CONNECT + SDLK_WORLD_0;
		actions[x].maplist.push_back(map);
	}

	while (getline(inf, line, '\n')) {
		linenum++;
		pos = line.find("=");
		name = trim(line.substr(0, pos));
		value = trim(line.substr(pos + 1));

		if (name == "up")                action = UP;
		else if (name == "down")         action = DOWN;
		else if (name == "left")         action = LEFT;
		else if (name == "right")        action = RIGHT;
		else if (name == "modifier")     action = MODIFIER;
		else if (name == "confirm")      action = CONFIRM;
		else if (name == "cancel")       action = CANCEL;
		else if (name == "manual")       action = MANUAL;
		else if (name == "dec")          action = DEC;
		else if (name == "inc")          action = INC;
		else if (name == "section_prev") action = SECTION_PREV;
		else if (name == "section_next") action = SECTION_NEXT;
		else if (name == "pageup")       action = PAGEUP;
		else if (name == "pagedown")     action = PAGEDOWN;
		else if (name == "settings")     action = SETTINGS;
		else if (name == "menu")         action = MENU;
		else if (name == "volup")        action = VOLUP;
		else if (name == "voldown")      action = VOLDOWN;
		else if (name == "backlight")    action = BACKLIGHT;
		else if (name == "power")        action = POWER;
		else if (name == "speaker") {}
		else {
			ERROR("%s:%d Unknown action '%s'.", conffile.c_str(), linenum, name.c_str());
			continue;
		}

		split(values, value, ",");
		if (values.size() >= 2) {
			if (values[0] == "joystickbutton" && values.size() == 3) {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_BUTTON;
				map.num = atoi(values[1].c_str());
				map.value = atoi(values[2].c_str());
				map.treshold = 0;
				actions[action].maplist.push_back(map);
				// INFO("ADDING: joystickbutton %d  %d ", map.num, map.value);
			} else if (values[0] == "joystickaxis" && values.size() == 4) {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_AXIS;
				map.num = atoi(values[1].c_str());
				map.value = atoi(values[2].c_str());
				map.treshold = atoi(values[3].c_str());
				actions[action].maplist.push_back(map);
			} else if (values[0] == "keyboard") {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_KEYPRESS;
				map.value = atoi(values[1].c_str());
				actions[action].maplist.push_back(map);
			} else {
				ERROR("%s:%d Invalid syntax or unsupported mapping type '%s'.", conffile.c_str(), linenum, value.c_str());
				continue;
			}

		} else {
			ERROR("%s:%d Every definition must have at least 2 values (%s).", conffile.c_str(), linenum, value.c_str());
			continue;
		}
	}
	inf.close();
	return true;
}

void InputManager::setActionsCount(int count) {
	actions.clear();
	for (int x = 0; x < count; x++) {
		InputManagerAction action;
		action.active = false;
		action.interval = 0;
		action.timer = NULL;
		actions.push_back(action);
	}
}

bool InputManager::update(bool wait) {
	bool anyactions = false;
	SDL_JoystickUpdate();

	SDL_Event event;

	if (wait) {
		SDL_WaitEvent(&event);
		if (event.type == SDL_KEYDOWN) {
			anyactions = true;
			keystate[event.key.keysym.sym] = true;
		} else {
			if (event.type == SDL_KEYUP) {
				anyactions = true;
			}

			#if !defined(TARGET_PC)
				keystate[event.key.keysym.sym] = false;
			#endif
		}
	}

	// WARNING("SDL_JOYBUTTONDOWN=%d SDL_JOYAXISMOTION=%d event.type: %d", SDL_JOYBUTTONDOWN, SDL_JOYAXISMOTION, event.type); // clear event queue
	// WARNING("event.jbutton.button=%d event.jaxis.axis=%d event.jaxis.value=%d", event.jbutton.button, event.jaxis.axis, event.jaxis.value); // clear event queue

	if (event.type != SDL_JOYBUTTONDOWN && event.type != SDL_JOYAXISMOTION) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYUP){
				keystate[event.key.keysym.sym] = false;
				// WARNING("Skipping event.type: %d", event.type); // clear event queue
			}
		}
	}

	for (uint32_t x = 0; x < actions.size(); x++) {
		actions[x].active = isActive(x);
		// WARNING("is active: %d %d", x, actions[x].active);
		if (actions[x].active) {
			memcpy(input_combo, input_combo + 1, sizeof(input_combo) - 1); // eegg
			input_combo[sizeof(input_combo) - 1] = x; // eegg
			if (actions[x].timer == NULL) {
				SDL_RemoveTimer(actions[x].timer); actions[x].timer = NULL;
				actions[x].timer = SDL_AddTimer(actions[x].interval, wakeUp, (void*)true);
			}
			anyactions = true;
		} else {
			if (actions[x].timer != NULL) {
				SDL_RemoveTimer(actions[x].timer);
				actions[x].timer = NULL;
			}
		}
	}
	return anyactions;
}

bool InputManager::combo() { // eegg
	return !memcmp(input_combo, konami, sizeof(input_combo));
}

void InputManager::dropEvents() {
	for (uint32_t x = 0; x < actions.size(); x++) {
		actions[x].active = false;
		if (actions[x].timer != NULL) {
			SDL_RemoveTimer(actions[x].timer);
			actions[x].timer = NULL;
		}
	}
}

void InputManager::pushEvent(int action) {
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.state = SDL_PRESSED;
	event.key.keysym.sym = (SDLKey)(action - UDC_CONNECT + SDLK_WORLD_0);
	SDL_PushEvent(&event);
	SDL_AddTimer(100, pushEventEnd, (void*)action);
	SDL_AddTimer(500, pushEventEnd, (void*)action);
	SDL_AddTimer(2000, pushEventEnd, (void*)action);
}

uint32_t InputManager::pushEventEnd(uint32_t interval, void *action) {
	SDL_Event event;
	event.type = SDL_KEYUP;
	event.key.state = SDL_RELEASED;
	event.key.keysym.sym = (SDLKey)(int)((size_t)action - UDC_CONNECT + SDLK_WORLD_0);
	SDL_PushEvent(&event);
	return 0;
}

uint32_t InputManager::checkRepeat(uint32_t interval, void *_data) {
	RepeatEventData *data = (RepeatEventData *)_data;
	InputManager *im = (class InputManager*)data->im;
	SDL_JoystickUpdate();
	if (im->isActive(data->action)) {
		SDL_PushEvent( im->fakeEventForAction(data->action) );
		return interval;
	} else {
		im->actions[data->action].timer = NULL;
		return 0;
	}
}

SDL_Event *InputManager::fakeEventForAction(int action) {
	MappingList mapList = actions[action].maplist;
	// Take the first mapping. We only need one of them.
	InputMap map = *mapList.begin();

	SDL_Event *event = new SDL_Event();
	switch (map.type) {
		case InputManager::MAPPING_TYPE_BUTTON:
			event->type = SDL_JOYBUTTONDOWN;
			event->jbutton.type = SDL_JOYBUTTONDOWN;
			event->jbutton.which = map.num;
			event->jbutton.button = map.value;
			event->jbutton.state = SDL_PRESSED;
		break;
		case InputManager::MAPPING_TYPE_AXIS:
			event->type = SDL_JOYAXISMOTION;
			event->jaxis.type = SDL_JOYAXISMOTION;
			event->jaxis.which = map.num;
			event->jaxis.axis = map.value;
			event->jaxis.value = map.treshold;
		break;
		case InputManager::MAPPING_TYPE_KEYPRESS:
			event->type = SDL_KEYDOWN;
			event->key.keysym.sym = (SDLKey)map.value;
		break;
	}
	return event;
}

int InputManager::count() {
	return actions.size();
}

void InputManager::setInterval(int ms, int action) {
	if (action < 0)
		for (uint32_t x = 0; x < actions.size(); x++)
			actions[x].interval = ms;
	else if ((uint32_t)action < actions.size())
		actions[action].interval = ms;
}

void InputManager::setWakeUpInterval(int ms) {
	return; // temp
	if (wakeUpTimer != NULL)
		SDL_RemoveTimer(wakeUpTimer);

	if (ms > 0)
		wakeUpTimer = SDL_AddTimer(ms, wakeUp, (void*)true);
}

uint32_t InputManager::wakeUp(uint32_t interval, void *repeat) {
	SDL_Event *event = new SDL_Event();
	event->type = SDL_WAKEUPEVENT;
	SDL_PushEvent( event );
	if ((bool*) repeat) return interval;
	return 0;
}

bool &InputManager::operator[](int action) {
	// if (action<0 || (uint32_t)action>=actions.size()) return false;
	return actions[action].active;
}

bool InputManager::isActive(int action) {
	MappingList mapList = actions[action].maplist;
	for (MappingList::const_iterator it = mapList.begin(); it != mapList.end(); ++it) {
		InputMap map = *it;
		// WARNING("map.type: %d   joysticks.size(): %d ", map.type, joysticks.size());

		switch (map.type) {
			case InputManager::MAPPING_TYPE_BUTTON:
				// WARNING("map.num: %d   joysticks.size(): %d  %d", map.num, joysticks.size(), SDL_JoystickGetButton(joysticks[map.num], map.value));
				// if (map.num < joysticks.size() && SDL_JoystickGetButton(joysticks[map.num], map.value))
					// return true;
				for (int j = 0; j < joysticks.size(); j++) {
					if (SDL_JoystickGetButton(joysticks[j], map.value)) return true;
				}
				break;
			case InputManager::MAPPING_TYPE_AXIS:
				if (map.num < joysticks.size()) {
					int axyspos = SDL_JoystickGetAxis(joysticks[map.num], map.value);
					if (map.treshold < 0 && axyspos < map.treshold) return true;
					if (map.treshold > 0 && axyspos > map.treshold) return true;
				}
				break;
			case InputManager::MAPPING_TYPE_KEYPRESS:
				if (keystate[map.value]) return true;
				break;
		}
	}
	return false;
}
