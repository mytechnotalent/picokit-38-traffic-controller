// MIT License
//
// Copyright (c) 2026 Kevin Thomas
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Author:  Kevin Thomas
// Email:   kevin@mytechnotalent.com
// GitHub:  https://github.com/mytechnotalent/picokit-38-traffic-controller
// File:    monitor.c
// Desc:    Implements the traffic controller state machine with timed phases
//          and a pedestrian request button.
// Created: 2026

#include "picokit_38_traffic_controller.h"
#include "monitor.h"
#include "button.h"
#include "radio.h"
#include "status_led.h"
#include "crypto_aead.h"
#include "crypto_kdf.h"
#include "envelope.h"
#include "field_secrets.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Module-ready flag.
 *
 * Set to true by monitor_init() once the peripherals are configured.
 * monitor_step() returns false while this flag is clear.
 */
static bool g_ready;

/**
 * @brief Current traffic phase index.
 */
static uint8_t g_phase;

/**
 * @brief True while a pedestrian request is pending.
 */
static bool g_ped_requested;

/**
 * @brief Monotonic transmit sequence number.
 */
static uint16_t g_seq;

/**
 * @brief Absolute time in microseconds of the next traffic phase.
 */
static uint64_t g_next_phase_us;

/**
 * @brief Absolute time in microseconds of the next authenticated transmit.
 */
static uint64_t g_next_tx_us;

/**
 * @brief Inbound radio line accumulator.
 */
static char g_rx_line[RADIO_LINE_BUF_LEN];

/**
 * @brief Number of bytes currently held in the inbound line accumulator.
 */
static size_t g_rx_len;

/**
 * @brief Derived XChaCha20-Poly1305 session key for telemetry.
 */
static uint8_t g_key[CRYPTO_AEAD_KEY_LEN];

/**
 * @brief True once the telemetry session key has been derived.
 */
static bool g_key_ready;

/**
 * @brief Annunciator step driven by each traffic phase.
 */
static const uint8_t g_phase_steps[MONITOR_PHASE_COUNT] = { 0u, 2u, 1u, 0u };

/**
 * @brief Duration in milliseconds of each traffic phase.
 */
static const uint16_t g_phase_ms[MONITOR_PHASE_COUNT] = {
    MONITOR_RED_MS, MONITOR_GREEN_MS, MONITOR_YELLOW_MS,
    MONITOR_PEDESTRIAN_MS,
};

/**
 * @brief Configure the onboard heartbeat LED as a dark output.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_state_init_io(void) {
    gpio_init(PICOKIT_38_TRAFFIC_CONTROLLER_LED_PIN);
    gpio_set_dir(PICOKIT_38_TRAFFIC_CONTROLLER_LED_PIN, GPIO_OUT);
    gpio_put(PICOKIT_38_TRAFFIC_CONTROLLER_LED_PIN, 0);
}

/**
 * @brief Reset the phase, pedestrian request, sequence, and timing.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_state_init(void) {
    uint64_t now_us = time_us_64();
    g_phase = MONITOR_PHASE_RED;
    g_ped_requested = false;
    g_seq = 0u;
    g_next_phase_us = now_us;
    g_next_tx_us = now_us + (uint64_t)PICOKIT_38_TRAFFIC_CONTROLLER_TX_INTERVAL_MS * 1000u;
    g_ready = true;
}

/**
 * @brief Derive the telemetry session key from the field secret.
 *
 * LAB-ONLY: production must provision the session key through OTP rather
 * than deriving it from a committed passphrase and salt.
 *
 * @param void No parameters.
 * @return bool true when the session key was derived.
 */
static bool monitor_derive_key(void) {
    bool ok = crypto_kdf_argon2id((const uint8_t *)FIELD_SECRET_PASSPHRASE, strlen(FIELD_SECRET_PASSPHRASE), FIELD_SECRET_SALT, 16u, g_key);
    g_key_ready = ok;
    return ok;
}

/**
 * @brief Print the boot banner for the traffic controller lesson.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_banner(void) {
    printf("=== PICOKIT-38 TRAFFIC CONTROLLER // TIMED PHASES + PED REQUEST ===\n");
}

/**
 * @brief Derive the field key and announce a ready monitor.
 *
 * @param void No parameters.
 * @return bool true when the field key was derived and installed.
 */
static bool monitor_finish(void) {
    bool ok = monitor_derive_key();
    if (ok) {
        monitor_banner();
    }
    return ok;
}

/**
 * @brief Blink the onboard heartbeat LED exactly once.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_heartbeat(void) {
    gpio_put(PICOKIT_38_TRAFFIC_CONTROLLER_LED_PIN, 1);
    sleep_us(MONITOR_HEARTBEAT_BLINK_US);
    gpio_put(PICOKIT_38_TRAFFIC_CONTROLLER_LED_PIN, 0);
    sleep_us(MONITOR_HEARTBEAT_BLINK_US);
}

/**
 * @brief Return the configured duration of a traffic phase.
 *
 * @param phase Traffic phase index.
 * @return uint16_t Phase duration in milliseconds.
 */
static uint16_t monitor_phase_duration(uint8_t phase) {
    return g_phase_ms[phase % MONITOR_PHASE_COUNT];
}

/**
 * @brief Resolve the phase that follows the current one.
 *
 * @param phase Current traffic phase index.
 * @return uint8_t Next traffic phase index.
 */
static uint8_t monitor_next_phase(uint8_t phase) {
    if (phase == MONITOR_PHASE_GREEN) {
        return MONITOR_PHASE_YELLOW;
    }
    if (phase == MONITOR_PHASE_YELLOW) {
        return g_ped_requested ? MONITOR_PHASE_PEDESTRIAN : MONITOR_PHASE_RED;
    }
    if (phase == MONITOR_PHASE_PEDESTRIAN) {
        return MONITOR_PHASE_RED;
    }
    return MONITOR_PHASE_GREEN;
}

/**
 * @brief Advance to the next phase and schedule its duration.
 *
 * @param now_us Current monotonic time in microseconds.
 * @return void
 */
static void monitor_phase_tick(uint64_t now_us) {
    uint8_t next = monitor_next_phase(g_phase);
    g_ped_requested = (next == MONITOR_PHASE_PEDESTRIAN) ? false : g_ped_requested;
    g_phase = next;
    status_led_show_step(g_phase_steps[g_phase]);
    printf("PHASE %u\n", (unsigned)g_phase);
    g_next_phase_us = now_us + (uint64_t)monitor_phase_duration(g_phase) * 1000u;
}

/**
 * @brief Latch a pending pedestrian request from the button.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_poll_button(void) {
    if (button_consume_press()) {
        g_ped_requested = true;
        printf("PED REQUEST\n");
    }
}

/**
 * @brief Format the heartbeat JSON body for the current traffic phase.
 *
 * @param frame Pointer to the mutable frame output buffer.
 * @param frame_len Capacity of the frame output buffer in bytes.
 * @return size_t Number of JSON bytes written, or zero on overflow.
 */
static size_t monitor_build_frame(char *frame, size_t frame_len) {
    int written = snprintf(frame, frame_len, "{\"n\":%u,\"s\":%u,\"p\":%u}", (unsigned)PACKET_NODE_ID, (unsigned)g_seq, (unsigned)g_phase);
    return (written > 0 && (size_t)written < frame_len) ? (size_t)written : 0u;
}

/**
 * @brief Seal the current heartbeat body into a hex envelope.
 *
 * @param hex Pointer to the NUL-terminated hex output buffer.
 * @param hex_len Capacity of the hex output buffer in bytes.
 * @return bool true when the heartbeat was sealed and encoded.
 */
static bool monitor_seal_frame(char *hex, size_t hex_len) {
    char frame[PICOKIT_38_TRAFFIC_CONTROLLER_FRAME_SIZE];
    uint8_t nonce[ENVELOPE_NONCE_LEN];
    uint8_t ad = (uint8_t)PACKET_NODE_ID;
    size_t frame_len = monitor_build_frame(frame, sizeof(frame));
    envelope_fill_nonce(nonce);
    return envelope_seal_hex(g_key, nonce, &ad, 1u, (const uint8_t *)frame, frame_len, hex, hex_len);
}

/**
 * @brief Build and transmit the authenticated heartbeat frame.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_transmit(void) {
    char hex[ENVELOPE_MAX_HEX_LEN];
    if (!g_key_ready) {
        return;
    }
    if (monitor_seal_frame(hex, sizeof(hex))) {
        radio_send_frame(PICOKIT_38_TRAFFIC_CONTROLLER_UART, (const uint8_t *)hex, strlen(hex));
        g_seq += 1u;
    }
}

/**
 * @brief Print one console line for the current heartbeat transmit.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_log_tx(void) {
    printf("PHASE %u seq=%u\n", (unsigned)g_phase, (unsigned)g_seq);
}

/**
 * @brief Transmit one heartbeat and schedule the next transmit.
 *
 * @param now_us Current monotonic time in microseconds.
 * @return void
 */
static void monitor_tx_tick(uint64_t now_us) {
    monitor_transmit();
    monitor_heartbeat();
    monitor_log_tx();
    g_next_tx_us = now_us + (uint64_t)PICOKIT_38_TRAFFIC_CONTROLLER_TX_INTERVAL_MS * 1000u;
}

/**
 * @brief Drain inbound radio lines and log every valid +RCV report.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_rx_tick(void) {
    radio_rcv_t rcv;
    while (radio_line_pump(PICOKIT_38_TRAFFIC_CONTROLLER_UART, g_rx_line, &g_rx_len)) {
        if (radio_parse_rcv(g_rx_line, &rcv) == RADIO_RESULT_OK) {
            printf("RX from 0x%04X, %u bytes\n", (unsigned)rcv.sender, (unsigned)rcv.len);
        }
    }
}

/**
 * @brief Service the traffic phase and heartbeat transmit timers.
 *
 * @param now_us Current monotonic time in microseconds.
 * @return void
 */
static void monitor_service_timers(uint64_t now_us) {
    if (now_us >= g_next_phase_us) {
        monitor_phase_tick(now_us);
    }
    if (now_us >= g_next_tx_us) {
        monitor_tx_tick(now_us);
    }
}

bool monitor_init(void) {
    bool ok;
    ok = status_led_init() && button_init() && radio_init(PICOKIT_38_TRAFFIC_CONTROLLER_UART);
    monitor_state_init_io();
    monitor_state_init();
    return ok && monitor_finish();
}

void monitor_deinit(void) {
    g_ready = false;
}

bool monitor_step(void) {
    uint64_t now_us;
    if (!g_ready) {
        return false;
    }
    now_us = time_us_64();
    monitor_poll_button();
    monitor_service_timers(now_us);
    monitor_rx_tick();
    return true;
}
