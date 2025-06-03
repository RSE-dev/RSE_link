
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
    PC_HOST_ID = 0,
    RSE_LEX_MASTERCOM_CH1_ID = 1,
    RSE_LEX_MASTERCOM_CH2_ID = 2
} RSE_SYSTEM_ID;

/* ComponentID ranges: */
/* 0x00–0x1F: Preamp Parameters */
/* 0x20–0x3F: Compressor Parameters */
/* 0x40–0x5F: Equalizer Parameters */
/* 0x60–0x7F: Effects (Delay, Reverb, etc.) */
/* 0x80–0x9F: Modulation Effects */
/* 0xA0–0xBF: Routing & Patchbay Settings */
/* 0xC0–0xDF: Dynamics (Expander, Gate, Limiter) */
/* 0xE0–0xEF: Real-Time Meters & Status */
/* 0xF0–0xFF: Service & Identification */

typedef enum {
    /* Preamp */
    PREAMP_GAIN      = 0x00,
    PREAMP_IMPEDANCE = 0x01,
    PREAMP_PHANTOM   = 0x02,
    PREAMP_PAN       = 0x03,

    /* Compressor */
    COMP_THRESHOLD   = 0x20,
    COMP_RATIO       = 0x21,
    COMP_ATTACK      = 0x22,
    COMP_RELEASE     = 0x23,
    COMP_MAKEUP      = 0x24,
    COMP_MODE        = 0x25,
    COMP_SIDECHAIN   = 0x26,
    COMP_GAIN_IN     = 0x27,
    COMP_GAIN_OUT     = 0x28,
    COMP_MIX         = 0x29,

    /* Equalizer */
    EQ_BAND1_FREQ    = 0x40,
    EQ_BAND1_GAIN    = 0x41,
    EQ_BAND1_Q       = 0x42,
    EQ_BAND2_FREQ    = 0x43,
    EQ_BAND2_GAIN    = 0x44,
    EQ_BAND2_Q       = 0x45,
    EQ_BAND3_FREQ    = 0x46,
    EQ_BAND3_GAIN    = 0x47,
    EQ_BAND3_Q       = 0x48,

    /* Effects */
    FX_DELAY_TIME    = 0x60,
    FX_DELAY_FEEDBACK= 0x61,
    FX_REVERB_SIZE   = 0x62,
    FX_REVERB_DECAY  = 0x63,
    FX_WET_DRY       = 0x64,

    /* Real-Time Meters & Status */
    METER_INPUT_LEVEL  = 0xE0,
    METER_OUTPUT_LEVEL = 0xE1,
    METER_COMPRESSION  = 0xE2,
    STATUS_TEMPERATURE = 0xE3,

    /* Service & Identification */
    HOST_LINE_0      = 0xF0,
    DEVICE_ID        = 0xF1,
    FIRMWARE_VERSION = 0xF2,
    POWER_OFF = 0xF3,
    POWER_ON = 0xF4,
} RSE_COMPONENT_ID;

typedef enum {
    READ_VALUE_MSG         = 0,
    SET_VALUE_MSG          = 1,
    UPDATE_VALUE_MSG       = 2,
    RESPONSE               = 3,
    IDENTIFY_DEVICE_MSG    = 4
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

uint8_t RSE_link_processInputData(uint32_t data);
uint32_t RSE_link_processOutputMessage(RSE_PACKET packet);
uint8_t RSE_link_respondResult(uint8_t success);
uint8_t RSE_link_respondValue(uint8_t channel, uint8_t value);

#endif /* INC_RSELINK_H_ */
