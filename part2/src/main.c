#include "test.h"

int main(void) {
    test_flash_failure_injection();
    test_flash_initialization_errors();
    test_queue_failed_ack_during_erase();
    test_queue_failed_ack_during_program();
    test_queue_failed_push_during_erase();
    test_queue_failed_push_during_program();
    test_queue_failed_push_before_commit();
    test_queue_failed_push_during_commit();
    test_queue_flash_insufficient();
    test_queue_full_error();
    test_queue_initialization_empty();
    test_queue_initialization_errors();
    test_queue_persistence();

    return 0;
}
