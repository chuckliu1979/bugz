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

static struct option bugz_modify_options[] = {
    {"help",             no_argument,       0, 'h'},
    {"alias",            required_argument, 0,  0 },
    {"assigned-to",      required_argument, 0, 'a'},
    {"add-blocked",      required_argument, 0,  0 },
    {"remove-blocked",   required_argument, 0,  0 },
    {"add-dependson",    required_argument, 0,  0 },
    {"remove-dependson", required_argument, 0,  0 },
    {"add-cc",           required_argument, 0,  0 },
    {"remove-cc",        required_argument, 0,  0 },
    {"comment",          required_argument, 0, 'c'},
    {"comment-editor",   no_argument,       0, 'C'},
    {"comment-from",     required_argument, 0, 'F'},
    {"component",        required_argument, 0,  0 },
    {"deadline",         required_argument, 0,  0 },
    {"duplicate",        required_argument, 0, 'd'},
    {"estimated-time",   required_argument, 0,  0 },
    {"remaining-time",   required_argument, 0,  0 },
    {"work-time",        required_argument, 0,  0 },
    {"add-group",        required_argument, 0,  0 },
    {"remove-group",     required_argument, 0,  0 },
    {"set-keywords",     required_argument, 0,  0 },
    {"keywords",         required_argument, 0, 'k'},
    {"op-sys",           required_argument, 0,  0 },
    {"platform",         required_argument, 0,  0 },
    {"priority",         required_argument, 0,  0 },
    {"product",          required_argument, 0,  0 },
    {"resolution",       required_argument, 0, 'r'},
    {"add-see-also",     required_argument, 0,  0 },
    {"remove-see-also",  required_argument, 0,  0 },
    {"severity",         required_argument, 0, 'S'},
    {"status",           required_argument, 0, 's'},
    {"title",            required_argument, 0, 't'},
    {"unassign",         no_argument,       0, 'u'},
    {"url",              required_argument, 0, 'U'},
    {"version",          required_argument, 0, 'v'},
    {"whiteboard",       required_argument, 0, 'w'},
    {"fixed",            no_argument,       0,  0 },
    {"invalid",          no_argument,       0,  0 },
    { 0 }
};

typedef enum bugz_modify_longopt_t {
    opt_modify_help = 0,
    opt_modify_alias,
    opt_modify_assigned_to,
    opt_modify_add_blocked,
    opt_modify_remove_blocked,
    opt_modify_add_dependson,
    opt_modify_remove_dependson,
    opt_modify_add_cc,
    opt_modify_remove_cc,
    opt_modify_comment,
    opt_modify_comment_editor,
    opt_modify_comment_from,
    opt_modify_component,
    opt_modify_deadline,
    opt_modify_duplicate,
    opt_modify_estimated_time,
    opt_modify_remaining_time,
    opt_modify_work_time,
    opt_modify_add_group,
    opt_modify_remove_group,
    opt_modify_set_keywords,
    opt_modify_keywords,
    opt_modify_op_sys,
    opt_modify_platform,
    opt_modify_priority,
    opt_modify_product,
    opt_modify_resolution,
    opt_modify_add_see_also,
    opt_modify_remove_see_also,
    opt_modify_severity,
    opt_modify_status,
    opt_modify_title,
    opt_modify_unassign,
    opt_modify_url,
    opt_modify_version,
    opt_modify_whiteboard,
    opt_modify_fixed,
    opt_modify_invalid,
    opt_modify_end
} bugz_modify_longopt_t;

void bugz_modify_helper(int status) {
    char help_header[] =
    N_("Usage: bugz modify [options] bug\n"
       "Modify a bug (e.g. post a comment)\n"
       "\n"
       "Arguments:\n"
       "bug      : the ID of the bug to modify\n"
       "\n"
       "Valid options:\n"
       "-h [--help]             : show this help message and exit\n"
       "--alias ALIAS           : change the alias for this bug\n"
       "-a [--assigned-to] ARG  : change assignee for this bug\n"
       "--add-blocked ARG       : add a bug to the blocked list\n"
       "--remove-blocked ARG    : remove a bug from the blocked list\n"
       "--add-dependson ARG     : add a bug to the depends list\n"
       "--remove-dependson ARG  : remove a bug from the depends list\n"
       "--add-cc CC_ADD         : add an email to the CC list\n"
       "--remove-cc CC_REMOVE   : remove an email from the CC list\n"
       "-c [--comment] COMMENT  : add comment from command line\n"
       "-C [--comment-editor]   : add comment via default editor\n"
       "-F [--comment-from] ARG : add comment from file. If -C is also\n"
       "                          specified, the editor will be opened\n"
       "                          with this file as its contents\n"
       "--component COMPONENT   : change the component for this bug\n"
       "--deadline DEADLINE     : deadline for bug (format YYYY-MM-DD)\n"
       "-d [--duplicate] ARG    : this bug is a duplicate\n"
       "--estimated-time ARG    : total estimate of time required, in hours\n"
       "--remaining-time ARG    : how much work time is remaining, in hours\n"
       "--work-time ARG         : hours spent on this bug\n"
       "--add-group ARG         : add a group to this bug\n"
       "--remove-group ARG      : remove a group from this bug\n"
       "--set-keywords ARG      : set bug keywords\n"
       "-k [--keywords] ARG     : set bug keywords(deprecated)\n"
       "--op-sys OP_SYS         : change the operating system for this bug\n"
       "--platform PLATFORM     : change the hardware platform for this bug\n"
       "--priority PRIORITY     : change the priority for this bug\n"
       "--product PRODUCT       : change the product for this bug\n"
       "-r [--resolution] ARG   : set new resolution (only if status = RESOLVED)\n"
       "--add-see-also ARG      : add a \"see also\" URL to this bug\n"
       "--remove-see-also ARG   : remove a \"see also\" URL from this bug\n"
       "-S [--severity] ARG     : set severity for this bug\n"
       "-s [--status] STATUS    : set new status of bug (e.g. RESOLVED)\n"
       "-t [--title] SUMMARY    : set title of bug\n"
       "-u [--unassign]         : reassign the bug to the default owner\n"
       "-U [--url] URL          : set URL field of bug\n"
       "-v [--version] VERSION  : set the version for this bug\n"
       "-w [--whiteboard] ARG   : set status whiteboard\n"
       "--fixed                 : mark bug as RESOLVED/FIXED\n"
       "--invalid               : mark bug as RESOLVED/INVALID\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

struct bugz_modify_arguments_t {
    struct curl_slist *alias;
    struct curl_slist *assigned_to;
    struct curl_slist *add_blocked;
    struct curl_slist *remove_blocked;
    struct curl_slist *add_dependson;
    struct curl_slist *remove_dependson;
    struct curl_slist *add_cc;
    struct curl_slist *remove_cc;
    struct curl_slist *comment;
    struct curl_slist *comment_from;
    struct curl_slist *component;
    struct curl_slist *deadline;
    struct curl_slist *duplicate;
    struct curl_slist *estimated_time;
    struct curl_slist *remaining_time;
    struct curl_slist *work_time;
    struct curl_slist *add_group;
    struct curl_slist *remove_group;
    struct curl_slist *set_keywords;
    struct curl_slist *op_sys;
    struct curl_slist *platform;
    struct curl_slist *priority;
    struct curl_slist *product;
    struct curl_slist *resolution;
    struct curl_slist *add_see_also;
    struct curl_slist *remove_see_also;
    struct curl_slist *severity;
    struct curl_slist *status;
    struct curl_slist *title;
    struct curl_slist *url;
    struct curl_slist *version;
    struct curl_slist *whiteboard;
    int bug;
    int comment_editor;
    int unassign;
    int fixed;
    int invalid;
};
static struct bugz_modify_arguments_t bugz_modify_arguments = { 0 };
#define _append_modify_arg_(m) bugz_modify_arguments.m = \
                               curl_slist_append(bugz_modify_arguments.m, optarg)

int bugz_modify_main(int argc, char **argv) {
    CURL *curl;
    json_object *json;
    char url[PATH_MAX] = {0};
    char *base, *username, *password;
    int opt, longindex, retval = 1;
    struct bugz_config_t *config;
    struct curl_slist *headers = NULL;
    int has_comment = FALSE;
    int has_work_time = FALSE;

    optind++;
    bugz_modify_arguments.bug = -1;
    while (optind < argc) {
        opt = getopt_long(argc, argv, "-:ha:c:CF:d:k:r:S:s:t:uU:v:w:",
                          bugz_modify_options, &longindex);
        switch (opt) {
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s modify: '%s' requires an argument\n") :
                            N_("ERROR: %s modify: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_modify_helper(opt == 'h' ? 0 : 1);
        case 'a' :
            _append_modify_arg_(assigned_to);
            break;
        case 'c' :
            _append_modify_arg_(comment);
            break;
        case 'C' :
            bugz_modify_arguments.comment_editor = TRUE;
            break;
        case 'F' :
            _append_modify_arg_(comment_from);
            break;
        case 'd' :
            _append_modify_arg_(duplicate);
            break;
        case 'k' :
            _append_modify_arg_(set_keywords);
            break;
        case 'r' :
            _append_modify_arg_(resolution);
            break;
        case 'S' :
            _append_modify_arg_(severity);
            break;
        case 's' :
            _append_modify_arg_(status);
            break;
        case 't' :
            _append_modify_arg_(title);
            break;
        case 'u' :
            bugz_modify_arguments.unassign = TRUE;
            break;
        case 'U' :
            _append_modify_arg_(url);
            break;
        case 'v' :
            _append_modify_arg_(version);
            break;
        case 'w' :
            _append_modify_arg_(whiteboard);
            break;
        case -1:
            bugz_modify_arguments.bug = atoi(argv[optind++]);
            break;
        case 0 :
            switch (longindex) {
            case opt_modify_alias :
                _append_modify_arg_(alias);
                break;
            case opt_modify_add_blocked :
                _append_modify_arg_(add_blocked);
                break;
            case opt_modify_remove_blocked :
                _append_modify_arg_(remove_blocked);
                break;
            case opt_modify_add_dependson :
                _append_modify_arg_(add_dependson);
                break;
            case opt_modify_remove_dependson :
                _append_modify_arg_(remove_dependson);
                break;
            case opt_modify_add_cc :
                _append_modify_arg_(add_cc);
                break;
            case opt_modify_remove_cc :
                _append_modify_arg_(remove_cc);
                break;
            case opt_modify_component :
                _append_modify_arg_(component);
                break;
            case opt_modify_deadline :
                _append_modify_arg_(deadline);
                break;
            case opt_modify_estimated_time :
                _append_modify_arg_(estimated_time);
                break;
            case opt_modify_remaining_time :
                _append_modify_arg_(remaining_time);
                break;
            case opt_modify_work_time :
                _append_modify_arg_(work_time);
                break;
            case opt_modify_add_group :
                _append_modify_arg_(add_group);
                break;
            case opt_modify_remove_group :
                _append_modify_arg_(remove_group);
                break;
            case opt_modify_set_keywords :
                _append_modify_arg_(set_keywords);
                break;
            case opt_modify_op_sys :
                _append_modify_arg_(op_sys);
                break;
            case opt_modify_platform :
                _append_modify_arg_(platform);
                break;
            case opt_modify_priority :
                _append_modify_arg_(priority);
                break;
            case opt_modify_product :
                _append_modify_arg_(product);
                break;
            case opt_modify_add_see_also :
                _append_modify_arg_(add_see_also);
                break;
            case opt_modify_remove_see_also :
                _append_modify_arg_(remove_see_also);
                break;
            case opt_modify_fixed :
                bugz_modify_arguments.fixed = TRUE;
                break;
            case opt_modify_invalid :
                bugz_modify_arguments.invalid = TRUE;
                break;
            }
        }
    }
    if (bugz_modify_arguments.bug <= 0) {
        fprintf(stderr, bugz_modify_arguments.bug == -1 ?
                        N_("ERROR: %s modify: no bug specified\n"):
                        N_("ERROR: %s modify: invalid bug specified\n"), argv[0]);
        exit(1);
    }
    if (bugz_modify_arguments.assigned_to && bugz_modify_arguments.unassign) {
        fprintf(stderr, N_("ERROR: %s modify: --assigned-to and --unassign cannot be used together\n"),
                        argv[0]);
        exit(1);
    }
    if (bugz_modify_arguments.comment_from) {
        FILE *fp;
        char *q, *p = bugz_modify_arguments.comment_from->data;
        int i = 0, s = 1024;
        if (strcmp(p, "-")) {
            if ((fp = fopen(p, "rb")) == NULL) {
                fprintf(stderr, N_("ERROR: unable to read file for '--comment-from': %s\n"), p);
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
        curl_slist_free_all(bugz_modify_arguments.comment);
        bugz_modify_arguments.comment = curl_slist_append(NULL, p);
        free(p);
        if (fp != stdin)
            fclose(fp);
    }
    if (bugz_modify_arguments.comment_editor) {
        char *p = bugz_raw_input("Enter comment:");
        curl_slist_free_all(bugz_modify_arguments.comment);
        bugz_modify_arguments.comment = curl_slist_append(NULL, p);
        free(p);
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
                bugz_modify_arguments.bug, username, password);
    else if (username)
        sprintf(url, "%s/rest/bug/%d?api_key=%s", base,
                bugz_modify_arguments.bug, username);
    else
        sprintf(url, "%s/rest/bug/%d", base, bugz_modify_arguments.bug);

    bugz_config_free(config);
    if ((curl = curl_easy_init()) == NULL) {
        fprintf(stderr, N_("ERROR: %s modify: curl_easy_init() failed\n"), argv[0]);
        exit(1);
    }
    headers = curl_slist_append(headers, "charsets: utf-8");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    json = json_object_new_object();
    if (1) {
        json_object *jarray = json_object_new_array();
        json_object_array_add(jarray, json_object_new_int(bugz_modify_arguments.bug));
        json_object_object_add(json, "ids", jarray);

        if (bugz_modify_arguments.alias)
            json_object_object_add(json, "alias", 
            bugz_slist_to_json_string(bugz_modify_arguments.alias));

        if (bugz_modify_arguments.assigned_to)
            json_object_object_add(json, "assigned_to", 
            bugz_slist_to_json_string(bugz_modify_arguments.assigned_to));

        if (bugz_modify_arguments.unassign)
            json_object_object_add(json, "reset_assigned_to", 
            json_object_new_boolean(bugz_modify_arguments.unassign));

        if (bugz_modify_arguments.add_blocked) {
            int j;
            json_object *blocks;
            j=json_object_object_get_ex(json, "blocks", &blocks);
            if (j == 0) {
                blocks = json_object_new_object();
                json_object_object_add(json, "blocks", blocks);
            }
            json_object_object_add(blocks, "add",
            bugz_slist_to_json_array(bugz_modify_arguments.add_blocked, json_type_int));
        }

        if (bugz_modify_arguments.remove_blocked) {
            int j;
            json_object *blocks;
            j=json_object_object_get_ex(json, "blocks", &blocks);
            if (j == 0) {
                blocks = json_object_new_object();
                json_object_object_add(json, "blocks", blocks);
            }
            json_object_object_add(blocks, "remove",
            bugz_slist_to_json_array(bugz_modify_arguments.remove_blocked, json_type_int));
        }

        if (bugz_modify_arguments.add_dependson) {
            int j;
            json_object *depends_on;
            j=json_object_object_get_ex(json, "depends_on", &depends_on);
            if (j == 0) {
                depends_on = json_object_new_object();
                json_object_object_add(json, "depends_on", depends_on);
            }
            json_object_object_add(depends_on, "add",
            bugz_slist_to_json_array(bugz_modify_arguments.add_dependson, json_type_int));
        }

        if (bugz_modify_arguments.remove_dependson) {
            int j;
            json_object *depends_on;
            j=json_object_object_get_ex(json, "depends_on", &depends_on);
            if (j == 0) {
                depends_on = json_object_new_object();
                json_object_object_add(json, "depends_on", depends_on);
            }
            json_object_object_add(depends_on, "remove",
            bugz_slist_to_json_array(bugz_modify_arguments.remove_dependson, json_type_int));
        }

        if (bugz_modify_arguments.add_cc) {
            int j;
            json_object *cc;
            j=json_object_object_get_ex(json, "cc", &cc);
            if (j == 0) {
                cc = json_object_new_object();
                json_object_object_add(json, "cc", cc);
            }
            json_object_object_add(cc, "add",
            bugz_slist_to_json_array(bugz_modify_arguments.add_cc, json_type_string));
        }

        if (bugz_modify_arguments.remove_cc) {
            int j;
            json_object *cc;
            j=json_object_object_get_ex(json, "cc", &cc);
            if (j == 0) {
                cc = json_object_new_object();
                json_object_object_add(json, "cc", cc);
            }
            json_object_object_add(cc, "remove",
            bugz_slist_to_json_array(bugz_modify_arguments.remove_cc, json_type_string));
        }

        if (bugz_modify_arguments.comment) {
            int j;
            json_object *comment;
            j=json_object_object_get_ex(json, "comment", &comment);
            if (j == 0) {
                comment = json_object_new_object();
                json_object_object_add(json, "comment", comment);
            }
            has_comment = TRUE;
            json_object_object_add(comment, "body",
            bugz_slist_to_json_string(bugz_modify_arguments.comment));
        }

        if (bugz_modify_arguments.component)
            json_object_object_add(json, "component",
            bugz_slist_to_json_string(bugz_modify_arguments.component));

        if (bugz_modify_arguments.deadline)
            json_object_object_add(json, "deadline",
            bugz_slist_to_json_string(bugz_modify_arguments.deadline));

        if (bugz_modify_arguments.duplicate)
            json_object_object_add(json, "dupe_of", 
            json_object_new_int(atoi(bugz_modify_arguments.duplicate->data)));

        if (bugz_modify_arguments.estimated_time)
            json_object_object_add(json, "estimated_time",
            json_object_new_double(atof(bugz_modify_arguments.estimated_time->data)));

        if (bugz_modify_arguments.remaining_time)
            json_object_object_add(json, "remaining_time",
            json_object_new_double(atof(bugz_modify_arguments.remaining_time->data)));

        if (bugz_modify_arguments.work_time) {
            has_work_time = TRUE;
            json_object_object_add(json, "work_time",
            json_object_new_double(atof(bugz_modify_arguments.work_time->data)));
        }

        if (bugz_modify_arguments.add_group) {
            int j;
            json_object *groups;
            j=json_object_object_get_ex(json, "groups", &groups);
            if (j == 0) {
                groups = json_object_new_object();
                json_object_object_add(json, "groups", groups);
            }
            json_object_object_add(groups, "add",
            bugz_slist_to_json_array(bugz_modify_arguments.add_group, json_type_string));
        }

        if (bugz_modify_arguments.remove_group) {
            int j;
            json_object *groups;
            j=json_object_object_get_ex(json, "groups", &groups);
            if (j == 0) {
                groups = json_object_new_object();
                json_object_object_add(json, "groups", groups);
            }
            json_object_object_add(groups, "remove",
            bugz_slist_to_json_array(bugz_modify_arguments.remove_group, json_type_string));
        }

        if (bugz_modify_arguments.set_keywords) {
            int j;
            json_object *keywords;
            j=json_object_object_get_ex(json, "keywords", &keywords);
            if (j == 0) {
                keywords = json_object_new_object();
                json_object_object_add(json, "keywords", keywords);
            }
            json_object_object_add(keywords, "set",
            bugz_slist_to_json_array(bugz_modify_arguments.set_keywords, json_type_string));
        }

        if (bugz_modify_arguments.op_sys)
            json_object_object_add(json, "op_sys",
            bugz_slist_to_json_string(bugz_modify_arguments.op_sys));

        if (bugz_modify_arguments.platform)
            json_object_object_add(json, "platform",
            bugz_slist_to_json_string(bugz_modify_arguments.platform));

        if (bugz_modify_arguments.priority)
            json_object_object_add(json, "priority",
            bugz_slist_to_json_string(bugz_modify_arguments.priority));

        if (bugz_modify_arguments.product)
            json_object_object_add(json, "product",
            bugz_slist_to_json_string(bugz_modify_arguments.product));

        if (bugz_modify_arguments.resolution) {
            if (bugz_modify_arguments.duplicate == NULL)
                json_object_object_add(json, "resolution",
                bugz_slist_to_json_string(bugz_modify_arguments.resolution));
        }

        if (bugz_modify_arguments.add_see_also) {
            int j;
            json_object *see_also;
            j=json_object_object_get_ex(json, "see_also", &see_also);
            if (j == 0) {
                see_also = json_object_new_object();
                json_object_object_add(json, "see_also", see_also);
            }
            json_object_object_add(see_also, "add",
            bugz_slist_to_json_array(bugz_modify_arguments.add_see_also, json_type_string));
        }

        if (bugz_modify_arguments.remove_see_also) {
            int j;
            json_object *see_also;
            j=json_object_object_get_ex(json, "see_also", &see_also);
            if (j == 0) {
                see_also = json_object_new_object();
                json_object_object_add(json, "see_also", see_also);
            }
            json_object_object_add(see_also, "remove",
            bugz_slist_to_json_array(bugz_modify_arguments.remove_see_also, json_type_string));
        }

        if (bugz_modify_arguments.severity)
            json_object_object_add(json, "severity",
            bugz_slist_to_json_string(bugz_modify_arguments.severity));

        if (bugz_modify_arguments.status) {
            if (bugz_modify_arguments.duplicate == NULL)
                json_object_object_add(json, "status",
                bugz_slist_to_json_string(bugz_modify_arguments.status));
        }

        if (bugz_modify_arguments.title)
            json_object_object_add(json, "summary",
            bugz_slist_to_json_string(bugz_modify_arguments.title));

        if (bugz_modify_arguments.url)
            json_object_object_add(json, "url",
            bugz_slist_to_json_string(bugz_modify_arguments.url));

        if (bugz_modify_arguments.version)
            json_object_object_add(json, "version",
            bugz_slist_to_json_string(bugz_modify_arguments.version));

        if (bugz_modify_arguments.whiteboard)
            json_object_object_add(json, "whiteboard",
            bugz_slist_to_json_string(bugz_modify_arguments.whiteboard));

        if (bugz_modify_arguments.fixed) {
            json_object_object_add(json, "status",
            json_object_new_string("RESOLVED"));
            json_object_object_add(json, "resolution",
            json_object_new_string("FIXED"));
        }

        if (bugz_modify_arguments.invalid) {
            json_object_object_add(json, "status",
            json_object_new_string("RESOLVED"));
            json_object_object_add(json, "resolution",
            json_object_new_string("INVALID"));
        }
    }
    if (json_object_object_length(json) < 2) {
        fprintf(stderr, N_("No changes were specified\n"));
        json_object_put(json);
        curl_easy_cleanup(curl);
        exit(1);
    }
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(json));

    fprintf(stderr, N_(" * Info: Using %s\n"), base);

    bugz_get_result(curl, url, &json);
    if (bugz_check_result(json)) {
        int j;
        json_object *bug, *bugs, *changes;
        json_object_object_get_ex(json, "bugs", &bugs);
        for (j=0; j<json_object_array_length(bugs); j++) {
            bug = json_object_array_get_idx(bugs, j);
            json_object_object_get_ex(bug, "changes", &changes);
            if (json_object_object_length(changes)) {
                json_object *added, *removed;
                fprintf(stderr, N_(" * Info: Modified the following fields in bug %d\n"),
                                bugz_modify_arguments.bug);
                json_object_object_foreach(changes,key,val) {
                    json_object_object_get_ex(val, "added", &added);
                    json_object_object_get_ex(val, "removed", &removed);
                    fprintf(stderr, " * Info: %-12s: added %s\n",  key, json_object_get_string(added));
                    fprintf(stderr, " * Info: %-12s: removed %s\n",key, json_object_get_string(removed));
                }
            }
            if (has_comment){
                fprintf(stderr, N_(" * Info: Added comment to bug %d\n"),
                                bugz_modify_arguments.bug);
            }
            if (has_work_time){
                fprintf(stderr, N_(" * Info: Updated work_time of bug %d\n"),
                                bugz_modify_arguments.bug);
            }
        }
        json_object_put(json);
        retval = 0;
    }
    curl_easy_cleanup(curl);

    return retval;
}

