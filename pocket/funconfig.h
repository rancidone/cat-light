#pragma once

/* 48 MHz via internal PLL (default) */
#define FUNCONF_SYSTEM_CORE_CLOCK 48000000
#define FUNCONF_USE_PLL           1

/* No debug printf — saves flash and removes stdio.h dependency */
#define FUNCONF_USE_DEBUGPRINTF   0
#define FUNCONF_USE_UARTPRINTF    0
#define FUNCONF_NULL_PRINTF       0

/* SysTick at 6 MHz (HCLK/8) — funMillis() uses this */
#define FUNCONF_SYSTICK_USE_HCLK  0
