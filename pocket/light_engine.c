#include "light_engine.h"
#include "ch32v003fun/ch32fun/ch32fun.h"

/*
 * Pin assignments (confirmed from schematic):
 *   PC1 → PWM_R  (TIM2 CH4, partial remap 1)
 *   PC2 → PWM_B  (TIM2 CH2, partial remap 1)
 *   PC3 → PWM_G  (TIM1 CH3, no remap)
 *   PC4 → PWM_W  (TIM1 CH4, no remap)
 *   PC6 → MODE_BTN (input, active low, external pull-up)
 *
 * PWM: 48 MHz / (PSC+1) / (ARR+1) = 48e6 / 1 / 4800 = 10 kHz
 */

#define PWM_ARR 4799u

static inline uint32_t duty(uint8_t val)   { return (uint32_t)val * PWM_ARR / 255u; }
static inline uint32_t dutyf(float v)      { return (uint32_t)(v * PWM_ARR); }

void light_engine_init(void)
{
    /* Clocks */
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1;
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

    /* TIM2 partial remap 1: CH2→PC2, CH4→PC1 */
    AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_TIM2_REMAP) | AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP1;

    /* GPIO: PC1/PC2/PC3/PC4 → AF push-pull 50 MHz (CNF=10, MODE=11 → 0xB) */
    uint32_t cfg = GPIOC->CFGLR;
    cfg &= ~(0xFFFFu << 4);   /* clear PC1..PC4 nibbles */
    cfg |= (0xBBBBu << 4);    /* set all four to AF push-pull */
    /* PC6 → input with pull-up (CNF=10, MODE=00 → 0x8), OUTDR bit set for pull-up */
    cfg &= ~(0xFu << 24);
    cfg |= (0x8u << 24);
    GPIOC->CFGLR = cfg;
    GPIOC->OUTDR |= (1u << 6); /* enable internal pull-up on PC6 */

    /* TIM2: CH2 (PWM_B / PC2) and CH4 (PWM_R / PC1) */
    TIM2->PSC    = 0;
    TIM2->ATRLR  = PWM_ARR;
    /* CH2 PWM mode 1, preload enable */
    TIM2->CHCTLR1 = (0x6u << 13) | (1u << 11); /* OC2M=110, OC2PE=1 */
    /* CH4 PWM mode 1, preload enable */
    TIM2->CHCTLR2 = (0x6u << 13) | (1u << 11); /* OC4M=110, OC4PE=1 */
    TIM2->CCER   = TIM_CC2E | TIM_CC4E;
    TIM2->CH2CVR = 0;
    TIM2->CH4CVR = 0;
    TIM2->CTLR1  = TIM_ARPE | TIM_CEN;

    /* TIM1: CH3 (PWM_G / PC3) and CH4 (PWM_W / PC4), no remap */
    TIM1->PSC    = 0;
    TIM1->ATRLR  = PWM_ARR;
    /* CH3 PWM mode 1, preload enable */
    TIM1->CHCTLR2 = (0x6u << 5) | (1u << 3)    /* OC3M=110, OC3PE=1 */
                  | (0x6u << 13) | (1u << 11);  /* OC4M=110, OC4PE=1 */
    TIM1->CCER   = TIM_CC3E | TIM_CC4E;
    TIM1->CH3CVR = 0;
    TIM1->CH4CVR = 0;
    TIM1->BDTR   = TIM_MOE;
    TIM1->CTLR1  = TIM_ARPE | TIM_CEN;
}

void light_engine_write(const light_frame_t *frame)
{
    TIM2->CH4CVR = duty(frame->eye_r);   /* PC1 */
    TIM2->CH2CVR = duty(frame->eye_b);   /* PC2 */
    TIM1->CH3CVR = duty(frame->eye_g);   /* PC3 */
    TIM1->CH4CVR = dutyf(frame->whisker < 0.0f ? 0.0f :
                          frame->whisker > 1.0f ? 1.0f : frame->whisker); /* PC4 */
}
