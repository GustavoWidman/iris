#pragma once
#include "led_calibration.h"

// Firmware build flag only; no persisted profile, runtime toggles or hidden sweep.
#ifndef IRIS_EXPERIMENTAL_RELATIVE_BALANCE
#define IRIS_EXPERIMENTAL_RELATIVE_BALANCE 0
#endif
#if IRIS_EXPERIMENTAL_RELATIVE_BALANCE != 0 && IRIS_EXPERIMENTAL_RELATIVE_BALANCE != 1
#error "IRIS_EXPERIMENTAL_RELATIVE_BALANCE must be 0 or 1"
#endif
inline LedCalibration firmware_led_calibration() {
 LedCalibration calibration{};
#if IRIS_EXPERIMENTAL_RELATIVE_BALANCE
 calibration.profile=LedColorProfile::ExperimentalRelativeBalance;
#endif
 return calibration;
}
