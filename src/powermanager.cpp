#include "powermanager.h"

PowerManager *PowerManager::instance = NULL;

PowerManager::PowerManager(GMenu2X *gmenu2x, uint32_t suspendTimeout, uint32_t powerTimeout) {
	instance = this;
	this->suspendTimeout = suspendTimeout;
	this->powerTimeout = powerTimeout;
	this->gmenu2x = gmenu2x;
	this->suspendActive = false;

	this->powerTimer = NULL;

	resetSuspendTimer();
}

PowerManager::~PowerManager() {
	clearTimer();
	instance = NULL;
}

void PowerManager::setSuspendTimeout(uint32_t suspendTimeout) {
	this->suspendTimeout = suspendTimeout;
	resetSuspendTimer();
};

void PowerManager::setPowerTimeout(uint32_t powerTimeout) {
	this->powerTimeout = powerTimeout;
	resetSuspendTimer();
};

void PowerManager::clearTimer() {
	SDL_RemoveTimer(powerTimer); powerTimer = NULL;
};

void PowerManager::resetSuspendTimer() {
	clearTimer();
	if (this->suspendTimeout > 0)
		powerTimer = SDL_AddTimer(this->suspendTimeout * 1e3, doSuspend, NULL);
};

void PowerManager::resetPowerTimer() {
	clearTimer();
	if (this->powerTimeout > 0)
		powerTimer = SDL_AddTimer(this->powerTimeout * 60e3, doPowerOff, NULL);
};

uint32_t PowerManager::doSuspend(uint32_t interval, void *param) {
	if (interval > 0) {
		PowerManager::instance->gmenu2x->setBacklight(-1);
		PowerManager::instance->resetPowerTimer();
		if (PowerManager::instance->gmenu2x->getBacklight() <= 1)
			PowerManager::instance->suspendActive = true;

		return interval;
	}

	PowerManager::instance->gmenu2x->setBacklight(max(10, PowerManager::instance->gmenu2x->confInt["backlight"]));
	PowerManager::instance->suspendActive = false;
	PowerManager::instance->resetSuspendTimer();
	return interval;
};

uint32_t PowerManager::doPowerOff(uint32_t interval, void *param) {
#if !defined(TARGET_PC)
	system("sync; poweroff");
#endif
	return interval;
};
