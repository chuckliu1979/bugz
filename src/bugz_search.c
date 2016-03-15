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

static struct option bugz_search_options[] = {
    {"help",             no_argument,       0, 'h'},
    {"alias",            required_argument, 0,  0 },
    {"assigned-to",      required_argument, 0, 'a'},
    {"component",        required_argument, 0, 'C'},
    {"creator",          required_argument, 0, 'r'},
    {"limit",            required_argument, 0, 'l'},
    {"offset",           required_argument, 0,  0 },
    {"op-sys",           required_argument, 0,  0 },
    {"platform",         required_argument, 0,  0 },
    {"priority",         required_argument, 0,  0 },
    {"product",          required_argument, 0,  0 },
    {"resolution",       required_argument, 0,  0 },
    {"severity",         required_argument, 0,  0 },
    {"status",           required_argument, 0, 's'},
    {"version",          required_argument, 0, 'v'},
    {"whiteboard",       required_argument, 0, 'w'},
    {"comments",         no_argument,       0, 'c'},
    {"creation-time",    required_argument, 0,  0 },
    {"last-change-time", required_argument, 0,  0 },
    {"show-status",      no_argument,       0,  0 },
    {"show-priority",    no_argument,       0,  0 },
    {"show-severity",    no_argument,       0,  0 },
    { 0 }
};

typedef enum bugz_search_longopt_t {
    opt_search_help = 0,
    opt_search_alias,
    opt_search_assigned_to,
    opt_search_component,
    opt_search_creator,
    opt_search_limit,
    opt_search_offset,
    opt_search_op_sys,
    opt_search_platform,
    opt_search_priority,
    opt_search_product,
    opt_search_resolution,
    opt_search_severity,
    opt_search_status,
    opt_search_version,
    opt_search_whiteboard,
    opt_search_comments,
    opt_search_creation_time,
    opt_search_last_change_time,
    opt_search_show_status,
    opt_search_show_priority,
    opt_search_show_severity,
    opt_search_end
} bugz_search_longopt_t;

void bugz_search_helper(int status) {
    char help_header[] =
    N_("Usage: bugz search [options] [terms]\n"
       "Search for bugs in Bugzilla\n"
       "\n"
       "Arguments:\n"
       "terms    : strings to search for in title and/or body\n"
       "\n"
       "Valid options:\n"
       "-h [--help]                : show this help message and exit\n"
       "--alias ALIAS              : the unique alias for this bug\n"
       "-a [--assigned-to] ARG     : email the bug is assigned to\n"
       "-C [--component] COMPONENT : restrict by component (one or more)\n"
       "-r [--creator] CREATOR     : email of the persion who created the bug\n"
       "-l [--limit] LIMIT         : limit the number of records returned in a\n"
       "                             search\n"
       "--offset OFFSET            : set the start position for a search\n"
       "--op-sys OP_SYS            : restrict by operating system (one or more)\n"
       "--platform PLATFORM        : restrict by platform (one or more)\n"
       "--priority PRIORITY        : restrict by priority (one or more)\n"
       "--product PRODUCT          : restrict by product (one or more)\n"
       "--resolution RESOLUTION    : restrict by resolution\n"
       "--severity SEVERITY        : restrict by severity (one or more)\n"
       "-s [--status] STATUS       : restrict by status (one or more)\n"
       "-v [--version] VERSION     : restrict by version (one or more)\n"
       "-w [--whiteboard] ARG      : status whiteboard\n"
       "-c [--comments]            : search comments instead of title\n"
       "--creation-time ARG        : bug created at this time or later\n"
       "--last-change-time ARG     : bug modified at this time or later\n"
       "--show-status              : show status of bugs\n"
       "--show-priority            : show priority of bugs\n"
       "--show-severity            : show severity of bugs\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

struct bugz_search_arguments_t {
    struct curl_slist *terms;
    struct curl_slist *alias;
    struct curl_slist *assigned_to;
    struct curl_slist *component;
    struct curl_slist *creator;
    struct curl_slist *op_sys;
    struct curl_slist *platform;
    struct curl_slist *priority;
    struct curl_slist *product;
    struct curl_slist *resolution;
    struct curl_slist *severity;
    struct curl_slist *status;
    struct curl_slist *version;
    struct curl_slist *whiteboard;
    struct curl_slist *creation_time;
    struct curl_slist *last_change_time;
    int limit;
    int offset;
    int comments;
    int show_status;
    int show_priority;
    int show_severity;
};
static struct bugz_search_arguments_t bugz_search_arguments = { 0 };
#define _append_search_arg_(m) bugz_search_arguments.m = \
                               curl_slist_append(bugz_search_arguments.m, optarg)

static void bugz_search_list_bugs(json_object *bugs) {
    int i, len;
    json_object *bug;
    char line[1024] = {0};

    len = json_object_array_length(bugs);
    fprintf(stderr, N_(" * Info: %d bug(s) found.\n"), len);
    for (i=0; i<len; i++) {
        bug = json_object_array_get_idx(bugs, i);
        if (1) {
            json_object *id, *status, *priority, *severity, *assigned_to, *summary;
            
            json_object_object_get_ex(bug, "id", &id);
            sprintf(line, "%d", json_object_get_int(id));
            if (bugz_search_arguments.show_status) {
                json_object_object_get_ex(bug, "status", &status);
                sprintf(line + strlen(line), " %-12s", json_object_get_string(status));
            }
            if (bugz_search_arguments.show_priority) {
                json_object_object_get_ex(bug, "priority", &priority);
                sprintf(line + strlen(line), " %-12s", json_object_get_string(priority));
            }
            if (bugz_search_arguments.show_severity) {
                json_object_object_get_ex(bug, "severity", &severity);
                sprintf(line + strlen(line), " %-12s", json_object_get_string(severity));
            }
            json_object_object_get_ex(bug, "assigned_to", &assigned_to);
            json_object_object_get_ex(bug, "summary", &summary);
            sprintf(line + strlen(line), " %-20s", json_object_get_string(assigned_to));
            sprintf(line + strlen(line), " %s", json_object_get_string(summary));
            line[bugz_arguments.columns] = '\0';
            fprintf(stdout, "%s\n", line);
        }
    }
}

static const char *quote(const char *s, int l) {
    static char p[8192];
    /*static char reserved[] = ";:,=+@&$/\?";*/
    static char always_safe[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789_.-";
    int i;
    char *q;
    p[0] = 0;
    if (l > strlen(s))
        l = strlen(s);
    for (i=0; i<l; i++) {
        if ((q = strchr(always_safe, s[i])) != NULL)
            sprintf(p+strlen(p), "%c", s[i]);
        else
            sprintf(p+strlen(p), "%%%02X", s[i]);
    }
    return p;
}

static const char *quote_plus(const char *s) {
    char *q, *t;
    static char p[8192];

    if ((q = strchr(s, '\x20')) == NULL)
        sprintf(p, "%s", quote(s, strlen(s)));
    else {
        sprintf(p, "%s+", quote(s, q-s));
        while ((t = strchr(++q, '\x20')) != NULL) {
            if (t==q)
                sprintf(p+strlen(p), "+");
            else
                sprintf(p+strlen(p), "%s+", quote(q, t-q));
            q = t;
        }
        sprintf(p+strlen(p), "%s", quote(q, strlen(q)));
    }
    
    return p;
}

static char *bugz_search_urlencode(json_object *json) {
    int i, j=0, c=0;
    char *q, p[8192];
    json_object *ele;
    struct curl_slist *params = NULL;
    json_object_object_foreach(json,key,val) {
        if (strcmp(key, "login") == 0 ||
            strcmp(key, "password") == 0 ||
            strcmp(key, "api_key") == 0) {
            c++;
            sprintf(p, "%s=%s", key, json_object_get_string(val));
            j += strlen(p);
            params = curl_slist_append(params, p);
            continue;
        }
        switch (json_object_get_type(val)) {
        case json_type_int :
        case json_type_double :
        case json_type_boolean :
        case json_type_string :
            c++;
            sprintf(p, "%s=", quote_plus(key));
            sprintf(p+strlen(p), "%s", quote_plus(json_object_get_string(val)));
            j += strlen(p);
            params = curl_slist_append(params, p);
            break;
        case json_type_array :
            for(i=0; i<json_object_array_length(val); i++) {
                ele = json_object_array_get_idx(val, i);
                switch (json_object_get_type(ele)) {
                case json_type_int :
                case json_type_double :
                case json_type_boolean :
                case json_type_string :
                    c++;
                    sprintf(p, "%s=", quote_plus(key));
                    sprintf(p+strlen(p), "%s", quote_plus(json_object_get_string(ele)));
                    j += strlen(p);
                    params = curl_slist_append(params, p);
                    break;
                default :
                    break;
                }
            }
            break;
        default :
            break;
        }
    }
    q = NULL;
    if (params) {
        struct curl_slist *head = params;
        q = (char *)malloc(j+c);
        if (q) {
            c--;
            memset(q, 0, j+c);
            sprintf(q, "%s", head->data);
            i = strlen(head->data);
            head = head->next;
        }
        else
            head = NULL;
        while (c && head) {
            sprintf(q+i, "&%s", head->data);
            c--;
            i += 1 + strlen(head->data);
            head = head->next;
        }
        curl_slist_free_all(params);
    }
    return q;
}

int bugz_search_main(int argc, char **argv) {
    CURL *curl;
    json_object *json;
    char *url, *base, *username, *password;
    int opt, longindex, retval;
    struct bugz_config_t *config;
    struct curl_slist *headers = NULL;

    optind++;
    bugz_search_arguments.offset = -1;
    while (optind < argc) {
        opt = getopt_long(argc, argv, "-:ha:C:r:l:s:v:w:c",
                          bugz_search_options, &longindex);
        switch (opt) {
        case -1 :
            bugz_search_arguments.terms = \
            curl_slist_append(bugz_search_arguments.terms, argv[optind++]);
            break;
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s search: '%s' requires an argument\n") :
                            N_("ERROR: %s search: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_search_helper(opt == 'h' ? 0 : 1);
        case 'a' :
            _append_search_arg_(assigned_to);
            break;
        case 'C' :
            _append_search_arg_(component);
            break;
        case 'r' :
            _append_search_arg_(creator);
            break;
        case 'l' :
            bugz_search_arguments.limit = atoi(optarg);
            break;
        case 's' :
            _append_search_arg_(status);
            break;
        case 'v' :
            _append_search_arg_(version);
            break;
        case 'w' :
            _append_search_arg_(whiteboard);
            break;
        case 'c' :
            bugz_search_arguments.comments = TRUE;
            break;
        case 0 :
            switch (longindex) {
            case opt_search_alias :
                _append_search_arg_(alias);
                break;
            case opt_search_offset :
                bugz_search_arguments.offset = atoi(optarg);
                break;
            case opt_search_op_sys :
                _append_search_arg_(op_sys);
                break;
            case opt_search_platform :
                _append_search_arg_(platform);
                break;
            case opt_search_priority :
                _append_search_arg_(priority);
                break;
            case opt_search_product :
                _append_search_arg_(product);
                break;
            case opt_search_resolution :
                _append_search_arg_(resolution);
                break;
            case opt_search_severity :
                _append_search_arg_(severity);
                break;
            case opt_search_creation_time :
                _append_search_arg_(creation_time);
                break;
            case opt_search_last_change_time :
                _append_search_arg_(last_change_time);
                break;
            case opt_search_show_status :
                bugz_search_arguments.show_status = TRUE;
                break;
            case opt_search_show_priority :
                bugz_search_arguments.show_priority = TRUE;
                break;
            case opt_search_show_severity :
                bugz_search_arguments.show_severity = TRUE;
                break;
            }
        }
    }

    config = bugz_config();
    base = bugz_get_base(config);
    if (base == NULL) {
        fprintf(stderr, N_("ERROR: No base URL specified\n"));
        bugz_config_free(config);
        exit(1);
    }
    url = username = password = NULL;
    if (bugz_arguments.skip_auth == NULL) {
        username = bugz_get_auth(config, &password);
        if (username == NULL) {
            fprintf(stderr, N_("ERROR: failed to get auth\n"));
            bugz_config_free(config);
            exit(1);
        }
    }
    if (bugz_search_arguments.status == NULL)
        bugz_search_arguments.status = bugz_get_search_statuses(config);

    bugz_config_free(config);
    fprintf(stderr, N_(" * Info: Using %s\n"), base);

    json = json_object_new_object();
    #define _add_string_(n) if (bugz_search_arguments.n)         \
                                json_object_object_add(json, #n, \
                                bugz_slist_to_json_string(bugz_search_arguments.n));
    #define _add_array_(n)  if (bugz_search_arguments.n)         \
                                json_object_object_add(json, #n, \
                                bugz_slist_to_json_array(bugz_search_arguments.n, json_type_string));
    _add_string_(alias)
    _add_string_(assigned_to)
    _add_array_(component)
    _add_string_(creator)
    _add_array_(op_sys)
    _add_array_(platform)
    _add_array_(priority)
    _add_array_(product)
    _add_string_(resolution)
    _add_array_(severity)
    _add_array_(version)
    _add_string_(whiteboard)
    _add_string_(creation_time)
    _add_string_(last_change_time)

    if (bugz_search_arguments.limit > 0)
        json_object_object_add(json, "limit",
        json_object_new_int(bugz_search_arguments.limit));
    if (bugz_search_arguments.offset >= 0)
        json_object_object_add(json, "offset",
        json_object_new_int(bugz_search_arguments.offset));
    if (bugz_search_arguments.status) {
        int all = FALSE;
        struct curl_slist *head = bugz_search_arguments.status;
        while (head) {
            if (strcmp(head->data, "all") == 0) {
                all = TRUE;
                break;
            }
            head = head->next;
        }
        if (all == FALSE)
            json_object_object_add(json, "status",
            bugz_slist_to_json_array(bugz_search_arguments.status, json_type_string));
    }
    if (bugz_search_arguments.terms) {
        int i;
        char *q, *p = NULL;
        struct curl_slist *head = bugz_search_arguments.terms;
        while (head) {
            i = strlen(head->data);
            if (p == NULL) {
                p = (char *)malloc(i+1);
                memcpy(p, head->data, i);
                p[i] = '\0';
            }
            else {
                q = (char *)malloc(i+strlen(p)+2);
                memcpy(q, p, strlen(p));
                q[strlen(p)] = '\x20';
                memcpy(q+strlen(p)+1, head->data, i);
                q[i+strlen(p)+1] = '\0';
                free(p);
                p = q;
            }
            head = head->next;
        }
        q = p + strlen(p);
        while (q > p && isspace((unsigned char)(*--q)))
            *p = '\0';
        q = p;
        while (*q && isspace((unsigned char)(*q)))
            q++;
        if (bugz_search_arguments.comments) {
            json_object_object_add(json, "longdesc_type",
            json_object_new_string("substring"));
            json_object_object_add(json, "query_format",
            json_object_new_string("advanced"));
            json_object_object_add(json, "longdesc",
            json_object_new_string(q));
            if (strchr(q, '\x20')) { /* get every bug because of the space, so set limit = 1 for safe*/
                json_object *limit = json_object_new_int(1);
                json_object_object_add(json, "limit", limit);
            }
        }
        else {
            json_object_object_add(json, "summary",
            json_object_new_string(q));
        }
        free(p);
    }
    if (json_object_object_length(json) <= 0) {
        fprintf(stderr, N_("Please give search terms or options.\n"));
        json_object_put(json);
        exit(1);
    }
    fprintf(stderr, N_(" * Info: Searching for bugs meeting the following criteria:\n"));
    if (1) {
        json_object_object_foreach(json,key,val) {
            fprintf(stderr, " * Info: %-20s = %s\n", key, json_object_get_string(val));
        }
    }
    if (password) {
        json_object_object_add(json, "login",
        json_object_new_string(username));
        json_object_object_add(json, "password",
        json_object_new_string(password));
    }
    else if (username) {
        json_object_object_add(json, "api_key",
        json_object_new_string(username));
    }

    if ((url = bugz_search_urlencode(json)) != NULL) {
        int i = strlen(base) + strlen("/rest/bug?") + strlen(url);
        char *p = (char *)malloc(i+1);
        memset(p, 0, i+1);
        sprintf(p, "%s/rest/bug?%s", base, url);
        free(url);
        url = p;
    }
    json_object_put(json);
    if (url == NULL) {
        fprintf(stderr, N_("ERROR: %s search: urlencode failed\n"), argv[0]);
        exit(1);
    }

    if ((curl = curl_easy_init()) == NULL) {
        fprintf(stderr, N_("ERROR: %s search: curl_easy_init() failed\n"), argv[0]);
        exit(1);
    }
    headers = curl_slist_append(headers, "charsets: utf-8");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    bugz_get_result(curl, url, &json);
    if (bugz_check_result(json)) {
        json_object *bugs;
        json_object_object_get_ex(json, "bugs", &bugs);
        if (json_object_array_length(bugs) <= 0)
            fprintf(stderr, N_(" * Info: No bugs found.\n"));
        else
            bugz_search_list_bugs(bugs);
        json_object_put(json);
        retval = 0;
    }
    free(url);
    curl_easy_cleanup(curl);

    return retval;
}

