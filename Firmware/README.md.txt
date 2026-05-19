# Unified Firmware Branch

## Overview

This branch contains the firmware versions previously referred to as **WMORE v1.6** and **WMORE v2.0**.

The main objectives of this branch were:

1. Replace the Arduino Nano with the Seeed platform (**WMORE v1.6**)
2. Create a unified firmware architecture capable of running both **logger** and **coordinator** modes on Seeed or Nano (**WMORE v2.0**).

---

# Firmware Structure

## WMORE v1.6

### `WMORE_Openlog_Coordinator`

Coordinator firmware for the OpenLog Artemis (OLA), based on firmware v2.11 from the `firmware_upgrade` branch, including the RTC branch modifications.

### `WMORE_Openlog_Logger`

Logger firmware for the OpenLog Artemis (OLA), based on firmware v2.11 from the `firmware_upgrade` branch, including the RTC branch modifications.

### `Seeed_Logger`

Logger firmware for the Seeed platform, including RTC branch modifications.

### `Seeed_Coordinator`

Coordinator firmware for the Seeed platform, including RTC branch modifications.

---

## WMORE v2.0

### `WMORE_Openlog`

Unified firmware for both logger and coordinator modes on the OpenLog Artemis (OLA), using firmware v2.11 as the base.

### `Seeed`

Unified firmware for both logger and coordinator modes on the Seeed platform.

### `Nano`

Unified firmware for both logger and coordinator modes on the Arduino Nano platform.

---

# Relationship with Other Branches

This branch preserves the modifications introduced in the `RTC` branch.

However, a separate branch was created because the firmware upgrade had already been completed by this stage. Therefore:

- This branch uses **OLA firmware v2.11** as the base.
- The OLA firmware versions were derived from the `firmware_upgrade` branch.
- The RTC-related modifications were manually merged into this branch.

---

# Hardware Connection

For the hardware hookup and wiring connections between the Seeed board and the OpenLog Artemis (OLA), refer to:

```text
WMORE_wiring_diagram_Seeed.png
```

This diagram provides the required pin connections and wiring configuration used by the unified firmware setup.

---

# Unified Firmware Operation

In the unified firmware, the push-button determines whether the device should behave as a **Coordinator** or a **Logger** during runtime.

At startup, the firmware simultaneously:

- Monitors a debounced button press (interrupt-driven)
- Listens for incoming ESB packets

The first detected event determines the device role.

---

## Coordinator Selection

If the local button press occurs first:

1. The device becomes the **Coordinator**
2. A `START` command is transmitted via ESB
3. The firmware enters the coordinator execution loop
4. The device continues:
   - handling debounced button presses
   - transmitting synchronization data

If the button is pressed again:

1. `START` is toggled to `false`
2. A `STOP` command is transmitted via ESB
3. The coordinator loop terminates
4. The device returns to the initial idle state

---

## Logger Selection

If an ESB packet is received first:

1. The device becomes the **Logger**
2. Logging begins
3. The firmware enters the logger execution loop
4. The device continues:
   - listening for ESB instructions
   - handling local button presses

If the local button is pressed:

1. A `STOP` command is transmitted via ESB
2. Logging stops
3. The firmware returns to the idle state

---

# Firmware State Machine

## States

```text
IDLE
COORDINATOR_ACTIVE
LOGGER_ACTIVE
```

---

## Events

```text
EVT_BTN          // Debounced local button press
EVT_ESB_START    // Valid ESB frame with CMD=START
EVT_ESB_STOP     // Valid ESB frame with CMD=STOP
EVT_TICK         // Periodic timer event (e.g. every 10 ms)
```

---

# State Transitions

## IDLE

```text
EVT_BTN
    → COORDINATOR_ACTIVE
    → Send START command once
    → Begin coordinator streaming loop

EVT_ESB_START
    → LOGGER_ACTIVE
    → Begin logging
```

---

## COORDINATOR_ACTIVE

```text
EVT_TICK
    → Read UART (5 bytes)
    → Send ESB START frame / keep-alive
    → Pulse local SYNC signal

EVT_BTN
    → Send ESB STOP
    → Stop local operation
    → Return to IDLE

EVT_ESB_STOP
    → Stop local operation
    → Return to IDLE
```

---

## LOGGER_ACTIVE

```text
EVT_ESB_STOP
    → Stop local operation
    → Return to IDLE

EVT_BTN
    → Local override
    → Send ESB STOP
    → Stop local operation
    → Return to IDLE

Optional:
EVT_ESB_START
    → Continue synchronization updates
```