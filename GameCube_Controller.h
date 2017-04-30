/*
	GameCube_Controller - Library for interfacing with a Nintendo GameCube Controller.
	Created 4/29/17 by Samuel Moody.
	See GameCube_Controller.cpp for copyright info.
*/
#ifndef GAMECUBE_CONTROLLER_HPP
#define GAMECUBE_CONTROLLER_HPP

class GameCubeController
{
protected:
	enum BTN1 {
		BTN_START=0x10,
		BTN_Y = 0x08,
		BTN_X = 0x04,
		BTN_B = 0x02,
		BTN_A = 0x01,
	};
	enum BTN2 {
		BTN_L = 0x40,
		BTN_R = 0x20,
		BTN_Z = 0x10,
		BTN_UP = 0x08,
		BTN_DN = 0x04,
		BTN_RT = 0x02,
		BTN_LF = 0x01,
	};

	struct {
		// bits: 0, 0, 0, start, y, x, b, a
		unsigned char data1;
		// bits: 1, L, R, Z, Dup, Ddown, Dright, Dleft
		unsigned char data2;
		unsigned char stick_x;
		unsigned char stick_y;
		unsigned char cstick_x;
		unsigned char cstick_y;
		unsigned char left;
		unsigned char right;
	} gc_status;

	// Zero points for controller stick
	unsigned char zero_main_x, zero_main_y;
	unsigned char zero_c_x, zero_c_y;
	bool enable_rumble;

	void gc_send(unsigned char* buffer, char length);
	int gc_get();
	void init_gc_controller();

public:
	// Initialize the controller.
	// Returns true if successful, otherwise false.
	// Blocks if data line does not idle high.
	bool initialize();

	// Update the controller values.
	// Do this periodically or before control data is needed.
	// Returns true if successful.
	// Call initialize() again if this call fails.
	bool update();

	// Sets the current values as the center for
	// the main joystick and C stick.
	void zero();

	// Get the status of various buttons.
	bool buttonStart() const { return gc_status.data1 & BTN_START; }
	bool buttonA() const { return gc_status.data1 & BTN_A; }
	bool buttonB() const { return gc_status.data1 & BTN_B; }
	bool buttonX() const { return gc_status.data1 & BTN_X; }
	bool buttonY() const { return gc_status.data1 & BTN_Y; }
	bool buttonZ() const { return gc_status.data2 & BTN_Z; }
	bool buttonL() const { return gc_status.data2 & BTN_L; }
	bool buttonR() const { return gc_status.data2 & BTN_R; }
	bool buttonLeft() const { return gc_status.data2 & BTN_LF; }
	bool buttonRight() const { return gc_status.data2 & BTN_RT; }
	bool buttonUp() const { return gc_status.data2 & BTN_UP; }
	bool buttonDown() const { return gc_status.data2 & BTN_DN; }

	// Get joystick positions in the range [-128,127].
	// These are zero-adjusted but not calibrated.
	signed char joystickX() const { return gc_status.stick_x-zero_main_x; }
	signed char joystickY() const { return gc_status.stick_y-zero_main_y; }
	signed char CstickX() const { return gc_status.cstick_x-zero_c_x; }
	signed char CstickY() const { return gc_status.cstick_y-zero_c_y; }

	// L and R values (the analog part, not the buttons).
	unsigned char left() const { return gc_status.left; }
	unsigned char right() const { return gc_status.right; }

	// Turn the rumble on/off.
	// Takes effect on the next call to update().
	void setRumble(bool enable) { enable_rumble=enable; }
};

#endif // GAMECUBE_CONTROLLER_HPP

