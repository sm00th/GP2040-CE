/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#include "addons/i2cdisplay.h"
#include "GamepadState.h"
#include "enums.h"
#include "helper.h"
#include "rp2040-oled.h"
#include "storagemanager.h"
#include "pico/stdlib.h"
#include "bitmaps.h"
#include "ps4_driver.h"
#include "helper.h"
#include "config.pb.h"
#include "usb_driver.h"

bool I2CDisplayAddon::available() {
	const DisplayOptions& options = Storage::getInstance().getDisplayOptions();
	return options.enabled &&
		isValidPin(options.i2cSDAPin) &&
		isValidPin(options.i2cSCLPin);
}

void I2CDisplayAddon::setup() {
	const DisplayOptions& options = Storage::getInstance().getDisplayOptions();

	oled.size = options.size ? static_cast<rp2040_oled_size_t>(options.size) : OLED_128x64;
	oled.addr = options.i2cAddress;
	oled.flip = options.flip ? FLIP_HORIZONTAL : FLIP_NONE;
	oled.invert = options.invert;
	oled.sda_pin = options.i2cSDAPin;
	oled.scl_pin = options.i2cSCLPin;
	oled.i2c = options.i2cBlock == 0 ? i2c0 : i2c1;
	oled.baudrate = options.i2cSpeed;
	oled.use_doublebuf = true;

	rp2040_oled_init(&oled);

	rp2040_oled_set_contrast(&oled, 0xff);
	rp2040_oled_clear(&oled);

	gamepad = Storage::getInstance().GetGamepad();
	pGamepad = Storage::getInstance().GetProcessedGamepad();

	const FocusModeOptions& focusModeOptions = Storage::getInstance().getAddonOptions().focusModeOptions;
	isFocusModeEnabled = focusModeOptions.enabled && focusModeOptions.oledLockEnabled &&
		isValidPin(focusModeOptions.pin);
	prevButtonState = 0;
	displaySaverTimer = options.displaySaverTimeout;
	displaySaverTimeout = displaySaverTimer;
	configMode = Storage::getInstance().GetConfigMode();
	turnOffWhenSuspended = options.turnOffWhenSuspended;
}

bool I2CDisplayAddon::isDisplayPowerOff()
{
	if (turnOffWhenSuspended && get_usb_suspended()) {
		if (displayIsPowerOn) setDisplayPower(0);
		return true;
	} else {
		if (!displayIsPowerOn) setDisplayPower(1);
	}

	if (!displaySaverTimeout && !isFocusModeEnabled) return false;

	float diffTime = getMillis() - prevMillis;
	displaySaverTimer -= diffTime;
	if (!!displaySaverTimeout && (gamepad->state.buttons || gamepad->state.dpad) && !focusModePrevState) {
		displaySaverTimer = displaySaverTimeout;
		setDisplayPower(1);
	} else if (!!displaySaverTimeout && displaySaverTimer <= 0) {
		setDisplayPower(0);
	}

	if (isFocusModeEnabled) {
		const FocusModeOptions& focusModeOptions = Storage::getInstance().getAddonOptions().focusModeOptions;
		bool isFocusModeActive = !gpio_get(focusModeOptions.pin);
		if (focusModePrevState != isFocusModeActive) {
			focusModePrevState = isFocusModeActive;
			if (isFocusModeActive) {
				setDisplayPower(0);
			} else {
				setDisplayPower(1);
			}
		}
	}

	prevMillis = getMillis();

	return (isFocusModeEnabled && focusModePrevState) || (!!displaySaverTimeout && displaySaverTimer <= 0);
}

void I2CDisplayAddon::setDisplayPower(uint8_t status)
{
	if (displayIsPowerOn != status) {
		displayIsPowerOn = status;
		rp2040_oled_set_power(&oled, status);
	}
}

void I2CDisplayAddon::process() {
	if (!configMode && isDisplayPowerOff()) return;

	clearScreen(0);

	switch (getDisplayMode()) {
		case I2CDisplayAddon::DisplayMode::CONFIG_INSTRUCTION:
			drawStatusBar(gamepad);
			drawText(0, 2, "[Web Config Mode]");
			drawText(0, 3, std::string("GP2040-CE : ") + std::string(GP2040VERSION));
			drawText(0, 4, "[http://192.168.7.1]");
			drawText(0, 5, "Preview:");
			drawText(5, 6, "B1 > Button");
			drawText(5, 7, "B2 > Splash");
			break;
		case I2CDisplayAddon::DisplayMode::SPLASH:
			if (getDisplayOptions().splashMode == static_cast<SplashMode>(SPLASH_MODE_NONE)) {
				drawText(0, 4, " Splash NOT enabled.");
				break;
			}
			drawSplashScreen(getDisplayOptions().splashMode, (uint8_t*) Storage::getInstance().getDisplayOptions().splashImage.bytes, 90);
			break;
		case I2CDisplayAddon::DisplayMode::BUTTONS:
			drawStatusBar(gamepad);
			const DisplayOptions& options = getDisplayOptions();
			ButtonLayoutCustomOptions buttonLayoutCustomOptions = options.buttonLayoutCustomOptions;

			switch (options.buttonLayout) {
				case BUTTON_LAYOUT_STICK:
					drawArcadeStick(8, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_STICKLESS:
					drawStickless(8, 20, 8, 2);
					break;
				case BUTTON_LAYOUT_BUTTONS_ANGLED:
					drawWasdBox(8, 28, 7, 3);
					break;
				case BUTTON_LAYOUT_BUTTONS_BASIC:
					drawUDLR(8, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_KEYBOARD_ANGLED:
					drawKeyboardAngled(18, 28, 5, 2);
					break;
				case BUTTON_LAYOUT_KEYBOARDA:
					drawMAMEA(8, 28, 10, 1);
					break;
				case BUTTON_LAYOUT_DANCEPADA:
					drawDancepadA(39, 12, 15, 2);
					break;
				case BUTTON_LAYOUT_TWINSTICKA:
					drawTwinStickA(8, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_BLANKA:
					drawBlankA(0, 0, 0, 0);
					break;
				case BUTTON_LAYOUT_VLXA:
					drawVLXA(7, 28, 7, 2);
					break;
				case BUTTON_LAYOUT_CUSTOMA:
					drawButtonLayoutLeft(buttonLayoutCustomOptions.paramsLeft);
					break;
				case BUTTON_LAYOUT_FIGHTBOARD_STICK:
					drawArcadeStick(18, 22, 8, 2);
					break;
				case BUTTON_LAYOUT_FIGHTBOARD_MIRRORED:
					drawFightboardMirrored(0, 22, 7, 2);
					break;
			}

			switch (options.buttonLayoutRight) {
				case BUTTON_LAYOUT_ARCADE:
					drawArcadeButtons(8, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_STICKLESSB:
					drawSticklessButtons(8, 20, 8, 2);
					break;
				case BUTTON_LAYOUT_BUTTONS_ANGLEDB:
					drawWasdButtons(8, 28, 7, 3);
					break;
				case BUTTON_LAYOUT_VEWLIX:
					drawVewlix(8, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_VEWLIX7:
					drawVewlix7(8, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_CAPCOM:
					drawCapcom(6, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_CAPCOM6:
					drawCapcom6(16, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_SEGA2P:
					drawSega2p(8, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_NOIR8:
					drawNoir8(8, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_KEYBOARDB:
					drawMAMEB(68, 28, 10, 1);
					break;
				case BUTTON_LAYOUT_DANCEPADB:
					drawDancepadB(39, 12, 15, 2);
					break;
				case BUTTON_LAYOUT_TWINSTICKB:
					drawTwinStickB(100, 28, 8, 2);
					break;
				case BUTTON_LAYOUT_BLANKB:
					drawSticklessButtons(0, 0, 0, 0);
					break;
				case BUTTON_LAYOUT_VLXB:
					drawVLXB(6, 28, 7, 2);
					break;
				case BUTTON_LAYOUT_CUSTOMB:
					drawButtonLayoutRight(buttonLayoutCustomOptions.paramsRight);
					break;
				case BUTTON_LAYOUT_FIGHTBOARD:
					drawFightboard(8, 22, 7, 3);
					break;
				case BUTTON_LAYOUT_FIGHTBOARD_STICK_MIRRORED:
					drawArcadeStick(90, 22, 8, 2);
					break;
			}
			break;
	}

	rp2040_oled_flush(&oled);
}

I2CDisplayAddon::DisplayMode I2CDisplayAddon::getDisplayMode() {
	if (configMode) {
		gamepad->read();
		uint16_t buttonState = gamepad->state.buttons;
		if (prevButtonState && !buttonState) { // has button been pressed (held and released)?
			switch (prevButtonState) {
				case (GAMEPAD_MASK_B1):
					prevDisplayMode =
						prevDisplayMode == I2CDisplayAddon::DisplayMode::BUTTONS ?
							I2CDisplayAddon::DisplayMode::CONFIG_INSTRUCTION : I2CDisplayAddon::DisplayMode::BUTTONS;
						break;
				case (GAMEPAD_MASK_B2):
					prevDisplayMode =
						prevDisplayMode == I2CDisplayAddon::DisplayMode::SPLASH ?
							I2CDisplayAddon::DisplayMode::CONFIG_INSTRUCTION : I2CDisplayAddon::DisplayMode::SPLASH;
					break;
				default:
					prevDisplayMode = I2CDisplayAddon::DisplayMode::CONFIG_INSTRUCTION;
			}
		}
		prevButtonState = buttonState;
		return prevDisplayMode;
	} else {
		if (Storage::getInstance().getDisplayOptions().splashMode != static_cast<SplashMode>(SPLASH_MODE_NONE)) {
			uint32_t splashDuration = getDisplayOptions().splashDuration;
			if (splashDuration == 0 || getMillis() < splashDuration) {
				return I2CDisplayAddon::DisplayMode::SPLASH;
			}
		}
	}

	return I2CDisplayAddon::DisplayMode::BUTTONS;
}

const DisplayOptions& I2CDisplayAddon::getDisplayOptions() {
	bool configMode = Storage::getInstance().GetConfigMode();
	return configMode ? Storage::getInstance().getPreviewDisplayOptions() : Storage::getInstance().getDisplayOptions();
}

int I2CDisplayAddon::initDisplay(int typeOverride) {
	if (typeOverride > 0)
		oled.size = static_cast<rp2040_oled_size_t>(typeOverride);
	return rp2040_oled_init(&oled);
}

void I2CDisplayAddon::clearScreen(int render) {
	if (render)
		rp2040_oled_clear(&oled);
	else
		rp2040_oled_clear_gdram(&oled);
}

void I2CDisplayAddon::drawButtonLayoutLeft(ButtonLayoutParamsLeft& options)
{
	int32_t& startX    = options.common.startX;
	int32_t& startY    = options.common.startY;
	int32_t& buttonRadius  = options.common.buttonRadius;
	int32_t& buttonPadding = options.common.buttonPadding;

	switch (options.layout)
		{
			case BUTTON_LAYOUT_STICK:
				drawArcadeStick(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_STICKLESS:
				drawStickless(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_BUTTONS_ANGLED:
				drawWasdBox(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_BUTTONS_BASIC:
				drawUDLR(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_KEYBOARD_ANGLED:
				drawKeyboardAngled(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_KEYBOARDA:
				drawMAMEA(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_DANCEPADA:
				drawDancepadA(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_TWINSTICKA:
				drawTwinStickA(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_BLANKA:
				drawBlankA(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_VLXA:
				drawVLXA(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_FIGHTBOARD_STICK:
				drawArcadeStick(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_FIGHTBOARD_MIRRORED:
				drawFightboardMirrored(startX, startY, buttonRadius, buttonPadding);
				break;
		}
}

void I2CDisplayAddon::drawButtonLayoutRight(ButtonLayoutParamsRight& options)
{
	int32_t& startX        = options.common.startX;
	int32_t& startY        = options.common.startY;
	int32_t& buttonRadius  = options.common.buttonRadius;
	int32_t& buttonPadding = options.common.buttonPadding;

	switch (options.layout)
		{
			case BUTTON_LAYOUT_ARCADE:
				drawArcadeButtons(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_STICKLESSB:
				drawSticklessButtons(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_BUTTONS_ANGLEDB:
				drawWasdButtons(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_VEWLIX:
				drawVewlix(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_VEWLIX7:
				drawVewlix7(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_CAPCOM:
				drawCapcom(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_CAPCOM6:
				drawCapcom6(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_SEGA2P:
				drawSega2p(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_NOIR8:
				drawNoir8(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_KEYBOARDB:
				drawMAMEB(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_DANCEPADB:
				drawDancepadB(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_TWINSTICKB:
				drawTwinStickB(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_BLANKB:
				drawSticklessButtons(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_VLXB:
				drawVLXB(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_FIGHTBOARD:
				drawFightboard(startX, startY, buttonRadius, buttonPadding);
				break;
			case BUTTON_LAYOUT_FIGHTBOARD_STICK_MIRRORED:
				drawArcadeStick(startX, startY, buttonRadius, buttonPadding);
				break;
		}
}

void I2CDisplayAddon::drawDiamond(int cx, int cy, int size, rp2040_oled_color_t colour, uint8_t filled)
{
	if (filled) {
		int i;
		for (i = 0; i < size; i++) {
			rp2040_oled_draw_line(&oled, cx - i, cy - size + i, cx + i, cy - size + i, colour, 0);
			rp2040_oled_draw_line(&oled, cx - i, cy + size - i, cx + i, cy + size - i, colour, 0);
		}
		rp2040_oled_draw_line(&oled, cx - size, cy, cx + size, cy, colour, 0); // Fill in the middle
	}
	rp2040_oled_draw_line(&oled, cx - size, cy, cx, cy - size, colour, 0);
	rp2040_oled_draw_line(&oled, cx, cy - size, cx + size, cy, colour, 0);
	rp2040_oled_draw_line(&oled, cx + size, cy, cx, cy + size, colour, 0);
	rp2040_oled_draw_line(&oled, cx, cy + size, cx - size, cy, colour, 0);
}

void I2CDisplayAddon::drawStickless(int startX, int startY, int buttonRadius, int buttonPadding)
{

	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	rp2040_oled_draw_circle(&oled, startX, startY, buttonRadius,
				OLED_COLOR_WHITE, pressedLeft(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin, startY, buttonRadius,
				OLED_COLOR_WHITE, pressedDown(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 1.875), startY + (buttonMargin / 2), buttonRadius,
				OLED_COLOR_WHITE, pressedRight(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.25), startY + buttonMargin * 1.875, buttonRadius,
				OLED_COLOR_WHITE, pressedUp(), false);
}

void I2CDisplayAddon::drawWasdBox(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// WASD
	rp2040_oled_draw_circle(&oled, startX, startY + buttonMargin * 0.5, buttonRadius,
				OLED_COLOR_WHITE, pressedLeft(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin, startY + buttonMargin * 0.875, buttonRadius,
				OLED_COLOR_WHITE, pressedDown(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 1.5, startY - buttonMargin * 0.125, buttonRadius,
				OLED_COLOR_WHITE, pressedUp(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2), startY + buttonMargin * 1.25, buttonRadius,
				OLED_COLOR_WHITE, pressedRight(), false);
}

void I2CDisplayAddon::drawUDLR(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// UDLR
	rp2040_oled_draw_circle(&oled, startX, startY + buttonMargin / 2, buttonRadius,
				OLED_COLOR_WHITE, pressedLeft(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pressedUp(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY + buttonMargin * 1.25, buttonRadius,
				OLED_COLOR_WHITE, pressedDown(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 1.625), startY + buttonMargin / 2, buttonRadius,
				OLED_COLOR_WHITE, pressedRight(), false);
}

void I2CDisplayAddon::drawArcadeStick(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// Stick
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 2), startY + (buttonMargin / 2), buttonRadius * 1.25,
				OLED_COLOR_WHITE, false, false);

	if (pressedUp()) {
		if (pressedLeft()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 5), startY + (buttonMargin / 5), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else if (pressedRight()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY + (buttonMargin / 5), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 2), startY, buttonRadius,
						OLED_COLOR_WHITE, true, false);
		}
	} else if (pressedDown()) {
		if (pressedLeft()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 5), startY + (buttonMargin * 0.875), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else if (pressedRight()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY + (buttonMargin * 0.875), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else {
			rp2040_oled_draw_circle(&oled, startX + buttonMargin / 2, startY + buttonMargin, buttonRadius,
						OLED_COLOR_WHITE, true, false);
		}
	} else if (pressedLeft()) {
		rp2040_oled_draw_circle(&oled, startX, startY + buttonMargin / 2, buttonRadius,
					OLED_COLOR_WHITE, true, false);
	} else if (pressedRight()) {
		rp2040_oled_draw_circle(&oled, startX + buttonMargin, startY + buttonMargin / 2, buttonRadius,
					OLED_COLOR_WHITE, true, false);
	} else {
		rp2040_oled_draw_circle(&oled, startX + buttonMargin / 2, startY + buttonMargin / 2, buttonRadius,
					OLED_COLOR_WHITE, true, false);
	}
}

void I2CDisplayAddon::drawVLXA(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// Stick
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 2), startY + (buttonMargin / 2), buttonRadius * 1.25,
				OLED_COLOR_WHITE, false, false);

	if (pressedUp()) {
		if (pressedLeft()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 5), startY + (buttonMargin / 5), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else if (pressedRight()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY + (buttonMargin / 5), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 2), startY, buttonRadius,
						OLED_COLOR_WHITE, true, false);
		}
	} else if (pressedDown()) {
		if (pressedLeft()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 5), startY + (buttonMargin * 0.875), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else if (pressedRight()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY + (buttonMargin * 0.875), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else {
			rp2040_oled_draw_circle(&oled, startX + buttonMargin / 2, startY + buttonMargin, buttonRadius,
						OLED_COLOR_WHITE, true, false);
		}
	} else if (pressedLeft()) {
		rp2040_oled_draw_circle(&oled, startX, startY + buttonMargin / 2, buttonRadius,
					OLED_COLOR_WHITE, true, false);
	} else if (pressedRight()) {
		rp2040_oled_draw_circle(&oled, startX + buttonMargin, startY + buttonMargin / 2, buttonRadius,
					OLED_COLOR_WHITE, true, false);
	} else {
		rp2040_oled_draw_circle(&oled, startX + buttonMargin / 2, startY + buttonMargin / 2, buttonRadius,
					OLED_COLOR_WHITE, true, false);
	}
}

void I2CDisplayAddon::drawFightboardMirrored(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);
    const int leftMargin = startX + buttonPadding + buttonRadius;

	rp2040_oled_draw_circle(&oled, leftMargin, startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);
	rp2040_oled_draw_circle(&oled, leftMargin + buttonMargin, startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, leftMargin + (buttonMargin*2), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, leftMargin + (buttonMargin*3), startY * 1.25, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);

	rp2040_oled_draw_circle(&oled, leftMargin, startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);
	rp2040_oled_draw_circle(&oled, leftMargin + buttonMargin, startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, leftMargin + (buttonMargin*2), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, leftMargin + (buttonMargin*3), startY + buttonMargin * 1.25, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);

    // Extra buttons
    rp2040_oled_draw_circle(&oled, startX + buttonMargin * 0.5, startY + (buttonMargin * 1.5), 3,
			    OLED_COLOR_WHITE, pGamepad->pressedL3(), false);
    rp2040_oled_draw_circle(&oled, startX + buttonMargin * 1.0625, startY + (buttonMargin * 1.5), 3,
			    OLED_COLOR_WHITE, pGamepad->pressedS1(), false);
    rp2040_oled_draw_circle(&oled, startX + buttonMargin * 1.625, startY + (buttonMargin * 1.5), 3,
			    OLED_COLOR_WHITE, pGamepad->pressedA1(), false);
    rp2040_oled_draw_circle(&oled, startX + buttonMargin * 2.125+0.0625, startY + (buttonMargin * 1.5), 3,
			    OLED_COLOR_WHITE, pGamepad->pressedS2(), false);
    rp2040_oled_draw_circle(&oled, startX + buttonMargin * 2.75, startY + (buttonMargin * 1.5), 3,
			    OLED_COLOR_WHITE, pGamepad->pressedR3(), false);
}

void I2CDisplayAddon::drawTwinStickA(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// Stick
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 2), startY + (buttonMargin / 2), buttonRadius * 1.25,
				OLED_COLOR_WHITE, false, false);

	if (pressedUp()) {
		if (pressedLeft()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 5), startY + (buttonMargin / 5), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else if (pressedRight()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY + (buttonMargin / 5), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 2), startY, buttonRadius,
						OLED_COLOR_WHITE, true, false);
		}
	} else if (pressedDown()) {
		if (pressedLeft()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 5), startY + (buttonMargin * 0.875), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else if (pressedRight()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY + (buttonMargin * 0.875), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else {
			rp2040_oled_draw_circle(&oled, startX + buttonMargin / 2, startY + buttonMargin, buttonRadius,
						OLED_COLOR_WHITE, true, false);
		}
	} else if (pressedLeft()) {
		rp2040_oled_draw_circle(&oled, startX, startY + buttonMargin / 2, buttonRadius,
						OLED_COLOR_WHITE, true, false);
	} else if (pressedRight()) {
		rp2040_oled_draw_circle(&oled, startX + buttonMargin, startY + buttonMargin / 2, buttonRadius,
						OLED_COLOR_WHITE, true, false);
	} else {
		rp2040_oled_draw_circle(&oled, startX + buttonMargin / 2, startY + buttonMargin / 2, buttonRadius,
						OLED_COLOR_WHITE, true, false);
	}
}

void I2CDisplayAddon::drawTwinStickB(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// Stick
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 2), startY + (buttonMargin / 2), buttonRadius * 1.25,
				OLED_COLOR_WHITE, false, false);

	if (pGamepad->pressedB4()) {
		if (pGamepad->pressedB3()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 5), startY + (buttonMargin / 5), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else if (pGamepad->pressedB2()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY + (buttonMargin / 5), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 2), startY, buttonRadius,
						OLED_COLOR_WHITE, true, false);
		}
	} else if (pGamepad->pressedB1()) {
		if (pGamepad->pressedB3()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin / 5), startY + (buttonMargin * 0.875), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else if (pGamepad->pressedB2()) {
			rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 0.875), startY + (buttonMargin * 0.875), buttonRadius,
						OLED_COLOR_WHITE, true, false);
		} else {
			rp2040_oled_draw_circle(&oled, startX + buttonMargin / 2, startY + buttonMargin, buttonRadius,
						OLED_COLOR_WHITE, true, false);
		}
	} else if (pGamepad->pressedB3()) {
		rp2040_oled_draw_circle(&oled, startX, startY + buttonMargin / 2, buttonRadius,
					OLED_COLOR_WHITE, true, false);
	} else if (pGamepad->pressedB2()) {
		rp2040_oled_draw_circle(&oled, startX + buttonMargin, startY + buttonMargin / 2, buttonRadius,
					OLED_COLOR_WHITE, true, false);
	} else {
		rp2040_oled_draw_circle(&oled, startX + buttonMargin / 2, startY + buttonMargin / 2, buttonRadius,
					OLED_COLOR_WHITE, true, false);
	}
}

void I2CDisplayAddon::drawMAMEA(int startX, int startY, int buttonSize, int buttonPadding)
{
	const int buttonMargin = buttonPadding + buttonSize;

	// MAME
	rp2040_oled_draw_rectangle(&oled, startX, startY + buttonMargin, startX + buttonSize, startY + buttonSize + buttonMargin,
				   OLED_COLOR_WHITE, pressedLeft(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin, startY + buttonMargin, startX + buttonSize + buttonMargin, startY + buttonSize + buttonMargin,
				   OLED_COLOR_WHITE, pressedDown(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin, startY, startX + buttonSize + buttonMargin, startY + buttonSize,
				   OLED_COLOR_WHITE, pressedUp(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin * 2, startY + buttonMargin, startX + buttonSize + buttonMargin * 2, startY + buttonSize + buttonMargin,
				   OLED_COLOR_WHITE, pressedRight(), false);
}

void I2CDisplayAddon::drawMAMEB(int startX, int startY, int buttonSize, int buttonPadding)
{
	const int buttonMargin = buttonPadding + buttonSize;

	// 6-button MAME Style
	rp2040_oled_draw_rectangle(&oled, startX, startY, startX + buttonSize, startY + buttonSize,
				   OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin, startY, startX + buttonSize + buttonMargin, startY + buttonSize,
				   OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin * 2, startY, startX + buttonSize + buttonMargin * 2, startY + buttonSize,
				   OLED_COLOR_WHITE, pGamepad->pressedR1(), false);

	rp2040_oled_draw_rectangle(&oled, startX, startY + buttonMargin, startX + buttonSize, startY + buttonMargin + buttonSize,
				   OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin, startY + buttonMargin, startX + buttonSize + buttonMargin, startY + buttonMargin + buttonSize,
				   OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin * 2, startY + buttonMargin, startX + buttonSize + buttonMargin * 2, startY + buttonMargin + buttonSize,
				   OLED_COLOR_WHITE, pGamepad->pressedR2(), false);

}

void I2CDisplayAddon::drawKeyboardAngled(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// MixBox
	drawDiamond(startX, startY, buttonRadius, OLED_COLOR_WHITE, pressedLeft());
	drawDiamond(startX + buttonMargin / 2, startY + buttonMargin / 2, buttonRadius, OLED_COLOR_WHITE, pressedDown());
	drawDiamond(startX + buttonMargin, startY, buttonRadius, OLED_COLOR_WHITE, pressedUp());
	drawDiamond(startX + buttonMargin, startY + buttonMargin, buttonRadius, OLED_COLOR_WHITE, pressedRight());
}

void I2CDisplayAddon::drawVewlix(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Vewlix
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75), startY + (buttonMargin * 0.2), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75) - (buttonMargin / 3), startY + buttonMargin + (buttonMargin * 0.2), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);
}

void I2CDisplayAddon::drawVLXB(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 9-button Hori VLX
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75), startY + (buttonMargin * 0.2), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75) - (buttonMargin / 3), startY + buttonMargin + (buttonMargin * 0.2), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);

	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 7.4) - (buttonMargin / 3.5), startY + buttonMargin - (buttonMargin / 1.5), buttonRadius *.8,
				OLED_COLOR_WHITE, pGamepad->pressedS2(), false);
}

void I2CDisplayAddon::drawFightboard(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	rp2040_oled_draw_circle(&oled, (startX + buttonMargin * 3.625), startY * 1.25, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, (startX + buttonMargin * 4.625), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, (startX + buttonMargin * 5.625), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, (startX + buttonMargin * 6.625), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, (startX + buttonMargin * 3.625), startY + buttonMargin * 1.25, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, (startX + buttonMargin * 4.625), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, (startX + buttonMargin * 5.625), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, (startX + buttonMargin * 6.625), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);

    // Extra buttons
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 4.5, startY + (buttonMargin * 1.5), 3,
				OLED_COLOR_WHITE, pGamepad->pressedL3(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.0625, startY + (buttonMargin * 1.5), 3,
				OLED_COLOR_WHITE, pGamepad->pressedS1(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.625, startY + (buttonMargin * 1.5), 3,
				OLED_COLOR_WHITE, pGamepad->pressedA1(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 6.125+0.0625, startY + (buttonMargin * 1.5), 3,
				OLED_COLOR_WHITE, pGamepad->pressedS2(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 6.75, startY + (buttonMargin * 1.5), 3,
				OLED_COLOR_WHITE, pGamepad->pressedR3(), false);
}

void I2CDisplayAddon::drawVewlix7(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Vewlix
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75), startY + (buttonMargin * 0.2), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75) - (buttonMargin / 3), startY + buttonMargin + (buttonMargin * 0.2), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
}

void I2CDisplayAddon::drawSega2p(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Sega2P
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75), startY + (buttonMargin / 3), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75), startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75), startY + buttonMargin + (buttonMargin / 3), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75), startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);
}

void I2CDisplayAddon::drawNoir8(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Noir8
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75), startY + (buttonMargin / 3.5), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75), startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75), startY + buttonMargin + (buttonMargin / 3.5), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75), startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);
}

void I2CDisplayAddon::drawCapcom(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Capcom
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 3.25, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 4.25, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.25, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 6.25, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 3.25, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 4.25, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.25, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 6.25, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);
}

void I2CDisplayAddon::drawCapcom6(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 6-button Capcom
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 3.25, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 4.25, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.25, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);

	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 3.25, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 4.25, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.25, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
}

void I2CDisplayAddon::drawSticklessButtons(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75), startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75), startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 2.75), startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 3.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 4.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, startX + (buttonMargin * 5.75), startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);
}

void I2CDisplayAddon::drawWasdButtons(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 3.625, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 4.625, startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.625, startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 6.625, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 3.25, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 4.25, startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.25, startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 6.25, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);
}

void I2CDisplayAddon::drawArcadeButtons(int startX, int startY, int buttonRadius, int buttonPadding)
{
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 3.125, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB3(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 4.125, startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB4(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.125, startY - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR1(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 6.125, startY, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL1(), false);

	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 2.875, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB1(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 3.875, startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedB2(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 4.875, startY + buttonMargin - (buttonMargin / 4), buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedR2(), false);
	rp2040_oled_draw_circle(&oled, startX + buttonMargin * 5.875, startY + buttonMargin, buttonRadius,
				OLED_COLOR_WHITE, pGamepad->pressedL2(), false);
}

// I pulled this out of my PR, brought it back because of recent talks re: SOCD and rhythm games
// Enjoy!

void I2CDisplayAddon::drawDancepadA(int startX, int startY, int buttonSize, int buttonPadding)
{
	const int buttonMargin = buttonPadding + buttonSize;

	rp2040_oled_draw_rectangle(&oled, startX, startY + buttonMargin, startX + buttonSize, startY + buttonSize + buttonMargin,
				   OLED_COLOR_WHITE, pressedLeft(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin, startY + buttonMargin * 2, startX + buttonSize + buttonMargin, startY + buttonSize + buttonMargin * 2,
				   OLED_COLOR_WHITE, pressedDown(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin, startY, startX + buttonSize + buttonMargin, startY + buttonSize,
				   OLED_COLOR_WHITE, pressedUp(), false);
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin * 2, startY + buttonMargin, startX + buttonSize + buttonMargin * 2, startY + buttonSize + buttonMargin,
				   OLED_COLOR_WHITE, pressedRight(), false);
}

void I2CDisplayAddon::drawDancepadB(int startX, int startY, int buttonSize, int buttonPadding)
{
	const int buttonMargin = buttonPadding + buttonSize;

	rp2040_oled_draw_rectangle(&oled, startX, startY, startX + buttonSize, startY + buttonSize,
				   OLED_COLOR_WHITE, pGamepad->pressedB2(), false); // Up/Left
	rp2040_oled_draw_rectangle(&oled, startX, startY + buttonMargin * 2, startX + buttonSize, startY + buttonSize + buttonMargin * 2,
				   OLED_COLOR_WHITE, pGamepad->pressedB4(), false); // Down/Left
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin * 2, startY, startX + buttonSize + buttonMargin * 2, startY + buttonSize,
				   OLED_COLOR_WHITE, pGamepad->pressedB1(), false); // Up/Right
	rp2040_oled_draw_rectangle(&oled, startX + buttonMargin * 2, startY + buttonMargin * 2, startX + buttonSize + buttonMargin * 2, startY + buttonSize + buttonMargin * 2,
				   OLED_COLOR_WHITE, pGamepad->pressedB3(), false); // Down/Right
}

void I2CDisplayAddon::drawBlankA(int startX, int startY, int buttonSize, int buttonPadding)
{
}

void I2CDisplayAddon::drawBlankB(int startX, int startY, int buttonSize, int buttonPadding)
{
}

void I2CDisplayAddon::drawSplashScreen(int splashMode, uint8_t * splashChoice, int splashSpeed)
{
    int mils = getMillis();
    switch (splashMode)
	{
		case SPLASH_MODE_STATIC: // Default, display static or custom image
			rp2040_oled_draw_sprite_pitched(&oled, splashChoice, 0, 0, 128, 64, 16, OLED_COLOR_WHITE, false);
			break;
		case SPLASH_MODE_CLOSEIN: // Close-in. Animate the GP2040 logo
			rp2040_oled_draw_sprite_pitched(&oled, (uint8_t *)bootLogoTop, 43, std::min<int>((mils / splashSpeed) - 39, 0), 43, 39, 6, OLED_COLOR_WHITE, false);
			rp2040_oled_draw_sprite_pitched(&oled, (uint8_t *)bootLogoBottom, 24, std::max<int>(64 - (mils / (splashSpeed * 2)), 44), 80, 21, 10, OLED_COLOR_WHITE, false);
			break;
        case SPLASH_MODE_CLOSEINCUSTOM: // Close-in on custom image or delayed close-in if custom image does not exist
            rp2040_oled_draw_sprite_pitched(&oled, splashChoice, 0, 0, 128, 64, 16, OLED_COLOR_WHITE, false);
            if (mils > 2500) {
                int milss = mils - 2500;
                rp2040_oled_draw_rectangle(&oled, 0, 0, 127, 1 + (milss / splashSpeed), OLED_COLOR_BLACK, true, false);
                rp2040_oled_draw_rectangle(&oled, 0, 63, 127, 62 - (milss / (splashSpeed * 2)), OLED_COLOR_BLACK, true, false);
                rp2040_oled_draw_sprite_pitched(&oled, (uint8_t *)bootLogoTop, 43, std::min<int>((milss / splashSpeed) - 39, 0), 43, 39, 6, OLED_COLOR_WHITE, false);
                rp2040_oled_draw_sprite_pitched(&oled, (uint8_t *)bootLogoBottom, 24, std::max<int>(64 - (milss / (splashSpeed * 2)), 44), 80, 21, 10, OLED_COLOR_WHITE, false);
            }
            break;
	}
}

void I2CDisplayAddon::drawText(int x, int y, std::string text) {
	rp2040_oled_write_string(&oled, x, y, (char*)text.c_str(), text.length(), false);
}

void I2CDisplayAddon::drawStatusBar(Gamepad * gamepad)
{
	const TurboOptions& turboOptions = Storage::getInstance().getAddonOptions().turboOptions;

	// Limit to 21 chars with 6x8 font for now
	statusBar.clear();

	switch (gamepad->getOptions().inputMode)
	{
		case INPUT_MODE_HID:    statusBar += "DINPUT"; break;
		case INPUT_MODE_SWITCH: statusBar += "SWITCH"; break;
		case INPUT_MODE_XINPUT: statusBar += "XINPUT"; break;
		case INPUT_MODE_PS4:
			if ( PS4Data::getInstance().ps4ControllerType == PS4ControllerType::PS4_CONTROLLER ) {
				if (PS4Data::getInstance().authsent == true )
					statusBar += "PS4:AS";
				else
					statusBar += "PS4   ";
			} else if ( PS4Data::getInstance().ps4ControllerType == PS4ControllerType::PS4_ARCADESTICK ) {
				if (PS4Data::getInstance().authsent == true )
					statusBar += "PS5:AS";
				else
					statusBar += "PS5   ";
			}
			break;
		case INPUT_MODE_KEYBOARD: statusBar += "HID-KB"; break;
		case INPUT_MODE_CONFIG: statusBar += "CONFIG"; break;
	}

	if ( turboOptions.enabled && isValidPin(turboOptions.buttonPin) ) {
		statusBar += " T";
		if ( turboOptions.shotCount < 10 ) // padding
			statusBar += "0";
		statusBar += std::to_string(turboOptions.shotCount);
	} else {
		statusBar += "    "; // no turbo, don't show Txx setting
	}
	switch (gamepad->getOptions().dpadMode)
	{

		case DPAD_MODE_DIGITAL:      statusBar += " DP"; break;
		case DPAD_MODE_LEFT_ANALOG:  statusBar += " LS"; break;
		case DPAD_MODE_RIGHT_ANALOG: statusBar += " RS"; break;
	}

	switch (Gamepad::resolveSOCDMode(gamepad->getOptions()))
	{
		case SOCD_MODE_NEUTRAL:               statusBar += " SOCD-N"; break;
		case SOCD_MODE_UP_PRIORITY:           statusBar += " SOCD-U"; break;
		case SOCD_MODE_SECOND_INPUT_PRIORITY: statusBar += " SOCD-L"; break;
		case SOCD_MODE_FIRST_INPUT_PRIORITY:  statusBar += " SOCD-F"; break;
		case SOCD_MODE_BYPASS:                statusBar += " SOCD-X"; break;
	}
	drawText(0, 0, statusBar);
}

bool I2CDisplayAddon::pressedUp()
{
	switch (gamepad->getOptions().dpadMode)
	{
		case DPAD_MODE_DIGITAL:      return pGamepad->pressedUp();
		case DPAD_MODE_LEFT_ANALOG:  return pGamepad->state.ly == GAMEPAD_JOYSTICK_MIN;
		case DPAD_MODE_RIGHT_ANALOG: return pGamepad->state.ry == GAMEPAD_JOYSTICK_MIN;
	}

	return false;
}

bool I2CDisplayAddon::pressedDown()
{
	switch (gamepad->getOptions().dpadMode)
	{
		case DPAD_MODE_DIGITAL:      return pGamepad->pressedDown();
		case DPAD_MODE_LEFT_ANALOG:  return pGamepad->state.ly == GAMEPAD_JOYSTICK_MAX;
		case DPAD_MODE_RIGHT_ANALOG: return pGamepad->state.ry == GAMEPAD_JOYSTICK_MAX;
	}

	return false;
}

bool I2CDisplayAddon::pressedLeft()
{
	switch (gamepad->getOptions().dpadMode)
	{
		case DPAD_MODE_DIGITAL:      return pGamepad->pressedLeft();
		case DPAD_MODE_LEFT_ANALOG:  return pGamepad->state.lx == GAMEPAD_JOYSTICK_MIN;
		case DPAD_MODE_RIGHT_ANALOG: return pGamepad->state.rx == GAMEPAD_JOYSTICK_MIN;
	}

	return false;
}

bool I2CDisplayAddon::pressedRight()
{
	switch (gamepad->getOptions().dpadMode)
	{
		case DPAD_MODE_DIGITAL:      return pGamepad->pressedRight();
		case DPAD_MODE_LEFT_ANALOG:  return pGamepad->state.lx == GAMEPAD_JOYSTICK_MAX;
		case DPAD_MODE_RIGHT_ANALOG: return pGamepad->state.rx == GAMEPAD_JOYSTICK_MAX;
	}

	return false;
}
