#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "test_interface.c"
#include "test_midi.c"
#include "test_synth.c"
#include "test_psg_chip.c"
#include <cmocka.h>

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_interface_initialises_synth),
        cmocka_unit_test(test_interface_tick_passes_note_on_to_midi_processor),
        cmocka_unit_test(test_interface_tick_passes_note_off_to_midi_processor),
        cmocka_unit_test(test_interface_does_nothing_for_control_change),
        cmocka_unit_test(test_interface_sets_pan),
        cmocka_unit_test(test_interface_sets_unknown_event_for_system_messages),
        cmocka_unit_test(test_interface_sets_unknown_CC),
        cmocka_unit_test(test_interface_sets_channel_volume),
        cmocka_unit_test(test_interface_sets_all_notes_off),
        cmocka_unit_test(test_interface_does_not_set_unknown_CC_for_known_CC),
        cmocka_unit_test(test_interface_sets_fm_algorithm),
        cmocka_unit_test(test_interface_sets_operator_total_level),

        cmocka_unit_test(test_midi_triggers_synth_note_on),
        cmocka_unit_test(test_midi_triggers_synth_note_on_2),
        cmocka_unit_test(test_midi_triggers_synth_note_off),
        cmocka_unit_test(test_midi_triggers_psg_note_on),
        cmocka_unit_test(test_midi_triggers_psg_note_off),
        cmocka_unit_test(test_midi_channel_volume_sets_total_level),
        cmocka_unit_test(test_midi_pan_sets_synth_stereo_mode_right),
        cmocka_unit_test(test_midi_pan_sets_synth_stereo_mode_left),
        cmocka_unit_test(test_midi_pan_sets_synth_stereo_mode_centre),
        cmocka_unit_test(test_midi_channel_volume_sets_psg_attenuation),
        cmocka_unit_test(test_midi_channel_volume_sets_psg_attenuation_2),

        cmocka_unit_test(test_synth_init_sets_initial_registers),
        cmocka_unit_test(test_synth_sets_note_on_fm_reg_chan_0_to_2),
        cmocka_unit_test(test_synth_sets_note_on_fm_reg_chan_3_to_5),
        cmocka_unit_test(test_synth_sets_note_off_fm_reg_chan_0_to_2),
        cmocka_unit_test(test_synth_sets_note_off_fm_reg_chan_3_to_5),
        cmocka_unit_test(test_synth_sets_octave_and_freq_reg_chan),
        cmocka_unit_test(test_synth_sets_total_level_reg_chan),
        cmocka_unit_test(test_synth_sets_stereo_reg_chan),
        cmocka_unit_test(test_synth_sets_algorithm),
        cmocka_unit_test(test_synth_sets_operator_total_level),

        cmocka_unit_test(test_psg_chip_sets_note_on_psg),
        cmocka_unit_test(test_psg_chip_sets_note_off_psg),
        cmocka_unit_test(test_psg_chip_sets_attenuation),
        cmocka_unit_test(test_psg_chip_sets_note_on_psg_with_attenuation)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
