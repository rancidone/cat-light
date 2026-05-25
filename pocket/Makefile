TARGET             := cat-light-pocket
ADDITIONAL_C_FILES := light_engine.c modes.c

TARGET_MCU     := CH32V003
LDFLAGS        += -lm

include ch32v003fun/ch32fun/ch32fun.mk

all : flash
flash : cv_flash
clean : cv_clean

flash-test :
	$(MAKE) TARGET=board-test ADDITIONAL_C_FILES=light_engine.c cv_flash
