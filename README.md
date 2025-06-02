
# RSE USB Control Protocol

**Version 2.0**

This repository contains the specification and reference implementation of a generalized USB-based control protocol for studio effect devices—such as preamps, compressors, equalizers, delay/reverb units, and more—connecting them to a host PC.  

---

## Table of Contents

1. [Overview](#overview)  
2. [Packet Structure](#packet-structure)  
3. [Field Descriptions](#field-descriptions)  
   - [ProtocolVersion](#protocolversion)  
   - [SystemID](#systemid)  
   - [PacketSequence](#packetsequence)  
   - [MessageID](#messageid)  
   - [ComponentID (Parameter Identifiers)](#componentid-parameter-identifiers)  
   - [MessageValue](#messagevalue)  
4. [CRC4 Calculation](#crc4-calculation)  
5. [Typical Command Sequences](#typical-command-sequences)  
   - [Device Identification](#device-identification)  
   - [Setting a Compressor Parameter](#setting-a-compressor-parameter)  
   - [Reading an Equalizer Band Gain](#reading-an-equalizer-band-gain)  
   - [Real-Time Metering (33 Hz)](#real-time-metering-33hz)  
6. [Error Handling & Codes](#error-handling--codes)  
7. [ComponentID Extension Guidelines](#componentid-extension-guidelines)  
8. [Implementation Notes](#implementation-notes)  
9. [Source Files](#source-files)  

---

## 1. Overview

This protocol defines a compact, 32-bit message format for bidirectional control and status-monitoring between a host PC (USB “host”) and a variety of studio effect devices (USB “device”). By standardizing on fixed-size packets and an extensible `ComponentID` namespace, it can cover:

- **Preamps** (gain, impedance, phantom power, pan)  
- **Compressors** (threshold, ratio, attack, release, makeup, mode)  
- **Equalizers** (three-band parametric/graphic)  
- **Effects** (delay, reverb, wet/dry mix, feedback, etc.)  
- **Real-Time Meters** (input level, output level, compression amount, temperature)  
- **Service & Identification** (device ID, firmware version, host-service channel)

By following the guidelines below, new device types or specialized parameters can be added without altering the core packet format.

---

## 2. Packet Structure

All messages are exactly **32 bits** (4 bytes). Bit-fields are packed as follows:

```
┌───────────────────────────────────────────────────────────┐
| 0–2   | ProtocolVersion  | Protocol version (currently 1) |  3 bits |
| 3–4   | SystemID         | System ID (PC/device)           |  2 bits |
| 5–9   | PacketSequence   | Packet sequence number          |  5 bits |
| 10–17 | ComponentID      | Component/parameter identifier  |  8 bits |
| 18–19 | MessageID        | Message type                    |  2 bits |
| 20–27 | MessageValue     | Payload value (0–255)           |  8 bits |
| 28–31 | CheckSum         | CRC4 over lower 28 bits         |  4 bits |
└───────────────────────────────────────────────────────────┘
```

- **Total size**: 32 bits (4 bytes).  
- **Endianess**: Transmitted in big-endian bit order (MSB first).

---

## 3. Field Descriptions

### ProtocolVersion

- **Bit 0–2**  
- Always set to `1` for this version of the protocol. In future revisions, increment this field.

```c
typedef enum {
  RSE_LINK_VER_1 = 1
} RSE_LINK_VER;
```

---

### SystemID

- **Bit 3–4**  
- Specifies the origin or destination of the packet:

| Value | Name                        | Description            |
|:-----:|-----------------------------|------------------------|
| `0`   | PC_HOST_ID                  | Host PC (USB host)     |
| `1`   | RSE_LEX_MASTERCOM_ID        | Generic studio device  |

*(Additional values can be reserved for specific manufacturers or device families.)*

```c
typedef enum {
  PC_HOST_ID           = 0,
  RSE_LEX_MASTERCOM_ID = 1
} RSE_SYSTEM_ID;
```

---

### PacketSequence

- **Bit 5–9**  
- A 5-bit sequence number (0–31).  
- For single-packet transactions (most parameter reads/writes), set to `0`.  
- For multi-packet transfers (e.g., bulk or firmware updates), increment mod 32 on each subsequent packet.

---

### MessageID

- **Bit 18–19**  
- Indicates the type of operation:

| Value | Enum                   | Description                          |
|:-----:|------------------------|--------------------------------------|
| `0`   | `READ_VALUE_MSG`       | Host → Device: Request current value |
| `1`   | `SET_VALUE_MSG`        | Host → Device: Set new value         |
| `2`   | `UPDATE_VALUE_MSG`     | Device → Host: Inform of real-time change (metering) |
| `3`   | `RESPONSE`             | Device → Host: Acknowledge or return value |
| `4`   | `IDENTIFY_DEVICE_MSG`  | Host → Device: Request device ID      |

```c
typedef enum {
  READ_VALUE_MSG        = 0,
  SET_VALUE_MSG         = 1,
  UPDATE_VALUE_MSG      = 2,
  RESPONSE              = 3,
  IDENTIFY_DEVICE_MSG   = 4
} RSE_MSG_TYPE_ID;
```

---

### ComponentID (Parameter Identifiers)

- **Bit 10–17** (8 bits)  
- Extensible “namespace” for all device parameters, meters, effects, and service codes.  
- Partitioned by function/category.  

#### 0x00–0x1F | Preamp Parameters

| ID   | Name               | Description                              | MessageValue Interpretation           |
|:----:|--------------------|------------------------------------------|----------------------------------------|
| 0x00 | `PREAMP_GAIN`      | Input gain                               | 0–255 mapped to 0 dB … +60 dB (device-specific scaling) |
| 0x01 | `PREAMP_IMPEDANCE` | Input impedance select                   | 0 = Lo-Z, 1 = Hi-Z                      |
| 0x02 | `PREAMP_PHANTOM`   | Phantom power enable/disable             | 0 = Off, 1 = On                        |
| 0x03 | `PREAMP_PAN`       | Pan position left/right                  | 0 = Full Left, 128 = Center, 255 = Full Right |

*(IDs 0x04–0x1F are reserved for future preamp features.)*

#### 0x20–0x3F | Compressor Parameters

| ID   | Name              | Description                              | MessageValue Interpretation           |
|:----:|-------------------|------------------------------------------|----------------------------------------|
| 0x20 | `COMP_THRESHOLD`  | Threshold level                          | 0–255 mapped to –60 dB … 0 dB (device-specific) |
| 0x21 | `COMP_RATIO`      | Ratio                                    | 0 = 1:1, 128 = 4:1, 255 = 20:1 (device-specific) |
| 0x22 | `COMP_ATTACK`     | Attack time                              | 0–255 mapped to 0.1 ms … 100 ms        |
| 0x23 | `COMP_RELEASE`    | Release time                             | 0–255 mapped to 10 ms … 1 s           |
| 0x24 | `COMP_MAKEUP`     | Makeup gain                              | 0–255 mapped to 0 dB … +20 dB         |
| 0x25 | `COMP_MODE`       | Compressor mode/type                     | 0 = Opto, 1 = VCA, 2 = FET …           |

*(IDs 0x26–0x3F reserved for future compressor features.)*

#### 0x40–0x5F | Equalizer Parameters

**Band 1 (Low Shelf)**

| ID   | Name           | Description                              | MessageValue Interpretation           |
|:----:|----------------|------------------------------------------|----------------------------------------|
| 0x40 | `EQ_BAND1_FREQ`| Center frequency                         | 0–255 mapped to 20 Hz … 500 Hz         |
| 0x41 | `EQ_BAND1_GAIN`| Gain                                     | 0–255 mapped to –12 dB … +12 dB       |
| 0x42 | `EQ_BAND1_Q`   | Q factor                                 | 0–255 mapped to 0.3 … 10             |

**Band 2 (Mid Peak)**

| ID   | Name           | Description                              | MessageValue Interpretation           |
|:----:|----------------|------------------------------------------|----------------------------------------|
| 0x43 | `EQ_BAND2_FREQ`| Center frequency                         | 0–255 mapped to 200 Hz … 5 kHz        |
| 0x44 | `EQ_BAND2_GAIN`| Gain                                     | 0–255 mapped to –12 dB … +12 dB       |
| 0x45 | `EQ_BAND2_Q`   | Q factor                                 | 0–255 mapped to 0.3 … 10             |

**Band 3 (High Shelf)**

| ID   | Name           | Description                              | MessageValue Interpretation           |
|:----:|----------------|------------------------------------------|----------------------------------------|
| 0x46 | `EQ_BAND3_FREQ`| Center frequency                         | 0–255 mapped to 1 kHz … 20 kHz        |
| 0x47 | `EQ_BAND3_GAIN`| Gain                                     | 0–255 mapped to –12 dB … +12 dB       |
| 0x48 | `EQ_BAND3_Q`   | Q factor                                 | 0–255 mapped to 0.3 … 10             |

*(IDs 0x49–0x5F reserved for future EQ features.)*

#### 0x60–0x7F | Effects (Delay, Reverb, etc.)

| ID   | Name               | Description                              | MessageValue Interpretation           |
|:----:|--------------------|------------------------------------------|----------------------------------------|
| 0x60 | `FX_DELAY_TIME`    | Delay time                               | 0–255 mapped to 0 ms … 2000 ms        |
| 0x61 | `FX_DELAY_FEEDBACK`| Delay feedback level                     | 0–255 mapped to 0 % … 95 %            |
| 0x62 | `FX_REVERB_SIZE`   | Reverb algorithm size                    | 0–255 mapped to small … large hall     |
| 0x63 | `FX_REVERB_DECAY`  | Reverb decay time                        | 0–255 mapped to 0.1 s … 10 s          |
| 0x64 | `FX_WET_DRY`       | Wet/dry mix balance                      | 0–255 mapped to 0 % dry … 100 % wet   |

*(IDs 0x65–0x7F reserved for future effect features.)*

#### 0x80–0x9F | Modulation Effects (Chorus, Flanger, Phaser, etc.)

*(For expandability—IDs and parameters defined per device.)*

#### 0xA0–0xBF | Routing & Patchbay Settings

*(For devices offering digital patch routing/matrix.)*

#### 0xC0–0xDF | Dynamics (Gate, Expander, Limiter, etc.)

*(Reserved for other dynamics beyond compressor.)*

#### 0xE0–0xEF | Real-Time Meters & Status

| ID   | Name               | Description                          | Value Interpretation         |
|:----:|--------------------|--------------------------------------|------------------------------|
| 0xE0 | `METER_INPUT_LEVEL`| Input level metering                 | 0–255 = –Infinity … 0 dBFS   |
| 0xE1 | `METER_OUTPUT_LEVEL`| Output level metering               | 0–255 = –Infinity … 0 dBFS   |
| 0xE2 | `METER_COMPRESSION`| Compression amount metering          | 0–255 scaled to 0 … 100 %    |
| 0xE3 | `STATUS_TEMPERATURE`| Internal temperature (°C)            | 0–255 mapped to 0 … 255 °C  |

#### 0xF0–0xFF | Service & Identification

| ID   | Name               | Description                              | MessageValue Interpretation           |
|:----:|--------------------|------------------------------------------|----------------------------------------|
| 0xF0 | `HOST_LINE_0`      | Service/ACK channel                      | See [Error Codes](#error-handling--codes) |
| 0xF1 | `DEVICE_ID`        | Device type identifier                   | Each device assigns a unique ID (e.g., 0x10 = “LEX Tube Compressor”) |
| 0xF2 | `FIRMWARE_VERSION` | Firmware version (major/minor nibble)    | High nibble = major, low nibble = minor |
---

### MessageValue

- **8-bit unsigned** (0–255).  
- Interpretation depends on `ComponentID`:  
  - **Continuous parameters** (gain, frequency, time, level) are linearly or logarithmically mapped per device.  
  - **Discrete selectors** (mode, phantom on/off) use enumerated values (e.g., 0/1).  
  - **Identification fields** (DEVICE_ID, FIRMWARE_VERSION) use defined numeric codes or hex-packed versions.  

---

## 4. CRC4 Calculation

- **Polynomial**: `0b10111`  
- **Procedure**: Compute CRC4 over the 28 least significant bits (`ProtocolVersion` → `MessageValue`).  
- **Result**: 4-bit CRC stored in bits 28–31.  

Pseudo-code (C-style):

```c
uint8_t compute_crc4(uint32_t packet_bits28) {
    // `packet_bits28` = packet & 0x0FFFFFFF
    return crc4(0, packet_bits28, 28); // Using polynomial 0b10111
}
```

---

## 5. Typical Command Sequences

### 5.1 Device Identification

**PC → Device** (Identify request)

| Packet Field     | Value                                     |
|:-------------------------|:------------------------------------------|
| ProtocolVersion  | `1`                                       |
| SystemID         | `RSE_LEX_MASTERCOM_ID (1)`                |
| PacketSequence   | `0`                                       |
| ComponentID      | `DEVICE_ID (0xF1)`                        |
| MessageID        | `IDENTIFY_DEVICE_MSG (4)`                 |
| MessageValue     | `0`                                       |
| CheckSum         | `CRC4 over first 28 bits`                 |

**Device → PC** (Identify response)

| Packet Field     | Value                                  |
|:-----------------|:---------------------------------------|
| ProtocolVersion  | `1`                                    |
| SystemID         | `PC_HOST_ID (0)`                       |
| PacketSequence   | `0`                                    |
| ComponentID      | `DEVICE_ID (0xF1)`                     |
| MessageID        | `RESPONSE (3)`                         |
| MessageValue     | `0x10` (e.g., “LEX Tube Compressor”)  |
| CheckSum         | `CRC4`                                 |

---

### 5.2 Setting a Compressor Parameter

**PC → Device** (Set threshold to 128)

| Packet Field     | Value                                   |
|:-----------------|:----------------------------------------|
| ProtocolVersion  | `1`                                      |
| SystemID         | `RSE_LEX_MASTERCOM_ID (1)`               |
| PacketSequence   | `0`                                      |
| ComponentID      | `COMP_THRESHOLD (0x20)`                  |
| MessageID        | `SET_VALUE_MSG (1)`                      |
| MessageValue     | `128`                                    |
| CheckSum         | `CRC4`                                   |

**Device → PC** (ACK)

| Packet Field     | Value                                     |
|:-----------------|:------------------------------------------|
| ProtocolVersion  | `1`                                        |
| SystemID         | `PC_HOST_ID (0)`                           |
| PacketSequence   | `0`                                        |
| ComponentID      | `HOST_LINE_0 (0xF0)`                       |
| MessageID        | `RESPONSE (3)`                             |
| MessageValue     | `RSE_COMMAND_DONE (0)`                     |
| CheckSum         | `CRC4`                                     |

---

### 5.3 Reading an Equalizer Band Gain

**PC → Device** (Read EQ band 2 gain)

| Packet Field     | Value                                  |
|:-----------------|:---------------------------------------|
| ProtocolVersion  | `1`                                     |
| SystemID         | `RSE_LEX_MASTERCOM_ID (1)`              |
| PacketSequence   | `0`                                     |
| ComponentID      | `EQ_BAND2_GAIN (0x44)`                  |
| MessageID        | `READ_VALUE_MSG (0)`                    |
| MessageValue     | `0`                                     |
| CheckSum         | `CRC4`                                  |

**Device → PC** (Response: gain = 150)

| Packet Field     | Value                                   |
|:-----------------|:----------------------------------------|
| ProtocolVersion  | `1`                                      |
| SystemID         | `PC_HOST_ID (0)`                         |
| PacketSequence   | `0`                                      |
| ComponentID      | `EQ_BAND2_GAIN (0x44)`                   |
| MessageID        | `RESPONSE (3)`                           |
| MessageValue     | `150`                                    |
| CheckSum         | `CRC4`                                   |

---

### 5.4 Real-Time Metering (33 Hz)

**Device → PC** (Example: input level meter)

Sent every ~30 ms (33 Hz).

| Packet Field     | Value                                         |
|:-----------------|:----------------------------------------------|
| ProtocolVersion  | `1`                                            |
| SystemID         | `PC_HOST_ID (0)`                               |
| PacketSequence   | `n` (incremented 0→31)                        |
| ComponentID      | `METER_INPUT_LEVEL (0xE0)`                     |
| MessageID        | `UPDATE_VALUE_MSG (2)`                         |
| MessageValue     | Current input level (0–255)                    |
| CheckSum         | `CRC4`                                         |

Similarly, separate packets can be sent for:
- `METER_OUTPUT_LEVEL (0xE1)`  
- `METER_COMPRESSION (0xE2)`  

---

## 6. Error Handling & Codes

When a command cannot be executed, the device responds on `HOST_LINE_0 (0xF0)` using a `RESPONSE` packet. The `MessageValue` field is set to one of the following codes:

| Code | Enum                        | Description                          |
|:----:|-----------------------------|--------------------------------------|
| `0`  | `RSE_COMMAND_DONE`          | Success                              |
| `1`  | `RSE_COMMAND_FAILURE`       | General failure                      |
| `2`  | `RSE_COMMAND_UNSUPPORTED`   | Command not supported by this device |
| `3`  | `CRC_ERROR`                 | Checksum mismatch                    |
| `4`  | `UNKNOWN_COMMAND`           | Invalid `MessageID`                  |
| `5`  | `INVALID_VALUE`             | `MessageValue` out of allowed range  |

---

## 7. ComponentID Extension Guidelines

If your device supports parameters or features not defined above, follow these rules:

1. **Choose an unused range** for new categories:  
   - `0x80–0x9F`: Modulation Effects (chorus, flanger, phaser, etc.)  
   - `0xA0–0xBF`: Routing & Patchbay Settings (matrix routing, digital I/O)  
   - `0xC0–0xDF`: Other Dynamics (gate, expander, limiter)  

2. **Assign unique `ComponentID` values** in that range.  
3. **Define `MessageValue` encoding** precisely (e.g., 0–255 → XY parameters).  
4. **Ensure backward compatibility**: If the host requests an unknown `ComponentID`, respond with `RSE_COMMAND_UNSUPPORTED`.  

Document any new `ComponentID`s and their meaning in a device-specific appendix or README.

---

## 8. Implementation Notes

- **Endianess & Packing**  
  - Pack bits exactly as shown in [Packet Structure](#packet-structure).  
  - Transmit the 32-bit word with MSB first (bit 31 sent last).  

- **Timing & Flow Control**  
  - Host should wait for a `RESPONSE` (for `SET`/`READ`/`IDENTIFY`) before sending another command.  
  - Real-time `UPDATE_VALUE_MSG` packets (metering) may be sent continuously; host software should queue or buffer as needed.  

- **Firmware Version Reporting**  
  - Use `FIRMWARE_VERSION (0xF2)` to return major/minor version:  
    - High nibble (bits 4–7 of `MessageValue`) = major version.  
    - Low nibble (bits 0–3 of `MessageValue`) = minor version.  

- **Sequence Numbers**  
  - For single-packet commands, set `PacketSequence = 0`.  
  - For multi-packet transfers (e.g., firmware chunks), increment `PacketSequence` mod 32 on each subsequent packet, and check on the receiving side for missing or out-of-order packets.  

- **Data Ranges**  
  - All continuous parameter mappings (e.g., 0→255 range) must be documented per device firmware.  
  - For discrete parameters, reserve exact numeric codes (e.g., 0 = Off`, `1 = On`).  

---

## 9. Source Files

This repository includes a reference implementation for an STM32-based device:

```
/rseLink.h       – Protocol definitions (version, enums, structs)
/rseLink.c       – Packet parsing, CRC checking, response logic
/crc4.h, crc4.c  – 4-bit CRC calculation routines
/README.md       – This protocol documentation (you are here)
```

### Example: `rseLink.h` (excerpt)

```c
#ifndef INC_RSELINK_H_
#define INC_RSELINK_H_

#include <stdint.h>

typedef enum { RSE_LINK_VER_1 = 1 } RSE_LINK_VER;
typedef enum {
  MESSAGE_IS_EMPTY = 0,
  MESSAGE_NOT_EMPTY
} RSE_MESSAGE_EMPTY;

typedef enum {
  RSE_COMMAND_DONE = 0,
  RSE_COMMAND_FAILURE,
  RSE_COMMAND_UNSUPPORTED,
  CRC_ERROR,
  UNKNOWN_COMMAND,
  INVALID_VALUE
} RSE_COMMAND_RESULT;

typedef enum {
  PC_HOST_ID           = 0,
  RSE_LEX_MASTERCOM_ID = 1
} RSE_SYSTEM_ID;

// ComponentID ranges and definitions (preamp, compressor, EQ, effects, meters, service)

typedef enum {
  PREAMP_GAIN       = 0x00,
  PREAMP_IMPEDANCE  = 0x01,
  PREAMP_PHANTOM    = 0x02,
  PREAMP_PAN        = 0x03,
  COMP_THRESHOLD    = 0x20,
  COMP_RATIO        = 0x21,
  COMP_ATTACK       = 0x22,
  COMP_RELEASE      = 0x23,
  COMP_MAKEUP       = 0x24,
  COMP_MODE         = 0x25,
  EQ_BAND1_FREQ     = 0x40,
  EQ_BAND1_GAIN     = 0x41,
  EQ_BAND1_Q        = 0x42,
  EQ_BAND2_FREQ     = 0x43,
  EQ_BAND2_GAIN     = 0x44,
  EQ_BAND2_Q        = 0x45,
  EQ_BAND3_FREQ     = 0x46,
  EQ_BAND3_GAIN     = 0x47,
  EQ_BAND3_Q        = 0x48,
  FX_DELAY_TIME     = 0x60,
  FX_DELAY_FEEDBACK = 0x61,
  FX_REVERB_SIZE    = 0x62,
  FX_REVERB_DECAY   = 0x63,
  FX_WET_DRY        = 0x64,
  METER_INPUT_LEVEL  = 0xE0,
  METER_OUTPUT_LEVEL = 0xE1,
  METER_COMPRESSION  = 0xE2,
  STATUS_TEMPERATURE = 0xE3,
  HOST_LINE_0       = 0xF0,
  DEVICE_ID         = 0xF1,
  FIRMWARE_VERSION  = 0xF2
} RSE_COMPONENT_ID;

typedef enum {
  READ_VALUE_MSG       = 0,
  SET_VALUE_MSG        = 1,
  UPDATE_VALUE_MSG     = 2,
  RESPONSE             = 3,
  IDENTIFY_DEVICE_MSG  = 4
} RSE_MSG_TYPE_ID;

typedef struct {
  uint32_t ProtocolVersion : 3;
  uint32_t SystemID        : 2;
  uint32_t PacketSequence  : 5;
  uint32_t ComponentID     : 8;
  uint32_t MessageID       : 2;
  uint32_t MessageValue    : 8;
  uint32_t CheckSum        : 4;
} RSE_PACKET;

uint8_t  RSE_link_processInputData(uint32_t data);
uint32_t RSE_link_processOutputMessage(RSE_PACKET packet);
uint8_t  RSE_link_respondResult(uint8_t success);
uint8_t  RSE_link_respondValue(uint8_t channel, uint8_t value);

#endif /* INC_RSELINK_H_ */
```

---

## License

This protocol specification and reference implementation are released under the **MIT License**. See [LICENSE](LICENSE) for details.  

---

_Remark: Version 2.0 is designed to be extensible across a broad range of studio effect units. For any questions or contributions, please open an issue or submit a pull request._
