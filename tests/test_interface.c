#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>
#include <interface.h>
#include <midi.h>
#include <types.h>
#include <wraps.h>

static void test_interface_tick_passes_note_on_to_midi_processor(void** state)
{
    u8 expectedStatus = 0x90;
    u8 expectedData = 60;
    u8 expectedData2 = 127;
    Message expectedMessage = { expectedStatus, expectedData, expectedData2 };

    will_return(__wrap_comm_read, expectedStatus);
    will_return(__wrap_comm_read, expectedData);
    will_return(__wrap_comm_read, expectedData2);

    expect_value(__wrap_midi_noteOn, pitch, expectedData);
    expect_value(__wrap_midi_noteOn, velocity, expectedData2);

    interface_tick();
}

static void test_interface_tick_passes_note_off_to_midi_processor(void** state)
{
    u8 expectedStatus = 0x80;
    u8 expectedData = 60;
    u8 expectedData2 = 127;
    Message expectedMessage = { expectedStatus, expectedData, expectedData2 };

    will_return(__wrap_comm_read, expectedStatus);
    will_return(__wrap_comm_read, expectedData);
    will_return(__wrap_comm_read, expectedData2);

    expect_function_call(__wrap_midi_noteOff);

    interface_tick();
}

static void test_interface_initialises_synth(void** state)
{
    expect_function_call(__wrap_synth_init);

    interface_init();
}
