#include "test.h"

int main(void) {
    test_decode_encode();

    test_error_decode_crc8();
    test_error_decode_version();
    test_error_encode_version();
    test_error_decode_device_type();
    test_error_encode_device_type();
    test_error_decode_battery_above_maximal();
    test_error_encode_battery_above_maximal();
    test_error_decode_battery_below_minimal();
    test_error_encode_battery_below_minimal();
    test_error_decode_temperature_above_maximal();
    test_error_encode_temperature_above_maximal();
    test_error_decode_temperature_below_minimal();
    test_error_encode_temperature_below_minimal();
    test_error_decode_flow_exponent_invalid();
    test_error_encode_flow_exponent_invalid();
    test_error_decode_reserved();
    test_error_encode_reserved();

    test_flags_combinations();

    return 0;
}