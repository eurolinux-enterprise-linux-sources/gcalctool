
/*  $Header$
 *
 *  Copyright (c) 1987-2008 Sun Microsystems, Inc. All Rights Reserved.
 *  Copyright (c) 2008 Robert Ancell
 *           
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *           
 *  This program is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 *  General Public License for more details.
 *           
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <glib.h>

#include "display.h"

#include "get.h"
#include "mp.h"
#include "functions.h"
#include "ui.h"
#include "mp-equation.h" // For mp_equation_parse()
#include "register.h"

static const char *display_types[] = { "ENG", "FIX", "SCI", NULL };

static GCDisplayState *
get_state(GCDisplay *display)
{
    return &(display->h.e[display->h.current]);
}

static gboolean
exp_has_postfix(char *str, char *postfix)
{
    int len, plen;

    if (!str) {
        return FALSE;
    }

    assert(postfix);

    len = strlen(str);
    plen = strlen(postfix);

    if (plen > len) {
        return FALSE;
    }

    return strcasecmp(str + len - plen, postfix) == 0;
}

static char *
str_replace(char *str, char *from, char *to)
{
    char output[MAX_DISPLAY];
    int offset = 0;
    char *c;
    int flen = strlen(from);
    int tlen = strlen(to);
    
    for (c = str; *c && offset < MAX_DISPLAY - 1; c++, offset++) {
        if (strncasecmp(from, c, flen) == 0) {
            SNPRINTF(output + offset, MAX_DISPLAY - offset, "%s", to);
            c += flen - 1;
            offset += tlen - 1;
        } else {
            output[offset] = *c;
        }
    }

    if (offset >= MAX_DISPLAY)
        offset = MAX_DISPLAY - 1;
    output[offset] = '\0';
    
    free(str);
    
    return strdup(output);
}

/* Add in the thousand separators characters if required and if we are
 * currently in the decimal numeric base, use the "right" radix character.
 */

/* Add in the thousand separators characters if required */
static void
localize_expression(char *dest, const char *src, int dest_length, int *cursor)
{
    GString *output;
    const char *c, *d;
    int digit_count = -1, read_cursor, new_cursor;
    gboolean after_radix = FALSE;
    
    if (cursor) {
        new_cursor = *cursor;
    } else {
        new_cursor = -1;
    }

    /* Scan expression looking for numbers and inserting separators */
    output = g_string_sized_new(dest_length);
    for (c = src, read_cursor = 1; *c; c = g_utf8_next_char(c), read_cursor++) {
        /* Insert separators between digits */
        if (*c >= '0' && *c <= '9') {
            /* Read ahead to find the number of digits */
            if (digit_count < 0) {
                digit_count = 1;
                for (d = c + 1; *d >= '0' && *d <= '9'; d++) {
                    digit_count++;
                }
            }
            
            g_string_append_unichar(output, g_utf8_get_char(c));
            
            /* Insert separator after nth digit */
            if (v->display.show_tsep && v->base == 10 &&
                !after_radix && digit_count > 1 && digit_count % v->tsep_count == 1) {
                g_string_append(output, v->tsep);
                if (new_cursor > read_cursor) {
                    new_cursor++;
                }
                read_cursor++;
            }
            digit_count--;
        }
        /* Ignore digits after the radix */
        else if (*c == '.') {
            digit_count = -1;
            after_radix = TRUE;
            g_string_append(output, v->radix);
            // FIXME: Handle cursor if radix is more than one character?
        }
        /* Reset when encountering other characters (e.g. '+') */
        else {
            digit_count = -1;
            after_radix = FALSE;
            g_string_append_unichar(output, g_utf8_get_char(c));
        }
    }
    
    STRNCPY(dest, output->str, dest_length - 1);
    g_string_free(output, TRUE);
    
    if (cursor != NULL && *cursor != -1) {
        *cursor = new_cursor;
    }
}


void
display_clear(GCDisplay *display)
{
    v->error = 0;
    display_set_string(display, "", -1);
}


const char *
display_get_text(GCDisplay *display)
{
    return get_state(display)->expression;
}


gboolean display_get_integer(GCDisplay *display, gint64 *value)
{
    const char *text;
    char buf[MAX_DISPLAY];
    gchar *endptr;
    guint bases[] = {2, 8, 10, 16};

    text = display_get_text(display);
    if (text[0] == '\0') {
        text = "0";
    }
    else if (display_is_result(display)) {
        display_make_number(display, buf, MAX_DISPLAY, display_get_answer(display), v->base, FALSE);
        text = buf;
    }
    
    *value = g_ascii_strtoll(text, &endptr, bases[v->base]);
    if(*endptr != '\0' || ((*value == G_MAXINT64 || *value == G_MININT64) && errno == ERANGE))
        return FALSE;
    return TRUE;
}


gboolean display_get_unsigned_integer(GCDisplay *display, guint64 *value)
{
    const char *text;
    char buf[MAX_DISPLAY];
    gchar *endptr;

    text = display_get_text(display);
    if (text[0] == '\0') {
        text = "0";
    }
    else if (display_is_result(display)) {
        display_make_number(display, buf, MAX_DISPLAY, display_get_answer(display), v->base, FALSE);
        text = buf;
    }
    
    /* strtoull() treats the string like a 2's complement number which is not what we want */
    if(strncmp(text, "-", strlen("-")) == 0 || strncmp(text, "−", strlen("−")) == 0)
        return FALSE;

    *value = g_ascii_strtoull(text, &endptr, v->base);
    if(*endptr != '\0' || (*value == G_MAXUINT64 && errno == ERANGE))
        return FALSE;
    return TRUE;
}


MPNumber *display_get_answer(GCDisplay *display)
{
    return &get_state(display)->ans;
}


int
display_get_cursor(GCDisplay *display)
{
    return get_state(display)->cursor;
}


void
display_set_number(GCDisplay *display, const MPNumber *MPval)
{
   char text[MAX_DISPLAY];
   display_make_number(display, text, MAX_DISPLAY, MPval, v->base, FALSE);
   display_set_string(display, text, -1);
}


void
display_set_answer(GCDisplay *display)
{
    display_set_string(display, "ans", -1);
}


static void
display_make_text(GCDisplay *display, char *localized, int length, int *cursor)
{
    int i;
    MPNumber MP_reg;
    char temp[MAX_LOCALIZED], *str, reg[3];
    GCDisplayState *e;

    e = get_state(display);
    if (display_is_empty(display)) {
        mp_set_from_integer(0, &MP_reg);
        display_make_number(display, temp, MAX_LOCALIZED, &MP_reg, v->base, FALSE);
        str = strdup(temp);
    } else {           
        str = strdup(e->expression);
    }
        
    /* Substitute answer register */
    display_make_number(display, temp, MAX_LOCALIZED, &e->ans, v->base, TRUE);
    str = str_replace(str, "ans", temp);

    /* Replace registers with values. */
    for (i = 0; i < 10; i++) {
        SNPRINTF(reg, 3, "R%d", i);
        register_get(i, &MP_reg);
        display_make_number(display, temp, MAX_LOCALIZED, &MP_reg, v->base, FALSE);
        str = str_replace(str, reg, temp);
    }

    localize_expression(localized, str, length, cursor);
    free(str);
}


static void
display_refresh(GCDisplay *display)
{
    char localized[MAX_LOCALIZED];
    int cursor;
    
    cursor = display_get_cursor(display);
    display_make_text(display, localized, MAX_LOCALIZED, &cursor);
    ui_set_display(localized, cursor);
}


void
display_set_string(GCDisplay *display, const char *value, int cursor)
{
    GCDisplayState *e;
    
    if (value[0] == '\0')
        cursor = -1;
    
    e = get_state(display);
    free(e->expression);
    e->expression = strdup(value);
    e->cursor = cursor;
    
    display_refresh(display);
}


void
display_set_cursor(GCDisplay *display, int cursor)
{
    GCDisplayState *e;

    e = get_state(display);
    e->cursor = cursor;
    display_refresh(display);
}


void
display_set_error(GCDisplay *display, const char *message)
{
    ui_set_statusbar(message, "gtk-dialog-error");
}


static void
copy_state(GCDisplayState *dst, GCDisplayState *src)
{
    memcpy(dst, src, sizeof(GCDisplayState));
    dst->expression = strdup(src->expression);
}


static void
update_undo_redo_button_sensitivity(GCDisplay *display)
{
    int undo = 0;
    int redo = 0;

    if (display->h.current != display->h.end) {
        redo = 1;
    }

    if (display->h.current != display->h.begin) {
        undo = 1;
    }

    ui_set_undo_enabled(undo, redo);
}


void display_clear_stack(GCDisplay *display)
{
    int i = display->h.begin;
    while (i != display->h.end) {
        if (i != display->h.current) {
            free(display->h.e[i].expression);
            display->h.e[i].expression = NULL;
        }
        i = ((i + 1) % UNDO_HISTORY_LENGTH);
    }
    display->h.begin = display->h.end = display->h.current;
    update_undo_redo_button_sensitivity(display);
}


void display_push(GCDisplay *display)
{
    int c;
    
    if (display->h.current != display->h.end) {
        int i = display->h.current;

        do {
            i = ((i + 1) % UNDO_HISTORY_LENGTH);
            free(display->h.e[i].expression);
            display->h.e[i].expression = strdup("ans");
        } while (i != display->h.end);
    }

    display->h.end = display->h.current;

    c = display->h.current;
    display->h.end = display->h.current = ((display->h.current + 1) % UNDO_HISTORY_LENGTH);
    if (display->h.current == display->h.begin) {
        free(display->h.e[display->h.begin].expression);
        display->h.e[display->h.begin].expression = NULL;
        display->h.begin = ((display->h.begin + 1) % UNDO_HISTORY_LENGTH);
    }

    copy_state(&(display->h.e[display->h.current]), &(display->h.e[c]));
    update_undo_redo_button_sensitivity(display);
}


void display_pop(GCDisplay *display)
{
    if (display->h.current != display->h.begin) {
        display->h.current = ((display->h.current - 1) % UNDO_HISTORY_LENGTH);
        ui_set_statusbar("", "");
    } else {
        ui_set_statusbar(_("No undo history"), "gtk-dialog-warning");
    }
    update_undo_redo_button_sensitivity(display);
    
    display_refresh(display);
}


void
display_unpop(GCDisplay *display)
{
    if (display->h.current != display->h.end) {
        display->h.current = ((display->h.current + 1) % UNDO_HISTORY_LENGTH);
        ui_set_statusbar("", "");
    } else {
        ui_set_statusbar(_("No redo steps"), "gtk-dialog-warning");
    }
    update_undo_redo_button_sensitivity(display);
    get_state(display)->cursor = -1;
    display_refresh(display);
}


gboolean
display_is_undo_step(GCDisplay *display)
{
    return(display->h.current != display->h.begin);
}


void
display_insert(GCDisplay *display, int cursor_start, int cursor_end, const char *text)
{
    char buf[MAX_DISPLAY];
    
    if (cursor_start < 0) {
        SNPRINTF(buf, MAX_DISPLAY, "%s%s", display_get_text(display), text);
        display_set_string(display, buf, -1);
    } else {
        GString *new_text;
        const char *c;
        gint cursor, new_cursor;
        
        /* Get display text and strip out thousand separators */
        new_text = g_string_new("");
        new_cursor = 0;
        if (cursor_start == 0) {
            g_string_append(new_text, text);
            new_cursor += g_utf8_strlen(text, -1);
        }
        
        cursor = 0;
        for (c = ui_get_display(); *c; c = g_utf8_next_char(c), cursor++) {
            gboolean use = TRUE;
            
            /* Ignore selected part */
            if (cursor_start != cursor_end && cursor >= cursor_start && cursor < cursor_end)
                use = FALSE;
            
            /* Ignore thousands separators */
            if (v->tsep[0] != '\0' && strncmp(c, v->tsep, strlen(v->tsep)) == 0)
                use = FALSE;
            
            /* Copy existing text */
            if (use) {
                g_string_append_unichar(new_text, g_utf8_get_char(c));
                if (cursor < cursor_start)
                    new_cursor++;
            }
            
            /* Insert text */
            if ((cursor + 1) == cursor_start) {
                g_string_append(new_text, text);
                new_cursor += g_utf8_strlen(text, -1);
            }
        }
        display_set_string(display, new_text->str, new_cursor);
        g_string_free(new_text, TRUE);
    }

}


void
display_insert_number(GCDisplay *display, int cursor_start, int cursor_end, const MPNumber *value)
{
    char text[MAX_DISPLAY];
    display_make_number(display, text, MAX_DISPLAY, value, v->base, FALSE);
    display_insert(display, cursor_start, cursor_end, text);
}


void
display_backspace(GCDisplay *display, int cursor_start, int cursor_end)
{
    char buf[MAX_DISPLAY] = "", buf2[MAX_DISPLAY];
    GCDisplayState *e = get_state(display);
    int i, cursor;
    MPNumber MP_reg;
    
    /* Can't delete empty display */
    if (display_is_empty(display))
        return;

    cursor = display_get_cursor(display);
    
    /* If cursor is at end of the line then delete the last character preserving accuracy */
    if (cursor_start < 0) {
        int len;
        
        len = g_utf8_strlen(ui_get_display(), -1);
        
        if (exp_has_postfix(e->expression, "ans")) {
            display_make_number(display, buf, MAX_DISPLAY, &e->ans, v->base, FALSE);
            e->expression = str_replace(e->expression, "ans", buf);
        } else {
            for (i = 0; i < 10; i++) {
                SNPRINTF(buf, MAX_DISPLAY, "R%d", i);
                if (exp_has_postfix(e->expression, buf)) {
                    register_get(i, &MP_reg);
                    display_make_number(display, buf2, MAX_DISPLAY, &MP_reg, v->base, FALSE);
                    SNPRINTF(buf, MAX_DISPLAY, "%.*s%s", strlen(e->expression) - 2, e->expression - 2, buf2);
                    break;
                }
            }
        }
        
        display_insert(display, len - 1, len, "");
    } else if (cursor_start != cursor_end) {
        display_insert(display, cursor_start, cursor_end, "");
    } else if (cursor_start > 0) {
        display_insert(display, cursor_start - 1, cursor_start, "");
    }

}

void
display_delete(GCDisplay *display, int cursor_start, int cursor_end)
{
    /* Delete selected block */
    if (cursor_start != cursor_end)
        display_insert(display, cursor_start, cursor_end, "");
    else if (cursor_start >= 0)
        display_insert(display, cursor_start, cursor_start + 1, "");
}


void
display_surround(GCDisplay *display, const char *prefix, const char *suffix)
{
    char buffer[MAX_DISPLAY];
    
    SNPRINTF(buffer, MAX_DISPLAY, "%s%s%s", prefix, display_get_text(display), suffix);
    display_set_string(display, buffer, -1);
}


gboolean
display_is_empty(GCDisplay *display)
{
    return strcmp(display_get_text(display), "") == 0;
}


gboolean
display_is_result(GCDisplay *display)
{
    if (strcmp(display_get_text(display), "ans") == 0)
        return TRUE;
    
    return FALSE;
}


gboolean
display_is_usable_number(GCDisplay *display, MPNumber *z)
{
    if (display_is_empty(display)) {
        mp_set_from_integer(0, z);
        return TRUE;
    } else if (display_is_result(display)) {
        mp_set_from_mp(display_get_answer(display), z);
        return TRUE;
    } else {
        return mp_set_from_string(display_get_text(display), v->base, z) == 0;
    }
}


void
display_init(GCDisplay *display)
{
    int i;
   
    memset(display, 0, sizeof(GCDisplay));
   
    display->base = 10;

    if (get_boolean_resource(R_ZEROES, &i))
        display->show_zeroes = i;
    else
        display->show_zeroes = FALSE;         

    if (get_boolean_resource(R_TSEP, &i))
        display->show_tsep = i;
    else
        display->show_tsep = FALSE;

    if (get_enumerated_resource(R_DISPLAY, display_types, &i))
       display->format = (DisplayFormat) i;
    else
       display->format = FIX;

    for (i = 0; i < UNDO_HISTORY_LENGTH; i++)
        display->h.e[i].expression = strdup("");
}


void display_set_accuracy(GCDisplay *display, int accuracy)
{
    set_int_resource(R_ACCURACY, accuracy);
    get_state(display)->cursor = -1;
    display_refresh(display);   
}


void display_set_show_thousands_separator(GCDisplay *display, gboolean visible)
{
    display->show_tsep = visible;
    set_boolean_resource(R_TSEP, visible);
    display_set_cursor(display, -1);
    display_refresh(display);
}


void display_set_show_trailing_zeroes(GCDisplay *display, gboolean visible)
{
    display->show_zeroes = visible;
    set_boolean_resource(R_ZEROES, visible);
    get_state(display)->cursor = -1;
    display_refresh(display);
}


void display_set_base(GCDisplay *display, int base)
{
    display->base = base;
    get_state(display)->cursor = -1;
    display_refresh(display);
}


void display_set_format(GCDisplay *display, DisplayFormat type)
{
    v->display.format = type;
    set_enumerated_resource(R_DISPLAY, display_types, (int) type);
    get_state(display)->cursor = -1;
    display_refresh(display);
}


/* Convert engineering or scientific number in the given base. */
static void
make_eng_sci(GCDisplay *display, char *target, int target_len, const MPNumber *MPnumber, int base)
{
    static char digits[] = "0123456789ABCDEF";   
    char fixed[MAX_DIGITS], *optr;
    MPNumber MP1, MPatmp, MPval;
    MPNumber MP1base, MP3base, MP10base;
    int i, dval, len;
    MPNumber MPmant;            /* Mantissa. */
    int ddig;                   /* Number of digits in exponent. */
    int eng = 0;                /* Set if this is an engineering number. */
    int exp = 0;                /* Exponent */
    
    if (display->format == ENG) {
        eng = 1;
    }
    optr = target;
    mp_abs(MPnumber, &MPval);
    mp_set_from_integer(0, &MP1);
    if (mp_is_less_than(MPnumber, &MP1)) {
        strcpy(optr, "−");
        optr += strlen("−");
    }
    mp_set_from_mp(&MPval, &MPmant);

    mp_set_from_integer(base, &MP1base);
    mp_xpowy_integer(&MP1base, 3, &MP3base);
    mp_xpowy_integer(&MP1base, 10, &MP10base);

    mp_set_from_integer(1, &MP1);
    mp_divide(&MP1, &MP10base, &MPatmp);

    mp_set_from_integer(0, &MP1);
    if (!mp_is_equal(&MPmant, &MP1)) {
        while (!eng && mp_is_greater_equal(&MPmant, &MP10base)) {
            exp += 10;
            mp_multiply(&MPmant, &MPatmp, &MPmant);
        }
 
        while ((!eng &&  mp_is_greater_equal(&MPmant, &MP1base)) ||
                (eng && (mp_is_greater_equal(&MPmant, &MP3base) || exp % 3 != 0))) {
            exp += 1;
            mp_divide(&MPmant, &MP1base, &MPmant);
        }
 
        while (!eng && mp_is_less_than(&MPmant, &MPatmp)) {
            exp -= 10;
            mp_multiply(&MPmant, &MP10base, &MPmant);
        }
 
        mp_set_from_integer(1, &MP1);
        while (mp_is_less_than(&MPmant, &MP1) || (eng && exp % 3 != 0)) {
            exp -= 1;
            mp_multiply(&MPmant, &MP1base, &MPmant);
        }
    }
 
    mp_cast_to_string(&MPmant, base, v->accuracy, !v->display.show_zeroes, fixed, MAX_DIGITS);
    len = strlen(fixed);
    for (i = 0; i < len; i++) {
        *optr++ = fixed[i];
    }
 
    *optr++ = 'e';
 
    if (exp < 0) {
        exp = -exp;
        strcpy(optr, "−");
        optr += strlen("−");
    } else {
        *optr++ = '+';
    }
 
    mp_set_from_string("0.5", 10, &MP1);
    mp_add_integer(&MP1, exp, &MPval);
    mp_set_from_integer(1, &MP1);
    for (ddig = 0; mp_is_greater_equal(&MPval, &MP1); ddig++) {
        mp_divide(&MPval, &MP1base, &MPval);
    }
 
    if (ddig == 0) {
        *optr++ = '0';
    }
 
    while (ddig-- > 0) {
        mp_multiply(&MPval, &MP1base, &MPval);
        dval = mp_cast_to_int(&MPval);
        *optr++ = digits[dval];
        dval = -dval;
        mp_add_integer(&MPval, dval, &MPval);
    }
    *optr++  = '\0';
}


/* Convert MP number to character string in the given base. */
void
display_make_number(GCDisplay *display, char *target, int target_len, const MPNumber *x, int base, int ignoreError)
{
    double val, max_fix;
    
    /*  NOTE: display_make_number can currently set v->error when converting to a double.
     *        This is to provide the same look&feel as V3 even though gcalctool
     *        now does internal arithmetic to "infinite" precision.
     *
     *  XXX:  Needs to be improved. Shouldn't need to convert to a double in
     *        order to do these tests.
     */

    double number = mp_cast_to_double(x);

    val = fabs(number);
    if (v->error && !ignoreError) {
        target[0] = '\0';
        return;
    }

    switch (base)
    {
    case 2:
        max_fix = 1.298074214e+33;
        break;
    case 8:
        max_fix = 2.037035976e+90;
        break;
    case 10:
        max_fix = 1.000000000e+100;
        break;
    default:
    case 16:
        max_fix = 2.582249878e+120;
        break;
    }
    
    // FIXME: Do this based on the number of digits, not actual values
    if ((display->format == ENG) ||
        (display->format == SCI) ||
        (display->format == FIX && val != 0.0 && (val > max_fix))) {
        make_eng_sci(display, target, target_len, x, base);
    } else {
        mp_cast_to_string(x, base, v->accuracy, !v->display.show_zeroes, target, target_len);
    }
}
