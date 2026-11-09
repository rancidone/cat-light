# Testing Brief

## Framework

Pocket has a host timer regression check (`make -C pocket test-host`), also run in CI. It checks fractional-tick accumulation, frame timing across hardware-counter rollover, an hour of simulated elapsed time across multiple wraps, and button-duration arithmetic across millisecond rollover. There is no broader automated firmware behavior-test suite. `.github/workflows/firmware.yml` also compiles ESP32 and the two Pocket programs. `pocket/board-test.c` is a manually flashed hardware diagnostic. Physical acceptance checks are in `docs/building.md`.

ESP32 firmware is retired with known bugs; fixes are considered on demand. If work resumes, use the ESP-IDF Unity test runner, with tests under `esp32/test/`, one subdirectory per unit. Prefer host-compilable tests where dependencies can be stubbed; run hardware-dependent checks on-device. The following rules describe the intended testing approach, not existing coverage.

## Rules

- **TDD against interfaces, not implementations.** Tests are written against published headers. If a test requires knowledge of internal state, the interface is wrong.
- **Test the contract, not the mechanism.** Assert observable outputs (frame values, return codes, state transitions) — never assert internal struct layout or call order.
- **Host first.** Any unit whose dependencies can be stubbed should have host-compilable tests. On-device only when hardware is genuinely required.
- **Stubs over mocks.** Implement minimal stub versions of dependencies; don't use a mocking framework. Stubs live in `test/<unit>/stubs/`.
- **One test per behavior.** Each test has one reason to fail. Name tests as `test_<condition>_<expected_outcome>`.
- **No tests for things we don't own.** Don't test ESP-IDF drivers, FreeRTOS scheduling, or Lua language semantics.
- **Integration tests are smoke tests.** Device-side and HTTP tests verify the system boots and round-trips correctly — not exhaustive coverage.
