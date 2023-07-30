/*
 * tests.c
 *
 * Copyright 2023 Nathan Geffen <nathan@nathan-dell>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */


#include <stdio.h>
#include <fit.h>

int main(int argc, char **argv)
{
        {
                double x[] = { 1.2, 223.1, 5.33232 };
                char *s = vecf_str(3, x);
                printf("%s\n", s);
                free(s);
        }
        {
                unsigned x[] = { 1, 223, 5 };
                char *s = vecu_str(3, x);
                printf("%s\n", s);
                free(s);
        }
        {
                struct fit_lo_hi x[] = { {1.0, 2.0}, {3.1, 4.2}, {55.4, 21.7}};
                char *s = vec_lo_hi_str(3, x);
                printf("%s\n", s);
                free(s);
        }
    return 0;
}

