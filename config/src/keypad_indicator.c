/*
 * Keypad status LED indicator
 *
 * - Not connected: blinks out (active profile index + 1) short pulses,
 *   pauses, then repeats - see ksn1-firmware's
 *   ksn1_conn_status_relay_peripheral.c for the full profile-count design
 *   rationale (ported here). This is a standalone, non-split board, so
 *   everything here is a local function call - no GATT relay needed.
 * - Connected, after showing the profile count at least
 *   KEYPAD_PROFILE_MIN_CYCLES times: LED on for KEYPAD_LED_ON_DURATION_MS
 *   (60s), then auto-off to save battery (original 60-second-timeout
 *   behavior, kept as-is). A fresh disconnect/reconnect or a profile
 *   switch always restarts the count-then-solid sequence from zero, even
 *   after the LED has already timed out and gone dark.
 *
 * Uses the `indicator_led` GPIO defined in keypad.overlay (the same LED
 * previously driven by zmk-poor-mans-led-indicator). That widget's BLE
 * indication is disabled in keypad.conf so it no longer drives this pin.
 *
 * NOTE: switched from zmk_ble_active_profile_is_connected() to
 * zmk_endpoint_is_connected() for consistency with the KSN boards and to
 * correctly reflect a wired USB connection, not just the active BLE
 * profile's own link.
 *
 * Guarded on DT_NODE_EXISTS(DT_NODELABEL(indicator_led)) so this file is a
 * no-op on any shield build that doesn't define that node (e.g. settings_reset,
 * which shares this project's build.yaml but has no keypad.overlay applied) -
 * mirrors the guard pattern used in ksn1-firmware's peripheral indicator driver.
 *
 * NOTE: <zephyr/devicetree.h> must be included BEFORE the #if below, since
 * DT_NODE_EXISTS/DT_NODELABEL are ordinary macros defined by that header - if
 * the #if runs first, the preprocessor treats them as plain (undefined) tokens
 * and errors out with "missing binary operator before token (".
 */

#include <zephyr/devicetree.h>

#if DT_NODE_EXISTS(DT_NODELABEL(indicator_led))

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>

LOG_MODULE_REGISTER(keypad_indicator, CONFIG_ZMK_LOG_LEVEL);

#define LED_NODE DT_NODELABEL(indicator_led)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

/* Profile-count blink cycle, played whenever there is no active host
 * connection: (active profile index + 1) pulses of ON_MS/OFF_MS, then a
 * CYCLE_PAUSE_MS gap, then repeat for as long as disconnected. The "+1"
 * is so profile 0 still blinks once instead of looking identical to "no
 * signal at all". MIN_CYCLES is the minimum number of full cycles played
 * after any profile switch (or a drop from a previously-solid state)
 * before "connected" is allowed to turn the LED solid. */
#define KEYPAD_PROFILE_BLINK_ON_MS 150
#define KEYPAD_PROFILE_BLINK_OFF_MS 150
#define KEYPAD_PROFILE_CYCLE_PAUSE_MS 700
#define KEYPAD_PROFILE_MIN_CYCLES 5
/* LED on duration once settled solid, before auto-off (original
 * battery-saving behavior, unchanged). */
#define KEYPAD_LED_ON_DURATION_MS 60000
/* Re-check interval while settled (solid-on or already timed-out-off) -
 * purely a backstop in case a disconnect is ever missed by the event
 * listener below. */
#define KEYPAD_RECHECK_CONNECTED_MS 1000

static bool led_is_on;

/* Profile-count cycle state. */
static uint8_t current_profile;
static uint8_t blink_index;
static bool blink_on_phase;
static bool in_pause;
static uint8_t cycles_played;
static bool settled_solid;
static bool led_timeout_reached;
static int64_t solid_start_time;
static bool have_applied_once;

static struct k_work_delayable led_work;

static void set_led(bool on) {
    led_is_on = on;
    gpio_pin_set_dt(&led, on ? 1 : 0);
}

static void start_cycle(void) {
    blink_index = 0;
    blink_on_phase = true;
    in_pause = false;
    set_led(true);
    k_work_reschedule(&led_work, K_MSEC(KEYPAD_PROFILE_BLINK_ON_MS));
}

static void led_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    bool connected = zmk_endpoint_is_connected();
    uint8_t profile = (uint8_t)zmk_ble_active_profile_index();

    bool profile_changed = (profile != current_profile);
    bool fresh_drop = (settled_solid && !connected);
    bool force_restart = !have_applied_once || profile_changed || fresh_drop;

    have_applied_once = true;
    current_profile = profile;

    if (force_restart) {
        cycles_played = 0;
        settled_solid = false;
        led_timeout_reached = false;
        start_cycle();
        return;
    }

    if (settled_solid) {
        /* fresh_drop above already restarts the cycle the instant we're
         * no longer connected, so reaching here means still connected. */
        if (!led_timeout_reached) {
            int64_t elapsed = k_uptime_get() - solid_start_time;
            if (elapsed >= KEYPAD_LED_ON_DURATION_MS) {
                set_led(false);
                led_timeout_reached = true;
                k_work_reschedule(&led_work, K_MSEC(KEYPAD_RECHECK_CONNECTED_MS));
            } else {
                k_work_reschedule(&led_work, K_MSEC(KEYPAD_LED_ON_DURATION_MS - elapsed));
            }
        } else {
            k_work_reschedule(&led_work, K_MSEC(KEYPAD_RECHECK_CONNECTED_MS));
        }
        return;
    }

    if (in_pause) {
        /* A cycle just finished. Only now do we check whether we're
         * allowed to settle solid - never mid-pulse, so a connect event
         * arriving mid-blink can't cut a pulse short and make it
         * unreadable. */
        if (connected && cycles_played >= KEYPAD_PROFILE_MIN_CYCLES) {
            settled_solid = true;
            led_timeout_reached = false;
            solid_start_time = k_uptime_get();
            set_led(true);
            k_work_reschedule(&led_work, K_MSEC(KEYPAD_LED_ON_DURATION_MS));
            return;
        }
        start_cycle();
        return;
    }

    if (blink_on_phase) {
        set_led(false);
        blink_on_phase = false;
        k_work_reschedule(&led_work, K_MSEC(KEYPAD_PROFILE_BLINK_OFF_MS));
        return;
    }

    /* Finished one full pulse (on+off). One cycle = (current_profile + 1)
     * pulses, so profile 0 still blinks once instead of looking like "no
     * signal". */
    if (++blink_index >= (uint8_t)(current_profile + 1)) {
        cycles_played++;
        in_pause = true;
        set_led(false);
        k_work_reschedule(&led_work, K_MSEC(KEYPAD_PROFILE_CYCLE_PAUSE_MS));
        return;
    }

    blink_on_phase = true;
    set_led(true);
    k_work_reschedule(&led_work, K_MSEC(KEYPAD_PROFILE_BLINK_ON_MS));
}

static int keypad_indicator_init(void) {
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("Indicator LED device not ready");
        return -ENODEV;
    }

    int err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (err) {
        LOG_ERR("Failed to configure indicator LED (%d)", err);
        return err;
    }

    k_work_init_delayable(&led_work, led_work_handler);
    k_work_schedule(&led_work, K_NO_WAIT);

    return 0;
}

SYS_INIT(keypad_indicator_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static int keypad_indicator_event_listener(const zmk_event_t *eh) {
    /* Re-evaluate connection state immediately on profile/connection change
     * instead of waiting for the next poll tick. */
    k_work_reschedule(&led_work, K_NO_WAIT);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(keypad_indicator, keypad_indicator_event_listener);
ZMK_SUBSCRIPTION(keypad_indicator, zmk_ble_active_profile_changed);

#endif /* DT_NODE_EXISTS(DT_NODELABEL(indicator_led)) */
