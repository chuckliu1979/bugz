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

static struct option bugz_post_options[] = {
    {"help",             no_argument,       0, 'h'},
    {"product",          required_argument, 0,  0 },
    {"component",        required_argument, 0,  0 },
    {"version",          required_argument, 0,  0 },
    {"prodversion",      required_argument, 0,  0 },
    {"title",            required_argument, 0, 't'},
    {"description",      required_argument, 0, 'd'},
    {"op-sys",           required_argument, 0,  0 },
    {"platform",         required_argument, 0,  0 },
    {"priority",         required_argument, 0,  0 },
    {"severity",         required_argument, 0, 'S'},
    {"alias",            required_argument, 0,  0 },
    {"assigned-to",      required_argument, 0, 'a'},
    {"cc",               required_argument, 0,  0 },
    {"url",              required_argument, 0, 'U'},
    {"description-from", required_argument, 0, 'F'},
    {"append-command",   required_argument, 0,  0 },
    {"batch",            no_argument,       0,  0 },
    {"default-confirm",  required_argument, 0,  0 },
    { 0 }
};

typedef enum bugz_post_longopt_t {
    opt_post_help = 0,
    opt_post_product,
    opt_post_component,
    opt_post_version,
    opt_post_prodversion,
    opt_post_title,
    opt_post_description,
    opt_post_op_sys,
    opt_post_platform,
    opt_post_priority,
    opt_post_severity,
    opt_post_alias,
    opt_post_assigned_to,
    opt_post_cc,
    opt_post_url,
    opt_post_description_from,
    opt_post_append_command,
    opt_post_batch,
    opt_post_default_confirm,
    opt_post_end
} bugz_post_longopt_t;

void bugz_post_helper(int status) {
    char help_header[] =
    N_("Usage: bugz post [options]\n"
       "Post new bug into Bugzilla\n"
       "\n"
       "Valid options:\n"
       "-h [--help]                 : show this help message and exit\n"
       "--product PRODUCT           : product\n"
       "--component COMPONENT       : component\n"
       "--version VERSION           : version of the product\n"
       "--prodversion VERSION       : version of the product(deprecated)\n"
       "-t [--title] SUMMARY        : title of bug\n"
       "-d [--description] ARG      : description of the bug\n"
       "--op-sys OP_SYS             : set the operating system\n"
       "--platform PLATFORM         : set the hardware platform\n"
       "--priority PRIORITY         : set priority for the new bug\n"
       "-S [--severity] ARG         : set the severity for the new bug\n"
       "--alias ALIAS               : set the alias for this bug\n"
       "-a [--assigned-to] ARG      : assign bug to someone other than the\n"
       "                              default assignee\n"
       "--cc CC                     : add a list of emails to CC list\n"
       "-U [--url] URL              : set URL field of bug\n"
       "-F [--description-from] ARG : description from contents of file\n"
       "--append-command ARG        : append the output of a command to\n"
       "                              the description\n"
       "--batch                     : do not prompt for any values\n"
       "--default-confirm {y,Y,n,N} : default answer to confirmation question\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

struct bugz_post_arguments_t {
    struct curl_slist *product;
    struct curl_slist *component;
    struct curl_slist *version;
    struct curl_slist *title;
    struct curl_slist *description;
    struct curl_slist *op_sys;
    struct curl_slist *platform;
    struct curl_slist *priority;
    struct curl_slist *severity;
    struct curl_slist *alias;
    struct curl_slist *assigned_to;
    struct curl_slist *cc;
    struct curl_slist *url;
    struct curl_slist *description_from;
    struct curl_slist *append_command;
    int batch;
    int default_confirm;
};
static struct bugz_post_arguments_t bugz_post_arguments = { 0 };
#define _append_post_arg_(m) bugz_post_arguments.m = \
                             curl_slist_append(bugz_post_arguments.m, optarg)

static void bugz_post_prompt_for_bug() {
    char *p, line[1024];
    struct curl_slist *last;
    fprintf(stderr, N_("Press Ctrl+C at any time to abort.\n"));
    
    last = bugz_slist_get_last(bugz_post_arguments.product);
    if (last == NULL) {
        line[0] = '\0';
        while (strlen(line) == 0) {
            fprintf(stderr, "Enter product: ");
            fgets(line, sizeof(line), stdin);
            p = strchr(line, '\n');
            if (p)
                *p = '\0';
        }
        bugz_post_arguments.product = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter product: %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.component);
    if (last == NULL) {
        line[0] = '\0';
        while (strlen(line) == 0) {
            fprintf(stderr, "Enter component: ");
            fgets(line, sizeof(line), stdin);
            p = strchr(line, '\n');
            if (p)
                *p = '\0';
        }
        bugz_post_arguments.component = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter component: %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.version);
    if (last == NULL) {
        fprintf(stderr, "Enter version (default: unspecified): ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line) == 0)
            strcpy(line, "unspecified");
        bugz_post_arguments.version = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter version: %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.title);
    if (last == NULL) {
        line[0] = '\0';
        while (strlen(line) == 0) {
            fprintf(stderr, "Enter title: ");
            fgets(line, sizeof(line), stdin);
            p = strchr(line, '\n');
            if (p)
                *p = '\0';
        }
        bugz_post_arguments.title = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter title: %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.description);
    if (last == NULL) {
        p = bugz_raw_input("Enter bug description (Press Ctrl+D to end): ");
        if (p)
            bugz_post_arguments.description = curl_slist_append(NULL, p);
        free(p);
    }
    else
        fprintf(stderr, "Enter bug description: %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.op_sys);
    if (last == NULL) {
        fprintf(stderr, "Enter operating system where this bug occurs: ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line))
            bugz_post_arguments.op_sys = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter operating system: %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.platform);
    if (last == NULL) {
        fprintf(stderr, "Enter hardware platform where this bug occurs: ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line))
            bugz_post_arguments.platform = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter hardware platform: %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.priority);
    if (last == NULL) {
        fprintf(stderr, "Enter priority (eg. Normal) (optional): ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line))
            bugz_post_arguments.priority = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter priority (optional): %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.severity);
    if (last == NULL) {
        fprintf(stderr, "Enter severity (eg. normal) (optional): ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line))
            bugz_post_arguments.severity = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter severity (optional): %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.alias);
    if (last == NULL) {
        fprintf(stderr, "Enter an alias for this bug (optional): ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line))
            bugz_post_arguments.alias = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter alias (optional): %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.assigned_to);
    if (last == NULL) {
        fprintf(stderr, "Enter assignee (eg. liquidx@gentoo.org) (optional): ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line))
            bugz_post_arguments.assigned_to = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter assignee (optional): %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.cc);
    if (last == NULL) {
        fprintf(stderr, "Enter a CC list (comma separated) (optional): ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line))
            bugz_post_arguments.cc = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter a CC list (optional): %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.url);
    if (last == NULL) {
        fprintf(stderr, "Enter a URL (optional): ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line))
            bugz_post_arguments.url = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Enter a URL (optional): %s\n", last->data);

    last = bugz_slist_get_last(bugz_post_arguments.append_command);
    if (last == NULL) {
        fprintf(stderr, "Append the output of the "
                        "following command (leave blank for none): ");
        fgets(line, sizeof(line), stdin);
        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        if (strlen(line))
            bugz_post_arguments.append_command = curl_slist_append(NULL, line);
    }
    else
        fprintf(stderr, "Append command (optional): %s\n", last->data);
}

static void bugz_post_append_command() {
    char *p, *q;
    int i = 0, s = 1024;
    FILE *fd = popen(bugz_post_arguments.append_command->data, "r");
    
    p = q = NULL;
    while (fd && feof(fd) == 0) {
        if (i == 0) {
            i += s;
            p = (char *)malloc(i);
            memset(p, 0, i);
        }
        if (strlen(p) >= i-1) {
            i += s;
            q = (char *)malloc(i);
            memset(q, 0, i);
            strcpy(q, p);
            free(p);
            p = q;
        }
        fgets(p+strlen(p), i - strlen(p), fd);
    }
    pclose(fd);
    if (p) {
        i = 0;
        json_object *val = bugz_slist_to_json_string(bugz_post_arguments.description);
        if (val)
            i += json_object_get_string_len(val);
        i += strlen(bugz_post_arguments.append_command->data);
        i += strlen(p);
        q = (char *)malloc(i+8);
        strcpy(q, json_object_get_string(val));
        strcat(q, "\n\n$");
        strcat(q, bugz_post_arguments.append_command->data);
        strcat(q, "\n");
        strcat(q, p);
        curl_slist_free_all(bugz_post_arguments.description);
        bugz_post_arguments.description = curl_slist_append(NULL, q);
        free(p); free(q);
    }
}

int bugz_post_main(int argc, char **argv) {
    CURL *curl;
    json_object *json;
    char url[PATH_MAX] = {0};
    char *base, *username, *password;
    int opt, longindex, retval = 1;
    struct bugz_config_t *config;
    struct curl_slist *headers = NULL;

    optind++;
    while (optind < argc) {
        opt = getopt_long(argc, argv, "-:ht:d:S:a:U:F:",
                          bugz_post_options, &longindex);
        switch (opt) {
        case -1 :
            fprintf(stderr, N_("ERROR: %s post: invalid argument '%s'\n"),
                            argv[0], argv[optind]);
            bugz_post_helper(1);
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s post: '%s' requires an argument\n") :
                            N_("ERROR: %s post: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_post_helper(opt == 'h' ? 0 : 1);
        case 't' :
            _append_post_arg_(title);
            break;
        case 'd' :
            _append_post_arg_(description);
            break;
        case 'S' :
            _append_post_arg_(severity);
            break;
        case 'a' :
            _append_post_arg_(assigned_to);
            break;
        case 'U' :
            _append_post_arg_(url);
            break;
        case 'F' :
            _append_post_arg_(description_from);
            break;
        case 0 :
            switch (longindex) {
            case opt_post_product :
                _append_post_arg_(product);
                break;
            case opt_post_component :
                _append_post_arg_(component);
                break;
            case opt_post_version :
            case opt_post_prodversion :
                _append_post_arg_(version);
                break;
            case opt_post_op_sys :
                _append_post_arg_(op_sys);
                break;
            case opt_post_platform :
                _append_post_arg_(platform);
                break;
            case opt_post_priority :
                _append_post_arg_(priority);
                break;
            case opt_post_alias :
                _append_post_arg_(alias);
                break;
            case opt_post_cc :
                _append_post_arg_(cc);
                break;
            case opt_post_append_command :
                _append_post_arg_(append_command);
                break;
            case opt_post_batch :
                bugz_post_arguments.batch = TRUE;
                break;
            case opt_post_default_confirm :
                bugz_post_arguments.default_confirm = -optind;
                break;
            }
        }
    }
    if (bugz_post_arguments.default_confirm < 0) {
        char *p = argv[-bugz_post_arguments.default_confirm-1];
        if (*p != 'y' && *p != 'Y' &&
            *p != 'n' && *p != 'N') {
            fprintf(stderr, N_("ERROR: %s post: --default-confirm: invalid choice: "
                               "'%s' (choose from 'y', 'Y', 'n', 'N')\n"), argv[0], p);
            exit(1);
        }
        bugz_post_arguments.default_confirm = *p;
    }
    else
        bugz_post_arguments.default_confirm = 'y';
    if (bugz_post_arguments.description_from) {
        FILE *fp;
        char *q, *p = bugz_post_arguments.description_from->data;
        int i = 0, s = 1024;
        if (strcmp(p, "-")) {
            if ((fp = fopen(p, "rb")) == NULL) {
                fprintf(stderr,
                        N_("ERROR: unable to read file for '--description-from': %s\n"), p);
                exit(1);
            }
        }
        else
            fp = stdin;
        p = q = NULL;
        while (feof(fp) == 0) {
            if (i == 0) {
                i += s;
                p = (char *)malloc(i);
                memset(p, 0, i);
            }
            if (strlen(p) >= i-1) {
                i += s;
                q = (char *)malloc(i);
                memset(q, 0, i);
                strcpy(q, p);
                free(p);
                p = q;
            }
            fgets(p+strlen(p), i - strlen(p), fp);
        }
        curl_slist_free_all(bugz_post_arguments.description);
        bugz_post_arguments.description = curl_slist_append(NULL, p);
        free(p);
        if (fp != stdin)
            fclose(fp);
    }
    if (bugz_post_arguments.batch == FALSE)
        bugz_post_prompt_for_bug();
    if (bugz_post_arguments.product == NULL) {
        fprintf(stderr, N_("ERROR: %s post: Product not specified\n"),
                        argv[0]);
        exit(1);
    }
    if (bugz_post_arguments.component == NULL) {
        fprintf(stderr, N_("ERROR: %s post: Component not specified\n"),
                        argv[0]);
        exit(1);
    }
    if (bugz_post_arguments.title == NULL) {
        fprintf(stderr, N_("ERROR: %s post: Title not specified\n"),
                        argv[0]);
        exit(1);
    }
    if (bugz_post_arguments.description == NULL) {
        fprintf(stderr, N_("ERROR: %s post: Description not specified\n"),
                        argv[0]);
        exit(1);
    }
    if (bugz_post_arguments.append_command &&
        strlen(bugz_post_arguments.append_command->data))
        bugz_post_append_command();

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
        sprintf(url, "%s/rest/bug?login=%s&password=%s", base,
                username, password);
    else if (username)
        sprintf(url, "%s/rest/bug?api_key=%s", base, username);
    else
        sprintf(url, "%s/rest/bug", base);
    bugz_config_free(config);
    
    json = json_object_new_object();
    if (1) {
        int j;
        json_object *val;
        for (j=0; j<bugz_arguments.columns; j++)
            fprintf(stdout, "%c", '-');
        fprintf(stdout, "\n");

        val = bugz_slist_to_json_string(bugz_post_arguments.product);
        json_object_object_add(json, "product", val);
        fprintf(stdout, "%-12s: %s\n", "Product", json_object_get_string(val));

        val = bugz_slist_to_json_string(bugz_post_arguments.component);
        json_object_object_add(json, "component", val);
        fprintf(stdout, "%-12s: %s\n", "Component", json_object_get_string(val));

        val = bugz_slist_to_json_string(bugz_post_arguments.title);
        json_object_object_add(json, "summary", val);
        fprintf(stdout, "%-12s: %s\n", "Title", json_object_get_string(val));

        if (bugz_post_arguments.version) {
            val = bugz_slist_to_json_string(bugz_post_arguments.version);
            json_object_object_add(json, "version", val);
            fprintf(stdout, "%-12s: %s\n", "Version", json_object_get_string(val));
        }

        val = NULL;
        if (bugz_post_arguments.description) {
            val = bugz_slist_to_json_string(bugz_post_arguments.description);
            json_object_object_add(json, "description", val);
        }
        fprintf(stdout, "%-12s: %s\n", "Description", json_object_get_string(val));

        if (bugz_post_arguments.op_sys) {
            val = bugz_slist_to_json_string(bugz_post_arguments.op_sys);
            json_object_object_add(json, "op_sys", val);
            fprintf(stdout, "%-12s: %s\n", "Operating System", json_object_get_string(val));
        }
        if (bugz_post_arguments.platform) {
            val = bugz_slist_to_json_string(bugz_post_arguments.platform);
            json_object_object_add(json, "platform", val);
            fprintf(stdout, "%-12s: %s\n", "Platform", json_object_get_string(val));
        }
        if (bugz_post_arguments.priority) {
            val = bugz_slist_to_json_string(bugz_post_arguments.priority);
            json_object_object_add(json, "priority", val);
            fprintf(stdout, "%-12s: %s\n", "Priority", json_object_get_string(val));
        }
        if (bugz_post_arguments.severity) {
            val = bugz_slist_to_json_string(bugz_post_arguments.severity);
            json_object_object_add(json, "severity", val);
            fprintf(stdout, "%-12s: %s\n", "Severity", json_object_get_string(val));
        }
        if (bugz_post_arguments.alias) {
            val = bugz_slist_to_json_array(bugz_post_arguments.alias, json_type_string);
            json_object_object_add(json, "alias", val);
            fprintf(stdout, "%-12s: %s\n", "Alias", json_object_get_string(val));
        }
        if (bugz_post_arguments.assigned_to) {
            val = bugz_slist_to_json_string(bugz_post_arguments.assigned_to);
            json_object_object_add(json, "assigned_to", val);
            fprintf(stdout, "%-12s: %s\n", "Assigned to", json_object_get_string(val));
        }
        if (bugz_post_arguments.cc) {
            char *q, *p, buf[1024] = { 0 };
            struct curl_slist *last = bugz_slist_get_last(bugz_post_arguments.cc);
            val = json_object_new_array();
            p = last->data;
            while ((q = strchr(p, ',')) != NULL){
                memcpy(buf, p, q - p);
                buf[q-p+1] = 0;
                json_object_array_add(val, json_object_new_string(buf));
                p = q + 1;
            }
            if (strlen(p))
                json_object_array_add(val, json_object_new_string(p));
            json_object_object_add(json, "cc", val);
            fprintf(stdout, "%-12s: %s\n", "CC", json_object_get_string(val));
        }
        if (bugz_post_arguments.url) {
            val = bugz_slist_to_json_string(bugz_post_arguments.url);
            json_object_object_add(json, "url", val);
            fprintf(stdout, "%-12s: %s\n", "URL", json_object_get_string(val));
        }
        for (j=0; j<bugz_arguments.columns; j++)
            fprintf(stdout, "%c", '-');
        fprintf(stdout, "\n");
    }
    if (bugz_post_arguments.batch == FALSE) {
        char *p, confirm[1024] = {0};
        if (bugz_post_arguments.default_confirm == 'y' ||
            bugz_post_arguments.default_confirm == 'Y') {
            fprintf(stderr, "Confirm bug submission (Y/n)? ");
            fgets(confirm, sizeof(confirm), stdin);
        }
        else {
            fprintf(stderr, "Confirm bug submission (y/N)? ");
            fgets(confirm, sizeof(confirm), stdin);
        }
        p = strchr(confirm, '\n');
        if (p)
            *p = '\0';
        if (strlen(confirm) == 0) {
            confirm[0] = (char)bugz_post_arguments.default_confirm;
            confirm[1] = 0;
        }
        if (confirm[0] != 'y' && confirm[0] != 'Y') {
            fprintf(stderr, N_("Submission aborted\n"));
            json_object_put(json);
            exit(1);
        }
    }

    if ((curl = curl_easy_init()) == NULL) {
        fprintf(stderr, N_("ERROR: %s post: curl_easy_init() failed\n"), argv[0]);
        json_object_put(json);
        exit(1);
    }
    headers = curl_slist_append(headers, "charsets: utf-8");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(json));

    fprintf(stderr, N_(" * Info: Using %s\n"), base);
    bugz_get_result(curl, url, &json);
    if (bugz_check_result(json)) {
        json_object *id;
        json_object_object_get_ex(json, "id", &id);
        fprintf(stderr, N_(" * Info: Bug %d submitted\n"),
                        json_object_get_int(id));
        fprintf(stdout, "Bug %d submitted\n", json_object_get_int(id));
        json_object_put(json);
        retval = 0;
    }
    curl_easy_cleanup(curl);

    return retval;
}

