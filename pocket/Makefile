.DEFAULT_GOAL := build

TARGET             := cat-light-pocket
ADDITIONAL_C_FILES := light_engine.c modes.c

TARGET_MCU     := CH32V003
LDFLAGS        += -lm

include ch32v003fun/ch32fun/ch32fun.mk

.PHONY: all build build-test flash flash-test clean

all : build
build : $(TARGET).bin
build-test :
	$(MAKE) TARGET=board-test ADDITIONAL_C_FILES=light_engine.c build
flash : cv_flash
clean : cv_clean

flash-test :
	$(MAKE) TARGET=board-test ADDITIONAL_C_FILES=light_engine.c cv_flash

# Host regression checks; no programmer or cross compiler required.
HOST_CC ?= cc
.PHONY: test-host
test-host:
	@set -eu; test_bin=$$(mktemp); trap 'rm -f "$$test_bin"' EXIT; \
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror test/timebase_test.c -o "$$test_bin"; \
	"$$test_bin"
