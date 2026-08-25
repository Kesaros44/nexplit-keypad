/*
 * Keypad status LED indicator
 *
 * - BLE connected:      LED steady on
 * - BLE not connected:  LED blinks, toggling every 500 ms
 *
 * Uses the `indicator_led` GPIO defined in keypad.overlay (the same LED
 * previously driven by zmk-poor-mans-led-indicator). That widget's BLE
 * indication is disabled in keypad.conf so it no longer drives this pin.
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
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>

LOG_MODULE_REGISTER(keypad_indicator, CONFIG_ZMK_LOG_LEVEL);

#define LED_NODE DT_NODELABEL(indicator_led)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

/* Blink interval while not connected. */
#define BLINK_INTERVAL_DISCONNECTED_MS 500
/* Re-check interval while connected (LED just stays on; this only guards
 * against missing a disconnect event). */
#define RECHECK_INTERVAL_CONNECTED_MS 1000

static bool led_is_on = false;
static struct k_work_delayable led_work;

static void set_led(bool on) {
    led_is_on = on;
    gpio_pin_set_dt(&led, on ? 1 : 0);
}

static void led_work_handler(struct k_work *work) {
    bool connected = zmk_ble_active_profile_is_connected();

    if (connected) {
        if (!led_is_on) {
            set_led(true);
        }
        k_work_schedule(&led_work, K_MSEC(RECHECK_INTERVAL_CONNECTED_MS));
    } else {
        set_led(!led_is_on);
        k_work_schedule(&led_work, K_MSEC(BLINK_INTERVAL_DISCONNECTED_MS));
    }
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
