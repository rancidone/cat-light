#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#define BUTTON_BIT (1ULL << GPIO_NUM_4)

void input_init();
bool button_down();
bool button_pressed();

#endif // INPUT_H