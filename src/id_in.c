//
//	ID Engine
//	ID_IN.c - Input Manager
//	v1.0d1
//	By Jason Blochowiak
//

//
//	This module handles dealing with the various input devices
//
//	Depends on: Memory Mgr (for demo recording), Sound Mgr (for timing stuff),
//				User Mgr (for command line parms)
//
//	Globals:
//		LastScan - The keyboard scan code of the last key pressed
//		LastASCII - The ASCII value of the last key pressed
//	DEBUG - there are more globals
//

#include "wl_def.h"

#define UNKNOWN_KEY KEYCOUNT

boolean MousePresent;
boolean forcegrabmouse = false;

volatile boolean KeyboardState[KEYCOUNT];
volatile boolean Paused;
volatile char LastASCII;
volatile ScanCode LastScan;

int JoyNumButtons;

// KeyboardDef	KbdDefs = {0x1d,0x38,0x47,0x48,0x49,0x4b,0x4d,0x4f,0x50,0x51};
static KeyboardDef KbdDefs = {
	sc_Control, // button0
	sc_Alt, // button1
	sc_Home, // upleft
	sc_UpArrow, // up
	sc_PgUp, // upright
	sc_LeftArrow, // left
	sc_RightArrow, // right
	sc_End, // downleft
	sc_DownArrow, // down
	sc_PgDn // downright
};

byte ASCIINames[] = // Unshifted ASCII for scan codes       // TODO: keypad
		{
			//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
			0,	 0,	  0,   0,	0,	 0,	  0,   0,
			8,	 9,	  0,   0,	0,	 13,  0,   0, // 0
			0,	 0,	  0,   0,	0,	 0,	  0,   0,
			0,	 0,	  0,   0,	27,	 0,	  0,   0, // 1
			' ', 0,	  0,   0,	0,	 0,	  0,   39,
			0,	 0,	  '*', '+', ',', '-', '.', '/', // 2
			'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 0,   ';', 0,	 '=', 0,   0, // 3
			'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
			'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', // 4
			'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
			'x', 'y', 'z', '[', 92,	 ']', 0,   0, // 5
			0,	 0,	  0,   0,	0,	 0,	  0,   0,
			0,	 0,	  0,   0,	0,	 0,	  0,   0, // 6
			0,	 0,	  0,   0,	0,	 0,	  0,   0,
			0,	 0,	  0,   0,	0,	 0,	  0,   0 // 7
		};
byte ShiftNames[] = // Shifted ASCII for scan codes
		{
			//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
			0,	 0,	  0,   0,	0,	 0,	  0,   0,
			8,	 9,	  0,   0,	0,	 13,  0,   0, // 0
			0,	 0,	  0,   0,	0,	 0,	  0,   0,
			0,	 0,	  0,   0,	27,	 0,	  0,   0, // 1
			' ', 0,	  0,   0,	0,	 0,	  0,   34,
			0,	 0,	  '*', '+', '<', '_', '>', '?', // 2
			')', '!', '@', '#', '$', '%', '^', '&',
			'*', '(', 0,   ':', 0,	 '+', 0,   0, // 3
			'~', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
			'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', // 4
			'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
			'X', 'Y', 'Z', '{', '|', '}', 0,   0, // 5
			0,	 0,	  0,   0,	0,	 0,	  0,   0,
			0,	 0,	  0,   0,	0,	 0,	  0,   0, // 6
			0,	 0,	  0,   0,	0,	 0,	  0,   0,
			0,	 0,	  0,   0,	0,	 0,	  0,   0 // 7
		};
byte SpecialNames[] = // ASCII for 0xe0 prefixed codes
		{
			//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
			0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0,  0, 0, 0, // 0
			0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 13, 0, 0, 0, // 1
			0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0,  0, 0, 0, // 2
			0, 0, 0, 0, 0, '/', 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, // 3
			0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0,  0, 0, 0, // 4
			0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0,  0, 0, 0, // 5
			0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0,  0, 0, 0, // 6
			0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0,  0, 0, 0 // 7
		};
static Direction DirTable[] = // Quick lookup for total direction
		{ dir_NorthWest, dir_North,		dir_NorthEast, dir_West,	 dir_None,
		  dir_East,		 dir_SouthWest, dir_South,	   dir_SouthEast };

static boolean IN_Started;

/*
============================================================================

                            Keyboard Functions

============================================================================
*/

static int KeyboardRead(int key);
static void KeyboardSet(int key, boolean state);

void IN_ClearKeysDown(void)
{
	LastScan = sc_None;
	LastASCII = key_None;
	memset((void *)KeyboardState, 0, sizeof(KeyboardState));
}

boolean IN_KeyDown(int key)
{
	int keyIndex = KeyboardRead(key);
	return keyIndex != UNKNOWN_KEY ? KeyboardState[keyIndex] : false;
}

void IN_ClearKey(int key)
{
	KeyboardSet(key, false);
	if (key == LastScan) {
		LastScan = sc_None;
	}
}

static int KeyboardRead(int key)
{
	switch (key) {
	case SDLK_UNKNOWN:
		return 0;
	case SDLK_BACKSPACE:
		return 1;
	case SDLK_TAB:
		return 2;
	case SDLK_CLEAR:
		return 3;
	case SDLK_RETURN:
		return 4;
	case SDLK_PAUSE:
		return 5;
	case SDLK_ESCAPE:
		return 6;
	case SDLK_SPACE:
		return 7;
	case SDLK_EXCLAIM:
		return 8;
	case SDLK_QUOTEDBL:
		return 9;
	case SDLK_HASH:
		return 10;
	case SDLK_DOLLAR:
		return 11;
	case SDLK_AMPERSAND:
		return 12;
	case SDLK_QUOTE:
		return 13;
	case SDLK_LEFTPAREN:
		return 14;
	case SDLK_RIGHTPAREN:
		return 15;
	case SDLK_ASTERISK:
		return 16;
	case SDLK_PLUS:
		return 17;
	case SDLK_COMMA:
		return 18;
	case SDLK_MINUS:
		return 19;
	case SDLK_PERIOD:
		return 20;
	case SDLK_SLASH:
		return 21;
	case SDLK_0:
		return 22;
	case SDLK_1:
		return 23;
	case SDLK_2:
		return 24;
	case SDLK_3:
		return 25;
	case SDLK_4:
		return 26;
	case SDLK_5:
		return 27;
	case SDLK_6:
		return 28;
	case SDLK_7:
		return 29;
	case SDLK_8:
		return 30;
	case SDLK_9:
		return 31;
	case SDLK_COLON:
		return 32;
	case SDLK_SEMICOLON:
		return 33;
	case SDLK_LESS:
		return 34;
	case SDLK_EQUALS:
		return 35;
	case SDLK_GREATER:
		return 36;
	case SDLK_QUESTION:
		return 37;
	case SDLK_AT:
		return 38;
	case SDLK_LEFTBRACKET:
		return 39;
	case SDLK_BACKSLASH:
		return 40;
	case SDLK_RIGHTBRACKET:
		return 41;
	case SDLK_CARET:
		return 42;
	case SDLK_UNDERSCORE:
		return 43;
	case SDLK_BACKQUOTE:
		return 44;
	case SDLK_a:
		return 45;
	case SDLK_b:
		return 46;
	case SDLK_c:
		return 47;
	case SDLK_d:
		return 48;
	case SDLK_e:
		return 49;
	case SDLK_f:
		return 50;
	case SDLK_g:
		return 51;
	case SDLK_h:
		return 52;
	case SDLK_i:
		return 53;
	case SDLK_j:
		return 54;
	case SDLK_k:
		return 55;
	case SDLK_l:
		return 56;
	case SDLK_m:
		return 57;
	case SDLK_n:
		return 58;
	case SDLK_o:
		return 59;
	case SDLK_p:
		return 60;
	case SDLK_q:
		return 61;
	case SDLK_r:
		return 62;
	case SDLK_s:
		return 63;
	case SDLK_t:
		return 64;
	case SDLK_u:
		return 65;
	case SDLK_v:
		return 66;
	case SDLK_w:
		return 67;
	case SDLK_x:
		return 68;
	case SDLK_y:
		return 69;
	case SDLK_z:
		return 70;
	case SDLK_DELETE:
		return 71;
	case SDLK_KP_PERIOD:
		return 72;
	case SDLK_KP_DIVIDE:
		return 73;
	case SDLK_KP_MULTIPLY:
		return 74;
	case SDLK_KP_MINUS:
		return 75;
	case SDLK_KP_PLUS:
		return 76;
	case SDLK_KP_ENTER:
		return 77;
	case SDLK_KP_EQUALS:
		return 78;
	case SDLK_UP:
		return 79;
	case SDLK_DOWN:
		return 80;
	case SDLK_RIGHT:
		return 81;
	case SDLK_LEFT:
		return 82;
	case SDLK_INSERT:
		return 83;
	case SDLK_HOME:
		return 84;
	case SDLK_END:
		return 85;
	case SDLK_PAGEUP:
		return 86;
	case SDLK_PAGEDOWN:
		return 87;
	case SDLK_F1:
		return 88;
	case SDLK_F2:
		return 89;
	case SDLK_F3:
		return 90;
	case SDLK_F4:
		return 91;
	case SDLK_F5:
		return 92;
	case SDLK_F6:
		return 93;
	case SDLK_F7:
		return 94;
	case SDLK_F8:
		return 95;
	case SDLK_F9:
		return 96;
	case SDLK_F10:
		return 97;
	case SDLK_F11:
		return 98;
	case SDLK_F12:
		return 99;
	case SDLK_F13:
		return 100;
	case SDLK_F14:
		return 101;
	case SDLK_F15:
		return 102;
	case SDLK_CAPSLOCK:
		return 103;
	case SDLK_RSHIFT:
		return 104;
	case SDLK_LSHIFT:
		return 105;
	case SDLK_RCTRL:
		return 106;
	case SDLK_LCTRL:
		return 107;
	case SDLK_RALT:
		return 108;
	case SDLK_LALT:
		return 109;
	case SDLK_MODE:
		return 110;
	case SDLK_HELP:
		return 111;
	case SDLK_SYSREQ:
		return 112;
	case SDLK_MENU:
		return 113;
	case SDLK_POWER:
		return 114;
	case SDLK_UNDO:
		return 115;
	case SDLK_KP_0:
		return 116;
	case SDLK_KP_1:
		return 117;
	case SDLK_KP_2:
		return 118;
	case SDLK_KP_3:
		return 119;
	case SDLK_KP_4:
		return 120;
	case SDLK_KP_5:
		return 121;
	case SDLK_KP_6:
		return 122;
	case SDLK_KP_7:
		return 123;
	case SDLK_KP_8:
		return 124;
	case SDLK_KP_9:
		return 125;
	case SDLK_PRINTSCREEN:
		return 126;
	case SDLK_NUMLOCKCLEAR:
		return 127;
	case SDLK_SCROLLLOCK:
		return 128;
	default:
		return UNKNOWN_KEY;
	}
}

static void KeyboardSet(int key, boolean state)
{
	int keyIndex = KeyboardRead(key);
	if (keyIndex != UNKNOWN_KEY) {
		KeyboardState[keyIndex] = state;
	}
}

static boolean CheckFullscreenToggle(SDL_Keysym sym)
{
	uint16_t flags = (KMOD_LALT | KMOD_RALT);
#if defined(__MACOSX__)
	flags |= (KMOD_LGUI | KMOD_RGUI);
#endif
	return (sym.scancode == SDL_SCANCODE_RETURN ||
			sym.scancode == SDL_SCANCODE_KP_ENTER) &&
		   (sym.mod & flags) != 0;
}

static boolean CheckScreenshot(SDL_Keysym sym)
{
	return (sym.sym == SDLK_PRINTSCREEN);
}

/*
============================================================================

                            Mouse Functions

============================================================================
*/

static int MouseButtonState = 0;
static boolean IsMouseGrabbed = false;

static boolean ShouldGrabMouse();
static void SetShowCursor(boolean show);

int IN_MouseButtons(void)
{
	int buttons = MouseButtonState;

	// Clear out the mouse scroll wheel buttons once reported.
	MouseButtonState &= ~((1 << 3) | (1 << 4));

	return buttons;
}

void IN_UpdateMouseGrab()
{
	boolean grab = ShouldGrabMouse();

	if (grab && !IsMouseGrabbed) {
		SetShowCursor(false);
	}

	if (!grab && IsMouseGrabbed) {
		int winW, winH;

		SetShowCursor(true);

		// When releasing the mouse from grab, warp the mouse cursor to
		// the bottom-right of the screen. This is a minimally distracting
		// place for it to appear - we may only have released the grab
		// because we're at an end of level intermission screen, for
		// example.
		VL_GetWindowSize(&winW, &winH);
		VL_WarpMouseInWindow(winW - 16, winH - 16);
		SDL_GetRelativeMouseState(NULL, NULL);
	}

	IsMouseGrabbed = grab;
}

boolean IN_IsInputGrabbed()
{
	return IsMouseGrabbed;
}

static boolean ShouldGrabMouse()
{
	if (!VL_GetWindowFocus()) {
		return false;
	}

	if (VL_GetFullscreen()) {
		return true;
	}

	if (!forcegrabmouse) {
		return false;
	}

	if (Paused || inmenu) {
		return false;
	}

	return ingame && !demoplayback;
}

static void SetShowCursor(boolean show)
{
	// When the cursor is hidden, grab the input.
	// Relative mode implicitly hides the cursor.
	SDL_SetRelativeMouseMode(show ? SDL_FALSE : SDL_TRUE);
	SDL_GetRelativeMouseState(NULL, NULL);
}

static void UpdateMouseButtonState(unsigned int button, boolean on)
{
	// Note: button "0" is left, button "1" is right,
	// button "2" is middle for the game.  This is different
	// to how SDL sees things.
	switch (button) {
	case SDL_BUTTON_LEFT:
		button = 0;
		break;

	case SDL_BUTTON_RIGHT:
		button = 1;
		break;

	case SDL_BUTTON_MIDDLE:
		button = 2;
		break;

	default:
		return;
	}

	// Turn bit representing this button on or off.
	if (on) {
		MouseButtonState |= (1 << button);
	} else {
		MouseButtonState &= ~(1 << button);
	}
}

static void MapMouseWheelToButtons(SDL_MouseWheelEvent *wheel)
{
	// SDL2 distinguishes button events from mouse wheel events.
	// We want to treat the mouse wheel as two buttons, as per SDL1.
	int button;

	boolean flipped = wheel->direction == SDL_MOUSEWHEEL_FLIPPED;
	if (wheel->y <= 0) { // scroll down
		button = flipped ? 3 : 4;
	} else { // scroll up
		button = flipped ? 4 : 3;
	}

	MouseButtonState |= (1 << button);
}

/*
============================================================================

                            Joystick Functions

============================================================================
*/

static SDL_Joystick *Joystick;
static int JoyNumHats;

boolean IN_JoyPresent()
{
	return Joystick != NULL;
}

int IN_JoyButtons(void)
{
	if (!Joystick)
		return 0;

	SDL_JoystickUpdate();

	int res = 0;
	for (int i = 0; i < JoyNumButtons && i < 32; i++) {
		res |= SDL_JoystickGetButton(Joystick, i) << i;
	}

	return res;
}

// IN_GetJoyDelta() - Returns the relative movement of the specified
// joystick (from -/+127).
void IN_GetJoyDelta(int *dx, int *dy)
{
	if (!Joystick) {
		*dx = *dy = 0;
		return;
	}

	SDL_JoystickUpdate();
	int x = SDL_JoystickGetAxis(Joystick, 0) >> 8;
	int y = SDL_JoystickGetAxis(Joystick, 1) >> 8;

	if (param_joystickhat != -1) {
		uint8_t hatState = SDL_JoystickGetHat(Joystick, param_joystickhat);
		if (hatState & SDL_HAT_RIGHT)
			x += 127;
		else if (hatState & SDL_HAT_LEFT)
			x -= 127;
		if (hatState & SDL_HAT_DOWN)
			y += 127;
		else if (hatState & SDL_HAT_UP)
			y -= 127;

		if (x < -128)
			x = -128;
		else if (x > 127)
			x = 127;

		if (y < -128)
			y = -128;
		else if (y > 127)
			y = 127;
	}

	*dx = x;
	*dy = y;
}

/*
============================================================================

                        Game Controller Functions

============================================================================
*/

static SDL_GameController *GameController;

// IN_GetGameControllerJoyDelta() - Returns the relative movement of the specified
// GameController joysticks (from +/-127)
void IN_GetGameControllerJoyDelta(int *dx, int *dy, GameControllerAxis axis)
{
	if (!GameController) {
		*dx = *dy = 0;
		return;
	}

	SDL_GameControllerUpdate();

	if (axis == gameControllerAxis_Left) {
		*dx = SDL_GameControllerGetAxis(GameController,
										SDL_CONTROLLER_AXIS_LEFTX) >>
			  8;
		*dy = SDL_GameControllerGetAxis(GameController,
										SDL_CONTROLLER_AXIS_LEFTY) >>
			  8;
	} else {
		*dx = SDL_GameControllerGetAxis(GameController,
										SDL_CONTROLLER_AXIS_RIGHTX) >>
			  8;
		*dy = SDL_GameControllerGetAxis(GameController,
										SDL_CONTROLLER_AXIS_RIGHTY) >>
			  8;
	}
}

// IN_GetGameControllerTriggerDelta() - Returns the relative movement of the specified
// GameController triggers (from +/-127)
void IN_GetGameControllerTriggerDelta(int *dl, int *dr)
{
	if (!GameController) {
		*dl = *dr = 0;
		return;
	}

	SDL_GameControllerUpdate();

	*dl = SDL_GameControllerGetAxis(GameController,
									SDL_CONTROLLER_AXIS_TRIGGERLEFT) >>
		  8;
	*dr = SDL_GameControllerGetAxis(GameController,
									SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >>
		  8;
}

int IN_GameControllerButtons()
{
	if (!GameController) {
		return 0;
	}

	SDL_GameControllerUpdate();

	int res = 0;
	for (int i = 0; i < GameControllerNumButtons; i++) {
		res |= SDL_GameControllerGetButton(GameController, i) << i;
	}

	return res;
}

boolean IN_GameControllerPresent()
{
	return GameController != NULL;
}

/*
============================================================================

                        Event Handling Functions

============================================================================
*/

static void HandleKeyEvent(SDL_Event *sdlevent);
static void HandleMouseEvent(SDL_Event *sdlevent);
static void HandleGameControllerEvent(SDL_Event *sdlevent);

static void ProcessEvent(SDL_Event *event)
{
	switch (event->type) {
	case SDL_QUIT:
		Quit(NULL);

	case SDL_WINDOWEVENT_FOCUS_GAINED:
	case SDL_WINDOWEVENT_FOCUS_LOST:
		VL_HandleWindowEvent(event);
		IN_UpdateMouseGrab();
		break;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		HandleKeyEvent(event);
		break;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEWHEEL:
		if (MousePresent) {
			HandleMouseEvent(event);
		}
		break;

	case SDL_CONTROLLERDEVICEADDED:
	case SDL_CONTROLLERDEVICEREMOVED:
		HandleGameControllerEvent(event);
		break;
	}
}

void IN_WaitAndProcessEvents()
{
	SDL_Event event;
	if (!SDL_WaitEvent(&event))
		return;
	do {
		ProcessEvent(&event);
	} while (SDL_PollEvent(&event));
}

void IN_ProcessEvents()
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		ProcessEvent(&event);
	}
}

static void HandleKeyEvent(SDL_Event *sdlevent)
{
	switch (sdlevent->type) {
	case SDL_KEYDOWN: {
		if (CheckFullscreenToggle(sdlevent->key.keysym)) {
			VL_SetFullscreen(!VL_GetFullscreen());
			if (VL_Apply() != 0) {
				LOG_Error("Failed to apply crispy video settings!");
			}

			IN_UpdateMouseGrab();
			return;
		}

		if (CheckScreenshot(sdlevent->key.keysym)) {
			VL_Screenshot((const char *)configdir);
			return;
		}

		LastScan = sdlevent->key.keysym.sym;
		SDL_Keymod mod = SDL_GetModState();

		if (IN_KeyDown(sc_Alt)) {
			if (LastScan == SDLK_F4)
				Quit(NULL);
		}

		if (LastScan == SDLK_KP_ENTER)
			LastScan = SDLK_RETURN;
		else if (LastScan == SDLK_RSHIFT)
			LastScan = SDLK_LSHIFT;
		else if (LastScan == SDLK_RALT)
			LastScan = SDLK_LALT;
		else if (LastScan == SDLK_RCTRL)
			LastScan = SDLK_LCTRL;
		else {
			if ((mod & KMOD_NUM) == 0) {
				switch (LastScan) {
				case SDLK_KP_2:
					LastScan = SDLK_DOWN;
					break;
				case SDLK_KP_4:
					LastScan = SDLK_LEFT;
					break;
				case SDLK_KP_6:
					LastScan = SDLK_RIGHT;
					break;
				case SDLK_KP_8:
					LastScan = SDLK_UP;
					break;
				}
			}
		}

		int sym = LastScan;
		if (sym >= 'a' && sym <= 'z')
			sym -= 32; // convert to uppercase

		if (mod & (KMOD_SHIFT | KMOD_CAPS)) {
			if (sym < lengthof(ShiftNames) && ShiftNames[sym])
				LastASCII = ShiftNames[sym];
		} else {
			if (sym < lengthof(ASCIINames) && ASCIINames[sym])
				LastASCII = ASCIINames[sym];
		}

		KeyboardSet(LastScan, true);

		if (LastScan == SDLK_PAUSE)
			Paused = true;
	} break;
	case SDL_KEYUP: {
		int key = sdlevent->key.keysym.sym;
		if (key == SDLK_KP_ENTER)
			key = SDLK_RETURN;
		else if (key == SDLK_RSHIFT)
			key = SDLK_LSHIFT;
		else if (key == SDLK_RALT)
			key = SDLK_LALT;
		else if (key == SDLK_RCTRL)
			key = SDLK_LCTRL;
		else {
			if ((SDL_GetModState() & KMOD_NUM) == 0) {
				switch (key) {
				case SDLK_KP_2:
					key = SDLK_DOWN;
					break;
				case SDLK_KP_4:
					key = SDLK_LEFT;
					break;
				case SDLK_KP_6:
					key = SDLK_RIGHT;
					break;
				case SDLK_KP_8:
					key = SDLK_UP;
					break;
				}
			}
		}

		KeyboardSet(key, false);
	} break;
	}
}

static void HandleMouseEvent(SDL_Event *sdlevent)
{
	switch (sdlevent->type) {
	case SDL_MOUSEBUTTONDOWN:
		UpdateMouseButtonState(sdlevent->button.button, true);
		break;

	case SDL_MOUSEBUTTONUP:
		UpdateMouseButtonState(sdlevent->button.button, false);
		break;

	case SDL_MOUSEWHEEL:
		MapMouseWheelToButtons(&(sdlevent->wheel));
		break;

	default:
		break;
	}
}

static void HandleGameControllerEvent(SDL_Event *sdlevent)
{
	{
		switch (sdlevent->type) {
		case SDL_CONTROLLERDEVICEADDED: {
			if (!GameController) {
				int id = sdlevent->cdevice.which;
				if (SDL_IsGameController(id)) {
					GameController = SDL_GameControllerOpen(id);
					LOG_Info("SDL GameController '%s' connected",
							 SDL_GameControllerName(GameController));
				}
			}
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED: {
			if (GameController) {
				LOG_Info("SDL GameController '%s' disconnected",
						 SDL_GameControllerName(GameController));
				SDL_GameControllerClose(GameController);
				GameController = NULL;
			}
			break;
		}
		}
	}
}

/*
============================================================================

                    Startup, Shutdown and Misc Functions

============================================================================
*/

// IN_Startup() - Starts up the Input Mgr
void IN_Startup(void)
{
	if (IN_Started)
		return;

	IN_ClearKeysDown();

	if (param_joystickindex >= 0 && param_joystickindex < SDL_NumJoysticks()) {
		if (!SDL_IsGameController(param_joystickindex)) {
			Joystick = SDL_JoystickOpen(param_joystickindex);
			if (Joystick) {
				JoyNumButtons = SDL_JoystickNumButtons(Joystick);
				if (JoyNumButtons > 32)
					JoyNumButtons = 32; // only up to 32 buttons are supported
				JoyNumHats = SDL_JoystickNumHats(Joystick);
				if (param_joystickhat < -1 || param_joystickhat >= JoyNumHats)
					Quit("The joystickhat param must be between 0 and %i!",
						 JoyNumHats - 1);
			}
		}
	}

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

	// I didn't find a way to ask libSDL whether a mouse is present, yet...
	// TODO(Anthony): Ensure this is false for console versions
	MousePresent = true;

	IN_UpdateMouseGrab();

	IN_Started = true;
}

// IN_Shutdown() - Shuts down the Input Mgr.
void IN_Shutdown(void)
{
	if (!IN_Started)
		return;

	if (Joystick)
		SDL_JoystickClose(Joystick);

	if (GameController)
		SDL_GameControllerClose(GameController);

	IN_Started = false;
}

// IN_ReadControl() - Reads the device associated with the specified
// player and fills in the control info struct.
void IN_ReadControl(int player, ControlInfo *info)
{
	word buttons;
	int dx, dy;
	Motion mx, my;

	dx = dy = 0;
	mx = my = motion_None;
	buttons = 0;

	IN_ProcessEvents();

	if (IN_KeyDown(KbdDefs.upleft))
		mx = motion_Left, my = motion_Up;
	else if (IN_KeyDown(KbdDefs.upright))
		mx = motion_Right, my = motion_Up;
	else if (IN_KeyDown(KbdDefs.downleft))
		mx = motion_Left, my = motion_Down;
	else if (IN_KeyDown(KbdDefs.downright))
		mx = motion_Right, my = motion_Down;

	if (IN_KeyDown(KbdDefs.up))
		my = motion_Up;
	else if (IN_KeyDown(KbdDefs.down))
		my = motion_Down;

	if (IN_KeyDown(KbdDefs.left))
		mx = motion_Left;
	else if (IN_KeyDown(KbdDefs.right))
		mx = motion_Right;

	if (IN_KeyDown(KbdDefs.button0))
		buttons += 1 << 0;
	if (IN_KeyDown(KbdDefs.button1))
		buttons += 1 << 1;

	dx = mx * 127;
	dy = my * 127;

	info->x = dx;
	info->xaxis = mx;
	info->y = dy;
	info->yaxis = my;
	info->button0 = (buttons & (1 << 0)) != 0;
	info->button1 = (buttons & (1 << 1)) != 0;
	info->button2 = (buttons & (1 << 2)) != 0;
	info->button3 = (buttons & (1 << 3)) != 0;
	info->dir = DirTable[((my + 1) * 3) + (mx + 1)];
}

// IN_WaitForKey() - Waits for a scan code, then clears LastScan and
// returns the scan code.
ScanCode IN_WaitForKey(void)
{
	ScanCode result;

	while ((result = LastScan) == 0)
		IN_WaitAndProcessEvents();
	LastScan = 0;
	return (result);
}

// IN_WaitForASCII() - Waits for an ASCII char, then clears LastASCII and
// returns the ASCII value.
char IN_WaitForASCII(void)
{
	char result;

	while ((result = LastASCII) == 0)
		IN_WaitAndProcessEvents();
	LastASCII = '\0';
	return (result);
}

// IN_Ack() - waits for a button or key press.  If a button is down, upon
// calling, it must be released for it to be recognized
boolean btnstate[NUMBUTTONS];

void IN_StartAck(void)
{
	int i;

	IN_ProcessEvents();
	//
	// get initial state of everything
	//
	IN_ClearKeysDown();
	memset(btnstate, 0, sizeof(btnstate));

	int buttons = IN_JoyButtons() << 4;

	if (MousePresent)
		buttons |= IN_MouseButtons();

	buttons |= IN_GameControllerButtons();

	for (i = 0; i < NUMBUTTONS; i++, buttons >>= 1)
		if (buttons & 1)
			btnstate[i] = true;
}

boolean IN_CheckAck(void)
{
	int i;

	IN_ProcessEvents();
	//
	// see if something has been pressed
	//
	if (LastScan)
		return true;

	int buttons = IN_JoyButtons() << 4;

	if (MousePresent)
		buttons |= IN_MouseButtons();

	buttons |= IN_GameControllerButtons();

	for (i = 0; i < NUMBUTTONS; i++, buttons >>= 1) {
		if (buttons & 1) {
			if (!btnstate[i]) {
				// Wait until button has been released
				do {
					IN_WaitAndProcessEvents();
					buttons = IN_JoyButtons() << 4;

					if (MousePresent)
						buttons |= IN_MouseButtons();

					buttons |= IN_GameControllerButtons();
				} while (buttons & (1 << i));

				return true;
			}
		} else
			btnstate[i] = false;
	}

	return false;
}

void IN_Ack(void)
{
	IN_StartAck();

	do {
		IN_WaitAndProcessEvents();
	} while (!IN_CheckAck());
}

// IN_UserInput() - Waits for the specified delay time (in ticks) or the
// user pressing a key or a mouse button. If the clear flag is set, it
// then either clears the key or waits for the user to let the mouse
// button up.
boolean IN_UserInput(longword delay)
{
	longword lasttime;

	lasttime = GetTimeCount();
	IN_StartAck();
	do {
		IN_ProcessEvents();
		if (IN_CheckAck())
			return true;
		SDL_Delay(5);
	} while (GetTimeCount() - lasttime < delay);
	return (false);
}
