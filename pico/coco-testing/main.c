#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "rom.h"

// define how the GPIOs are connected to the CoCo Bus
// these are used in main.c and cococart.pio
#define PINROMADDR  8
#define PINROMDATA  0
#define RWPIN      24
#define ADDRWIDTH  16 // 64k address space

#include "rom.pio.h"
#include "becker.pio.h"

PIO pioblk_rw = pio0;
#define SM_ROM 3
#define SM_SCS_FROM_COCO 2
#define SM_SCS_TO_COCO 1

void __time_critical_func(handle_becker)() {
	uint32_t what = 0;				// The data
	uint32_t where = 0;				// The address
	uint32_t have_value = 0;		// Whether or not data is available to read
	while (true) {
		if (!pio_sm_is_rx_fifo_empty (pioblk_rw, SM_SCS_FROM_COCO)) {
			printf ("I might have data\n");
			// We have a byte waiting for us from the CoCo
			uint32_t from = pio_sm_get (pioblk_rw, SM_SCS_FROM_COCO);
			// Get the I/O port address the CoCo's trying to access
			where = (from >> 8) & 0x1f;
			// We only listen to port 0x02 (0xff42)
			if (where != 0x02) continue;
			// Get the data byte
			what = from & 0xff;
			have_value = 0b10;
			printf ("I got data: %08x\n", what);
		}
		if (!pio_sm_is_rx_fifo_empty (pioblk_rw, SM_SCS_TO_COCO)) {
			where = pio_sm_get (pioblk_rw, SM_SCS_TO_COCO) & 0x1f;
			switch (where) {
				case 1:
					// This is the request to the becker status port.
					pio_sm_put (
						pioblk_rw, SM_SCS_TO_COCO, (have_value << 8) | 1
					);
					printf ("I was asked for becker status\n");
					break;
				case 2:
					// This is the request to the becker data port.
					pio_sm_put (
						pioblk_rw, SM_SCS_TO_COCO, (what << 8) | 1
					);
					printf ("I was asked for becker data\n");
					what = 0;
					have_value = 0;
					break;
				default:
					// This isn't the droid we're looking for.
					pio_sm_put (pioblk_rw, SM_SCS_TO_COCO, 0);
					printf ("I was asked for data outside my I/O range\n");
			}
		}
	}
}

void setup_becker_pio() {
	// Set up the port to receive from the CoCo
	uint offset = pio_add_program (pioblk_rw, &becker_from_coco_program);
	printf ("\nLoaded becker from CoCo at %d\n", offset);
	becker_from_coco_program_init (pioblk_rw, SM_SCS_FROM_COCO, offset);
	pio_sm_set_enabled (pioblk_rw, SM_SCS_FROM_COCO, true);
	// Set up the port to send to the CoCo
	offset = pio_add_program (pioblk_rw, &becker_to_coco_program);
	printf ("\nLoaded becker to CoCo at %d\n", offset);
	becker_to_coco_program_init (pioblk_rw, SM_SCS_TO_COCO, offset);
	pio_sm_set_enabled (pioblk_rw, SM_SCS_TO_COCO, true);
}

void setup_rom_emulator()
{
	int chan_rom_addr, chan_rom_data;
	dma_channel_config cfg_rom_addr, cfg_rom_data;

	uint offset = pio_add_program(pioblk_rw, &rom_program);
    printf("\nLoaded rom program at %d\n", offset);
    rom_program_init(pioblk_rw, SM_ROM, offset);
    pio_sm_put(pioblk_rw, SM_ROM, (uintptr_t)rom >> ROMWIDTH);
    pio_sm_exec_wait_blocking(pioblk_rw, SM_ROM, pio_encode_pull(false, true));
    pio_sm_exec_wait_blocking(pioblk_rw, SM_ROM, pio_encode_mov(pio_y, pio_osr));
    pio_sm_exec_wait_blocking(pioblk_rw, SM_ROM, pio_encode_out(pio_null, 1)); 
    pio_sm_set_enabled(pioblk_rw, SM_ROM, true);

    chan_rom_addr = dma_claim_unused_channel(true);
    cfg_rom_addr = dma_channel_get_default_config(chan_rom_addr);

    chan_rom_data = dma_claim_unused_channel(true);
    cfg_rom_data = dma_channel_get_default_config(chan_rom_data);
 
    channel_config_set_read_increment(&cfg_rom_data,false);
    channel_config_set_write_increment(&cfg_rom_data,false);
    channel_config_set_dreq(&cfg_rom_data, pio_get_dreq(pioblk_rw, SM_ROM, true)); // ROM PIO
    channel_config_set_chain_to(&cfg_rom_data, chan_rom_addr);
    channel_config_set_transfer_data_size(&cfg_rom_data, DMA_SIZE_8);
    channel_config_set_irq_quiet(&cfg_rom_data, true);
    channel_config_set_enable(&cfg_rom_data, true);
    dma_channel_configure(
        chan_rom_data,                          // Channel to be configured
        &cfg_rom_data,                        // The configuration we just created
        &pioblk_rw->txf[SM_ROM],                   // The initial write address
        rom,                      // The initial read address
        1,                                  // Number of transfers; in this case each is 1 byte.
        false                               // do not Start immediately.      
      );

    channel_config_set_read_increment(&cfg_rom_addr,false);
    channel_config_set_write_increment(&cfg_rom_addr,false);
    channel_config_set_dreq(&cfg_rom_addr, pio_get_dreq(pioblk_rw, SM_ROM, false)); // ROM PIO
    channel_config_set_chain_to(&cfg_rom_addr, chan_rom_data);
    channel_config_set_transfer_data_size(&cfg_rom_addr, DMA_SIZE_32);
    channel_config_set_irq_quiet(&cfg_rom_addr, true);
    channel_config_set_enable(&cfg_rom_addr, true);
    dma_channel_configure(
        chan_rom_addr,                          // Channel to be configured
        &cfg_rom_addr,                        // The configuration we just created
        &dma_channel_hw_addr(chan_rom_data)->read_addr,   // The initial write address
        &pioblk_rw->rxf[SM_ROM],            // The initial read address
        1,                                  // Number of transfers; in this case each is 1 byte.
        true                               // do Start immediately.      
      );
}

int main () {
	const uint32_t addrmask = 0xffff << PINROMADDR;
	const uint32_t datamask = 0xff << PINROMDATA;
	const uint32_t ctrlmask = (1 << CLKPIN) | (1 << CTSPIN) | (1 << RWPIN);

	stdio_init_all();
	
	gpio_init_mask(addrmask | datamask | ctrlmask);
	gpio_set_dir_all_bits(0);
  
	for (int i = 0; i < DATAWIDTH; i++)
	  gpio_disable_pulls(PINROMDATA + i);
  
	for (int i = 0; i < ADDRWIDTH; i++)
	  gpio_disable_pulls(PINROMADDR + i);
  
	gpio_set_pulls(CTSPIN, true, false);
	gpio_disable_pulls(CLKPIN);
	gpio_disable_pulls(RWPIN);

	setup_rom_emulator();
	setup_becker_pio();
	handle_becker();
}  