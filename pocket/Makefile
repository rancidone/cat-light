TARGET  := cat-light-pocket
SRCS    := main.c light_engine.c modes.c

TARGET_MCU := CH32V003

include ch32v003fun/ch32fun/ch32fun.mk

all : flash
flash : cv_flash
clean : cv_clean
