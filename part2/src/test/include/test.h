/**
 * @file test.h
 * @brief Host-side tests for the registry, flash and queue APIs.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-23
 */

#pragma once

/**
 * @brief Tests flash write and erase failure injection.
 *
 * Verifies that configured failures are triggered at the expected stages of
 * flash programming and erasure.
 */
void test_flash_failure_injection(void);

/**
 * @brief Tests flash initialization error handling.
 *
 * Verifies that invalid flash configurations and initialization arguments are
 * correctly rejected.
 */
void test_flash_initialization_errors(void);

/**
 * @brief Tests queue acknowledgement failure during flash erasure.
 *
 * Verifies that a failed erase operation does not remove the oldest registry
 * from the queue or alter its committed persistent state.
 */
void test_queue_failed_ack_during_erase(void);

/**
 * @brief Tests queue acknowledgement failure during flash programming.
 *
 * Verifies that a failed programming operation does not remove the oldest
 * registry from the queue or alter its committed persistent state.
 */
void test_queue_failed_ack_during_program(void);

/**
 * @brief Tests registry insertion failure during flash erasure.
 *
 * Verifies that a failed erase operation does not insert the new registry or
 * modify the existing logical queue state.
 */
void test_queue_failed_push_during_erase(void);

/**
 * @brief Tests registry insertion failure during flash programming.
 *
 * Verifies that a failed programming operation does not insert the new registry
 * or modify the existing logical queue state.
 */
void test_queue_failed_push_during_program(void);

/**
 * @brief Tests registry insertion failure before snapshot commitment.
 *
 * Verifies that an incomplete snapshot is not treated as committed and that the
 * previously committed queue state remains recoverable.
 */
void test_queue_failed_push_before_commit(void);

/**
 * @brief Tests registry insertion failure during snapshot commitment.
 *
 * Verifies that an interrupted commit operation does not replace the previously
 * committed queue state.
 */
void test_queue_failed_push_during_commit(void);

/**
 * @brief Tests initialization with insufficient flash capacity.
 *
 * Verifies that the queue reports an error when the configured flash storage
 * cannot accommodate its required snapshots.
 */
void test_queue_flash_insufficient(void);

/**
 * @brief Tests queue-full error handling.
 *
 * Verifies that insertion is rejected when the queue reaches its maximum
 * capacity and that existing registries remain unchanged.
 */
void test_queue_full_error(void);

/**
 * @brief Tests initialization of an empty queue.
 *
 * Verifies that an erased flash region is recognized as an empty queue and that
 * the queue is ready to accept new registries.
 */
void test_queue_initialization_empty(void);

/**
 * @brief Tests queue initialization error handling.
 *
 * Verifies that invalid or inconsistent persistent queue data is handled
 * correctly during initialization.
 */
void test_queue_initialization_errors(void);

/**
 * @brief Tests queue persistence across reinitialization.
 *
 * Verifies that successfully committed registries survive queue
 * reinitialization and retain their original FIFO ordering.
 */
void test_queue_persistence(void);
