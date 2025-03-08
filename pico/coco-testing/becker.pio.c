#include "becker.pio.h"

void becker_from_coco_program_init (PIO pio, uint sm, uint offset) {
	// Get the configuration object for our state machine
	pio_sm_config c = becker_from_coco_program_get_default_config (offset);

	// Set the R/~W pin as our jump pin
	sm_config_set_jmp_pin (&c, RWPIN);

	// We'll be inputting 8 data bits and 5 address bits
	sm_config_set_in_pins (&c, PINROMDATA);
	sm_config_set_in_shift (&c, false, true, FULLWIDTH);
	
	// Initialize the state machine with this configuration
	pio_sm_init (pio, sm, offset, &c);
}

void becker_to_coco_program_init (PIO pio, uint sm, uint offset) {
	// Get the configuration object for our state machine
	pio_sm_config c = becker_from_coco_program_get_default_config (offset);

	// Set the R/~W pin as our jump pin
	sm_config_set_jmp_pin (&c, RWPIN);

	sm_config_set_out_pins(&c, PINROMDATA, DATAWIDTH);
	// We don't want to autopull because we need the OSR to handle pin
	//   directions
	sm_config_set_out_shift(&c, true, false, DATAWIDTH);

	// We'll be grabbing the bottom 5 bits of the address bus and autopushing
	sm_config_set_in_pins (&c, PINROMADDR);
	sm_config_set_in_shift (&c, false, true, SCSWIDTH);

	// We don't need to do the GPIO pin function assignment if it's already been
	//   done for this PIO block.  In our case, it has, because we've set up the
	//   cart rom emulator *before* setting up the becker port.
	// for (int i = 0; i < DATAWIDTH; i++)
	// 	pio_gpio_init(pio, PINROMDATA + i);

	// Initialize the state machine with this configuration
	pio_sm_init (pio, sm, offset, &c);
}