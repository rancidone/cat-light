# Testing Brief

## Framework

ESP-IDF Unity test runner. Tests live in `test/` at project root, one subdirectory per unit. Host-compilable tests use `CONFIG_IDF_TARGET=linux`. Hardware-touching tests run on-device via `idf.py flash` + `pytest`.

## Rules

- **TDD against interfaces, not implementations.** Tests are written against published headers. If a test requires knowledge of internal state, the interface is wrong.
- **Test the contract, not the mechanism.** Assert observable outputs (frame values, return codes, state transitions) — never assert internal struct layout or call order.
- **Host first.** Any unit whose dependencies can be stubbed should have host-compilable tests. On-device only when hardware is genuinely required.
- **Stubs over mocks.** Implement minimal stub versions of dependencies; don't use a mocking framework. Stubs live in `test/<unit>/stubs/`.
- **One test per behavior.** Each test has one reason to fail. Name tests as `test_<condition>_<expected_outcome>`.
- **No tests for things we don't own.** Don't test ESP-IDF drivers, FreeRTOS scheduling, or Lua language semantics.
- **Integration tests are smoke tests.** Device-side and HTTP tests verify the system boots and round-trips correctly — not exhaustive coverage.
