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

#ifndef __BUGZ_H__
#define __BUGZ_H__

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <magic.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <curl/curl.h>
#include <json-c/json.h>

#ifndef TRUE
#define TRUE 1
#endif/*TRUE*/

#ifndef FALSE
#define FALSE 0
#endif/*FALSE*/

#define _(str) gettext(str)
#define gettext_noop(str) str
#define N_(str) gettext_noop(str)

struct bugz_arguments_t {
    char *help;
    char *config_file;
    char *connection;
    char *base;
    char *user;
    char *password;
    char *passwordcmd;
    char *key;
    char *quiet;
    char *optarg_debug;
    char *optarg_columns;
    char *encoding;
    char *skip_auth;
    char *version;
    int debug;
    int columns;
};
extern struct bugz_arguments_t bugz_arguments;
struct curl_slist *bugz_slist_get_last(struct curl_slist *list);

struct bugz_config_t {
    struct curl_slist *name;        /* section name */
    struct curl_slist *base;        /* base URL of bugzilla */
    struct curl_slist *user;        /* username for commands requiring authentication */
    struct curl_slist *password;    /* password for commands requiring authentication */
    struct curl_slist *passwordcmd; /* password command to evaluate for commands requiring
                                     * authentication
                                     */

    struct curl_slist *key;        /* key for commands requiring authentication */
    struct curl_slist *encoding;   /* output encoding */
    struct curl_slist *connection; /* use [connection] section of configuration file */

    struct curl_slist *product;
    struct curl_slist *component;
    struct curl_slist *search_statuses;

    int quiet;   /* quiet mode */
    int debug;   /* debug level (from 0 to 3) */
    int columns; /* maximum number of columns output should use */

    struct bugz_config_t *prev;
    struct bugz_config_t *next;
};
struct bugz_config_t *bugz_config(void);
struct bugz_config_t *bugz_config_get(struct bugz_config_t *config, const char *name);
struct bugz_config_t *bugz_config_get_head(struct bugz_config_t *config);
void bugz_config_free(struct bugz_config_t *config);
#define bugz_config_get_default(config) bugz_config_get(config, "default")

char *bugz_get_base(struct bugz_config_t *config);
char *bugz_get_auth(struct bugz_config_t *config, char **pass);
struct curl_slist *bugz_get_search_statuses(struct bugz_config_t *config);

/*
 * URL: scheme://netloc/path;params?query#fragment
 */
struct bugz_parsed_url_t {
    struct curl_slist *scheme;   /* URL scheme specifier */
    struct curl_slist *netloc;   /* Network location part */
    struct curl_slist *path;     /* Hierarchical path  */
    struct curl_slist *params;   /* Parameters for last path element */
    struct curl_slist *query;    /* Query component */
    struct curl_slist *fragment; /* Fragment identifier */
    struct curl_slist *username; /* User name */
    struct curl_slist *password; /* Password */
    struct curl_slist *hostname; /* Host name (lower case) */
    struct curl_slist *port;     /* Port number as integer, if present */
};
char *bugz_urlunparse(struct bugz_parsed_url_t *purl);
struct bugz_parsed_url_t *bugz_urlparse(const char *url);
void bugz_parsed_url_free(struct bugz_parsed_url_t *purl);

json_object *bugz_slist_to_json_string(struct curl_slist *list);
json_object *bugz_slist_to_json_array(struct curl_slist *list, int jtype);

void bugz_show_bug_info(json_object *bug, json_object *attachments, json_object *comments);
CURLcode bugz_get_result(CURL *curl, const char *url, json_object **jsonp);
const char *bugz_get_content_type(const char *filename);
char *bugz_raw_input(const char *prompt);
char *bugz_base64_encode(FILE *infile);
char *bugz_base64_decode(const char *decode, FILE *outfile);

int bugz_check_result(json_object *json);

#endif/*__BUGZ_H__*/

