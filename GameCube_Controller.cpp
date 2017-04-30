/*
	Nintendo GameCube Controller Object
*/

/*
 Copyright (c) 2017 Samuel Moody

 Portions of this code were derived from
 code Copyright (c) 2009 Andrew Brown.
 See https://github.com/brownan/Gamecube-N64-Controller

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * To use, hook up the following to the Arduino Duemilanove:
 * Digital I/O 2: Gamecube controller serial line
 * All appropriate grounding and power lines
 * A 1K resistor to bridge digital I/O 2 and the 3.3V supply
 *
 * The pin-out for the N64 and Gamecube wires can be found here:
 * http://svn.navi.cx/misc/trunk/wasabi/devices/cube64/hardware/cube64-basic.pdf
 * Note: that diagram is not for this project, but for a similar project which
 * uses a PIC microcontroller. However, the diagram does describe the pinouts
 * of the gamecube and N64 wires.
 */

#include "GameCube_Controller.h"

#include "Arduino.h"
#include "pins_arduino.h"

//Note: This is hardcoded below, careful if you change this!
#define GC_PIN 2
#define GC_PIN_DIR DDRD
// these two macros set arduino pin 2 to input or output, which with an
// external 1K pull-up resistor to the 3.3V rail, is like pulling it high or
// low.  These operations translate to 1 op code, which takes 2 cycles
#define GC_HIGH DDRD &= ~0x04
#define GC_LOW DDRD |= 0x04
#define GC_QUERY (PIND & 0x04)

bool GameCubeController::initialize()
{
	// Communication with gamecube controller on this pin
	// Don't remove these lines, we don't want to push +5V to the controller
	digitalWrite(GC_PIN, LOW);  
	pinMode(GC_PIN, INPUT);

	noInterrupts();
	init_gc_controller();
	interrupts();
	return true;
}

void GameCubeController::init_gc_controller()
{
	// Initialize the gamecube controller by sending it a null byte.
	// This is unnecessary for a standard controller, but is required for the
	// Wavebird.
	unsigned char initialize = 0x00;
	gc_send(&initialize, 1);

	gc_status.data1 = 0x00;
	gc_status.data2 = 0x00;
	gc_status.stick_x = 128;
	gc_status.stick_y = 128;
	gc_status.cstick_x = 128;
	gc_status.cstick_y = 128;
	zero_main_x = 128;
	zero_main_y = 128;
	zero_c_x = 128;
	zero_c_y = 128;
	enable_rumble = false;

	// Stupid routine to wait for the gamecube controller to stop
	// sending its response. We don't care what it is, but we
	// can't start asking for status if it's still responding
	int x;
	for (x=0; x<64; x++)
	{
		// make sure the line is idle for 64 iterations, should
		// be plenty.
		if (!GC_QUERY)
			x = 0;
	}
}


/**
 * This sends the given byte sequence to the controller
 * length must be at least 1
 * hardcoded for Arduino DIO 2 and external pull-up resistor
 */
void GameCubeController::gc_send(unsigned char *buffer, char length)
{
	asm volatile (
		"; Start of gc_send assembly\n"

		// passed in to this block are:
		// the Z register (r31:r30) is the buffer pointer
		// %[length] is the register holding the length of the buffer in bytes

		// Instruction cycles are noted in parentheses
		// branch instructions have two values, one if the branch isn't
		// taken and one if it is

		// r25 will be the current buffer byte loaded from memory
		// r26 will be the bit counter for the current byte. when this
		// reaches 0, we need to decrement the length counter, load
		// the next buffer byte, and loop. (if the length counter becomes
		// 0, that's our exit condition)
		
		"ld r25, Z\n" // load the first byte

		// This label starts the outer loop, which sends a single byte
		".L%=_byte_loop:\n"
		"ldi r26,lo8(8)\n" // (1)

		// This label starts the inner loop, which sends a single bit
		".L%=_bit_loop:\n"
		"sbi 0xa,2\n" // (2) pull the line low

		// line needs to stay low for 1µs for a 1 bit, 3µs for a 0 bit
		// this block figures out if the next bit is a 0 or a 1
		// the strategy here is to shift the register left, then test and
		// branch on the carry flag
		"lsl r25\n" // (1) shift left. MSB goes into carry bit of status reg
		"brcc .L%=_zero_bit\n" // (1/2) branch if carry is cleared

		
		// this block is the timing for a 1 bit (1µs low, 3µs high)
		// Stay low for 16 - 2 (above lsl,brcc) - 2 (below cbi) = 12 cycles
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\n" // (2)
		"cbi 0xa,2\n" // (2) set the line high again
		// Now stay high for 2µs of the 3µs to sync up with the branch below
		// 2*16 - 2 (for the rjmp) = 30 cycles
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"rjmp .L%=_finish_bit\n" // (2)


		// this block is the timing for a 0 bit (3µs low, 1µs high)
		// Need to go high in 3*16 - 3 (above lsl,brcc) - 2 (below cbi) = 43 cycles
		".L%=_zero_bit:\n"
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\n" // (3)
		"cbi 0xa,2\n" // (2) set the line high again


		// The two branches meet up here.
		// We are now *exactly* 3µs into the sending of a bit, and the line
		// is high again. We have 1µs to do the looping and iteration
		// logic.
		".L%=_finish_bit:\n"
		"subi r26,1\n" // (1) subtract 1 from our bit counter
		"breq .L%=_load_next_byte\n" // (1/2) branch if we've sent all the bits of this byte

		// At this point, we have more bits to send in this byte, but the
		// line must remain high for another 1µs (minus the above
		// instructions and the jump below and the sbi instruction at the
		// top of the loop)
		// 16 - 2(above) - 2 (rjmp below) - 2 (sbi after jump) = 10
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"rjmp .L%=_bit_loop\n"


		// This block starts 3 cycles into the last 1µs of the line being high
		// We need to decrement the byte counter. If it's 0, that's our exit condition.
		// If not we need to load the next byte and go to the top of the byte loop
		".L%=_load_next_byte:\n"
		"subi %[length], 1\n" // (1)
		"breq .L%=_loop_exit\n" // (1/2) if the byte counter is 0, exit
		"adiw r30,1\n" // (2) increment byte pointer
		"ld r25, Z\n" // (2) load the next byte
		// delay block:
		// needs to go high after 1µs or 16 cycles
		// 16 - 9 (above) - 2 (the jump itself) - 3 (after jump) = 2
		"nop\nnop\n" // (2)
		"rjmp .L%=_byte_loop\n" // (2)


		// Loop exit
		".L%=_loop_exit:\n"

		// final task: send the stop bit, which is a 1 (1µs low 3µs high)
		// the line goes low in:
		// 16 - 6 (above since line went high) - 2 (sbi instruction below) = 8 cycles
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\n" // (3)
		"sbi 0xa,2\n" // (2) pull the line low
		// stay low for 1µs
		// 16 - 2 (below cbi) = 14
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\nnop\n" // (5)
		"nop\nnop\nnop\nnop\n" // (4)
		"cbi 0xa,2\n" // (2) set the line high again

		// just stay high. no need to wait 3µs before returning

		:
		// outputs:
		"+z" (buffer) // (read and write)
		:
		// inputs:
		[length] "r" (length)
		:
		// clobbers:
			"r25", "r26"
	);

}

// Read 8 bytes from the gamecube controller
// hardwired to read from Arduino DIO2 with external pullup resistor
int GameCubeController::gc_get()
{
	// listen for the expected 8 bytes of data back from the controller and
	// and pack it into the gc_status struct.
	asm volatile (";Starting to listen");

	// treat the 8 byte struct gc_status as a raw char array.
	unsigned char *bitbin = reinterpret_cast<unsigned char*>(&gc_status);

	unsigned char retval;

	asm volatile (
		"; START OF MANUAL ASSEMBLY BLOCK\n"
		// r25 is our bit counter. We read 64 bits and increment the byte
		// pointer every 8 bits
		"ldi r25,lo8(0)\n"
		// read in the first byte of the gc_status struct
		"ld r23,Z\n"
		// default exit value is 1 (success)
		"ldi %[retval],lo8(1)\n"

		// Top of the main read loop label
		"L%=_read_loop:\n"

		// This first spinloop waits for the line to go low. It loops 64
		// times before it gives up and returns
		"ldi r24,lo8(64)\n" // r24 is the timeout counter
		"L%=_1:\n"
		"sbis 0x9,2\n" // reg 9 bit 2 is PIND2, or arduino I/O 2
		"rjmp L%=_2\n" // line is low. jump to below
		// the following happens if the line is still high
		"subi r24,lo8(1)\n"
		"brne L%=_1\n" // loop if the counter isn't 0
		// timeout? set output to 0 indicating failure and jump to
		// the end
		"ldi %[retval],lo8(0)\n"
		"rjmp L%=_exit\n"
		"L%=_2:\n"

		// Next block. The line has just gone low. Wait approx 2µs
		// each cycle is 1/16 µs on a 16Mhz processor
		"nop\nnop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\nnop\n"

		// This block left shifts the current gc_status byte in r23,
		// and adds the current line state as the LSB
		"lsl r23\n" // left shift
		"sbic 0x9,2\n" // read PIND2
		"sbr r23,lo8(1)\n" // set bit 1 in r23 if PIND2 is high
		"st Z,r23\n" // save r23 back to memory. We technically only have
		// to do this every 8 bits but this simplifies the branches below

		// This block increments the bitcount(r25). If bitcount is 64, exit
		// with success. If bitcount is a multiple of 8, then increment Z
		// and load the next byte.
		"subi r25,lo8(-1)\n" // increment bitcount
		"cpi r25,lo8(64)\n" // == 64?
		"breq L%=_exit\n" // jump to exit
		"mov r24,r25\n" // copy bitcounter(r25) to r24 for tmp
		"andi r24,lo8(7)\n" // get lower 3 bits
		"brne L%=_3\n" // branch if not 0 (is not divisble by 8)
		"adiw r30,1\n" // if divisible by 8, increment pointer
		"ld r23,Z\n" // ...and load the new byte into r23
		"L%=_3:\n"

		// This next block waits for the line to go high again. again, it
		// sets a timeout counter of 64 iterations
		"ldi r24,lo8(64)\n" // r24 is the timeout counter
		"L%=_4:\n"
		"sbic 0x9,2\n" // checks PIND2
		"rjmp L%=_read_loop\n" // line is high. ready for next loop
		// the following happens if the line is still low
		"subi r24,lo8(1)\n"
		"brne L%=_4\n" // loop if the counter isn't 0
		// timeout? set output to 0 indicating failure and fall through to
		// the end
		"ldi %[retval],lo8(0)\n"


		"L%=_exit:\n"
		";END OF MANUAL ASSEMBLY BLOCK\n"
		// ----------
		// outputs:
		: [retval] "=r" (retval),
		// About the bitbin pointer: The "z" constraint tells the
		// compiler to put the pointer in the Z register pair (r31:r30)
		// The + tells the compiler that we are both reading and writing
		// this pointer. This is important because otherwise it will
		// allocate the same register for retval (r30).
		"+z" (bitbin)
		// clobbers (registers we use in the assembly for the compiler to
		// avoid):
		:: "r25", "r24", "r23"
	);

	return retval;
}

bool GameCubeController::update()
{
	int status;
	unsigned char data, addr;

	// Command to send to the gamecube
	// The last bit is rumble, flip it to rumble
	unsigned char command[] = {0x40, 0x03, 0x00};
	if(enable_rumble)
		command[2] = 0x01;

	// don't want interrupts getting in the way
	noInterrupts();
	// send those 3 bytes
	gc_send(command, 3);
	// read in data and dump it to gc_raw_dump
	status = gc_get();
	// end of time sensitive code
	interrupts();

	return status != 0;
}

void GameCubeController::zero()
{
	zero_main_x = gc_status.stick_x;
	zero_main_y = gc_status.stick_y;
	zero_c_x = gc_status.cstick_x;
	zero_c_y = gc_status.cstick_y;
}


