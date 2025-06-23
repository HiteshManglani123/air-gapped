#ifndef SECRET_FORMULA_H
#define SECRET_FORMULA_H

#pragma once
enum SECRET_FORMULA_LOOKUP {
    TOP_BUN,
    LETTUCE,
    TOMATO,
    PATTY,
    CHEESE,
    ONION,
    KETCHUP,
    MUSTARD,
    FONTYS_SECRET_INGREDIENT,
    BOTTOM_BUN,
    END_CHARACTER,
    UNKNOWN = -1,
};

typedef struct {
    enum SECRET_FORMULA_LOOKUP id;
    char *name;
    char *image_path;
} Secret_Formula;

extern const Secret_Formula secret_formula_lookup[];

extern const int secret_formula_lookup_length;

#endif
