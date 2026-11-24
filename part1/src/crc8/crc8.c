#include "crc8.h"

/**
 * @brief Reflects the bit order of an 8-bit value.
 */
static uint8_t crc8_reflect(uint8_t value) {
    uint8_t reflected = 0;

    for (uint8_t bit = 0; bit < 8; bit++) {
        reflected = (uint8_t) ((reflected << 1) | (value & 0x01));
        value >>= 1;
    }

    return reflected;
}

crc8_value_t crc8_compute(
    crc8_config_t config, const uint8_t *bytes, uint8_t num_bytes
) {
    uint8_t crc = config.init_value;

    for (uint16_t i_byte = 0; i_byte < num_bytes; i_byte++) {
        uint8_t input = 0;
        if (config.reflection_in == true) {
            input = crc8_reflect(bytes[i_byte]);
        } else {
            input = bytes[i_byte];
        }

        crc ^= input;

        for (uint8_t bit = 0; bit < 8; bit++) {
            if ((crc & 0x80) != 0) {
                crc = (uint8_t) ((crc << 1) ^ config.polynomial);
            } else {
                crc = (uint8_t) ((crc << 1));
            }
        }
    }

    if (config.reflection_out == true) {
        crc = crc8_reflect(crc);
    }

    return (crc8_value_t) (crc ^ config.xor_out);
}
