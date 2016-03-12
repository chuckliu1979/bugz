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

#include <pwd.h>
#include <sys/types.h>

#include "bugz.h"

/* 
 * In Bugzilla 5.x, Token has been marked as deprecreated methods of authentications.
 * https://www.bugzilla.org/docs/5.0/en/html/api/Bugzilla/WebService.html#LOGGING_IN
 *
 */

static const char *token_obsoleted = 
    N_("* \n"
       "* In Bugzilla 5.x, Token has been marked as deprecreated\n"
       "* methods of authentications, and will be removed in the version\n"
       "* after Bugzilla 5.0\n"
       "* \n"
       "* For more information, see documentation\n"
       "* https://www.bugzilla.org/docs/5.0/en/html/api/Bugzilla/WebService.html#LOGGING_IN\n"
       "* \n"
       "* We remove support for tokens, but keep the login/logout subcommands\n"
       "* for compatibility purposes, therefore it does nothing.\n"
       "* \n");

void bugz_login_helper(int status) {
    char help_header[] =
    N_("Usage: bugz login [options]\n"
       "Log into Bugzilla\n"
       "\n"
       "Valid options:\n"
       "-h [--help]  : show this help message and exit\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

int bugz_login_main(int argc, char **argv) {
    int opt;
    optind++;
    while ((opt = getopt(argc, argv, "+:h")) != -1) {
        switch (opt) {
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s login: '%s' requires an argument\n") :
                            N_("ERROR: %s login: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_login_helper(opt == 'h' ? 0 : 1);
        }
    }

    fprintf(stderr, "%s\n", token_obsoleted);
    return 0;
}

void bugz_logout_helper(int status) {
    char help_header[] =
    N_("Usage: bugz logout [options]\n"
       "Log out of Bugzilla\n"
       "\n"
       "Valid options:\n"
       "-h [--help]  : show this help message and exit\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

int bugz_logout_main(int argc, char **argv) {
    int opt;
    optind++;
    while ((opt = getopt(argc, argv, "+:h")) != -1) {
        switch (opt) {
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s logout: '%s' requires an argument\n") :
                            N_("ERROR: %s logout: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
             bugz_logout_helper(opt == 'h' ? 0 : 1);
        }
    }

    fprintf(stderr, "%s\n", token_obsoleted);
    return 0;
}

void bugz_connections_helper(int status) {
    char help_header[] =
    N_("Usage: bugz connections [options]\n"
       "List known bug trackers\n"
       "\n"
       "Valid options:\n"
       "-h [--help]  : show this help message and exit\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

int bugz_connections_main(int argc, char **argv) {
    int opt;
    struct curl_slist *base;
    struct bugz_config_t *used, *head, *config;

    optind++;
    while ((opt = getopt(argc, argv, "+:h")) != -1) {
        switch (opt) {
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s connections: '%s' requires an argument\n") :
                            N_("ERROR: %s connections: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_connections_helper(opt == 'h' ? 0 : 1);
        }
    }

    fprintf(stdout, N_("Known bug trackers:\n"));
    if ((config = bugz_config()) == NULL)
        return 0;

    head = bugz_config_get_head(config);
    used = bugz_config_get_default(config);
    while (head) {
        base = bugz_slist_get_last(head->base);
        if (base == NULL && used) {
            struct curl_slist *conn = bugz_slist_get_last(used->connection);
            if (conn == NULL || strcmp(conn->data, head->name->data) == 0)
                base = bugz_slist_get_last(used->base);
        }
        if (base != NULL && head->connection == NULL) {
            struct bugz_parsed_url_t *purl;
            purl = bugz_urlparse(base->data);
            if (purl && purl->scheme && purl->netloc && \
                head->name && head->name->data)
                fprintf(stdout, "%s\n", head->name->data);
            bugz_parsed_url_free(purl);
        }
        head = head->next;
    }
    bugz_config_free(config);

    return 0;
}

