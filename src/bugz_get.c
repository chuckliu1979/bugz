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

static struct option bugz_get_options[] = {
    {"help",           no_argument, 0, 'h'},
    {"no-attachments", no_argument, 0, 'a'},
    {"no-comments",    no_argument, 0, 'n'},
    { 0 }
};

void bugz_get_helper(int status) {
    char help_header[] =
    N_("Usage: bugz get [options] bug\n"
       "Get bug from Bugzilla\n"
       "\n"
       "Arguments:\n"
       "bug      : the ID of the bug to retrieve\n"
       "\n"
       "Valid options:\n"
       "-h [--help]           : show this help message and exit\n"
       "-a [--no-attachments] : do not show attachments\n"
       "-n [--no-comments]    : do not show comments\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

struct bugz_get_arguments_t {
    int bug;
    int no_comments;
    int no_attachments;
};
static struct bugz_get_arguments_t bugz_get_arguments = { 0 };

static json_object *bugz_fetch_comments(CURL *curl, char *base, char *username, char *password) {
    char buf[PATH_MAX] = {0};
    json_object *json = NULL;
    json_object *comments = NULL;

    if (password)
        sprintf(buf, "%s/rest/bug/%d/comment?login=%s&password=%s", base,
                bugz_get_arguments.bug, username, password);
    else if (username)
        sprintf(buf, "%s/rest/bug/%d/comment?api_key=%s", base,
                bugz_get_arguments.bug, username);
    else
        sprintf(buf, "%s/rest/bug/%d/comment", base, bugz_get_arguments.bug);

    bugz_get_result(curl, buf, &json);
    if (bugz_check_result(json)) {
        json_object *bug = NULL, *bugs;
        json_object_object_get_ex(json, "bugs", &bugs);
        if (bugs) {
            sprintf(buf, "%d", bugz_get_arguments.bug);
            json_object_object_get_ex(bugs, buf, &bug);
            if (bug)
                json_object_object_get_ex(bug, "comments", &comments);
        }
    }
    return comments;
}

static json_object *bugz_fetch_attachments(CURL *curl, char *base, char *username, char *password) {
    char buf[PATH_MAX] = {0};
    json_object *json = NULL;
    json_object *attachments = NULL;

    if (password)
        sprintf(buf, "%s/rest/bug/%d/attachment?login=%s&password=%s", base,
                bugz_get_arguments.bug, username, password);
    else if (username)
        sprintf(buf, "%s/rest/bug/%d/attachment?api_key=%s", base,
                bugz_get_arguments.bug, username);
    else
        sprintf(buf, "%s/rest/bug/%d/attachment", base, bugz_get_arguments.bug);

    bugz_get_result(curl, buf, &json);
    if (bugz_check_result(json)) {
        json_object *bugs;
        json_object_object_get_ex(json, "bugs", &bugs);
        if (bugs) {
            sprintf(buf, "%d", bugz_get_arguments.bug);
            json_object_object_get_ex(bugs, buf, &attachments);
        }
    }
    return attachments;
}

int bugz_get_main(int argc, char **argv) {
    CURL *curl;
    json_object *json;
    char url[PATH_MAX] = {0};
    char *base, *username, *password;
    int opt, longindex, retval = 1;
    struct bugz_config_t *config;
    struct curl_slist *headers = NULL;

    optind++;
    bugz_get_arguments.bug = -1;
    while (optind < argc) {
        opt = getopt_long(argc, argv, "-:han", bugz_get_options, &longindex);
        switch (opt) {
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s get: '%s' requires an argument\n") :
                            N_("ERROR: %s get: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_get_helper(opt == 'h' ? 0 : 1);
        case 'a' :
            bugz_get_arguments.no_attachments = TRUE;
            break;
        case 'n' :
            bugz_get_arguments.no_comments = TRUE;
            break;
        case -1 :
            bugz_get_arguments.bug = atoi(argv[optind++]);
            break;
        }
    } 
    if (bugz_get_arguments.bug <= 0) {
        fprintf(stderr, bugz_get_arguments.bug == -1 ?
                        N_("ERROR: %s get: no bug specified\n"):
                        N_("ERROR: %s get: invalid bug specified\n"), argv[0]);
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
        sprintf(url, "%s/rest/bug/%d?login=%s&password=%s", base,
                bugz_get_arguments.bug, username, password);
    else if (username)
        sprintf(url, "%s/rest/bug/%d?api_key=%s", base,
                bugz_get_arguments.bug, username);
    else
        sprintf(url, "%s/rest/bug/%d", base, bugz_get_arguments.bug);

    bugz_config_free(config);
    if ((curl = curl_easy_init()) == NULL) {
        fprintf(stderr, N_("ERROR: %s get: curl_easy_init() failed\n"), argv[0]);
        exit(1);
    }
    headers = curl_slist_append(headers, "charsets: utf-8");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    fprintf(stderr, N_(" * Info: Using %s\n")
                    N_(" * Info: Getting bug %d ..\n"), base, bugz_get_arguments.bug);

    bugz_get_result(curl, url, &json);
    if (bugz_check_result(json)) {
        int j;
        json_object *bug, *bugs;
        json_object *comments = NULL;
        json_object *attachments = NULL;
        if (bugz_get_arguments.no_attachments == FALSE)
            attachments = bugz_fetch_attachments(curl,
                          base, username, password);
        if (bugz_get_arguments.no_comments == FALSE)
            comments = bugz_fetch_comments(curl,
                       base, username, password);
        json_object_object_get_ex(json, "bugs", &bugs);
        for (j=0; j<json_object_array_length(bugs); j++) {
            bug = json_object_array_get_idx(bugs, j);
            bugz_show_bug_info(bug, attachments, comments);
        }
        json_object_put(comments);
        json_object_put(attachments);
        json_object_put(json);
        retval = 0;
    }
    curl_easy_cleanup(curl);

    return retval;
}

