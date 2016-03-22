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

static struct option bugz_component_options[] = {
    {"help",             no_argument,       0, 'h'},
    {"name",             required_argument, 0,  0 },
    {"product",          required_argument, 0,  0 },
    {"description",      required_argument, 0, 'd'},
    {"default-assignee", required_argument, 0, 'a'},
    {"default-cc",       required_argument, 0,  0 },
    {"batch",            no_argument,       0,  0 },
    { 0 }
};

typedef enum bugz_component_longopt_t {
    opt_component_help = 0,
    opt_component_name,
    opt_component_product,
    opt_component_description,
    opt_component_default_assignee,
    opt_component_default_cc,
    opt_component_batch,
    opt_component_end
} bugz_component_longopt_t;

void bugz_component_helper(int status) {
    char help_header[] =
    N_("Usage: bugz component [options]\n"
       "Create a new component for specified product\n"
       "\n"
       "Valid options:\n"
       "-h [--help]                 : show this help message and exit\n"
       "--name NAME                 : the name of the new component\n"
       "--product PRODUCT           : product that the component must be added to\n"
       "-d [--description] ARG      : description of the new component\n"
       "-a [--default-assignee] ARG : default assignee of the component\n"
       "--default-cc CC             : optional list of emails to default CC list\n"
       "--batch                     : do not prompt for confirmation\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

struct bugz_component_arguments_t {
    struct curl_slist *name;
    struct curl_slist *product;
    struct curl_slist *description;
    struct curl_slist *default_assignee;
    struct curl_slist *default_cc;
    int batch;
};
static struct bugz_component_arguments_t bugz_component_arguments = { 0 };
#define _append_component_arg_(m) bugz_component_arguments.m = \
                                  curl_slist_append(bugz_component_arguments.m, optarg)

int bugz_component_main(int argc, char **argv) {
    CURL *curl;
    json_object *json;
    char url[PATH_MAX] = {0};
    char *base, *username, *password;
    int opt, longindex, retval = 1;
    struct bugz_config_t *config;
    struct curl_slist *headers = NULL;

    optind++;
    while (optind < argc) {
        opt = getopt_long(argc, argv, "-:hd:a:",
                          bugz_component_options, &longindex);
        switch (opt) {
        case -1 :
            fprintf(stderr, N_("ERROR: %s component: invalid argument '%s'\n"),
                            argv[0], argv[optind]);
            bugz_component_helper(1);
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s component: '%s' requires an argument\n") :
                            N_("ERROR: %s component: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_component_helper(opt == 'h' ? 0 : 1);
        case 'd' :
            _append_component_arg_(description);
            break;
        case 'a' :
            _append_component_arg_(default_assignee);
            break;
        case 0 :
            switch (longindex) {
            case opt_component_name :
                _append_component_arg_(name);
                break;
            case opt_component_product :
                _append_component_arg_(product);
                break;
            case opt_component_default_cc :
                _append_component_arg_(default_cc);
                break;
            case opt_component_batch :
                bugz_component_arguments.batch = TRUE;
                break;
            }
        }
    }
    if (bugz_component_arguments.name == NULL) {
        fprintf(stderr, N_("ERROR: %s component: Component name not specified\n"),
                        argv[0]);
        exit(1);
    }
    if (bugz_component_arguments.product == NULL) {
        fprintf(stderr, N_("ERROR: %s component: Product not specified\n"),
                        argv[0]);
        exit(1);
    }
    if (bugz_component_arguments.description == NULL) {
        fprintf(stderr, N_("ERROR: %s component: Description not specified\n"),
                        argv[0]);
        exit(1);
    }
    if (bugz_component_arguments.default_assignee == NULL) {
        fprintf(stderr, N_("ERROR: %s component: Default assignee not specified\n"),
                        argv[0]);
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
        sprintf(url, "%s/rest/component?login=%s&password=%s", base,
                username, password);
    else if (username)
        sprintf(url, "%s/rest/component?api_key=%s", base, username);
    else
        sprintf(url, "%s/rest/component", base);
    bugz_config_free(config);
    
    fprintf(stderr, N_(" * Info: Using %s\n"), base);
    json = json_object_new_object();
    if (1) {
        int j;
        json_object *val;

        val = bugz_slist_to_json_string(bugz_component_arguments.name);
        json_object_object_add(json, "name", val);
        fprintf(stderr, " * Info: %-12s: %s\n", "Name", json_object_get_string(val));

        val = bugz_slist_to_json_string(bugz_component_arguments.product);
        json_object_object_add(json, "product", val);
        fprintf(stderr, " * Info: %-12s: %s\n", "Product", json_object_get_string(val));

        val = bugz_slist_to_json_string(bugz_component_arguments.description);
        json_object_object_add(json, "description", val);
        fprintf(stderr, " * Info: %-12s: %s\n", "Description", json_object_get_string(val));

        val = bugz_slist_to_json_string(bugz_component_arguments.default_assignee);
        json_object_object_add(json, "default_assignee", val);
        fprintf(stderr, " * Info: %-12s: %s\n", "Default Assignee", json_object_get_string(val));

        if (bugz_component_arguments.default_cc) {
            char *q, *p;
            struct curl_slist *last = bugz_slist_get_last(bugz_component_arguments.default_cc);
            val = json_object_new_array();
            p = last->data;
            q = p;
            while (*q) {
                if (isspace((unsigned char)(*q)))
                    *q = ',';
                q++;
            }
            q = strtok(p, ",");
            while (q != NULL){
                if (strlen(q) > 0)
                    json_object_array_add(val, json_object_new_string(q));
                q = strtok(NULL, ",");
            }
            json_object_object_add(json, "default_cc", val);
            fprintf(stderr, " * Info: %-12s: %s\n", "Default CC", json_object_get_string(val));
        }
    }
    if (bugz_component_arguments.batch == FALSE) {
        char *p, confirm[1024] = {0};
        fprintf(stderr, "Confirm component submission (Y/n)? ");
        fgets(confirm, sizeof(confirm), stdin);
        p = strchr(confirm, '\n');
        if (p)
            *p = '\0';
        if (strlen(confirm) == 0 || confirm[0] != 'Y') {
            fprintf(stderr, N_("Submission aborted\n"));
            json_object_put(json);
            exit(1);
        }
    }

    if ((curl = curl_easy_init()) == NULL) {
        fprintf(stderr, N_("ERROR: %s component: curl_easy_init() failed\n"), argv[0]);
        json_object_put(json);
        exit(1);
    }
    headers = curl_slist_append(headers, "charsets: utf-8");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(json));

    bugz_get_result(curl, url, &json);
    if (bugz_check_result(json)) {
        json_object *id;
        json_object_object_get_ex(json, "id", &id);
        fprintf(stderr, N_(" * Info: Component %d submitted\n"),
                        json_object_get_int(id));
        fprintf(stdout, "Component %d submitted\n", json_object_get_int(id));
        json_object_put(json);
        retval = 0;
    }
    curl_easy_cleanup(curl);

    return retval;
}

