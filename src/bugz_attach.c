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
#include <libgen.h>

static struct option bugz_attach_options[] = {
    {"help",         no_argument,       0, 'h'},
    {"content-type", required_argument, 0, 'c'},
    {"description",  required_argument, 0, 'd'},
    {"patch",        no_argument,       0, 'p'},
    {"title",        required_argument, 0, 't'},
    { 0 }
};

void bugz_attach_helper(int status) {
    char help_header[] =
    N_("Usage: bugz attach [options] bug filename\n"
       "Attach the file to a bug\n"
       "\n"
       "Arguments:\n"
       "bug      : the ID of the bug where the file should be attached\n"
       "filename : the name of the file to attach\n"
       "\n"
       "Valid options:\n"
       "-h [--help]                      : show this help message and exit\n"
       "-c [--content-type] CONTENT_TYPE : mimetype of the file e.g. text/plain\n"
       "                                   (default: auto-detect)\n"
       "-d [--description] COMMENT       : a long description of the attachment\n"
       "-p [--patch]                     : attachment is a patch\n"
       "-t [--title] SUMMARY             : a short description of the attachment\n"
       "                                   (default:filename)\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

struct bugz_attach_arguments_t {
    int bug;
    int patch;
    struct curl_slist *filename;
    struct curl_slist *content_type;
    struct curl_slist *description;
    struct curl_slist *title;
};
static struct bugz_attach_arguments_t bugz_attach_arguments = { 0 };
#define _append_attach_arg_(m) bugz_attach_arguments.m = \
                               curl_slist_append(bugz_attach_arguments.m, optarg)

int bugz_attach_main(int argc, char **argv) {
    CURL *curl;
    json_object *json;
    char url[PATH_MAX] = {0};
    char *base, *username, *password;
    int opt, longindex, retval = 1;
    struct bugz_config_t *config;
    struct curl_slist *headers = NULL;

    optind++;
    bugz_attach_arguments.bug = -1;
    while (optind < argc) {
        opt = getopt_long(argc, argv, "-:hc:d:pt:",
                          bugz_attach_options, &longindex);
        switch (opt) {
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s attach: '%s' requires an argument\n") :
                            N_("ERROR: %s attach: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_attach_helper(opt == 'h' ? 0 : 1);
        case 'c' :
            _append_attach_arg_(content_type);
            break;
        case 'd' :
            _append_attach_arg_(description);
            break;
        case 'p' :
            bugz_attach_arguments.patch = TRUE;
            break;
        case 't' :
            _append_attach_arg_(title);
            break;
        case -1 :
            if (bugz_attach_arguments.bug < 0)
                bugz_attach_arguments.bug = atoi(argv[optind++]);
            else
                bugz_attach_arguments.filename = \
                curl_slist_append(bugz_attach_arguments.filename, argv[optind++]);
            break;
        }
    }
    if (bugz_attach_arguments.bug <= 0) {
        fprintf(stderr, bugz_attach_arguments.bug == -1 ?
                        N_("ERROR: %s attach: no bug specified\n"):
                        N_("ERROR: %s attach: invalid bug specified\n"), argv[0]);
        exit(1);
    }
    if (bugz_attach_arguments.filename == NULL) {
        fprintf(stderr, N_("ERROR: %s attach: no attachment file specified\n"), argv[0]);
        exit(1);
    }
    if (access(bugz_attach_arguments.filename->data, R_OK)) {
        fprintf(stderr, N_("ERROR: %s attach: arg filename: %s\n"), argv[0], strerror(errno));
        exit(1);
    }
    if (bugz_attach_arguments.content_type == NULL) {
        bugz_attach_arguments.content_type = \
        curl_slist_append(bugz_attach_arguments.content_type, 
        bugz_get_content_type(bugz_attach_arguments.filename->data));
    }
    if (bugz_attach_arguments.title == NULL)
        bugz_attach_arguments.title = \
        curl_slist_append(bugz_attach_arguments.title,
        basename(bugz_attach_arguments.filename->data));
    if (bugz_attach_arguments.description == NULL) {
        char *desc = bugz_raw_input("Enter optional long description of attachment (Press Ctrl+D to end)\n");
        bugz_attach_arguments.description = \
        curl_slist_append(bugz_attach_arguments.description, desc);
        free(desc);
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
        sprintf(url, "%s/rest/bug/%d/attachment?login=%s&password=%s", base,
                bugz_attach_arguments.bug, username, password);
    else if (username)
        sprintf(url, "%s/rest/bug/%d/attachment?api_key=%s", base,
                bugz_attach_arguments.bug, username);
    else
        sprintf(url, "%s/rest/bug/%d/attachment", base, bugz_attach_arguments.bug);

    bugz_config_free(config);
    if ((curl = curl_easy_init()) == NULL) {
        fprintf(stderr, N_("ERROR: %s attach: curl_easy_init() failed\n"), argv[0]);
        exit(1);
    }
    headers = curl_slist_append(headers, "charsets: utf-8");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    json = json_object_new_object();
    if (1) {
        char *d;
        FILE *fp;
        json_object *jarray = json_object_new_array();
        json_object_array_add(jarray, json_object_new_int(bugz_attach_arguments.bug));
        
        json_object_object_add(json, "ids", jarray);
        json_object_object_add(json, "summary",
                               bugz_slist_to_json_string(bugz_attach_arguments.title));
        json_object_object_add(json, "file_name",
                               bugz_slist_to_json_string(bugz_attach_arguments.filename));
        json_object_object_add(json, "comment",
                               bugz_slist_to_json_string(bugz_attach_arguments.description));
        json_object_object_add(json, "is_patch", json_object_new_int(bugz_attach_arguments.patch));
        if (bugz_attach_arguments.patch == FALSE)
            json_object_object_add(json, "content_type",
                                   bugz_slist_to_json_string(bugz_attach_arguments.content_type));
        if ((fp = fopen(bugz_attach_arguments.filename->data, "rb")) == NULL) {
            fprintf(stderr, N_("ERROR: %s attach: unable to read from '%s'\n"), argv[0], 
                            bugz_attach_arguments.filename->data);
            json_object_put(json);
            curl_easy_cleanup(curl);
            exit(1);
        }
        d = bugz_base64_encode(fp);
        json_object_object_add(json, "data", json_object_new_string(d));
        free(d);
        fclose(fp);
    }
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(json));

    fprintf(stderr, N_(" * Info: Using %s\n"), base);
    bugz_get_result(curl, url, &json);
    if (bugz_check_result(json)) {
        int attachid;
        json_object *ids, *idx0;
        json_object_object_get_ex(json, "ids", &ids);
        idx0 = json_object_array_get_idx(ids, 0);
        attachid = json_object_get_int(idx0);
        fprintf(stderr, N_(" * Info: %s (%d) has been attached to bug %d\n"),
                        bugz_attach_arguments.filename->data, attachid, 
                        bugz_attach_arguments.bug);
        fprintf(stdout, "[Attachment] [%d] [%s]\n", attachid, 
                        bugz_attach_arguments.filename->data);
        json_object_put(json);
        retval = 0;
    }
    curl_easy_cleanup(curl);

    return retval;
}

static struct option bugz_attachment_options[] = {
    {"help", no_argument, 0, 'h'},
    {"view", no_argument, 0, 'v'},
    { 0 }
};

void bugz_attachment_helper(int status) {
    char help_header[] =
    N_("Usage: bugz attachment [options] attachid\n"
       "Get attachment from Bugzilla\n"
       "\n"
       "Arguments:\n"
       "attachid : the ID of the attachment\n"
       "\n"
       "Valid options:\n"
       "-h [--help]  : show this help message and exit\n"
       "-v [--view]  : print attachment rather than save\n"
       "\n"
       "Type 'bugz --help' for valid global options\n");
    fprintf(stderr, "%s", help_header);
    exit(status);
}

struct bugz_attachment_arguments_t {
    int attachid;
    int view;
};
static struct bugz_attachment_arguments_t bugz_attachment_arguments = { 0 };

int bugz_attachment_main(int argc, char **argv) {
    CURL *curl;
    json_object *json;
    char url[PATH_MAX] = {0};
    char *base, *username, *password;
    int opt, longindex, retval = 1;
    struct bugz_config_t *config;
    struct curl_slist *headers = NULL;

    optind++;
    bugz_attachment_arguments.attachid = -1;
    while (optind < argc) {
        opt = getopt_long(argc, argv, "-:hv",
                          bugz_attachment_options, &longindex);
        switch (opt) {
        case ':' :
        case '?' :
            fprintf(stderr, opt == ':' ?
                            N_("ERROR: %s attachment: '%s' requires an argument\n") :
                            N_("ERROR: %s attachment: '%s' is not a recognized option\n") ,
                            argv[0], argv[optind - 1]);
        case 'h' :
            bugz_attachment_helper(opt == 'h' ? 0 : 1);
        case 'v' :
            bugz_attachment_arguments.view = TRUE;
            break;
        case -1 :
            bugz_attachment_arguments.attachid = atoi(argv[optind++]);
            break;
        }
    }
    if (bugz_attachment_arguments.attachid <= 0) {
        fprintf(stderr, bugz_attachment_arguments.attachid == -1 ?
                        N_("ERROR: %s attachment: no attachid specified\n"):
                        N_("ERROR: %s attachment: invalid attachid specified\n"), argv[0]);
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
        sprintf(url, "%s/rest/bug/attachment/%d?login=%s&password=%s", base,
                bugz_attachment_arguments.attachid, username, password);
    else if (username)
        sprintf(url, "%s/rest/bug/attachment/%d?api_key=%s", base,
                bugz_attachment_arguments.attachid, username);
    else
        sprintf(url, "%s/rest/bug/attachment/%d", base, bugz_attachment_arguments.attachid);

    bugz_config_free(config);
    if ((curl = curl_easy_init()) == NULL) {
        fprintf(stderr, N_("ERROR: %s attachment: curl_easy_init() failed\n"), argv[0]);
        exit(1);
    }
    headers = curl_slist_append(headers, "charsets: utf-8");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    fprintf(stderr, N_(" * Info: Using %s\n")
                    N_(" * Info: Getting attachment %d ..\n"), 
                    base, bugz_attachment_arguments.attachid);

    bugz_get_result(curl, url, &json);
    if (bugz_check_result(json)) {
        char attachmentid[32] = {0};
        json_object *attachments, *result, *file_name, *data;
        sprintf(attachmentid, "%d", bugz_attachment_arguments.attachid);
        json_object_object_get_ex(json, "attachments", &attachments);
        json_object_object_get_ex(attachments, attachmentid, &result);
        json_object_object_get_ex(result, "file_name", &file_name);
        json_object_object_get_ex(result, "data", &data);
        fprintf(stderr, N_(" * Info: %s attachment: %s\n"),
                        bugz_attachment_arguments.view ? "Viewing" : "Saving", 
                        json_object_to_json_string(file_name));
        if (bugz_attachment_arguments.view) {
            retval = 0;
            bugz_base64_decode(json_object_get_string(data), stdout);
        }
        else {
            char *p;
            FILE *fp;
            p = (char *)json_object_get_string(file_name);
            if (access(p, R_OK) == 0)
                fprintf(stderr, N_("ERROR: filename %s already exists\n"), p);
            else if ((fp = fopen(p, "wb")) == NULL)
                fprintf(stderr, N_("ERROR: failed to write into %s\n"), p);
            else {
                retval = 0;
                bugz_base64_decode(json_object_get_string(data), fp);
                fclose(fp);
            }
        }
        json_object_put(json);
    }
    curl_easy_cleanup(curl);

    return retval;
}

