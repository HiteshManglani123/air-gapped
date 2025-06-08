#include "secret_formula.h"
/*
    Needs to be same as transmitter
    These are the possible components of the secret formula.
    They consist from 0 through 9 which are 10 different values.
    To represent this in binary form, we would need 4 bits.
*/

const int secret_formula_lookup_length = 10;

const Secret_Formula secret_formula_lookup[] = {
    {TOP_BUN, "Top Bun", "images/top_bun.png"},
    {LETTUCE, "Lettuce", "images/lettuce.png"},
    {TOMATO, "Tomato", "images/tomato.png"},
    {PATTY, "Patty", "images/patty.webp"},
    {CHEESE, "Cheese", "images/cheese.png"},
    {ONION, "Onion", "images/onion.webp"},
    {KETCHUP, "Ketchup", "images/ketchup.webp"},
    {MUSTARD, "Mustard", "images/mustard.png"},
    {FONTYS_SECRET_INGREDIENT, "Fontys Secret Ingredient", "images/secret.png"},
    {BOTTOM_BUN, "Bottom Bun", "images/bottom_bun.jpeg"},
};
