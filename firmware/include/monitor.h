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
// File:    monitor.h
// Desc:    Declares the traffic controller state machine with timed phases
//          and a pedestrian request button.
// Created: 2026

#ifndef MONITOR_H
#define MONITOR_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Onboard heartbeat LED on and off time in microseconds.
 */
#define MONITOR_HEARTBEAT_BLINK_US 50000u

/**
 * @brief Traffic phase that holds the red lamp.
 */
#define MONITOR_PHASE_RED 0u

/**
 * @brief Traffic phase that holds the green lamp.
 */
#define MONITOR_PHASE_GREEN 1u

/**
 * @brief Traffic phase that holds the yellow lamp.
 */
#define MONITOR_PHASE_YELLOW 2u

/**
 * @brief Pedestrian phase that holds the red lamp for a walk window.
 */
#define MONITOR_PHASE_PEDESTRIAN 3u

/**
 * @brief Number of traffic phases in one controller cycle.
 */
#define MONITOR_PHASE_COUNT 4u

/**
 * @brief Red phase duration in milliseconds.
 */
#define MONITOR_RED_MS 3000u

/**
 * @brief Green phase duration in milliseconds.
 */
#define MONITOR_GREEN_MS 4000u

/**
 * @brief Yellow phase duration in milliseconds.
 */
#define MONITOR_YELLOW_MS 2000u

/**
 * @brief Pedestrian phase duration in milliseconds.
 */
#define MONITOR_PEDESTRIAN_MS 5000u

/**
 * @brief Initialize the traffic controller monitoring state machine.
 *
 * Configures the annunciator LEDs, the pedestrian button, the onboard
 * heartbeat LED, and the RYLR998 UART, derives the field key, and resets
 * the phase and sequence counters.
 *
 * @param void No parameters.
 * @return bool true when all submodules initialized.
 */
bool monitor_init(void);

/**
 * @brief Clear the monitor-ready flag.
 *
 * Test and recovery hook that returns the state machine to the
 * uninitialized policy state.
 *
 * @param void No parameters.
 * @return void
 */
void monitor_deinit(void);

/**
 * @brief Execute one monitor state-machine tick.
 *
 * Advances the timed traffic phase, samples the pedestrian button, transmits
 * the authenticated heartbeat frame on the telemetry interval, and pumps
 * inbound +RCV lines.
 *
 * @param void No parameters.
 * @return bool true when the tick completed without a policy error.
 */
bool monitor_step(void);

#endif // MONITOR_H
