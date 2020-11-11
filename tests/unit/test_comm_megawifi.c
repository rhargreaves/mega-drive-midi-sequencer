#include "cmocka_inc.h"

#include "applemidi.h"
#include "comm_megawifi.h"
#include "mw/loop.h"
#include "mw/megawifi.h"
#include "mw/mpool.h"
#include "mw/util.h"

extern void __real_comm_megawifi_init(void);

static int test_comm_megawifi_setup(UNUSED void** state)
{
    //  __real_comm_megawifi_init();
    log_init();
    wraps_enable_logging_checks();
    return 0;
}

#define expect_log_info(f)                                                     \
    {                                                                          \
        expect_memory(__wrap_log_info, fmt, f, sizeof(f));                     \
    }

#define expect_udp_port_open(c, d_port, s_port)                                \
    {                                                                          \
        expect_value(__wrap_mw_udp_set, ch, c);                                \
        expect_memory(__wrap_mw_udp_set, dst_addr, "127.0.0.1", 10);           \
        expect_memory(__wrap_mw_udp_set, dst_port, d_port, sizeof(d_port));    \
        expect_memory(__wrap_mw_udp_set, src_port, s_port, sizeof(s_port));    \
        will_return(__wrap_mw_udp_set, MW_ERR_NONE);                           \
                                                                               \
        expect_value(__wrap_mw_sock_conn_wait, ch, c);                         \
        expect_value(                                                          \
            __wrap_mw_sock_conn_wait, tout_frames, MS_TO_FRAMES(1000));        \
        will_return(__wrap_mw_sock_conn_wait, MW_ERR_NONE);                    \
        expect_log_info("AppleMIDI: %s UDP Port: %d");                         \
    }

static void test_comm_megawifi_initialises(UNUSED void** state)
{
    expect_any(__wrap_mw_init, cmd_buf);
    expect_any(__wrap_mw_init, buf_len);
    will_return(__wrap_mw_init, MW_ERR_NONE);

    expect_value(__wrap_loop_init, max_func, 2);
    expect_value(__wrap_loop_init, max_timer, 4);
    will_return(__wrap_loop_init, MW_ERR_NONE);

    expect_any(__wrap_loop_func_add, func);
    will_return(__wrap_loop_func_add, MW_ERR_NONE);

    mock_mw_detect(3, 1);
    will_return(__wrap_mw_detect, MW_ERR_NONE);
    expect_log_info("MegaWiFi: Found v%d.%d");

    expect_log_info("MegaWiFi: Connecting AP");
    expect_value(__wrap_mw_ap_assoc, slot, 0);
    will_return(__wrap_mw_ap_assoc, MW_ERR_NONE);

    expect_any(__wrap_mw_ap_assoc_wait, tout_frames);
    will_return(__wrap_mw_ap_assoc_wait, MW_ERR_NONE);
    expect_log_info("MegaWiFi: Connected.");

    mock_ip_cfg(ip_str_to_uint32("127.1.2.3"));
    will_return(__wrap_mw_ip_current, MW_ERR_NONE);
    expect_log_info("MegaWiFi: IP: %s");

    expect_udp_port_open(CH_CONTROL_PORT, "5004", "5006");
    expect_udp_port_open(CH_MIDI_PORT, "5005", "5007");

    __real_comm_megawifi_init();
}
