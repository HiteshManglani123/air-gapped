#ifndef BURGER_STACK_GUI_H
#define BURGER_STACK_GUI_H

#pragma once
#include "secret_formula.h"

int setup_sdl(void);
void cleanup_sdl(void);
void add_image(enum SECRET_FORMULA_LOOKUP image_path);

#endif
