// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000

#include "ch32v003fun.h"
#include <stdio.h>

#define APB_CLOCK SYSTEM_CORE_CLOCK

// This example shows how to flexibly drive arbitrary charlieplexed displays
// It uses two layers of lookup to determine the port data for each LED.  
// The first layer converts the LED# to two virtual pin#.
// The second layer converts the virtual pin# to the GPIO register values.

#define CHARLIE_PINS_N 16
#define CHARLIE_LEDS (CHARLIE_PINS_N * (CHARLIE_PINS_N - 1))
// Create masks for the GPIO configuration registers to show which bits we control
// We use all of port C
#define CHARLIE_CFGLRC_MASK 	(~(GPIO_CFGLR_MODE0 | \
				   GPIO_CFGLR_MODE1 | \
				   GPIO_CFGLR_MODE2 | \
				   GPIO_CFGLR_MODE3 | \
				   GPIO_CFGLR_MODE4 | \
				   GPIO_CFGLR_MODE5 | \
				   GPIO_CFGLR_MODE6 | \
				   GPIO_CFGLR_MODE7 | \
				   GPIO_CFGLR_CNF0 | \
				   GPIO_CFGLR_CNF1 | \
				   GPIO_CFGLR_CNF2 | \
				   GPIO_CFGLR_CNF3 | \
				   GPIO_CFGLR_CNF4 | \
				   GPIO_CFGLR_CNF5 | \
				   GPIO_CFGLR_CNF6 | \
				   GPIO_CFGLR_CNF7))
// We use all of port D
#define CHARLIE_CFGLRD_MASK 	(~(GPIO_CFGLR_MODE0 | \
				   GPIO_CFGLR_MODE1 | \
				   GPIO_CFGLR_MODE2 | \
				   GPIO_CFGLR_MODE3 | \
				   GPIO_CFGLR_MODE4 | \
				   GPIO_CFGLR_MODE5 | \
				   GPIO_CFGLR_MODE6 | \
				   GPIO_CFGLR_MODE7 | \
				   GPIO_CFGLR_CNF0 | \
				   GPIO_CFGLR_CNF1 | \
				   GPIO_CFGLR_CNF2 | \
				   GPIO_CFGLR_CNF3 | \
				   GPIO_CFGLR_CNF4 | \
				   GPIO_CFGLR_CNF5 | \
				   GPIO_CFGLR_CNF6 | \
				   GPIO_CFGLR_CNF7))
// We also need a value to set all of the pins to input
#define CHARLIE_CFGLRC_IN 	((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | \
			 	 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7))
#define CHARLIE_CFGLRD_IN 	((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | \
				 (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7))
// And a value to use to reset all the pins
#define CHARLIE_BSHRC_PINS 	(GPIO_BSHR_BR0 | GPIO_BSHR_BR1 | GPIO_BSHR_BR2 | GPIO_BSHR_BR3 | \
				GPIO_BSHR_BR4 | GPIO_BSHR_BR5 | GPIO_BSHR_BR6 | GPIO_BSHR_BR7)
#define CHARLIE_BSHRD_PINS 	(GPIO_BSHR_BR0 | GPIO_BSHR_BR1 | GPIO_BSHR_BR2 | GPIO_BSHR_BR3 | \
				GPIO_BSHR_BR4 | GPIO_BSHR_BR5 | GPIO_BSHR_BR6 | GPIO_BSHR_BR7)
// Define an array of structures containing the values for the port C and D
// configuration and bit set/reset registers to drive a physical pin--indexed
// by virtual pin number.  
// Virtual pin 0 is PC0 though pin 7 being PC7
// Then 8 is PD0 through 15 being PD7
const struct {
	uint32_t cfglrc;
	uint32_t cfglrd;
	uint16_t bshrc;
	uint16_t bshrd;
} charlie_pin_data[CHARLIE_PINS_N] = {
	{((GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*0) | 
 	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 GPIO_BSHR_BS0,
	 0x0},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 GPIO_BSHR_BS1,
	 0x0},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 GPIO_BSHR_BS2,
	 0x0},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 GPIO_BSHR_BS3,
	 0x0},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 GPIO_BSHR_BS4,
	 0x0},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 GPIO_BSHR_BS5,
	 0x0},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 GPIO_BSHR_BS6,
	 0x0},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) |
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 GPIO_BSHR_BS7,
	 0x0},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 0x0,
	 GPIO_BSHR_BS0},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 0x0,
	 GPIO_BSHR_BS1},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 0x0,
	 GPIO_BSHR_BS2},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 0x0,
	 GPIO_BSHR_BS3},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 0x0,
	 GPIO_BSHR_BS4},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 0x0,
	 GPIO_BSHR_BS5},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 0x0,
	 GPIO_BSHR_BS6},

	{((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*7)),
	 ((GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*0) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*1) |
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*2) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*3) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*4) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*5) | 
	  (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (4*6) | 
	  (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4*7)),
	 0x0,
	 GPIO_BSHR_BS7}
};

// Functions to translate and led# into virtual pins to set/reset
uint32_t get_led_set( uint32_t led ){
	return((led + 1 + (led / (CHARLIE_PINS_N))) % (CHARLIE_PINS_N));
}
uint32_t get_led_reset( uint32_t led ){
	return(led/(CHARLIE_PINS_N-1));
}

void set_led( uint32_t led){
	uint32_t temp, set_pin, reset_pin;

	// Start by setting all the pins to input/floating to prevent ghosting
	// Compute the virtual pin# for set and reset
	reset_pin = get_led_set( led );
	set_pin = get_led_reset( led );
	temp = GPIOC->CFGLR;
	temp &= CHARLIE_CFGLRC_MASK;
	temp |= (CHARLIE_CFGLRC_IN & CHARLIE_CFGLRC_MASK);
	GPIOC->CFGLR = temp;
	temp = GPIOD->CFGLR;
	temp &= CHARLIE_CFGLRD_MASK;
	temp |= (CHARLIE_CFGLRD_IN & CHARLIE_CFGLRD_MASK);
	GPIOD->CFGLR = temp;
	// Clear the output values for our pins
	GPIOC->BSHR = CHARLIE_BSHRC_PINS;
	GPIOD->BSHR = CHARLIE_BSHRD_PINS;
	// Configure the pins for input/output
	GPIOC->CFGLR |= charlie_pin_data[set_pin].cfglrc | charlie_pin_data[reset_pin].cfglrc;
	GPIOD->CFGLR |= charlie_pin_data[set_pin].cfglrd | charlie_pin_data[reset_pin].cfglrd;
	// Set the set pin
	GPIOC->BSHR = charlie_pin_data[set_pin].bshrc;
	GPIOD->BSHR = charlie_pin_data[set_pin].bshrd;
}

int main()
{
	SystemInit();

	Delay_Ms(1000);

	// Enable GPIOs
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_AFIOEN;
	// Convert PD1 from SWIO to GPIO
	AFIO->PCFR1 &= ~(AFIO_PCFR1_SWJ_CFG);
	AFIO->PCFR1 |= AFIO_PCFR1_SWJ_CFG_DISABLE;

	while(1){
		for( uint32_t led = 0; led < CHARLIE_LEDS; led++){
			set_led(led);
			Delay_Ms(1);
		}
	}
}

