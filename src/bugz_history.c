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

static struct option bugz_history_options[] = {
    {"help",      no_argument,       0, 'h'},
    {"new-since", required_argument, 0, 'n'},
    { 0 }
};

void bugz_history_helper(int status) {
    char help_header[] =
    N_("Usage: bugz history [options] bug\n"
       "Get the history for a specific bug\n"
       "\n"
       "Arguments:\n"
       "bug      : the ID of the bug to retrieve\n"
       "\n"
       "Valid options:\n"
       "-h [--help]           : show this help message and exit\n"
       "-n [--new-since]      : only changes newer than this time\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

struct bugz_history_arguments_t {
    int bug;
    struct curl_slist *new_since;
};
static struct bugz_history_arguments_t bugz_history_arguments = { 0 };
#define _append_history_arg_(m) bugz_history_arguments.m = \
                                curl_slist_append(bugz_history_arguments.m, optarg)

int bugz_history_main(int argc, char **argv) {
    CURL *curl;
    json_object *json;
    char url[PATH_MAX] = {0};
    char *base, *username, *password;
    int opt, longindex, retval = 1;
    struct bugz_config_t *config;
    struct curl_slist *headers = NULL;

    optind++;
    bugz_history_arguments.bug = -1;
    while (optind < argc) {
        opt = getopt_long(argc, argv, "-:hn:", bugz_history_options, &longindex);
        switch (opt) {
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s history: '%s' requires an argument\n") :
                            N_("ERROR: %s history: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_history_helper(opt == 'h' ? 0 : 1);
        case 'n' :
            _append_history_arg_(new_since);
            break;
        case -1 :
            bugz_history_arguments.bug = atoi(argv[optind++]);
            break;
        }
    } 
    if (bugz_history_arguments.bug <= 0) {
        fprintf(stderr, bugz_history_arguments.bug == -1 ?
                        N_("ERROR: %s history: no bug specified\n"):
                        N_("ERROR: %s history: invalid bug specified\n"), argv[0]);
        exit(1);
    }

    config = bugz_config();
    base = bugz_get_base(config);
    if (base == NULL) {
        fprintf(stderr, N_("ERROR: No base URL specified\n"));
        bugz_config_free(config);
        exit(1);
    }
    username = password = NULL;
    if (bugz_arguments.skip_auth == NULL) {
        username = bugz_get_auth(config, &password);
        if (username == NULL) {
            fprintf(stderr, N_("ERROR: failed to get auth\n"));
            bugz_config_free(config);
            exit(1);
        }
    }
    if (password)
        sprintf(url, "%s/rest/bug/%d/history?login=%s&password=%s", base,
                bugz_history_arguments.bug, username, password);
    else if (username)
        sprintf(url, "%s/rest/bug/%d/history?api_key=%s", base,
                bugz_history_arguments.bug, username);
    else
        sprintf(url, "%s/rest/bug/%d/history", base, bugz_history_arguments.bug);

    bugz_config_free(config);
    if ((curl = curl_easy_init()) == NULL) {
        fprintf(stderr, N_("ERROR: %s history: curl_easy_init() failed\n"), argv[0]);
        exit(1);
    }
    headers = curl_slist_append(headers, "charsets: utf-8");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    fprintf(stderr, N_(" * Info: Using %s\n")
                    N_(" * Info: Getting bug %d history ..\n"), base, bugz_history_arguments.bug);

    bugz_get_result(curl, url, &json);
    if (bugz_check_result(json)) {
        int j;
        json_object *bug, *bugs, *history;
        json_object_object_get_ex(json, "bugs", &bugs);
        for (j=0; j<json_object_array_length(bugs); j++) {
            bug = json_object_array_get_idx(bugs, j);
            json_object_object_get_ex(bug, "history", &history);
            if (history) {
                int i, k;
                json_object *item, *who, *when, *changes;
                json_object *added, *removed, *field_name;
                for (i=0; i<json_object_array_length(history); i++) {
                    item = json_object_array_get_idx(history, i);
                    json_object_object_get_ex(item, "who", &who);
                    json_object_object_get_ex(item, "when", &when);
                    json_object_object_get_ex(item, "changes", &changes);
                    fprintf(stdout, "[History #%d] %s : %s\n", i,
                            json_object_get_string(who),
                            json_object_get_string(when));
                    for (k=0; k<bugz_arguments.columns; k++) fprintf(stdout, "%c", '-');
                    fprintf(stdout, "\n");

                    for (k=0; k<json_object_array_length(changes); k++) {
                        item = json_object_array_get_idx(changes, k);
                        json_object_object_get_ex(item, "added", &added); 
                        json_object_object_get_ex(item, "removed", &removed); 
                        json_object_object_get_ex(item, "field_name", &field_name);
                        fprintf(stdout, "%s removed: %s\n",
                                json_object_get_string(field_name),
                                json_object_get_string(removed)); 
                        fprintf(stdout, "%s added  : %s\n\n",
                                json_object_get_string(field_name),
                                json_object_get_string(added)); 
                    }
                }
            }
        }
        json_object_put(json);
        retval = 0;
    }
    curl_easy_cleanup(curl);

    return retval;
}

