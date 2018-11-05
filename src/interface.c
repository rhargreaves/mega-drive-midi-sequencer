#include <comm.h>
#include <interface.h>
#include <midi.h>
#include <synth.h>

void interface_init(void)
{
    synth_init();
}

void interface_tick(void)
{
    u8 status = comm_read();
    if (status == 0x90) {
        u8 pitch = comm_read();
        u8 velocity = comm_read();
        midi_noteOn(
            pitch,
            velocity);
    } else if (status == 0x80) {
        u8 pitch = comm_read();
        u8 velocity = comm_read();
        midi_noteOff();
    }
}
