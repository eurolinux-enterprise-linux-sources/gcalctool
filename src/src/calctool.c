
/*  $Header$
 *
 *  Copyright (c) 1987-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <glib-object.h>

#include "calctool.h"
#include "unittest.h"
#include "get.h"
#include "display.h"
#include "functions.h"
#include "ui.h"
#include "mp.h"
#include "register.h"
#include "mp-equation.h"

time_t time();

/* Calctool variables and options. */
static CalculatorVariables calc_state;
CalculatorVariables *v;

static void
version()
{
    /* NOTE: Is not translated so can be easily parsed */
    fprintf(stderr, "%1$s %2$s\n", v->progname, VERSION);
}

static void
solve(const char *equation)
{
    MPEquationOptions options;
    int error;
    MPNumber result;
    char result_str[MAXLINE];
    
    memset(&options, 0, sizeof(options));
    options.base = 10;
    options.wordlen = 32;
    options.angle_units = MP_DEGREES;
    
    error = mp_equation_parse(equation, &options, &result);
    if(error != 0) {
        fprintf(stderr, "Error %d\n", error);
        exit(1);
    }
    else {
        mp_cast_to_string(&result, v->base, 9, 1, result_str, MAXLINE);
        printf("%s\n", result_str);
        exit(0);
    }
}

static void
usage(int show_gtk)
{
    /* Translators: Description on how to use gcalctool displayed on command-line */
    fprintf(stderr,
            _("Usage:\n"
              "  %s - Perform mathematical calculations"), v->progname);

    fprintf(stderr,
            "\n\n");

    /* Translators: Description on gcalctool command-line help options displayed on command-line */
    fprintf(stderr,
            _("Help Options:\n"
              "  -v, --version                   Show release version\n"
              "  -h, -?, --help                  Show help options\n"
              "  --help-all                      Show all help options\n"
              "  --help-gtk                      Show GTK+ options"));
    fprintf(stderr,
            "\n\n");
    
    if (show_gtk) {
        /* Translators: Description on gcalctool command-line GTK+ options displayed on command-line */
        fprintf(stderr,
                _("GTK+ Options:\n"
                  "  --class=CLASS                   Program class as used by the window manager\n"
                  "  --name=NAME                     Program name as used by the window manager\n"
                  "  --screen=SCREEN                 X screen to use\n"
                  "  --sync                          Make X calls synchronous\n"
                  "  --gtk-module=MODULES            Load additional GTK+ modules\n"
                  "  --g-fatal-warnings              Make all warnings fatal"));
        fprintf(stderr,
                "\n\n");
    }

    /* Translators: Description on gcalctool application options displayed on command-line */    
    fprintf(stderr,
            _("Application Options:\n"
              "  -u, --unittest                  Perform unittests\n"
              "  -s, --solve <equation>          Solve the given equation"));
    fprintf(stderr,
            "\n\n");
}

void
get_options(int argc, char *argv[])
{
    int i;
    char *arg;
   
    for (i = 1; i < argc; i++) {
        arg = argv[i];

        if (strcmp(arg, "-v") == 0 || 
                 strcmp(arg, "--version") == 0 ||
                 strcmp(arg, "-?") == 0) {
            version();
            exit(0);
        }
        else if (strcmp(arg, "-h") == 0 || 
                 strcmp(arg, "--help") == 0) {
            usage(FALSE);
            exit(0);
        }
        else if (strcmp(arg, "--help-all") == 0) {
            usage(TRUE);
            exit(0);
        }
        else if (strcmp(arg, "-s") == 0 ||
            strcmp(arg, "--solve") == 0) {
            i++;
            if (i >= argc) {
                /* Translators: Error printed to stderr when user uses --solve argument without an equation */
                fprintf(stderr, _("Argument --solve requires an equation to solve"));
                fprintf(stderr, "\n");
                exit(1);
            }
            else
                solve(argv[i]);
        }
        else if (strcmp(arg, "-u") == 0 ||
            strcmp(arg, "--unittest") == 0) {
            unittest();
        }
        else {
            /* Translators: Error printed to stderr when user provides an unknown command-line argument */
            fprintf(stderr, _("Unknown argument '%s'"), arg);
            fprintf(stderr, "\n");
            usage(FALSE);
            exit(1);
        }
    }
}


static void
init_state(void)
{
    int acc, i;

    acc              = MAX_DIGITS + 12;     /* MP internal accuracy. */

    v->error         = FALSE;  /* No calculator error initially. */
    v->radix         = get_radix();    /* Locale specific radix string. */
    v->tsep          = get_tsep();     /* Locale specific thousands separator. */
    v->tsep_count    = get_tsep_count();
    
    if (get_int_resource(R_ACCURACY, &i))
        v->accuracy = i;
    else
        v->accuracy = DEFAULT_ACCURACY;
    if (v->accuracy < 0 || v->accuracy > MAXACC) {
       /* Translators: A log message displayed when an invalid accuracy
        is read from the configuration */
       fprintf(stderr, _("%s: accuracy should be in the range 0-%d\n"), 
               v->progname, MAXACC);
       v->accuracy = DEFAULT_ACCURACY;
    }

    if (get_int_resource(R_BASE, &i))
       v->base = i;
    else
       v->base = 10;

    if (get_enumerated_resource(R_TRIG, Rtstr, &i))
       v->ttype = (MPAngleUnit) i;
    else
       v->ttype = MP_DEGREES;

    if (get_int_resource(R_WORDLEN, &i))
       v->wordlen = i;
    else
       v->wordlen = 64;
}


int
main(int argc, char **argv)
{
    memset(&calc_state, 0, sizeof(calc_state));
    v = &calc_state;
    
    g_type_init();

    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    v->progname = g_path_get_basename(argv[0]);
    
    /* Seed random number generator. */   
    srand48((long) time((time_t *) 0));
   
    resources_init();
    
    init_state();
    register_init();
    display_init(&v->display);
    ui_init(&argc, &argv);

    get_options(argc, argv);

    ui_load();
    ui_start();
    
    return(0);
}
