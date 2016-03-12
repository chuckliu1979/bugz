/* -*- mode: c; c-basic-offset: 4; -*-
 * vim: noexpandtab sw=4 ts=4 sts=0:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#include "bugz.h"

#define _subcommand_macro_(n,d) extern void bugz_##n##_helper(int);
#include "bugz_cmd.h"

#define _subcommand_macro_(n,d) extern int bugz_##n##_main(int, char**);
#include "bugz_cmd.h"

struct bugz_subcommand_t {
    void (*helper)(int);
    int (*submain)(int, char**);
    const char *name;
    const char *desc;
};

struct bugz_subcommand_t bugz_subcommands[] = {
    #define _subcommand_macro_(n,d) {bugz_##n##_helper, bugz_##n##_main, #n, N_(d)},
    #include "bugz_cmd.h"
    { 0 }
};

struct bugz_arguments_t bugz_arguments = { 0 };

static struct option bugz_global_options[] = {
    {"help",        no_argument,       0, 'h'},
    {"config-file", required_argument, 0,  0 },
    {"connection",  required_argument, 0,  0 },
    {"base",        required_argument, 0, 'b'},
    {"user",        required_argument, 0, 'u'},
    {"password",    required_argument, 0, 'p'},
    {"passwordcmd", required_argument, 0,  0 },
    {"key",         required_argument, 0, 'k'},
    {"quiet",       no_argument,       0, 'q'},
    {"debug",       required_argument, 0, 'd'},
    {"columns",     required_argument, 0,  0 },
    {"encoding",    required_argument, 0,  0 },
    {"skip-auth",   no_argument,       0,  0 },
    {"version",     no_argument,       0,  0 },
    { 0 }
};

typedef enum bugz_longopt_t {
    opt_help = 0,
    opt_config_file,
    opt_connection,
    opt_base,
    opt_user,
    opt_password,
    opt_passwordcmd,
    opt_key,
    opt_quiet,
    opt_debug,
    opt_columns,
    opt_encoding,
    opt_skip_auth,
    opt_version,
    opt_end
} bugz_longopt_t;

void bugz_helper(int status) {
    struct bugz_subcommand_t *subcommand;
    char help_header[] = 
    N_("Usage: bugz [options] <subcommand> [sub-options] [args]\n"
       "Bugzilla command-line client.\n"
       "Type 'bugz --help' to see the program options and subcommands.\n"
       "Type 'bugz <subcommand> --help' for help on a specific subcommand.\n"
       "\n"
       "Global options:\n"
       "-h [--help]               : show this help message and exit\n"
       "--config-file CONFIG_FILE : read an alternate configuration file\n"
       "--connection CONNECTION   : use [connection] section of configuration file\n"
       "-b [--base] BASE          : base URL of Bugzilla\n"
       "-u [--user] USER          : username for commands requiring authentication\n"
       "-p [--password] PASSWORD  : password for commands requiring authentication\n"
       "--passwordcmd PASSWORDCMD : password command to evaluate for commands requiring\n"
       "                            authentication\n"
       "-k [--key] KEY            : use Bugzilla API key\n"
       "-q [--quiet]              : quiet mode\n"
       "-d [--debug] DEBUG        : debug level (from 0 to 3)\n"
       "--columns COLUMNS         : maximum number of columns output should use\n"
       "--encoding ENCODING       : output encoding (default: utf-8) (deprecated)\n"
       "--skip-auth               : skip authentication\n"
       "--version                 : show program version and exit\n"
       "\n"
       "Available subcommands:\n");
    fprintf(stderr, "%s", help_header);

    subcommand = bugz_subcommands;
    while (subcommand->name) {
        fprintf(stderr, "    %-16s %-40s\n", subcommand->name, subcommand->desc);
        subcommand++;
    }
    exit(status);
}

static int opt_longindex(int opt, int longindex) {
    switch (opt) {
    case 'b' : return opt_base;
    case 'u' : return opt_user;
    case 'p' : return opt_password;
    case 'k' : return opt_key;
    case 'q' : return opt_quiet;
    case 'd' : return opt_debug;
    default  : return longindex;
    }
}

int main(int argc, char **argv) {
    int opt, longindex;
    
    setlocale(LC_ALL, "");
    textdomain(PACKAGE);
    
    opterr = 0;
    while ((opt = getopt_long(argc, argv, "+:hb:u:p:k:qd:",
                              bugz_global_options, &longindex)) != -1) {
        switch (opt) {
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s: '%s' requires an argument\n") :
                            N_("ERROR: %s: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_helper(opt == 'h' ? 0 : 1);
        case 'b' :
        case 'u' :
        case 'p' :
        case 'k' :
        case 'q' :
        case 'd' :
            longindex = opt_longindex(opt, longindex);
        case 0 :
            if (longindex == opt_version) {
                fprintf(stdout, PACKAGE_STRING "\n");
                exit(0);
            }
            char **p = (char **)((char *)(&bugz_arguments) + sizeof(char *) * longindex);
            *p = bugz_global_options[longindex].has_arg != no_argument ? optarg : argv[optind - 1];
        }
    }

    if (bugz_arguments.config_file) {
        if (access(bugz_arguments.config_file, R_OK)) {
            fprintf(stderr, N_("ERROR: %s: '--config-file %s' : %s\n"), 
                            argv[0], bugz_arguments.config_file, strerror(errno));
            exit(1);
        }
    }
    if (bugz_arguments.optarg_debug) {
        int d = atoi(bugz_arguments.optarg_debug);
        if (d > 3 || d < 0 || (d == 0 && bugz_arguments.optarg_debug[0] != '0')) {
            fprintf(stderr, N_("ERROR: %s: '-d/--debug %s' (choose from 0 to 3)\n"),
                            argv[0], bugz_arguments.optarg_debug);
            exit(1);
        }
        bugz_arguments.debug = d;
    }
    bugz_arguments.columns = 80;
    if (bugz_arguments.optarg_columns) {
        int d = atoi(bugz_arguments.optarg_columns);
        if (d < 80) { /* XXX: FIXME */
            fprintf(stderr, N_("ERROR: %s: '--columns %s' (choose 80+)\n"),
                            argv[0], bugz_arguments.optarg_columns);
            exit(1);
        }
        bugz_arguments.columns = d;
    }

    if (argv[optind]) {
        struct bugz_subcommand_t *subcommand = bugz_subcommands;
        while (subcommand->name) {
            if (!strcmp(argv[optind], subcommand->name))
                break;
            subcommand++;
        }
        if (subcommand->submain)
            return subcommand->submain(argc, argv);
    }

    fprintf(stderr, N_("ERROR: %s: "), argv[0]);
    argv[optind] ? fprintf(stderr, N_("invalid subcommand '%s'\n"), argv[optind]) : 
                   fprintf(stderr, N_("too few arguments\n"));

    return 1;
}

