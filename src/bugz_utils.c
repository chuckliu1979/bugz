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

#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bugz.h"

/* 
 * configuration files for pybugz, see
 * https://github.com/williamh/pybugz/blob/master/man/pybugz.d.5
 *
 */
struct bugz_config_t *bugz_config_get_head(struct bugz_config_t *config) {
    struct bugz_config_t *head = config;
    while (head && head->prev)
        head = head->prev;
    return head;
}

struct bugz_config_t *bugz_config_get(struct bugz_config_t *config, const char *name) {
    struct bugz_config_t *head;
    head = bugz_config_get_head(config);
    while (head && head->name && strcmp(head->name->data, name))
        head = head->next;
    if (NULL == head || \
        NULL == head->name || \
        strcmp(head->name->data, name))
        head = NULL;
    return head;
}

static inline void bugz_config_init(struct bugz_config_t *config) {
    memset(config, 0, sizeof(struct bugz_config_t));
}

void bugz_config_free(struct bugz_config_t *config) {
    struct bugz_config_t *head = config;
    if (head == NULL)
        return;
    while (head->prev)
        head = head->prev;
    do {
        config = head;
        if (config->next)
            config->next->prev = NULL;

        curl_slist_free_all(head->name);
        curl_slist_free_all(head->base);
        curl_slist_free_all(head->user);
        curl_slist_free_all(head->password);
        curl_slist_free_all(head->passwordcmd);

        curl_slist_free_all(head->key);
        curl_slist_free_all(head->encoding);
        curl_slist_free_all(head->connection);
        
        curl_slist_free_all(head->product);
        curl_slist_free_all(head->component);
        curl_slist_free_all(head->search_statuses);
        
        head = head->next;
        free(config);
    } while (head);
}

static inline char *bugz_rstrip(char *s) {
    char *p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p)))
        *p = '\0';
    return s;
}

static inline char *bugz_lskip(const char *s) {
    while (*s && isspace((unsigned char)(*s)))
        s++;
    return (char *)s;
}

static char *bugz_get_line(FILE *fp) {
    char buf[4096];
    char *nil = NULL;
    char *line = NULL;

    do {
        if (NULL == fgets(buf, sizeof(buf), fp))
            break;
        if (!line) {
            line = strdup(buf);
            if (!line)
                return NULL;
        } else {
            char *ptr;
            size_t len = strlen(line);
            ptr = realloc(line, len + strlen(buf) + 1);
            if (!ptr) {
                free(line);
                return NULL;
            }
            line = ptr;
            strcpy(&line[len], buf);
        }
        nil = strchr(line, '\n');
    } while (!nil);

    if (nil)
        *nil = '\0';

    return line;
}

static void bugz_append_config_val(struct bugz_config_t *config, char *key, char *val) {
    #define _append_config_arg_(m) config->m = curl_slist_append(config->m, val)
    #define _return_if_(m) if (!strcmp(key, #m)) {     \
                               _append_config_arg_(m); \
                               return;                 \
                           }
    _return_if_(base);
    _return_if_(user);
    _return_if_(password);
    _return_if_(passwordcmd);
    _return_if_(key);
    _return_if_(encoding);
    if (!strcmp(key, "connection")) {
        if (!strcmp(config->name->data, "default"))
            _append_config_arg_(connection);
        return;
    }
    _return_if_(product);
    _return_if_(component);
    _return_if_(search_statuses);
    if (!strcmp(key, "quiet")) {
        if (!strcmp(val, "True") || !strcmp(val, "Yes") || \
            !strcmp(val, "true") || !strcmp(val, "yes"))
            config->quiet = TRUE;
            return;
    }
    if (!strcmp(key, "debug")) {
        config->debug = atoi(val) & 0x11;
        return;
    }
    if (!strcmp(key, "columns"))
        config->columns = atoi(val);
}

static struct bugz_config_t *bugz_config_load(struct bugz_config_t *config, const char *filename) {
    FILE *fp;
    char *key, *val, *line;
    struct bugz_config_t *used;

    while (config && config->prev)
        config = config->prev;
    if ((fp = fopen(filename, "rb")) == NULL)
        return config;

    if (config == NULL) {
        if ((config = (struct bugz_config_t *)malloc(sizeof(struct bugz_config_t))) == NULL) {
            fclose(fp);
            return config;
        }
        bugz_config_init(config);
    }
    used = config;

    while ((line = bugz_get_line(fp)) != NULL) {
        val = strchr(line, '#');
        if (val)
            *val = '\0';
        val = strchr(line, ';');
        if (val)
            *val = '\0';
        key = bugz_lskip(line);
        key = bugz_rstrip(key);
        if (*key == '\0')
            goto clean;
        if (*key == '[') { /* section */
            val = strchr(++key, ']');
            if (val)
                *val = '\0';
            if (used->name == NULL || strcmp(key, used->name->data)) {
                struct bugz_config_t *head = config;
                do {
                    if (!head->name)
                        break;
                    if (!strcmp(key, head->name->data))
                        break;
                    head = head->next;
                } while (head);
                if (head)
                    used = head;
                else {
                    head = used;
                    while (head->next)
                        head = head->next;
                    head->next = (struct bugz_config_t *)malloc(sizeof(struct bugz_config_t));
                    bugz_config_init(head->next);
                    head->next->prev = head;
                    used = head->next; 
                }
                if (used->name == NULL)
                    used->name = curl_slist_append(used->name, key);
            }
            goto clean;
        }
        val = strchr(key, '=');
        if (!val)
            val = strchr(key, ':');
        if (!val)
            goto clean;
        *val++ = '\0';
        val = bugz_lskip(val);
        key = bugz_rstrip(key);
        bugz_append_config_val(used, key, val);
clean:  free(line);
    }
    fclose(fp);

    return config;
}

static int bugz_glob_errfunc(const char *epath, int eerrno) {
    if (eerrno != 2)  /* ENOENT 2 : No such file or directory */
        fprintf(stderr, "ERROR: %s: %s\n", epath, strerror(eerrno));
    return 0;
}

static glob_t *bugz_glob_conf(void) {
    int i;
    int flags = GLOB_TILDE;
    glob_t *results;
    results = (glob_t *)malloc(sizeof(glob_t));
    if (results == NULL)
        return NULL;
    memset(results, 0, sizeof(glob_t));
    const char *patterns[] = {"/usr/share/pybugz.d/*.conf",
                              "/usr/share/bugz.d/*.conf",
                              "/etc/pybugz.d/*.conf",
                              "/etc/bugz.d/*.conf",
                              "~/.bugzrc"
                             };
    for(i=0; i<sizeof(patterns)/sizeof(patterns[0]); i++) {
        flags |= (i > 0 ? GLOB_APPEND : 0);
        glob(patterns[i], flags, bugz_glob_errfunc, results);
    }
    return results;
}

static void bugz_update_debug_and_columns(struct bugz_config_t *config) {
    struct bugz_config_t *used = NULL;

    if (config) {
        used = bugz_config_get_default(config);
        if (used) {
            if (bugz_arguments.optarg_debug == NULL)
                bugz_arguments.debug = used->debug;
            if (bugz_arguments.optarg_columns == NULL)
                bugz_arguments.columns = used->columns;
            if (used->connection) {
                struct bugz_config_t *conn = bugz_config_get(config, used->connection->data);
                if (conn) {
                    if (bugz_arguments.optarg_debug == NULL)
                        bugz_arguments.debug = used->debug;
                    if (bugz_arguments.optarg_columns == NULL)
                        bugz_arguments.columns = used->columns;
                }
            }
        }
        if (bugz_arguments.connection) {
            used = bugz_config_get(config, bugz_arguments.connection);
            if (used) {
                if (bugz_arguments.optarg_debug == NULL)
                    bugz_arguments.debug = used->debug;
                if (bugz_arguments.optarg_columns == NULL)
                    bugz_arguments.columns = used->columns;
            }
        }
        if (bugz_arguments.columns < 80) {
            bugz_arguments.columns = 80;
        }
    }
}

struct bugz_config_t *bugz_config(void) {
    glob_t *files;
    struct bugz_config_t *config = NULL;

    files = bugz_glob_conf();
    if (files) {
        int i;
        for (i=0; i<files->gl_pathc; i++) {
            if (strstr(files->gl_pathv[i], "/.bugzrc") && \
                bugz_arguments.config_file)
                continue;
            config = bugz_config_load(config, files->gl_pathv[i]);
        }
        globfree(files);
    }
    if (bugz_arguments.config_file)
        config = bugz_config_load(config, bugz_arguments.config_file);
    if (config)
        bugz_update_debug_and_columns(config);

    return config;
}

struct curl_slist *bugz_slist_get_last(struct curl_slist *list) {
    struct curl_slist *last;
    if (list == NULL)
        return NULL;

    last = list;
    while (last->next)
        last = last->next;

    return last;
}

/* 
 * https://github.com/python-git/python/blob/master/Lib/urlparse.py
 *
 */
static const char uses_relative[] = 
    "#ftp,#http,#gopher,#nntp,#imap,#wais,#file,"
    "#https,#shttp,#mms,#prospero,#rtsp,#rtspu,#sftp,#,";

static const char uses_netloc[] = \
    "#ftp,#http,#gopher,#nntp,#telnet,#imap,#wais,#file,#mms,#https,"
    "#shttp,#snews,#prospero,#rtsp,#rtspu,#rsync,#svn,#svn+ssh,#sftp,#,";

static const char uses_hierarchical[] = \
    "#gopher,#hdl,#mailto,#news,#telnet,#wais,#imap,#snews,#sip,#sips,#,";

static const char uses_params[] = \
    "#ftp,#hdl,#prospero,#http,#imap,#https,#shttp,#rtsp,#rtspu,#sip,#sips,#mms,#sftp,#,";

static const char uses_query[] = \
    "#http,#wais,#imap,#https,#shttp,#mms,#gopher,#rtsp,#rtspu,#sip,#sips,#,";

static const char uses_fragment[] = \
    "#ftp,#hdl,#http,#gopher,#news,#nntp,#wais,#https,#shttp,#snews,#file,#prospero,#,";

static int scheme_use(const char *scheme, const char *use) {
    char buf[32] = {'#',0};
    strncat(buf, scheme, strlen(scheme));
    buf[strlen(buf)] = ',';
    return strstr(use, buf) ? 1 : 0;
}

void bugz_parsed_url_free(struct bugz_parsed_url_t *purl) {
    if (purl) {
        curl_slist_free_all(purl->scheme);
        curl_slist_free_all(purl->netloc);
        curl_slist_free_all(purl->path);
        curl_slist_free_all(purl->params);
        curl_slist_free_all(purl->query);
        curl_slist_free_all(purl->fragment);
        curl_slist_free_all(purl->username);
        curl_slist_free_all(purl->password);
        curl_slist_free_all(purl->hostname);
        curl_slist_free_all(purl->port);
        free(purl);
    }
}

static inline int is_scheme_char(int c) {
    return (c != '+' && \
            c != '-' && \
            c != '.' && \
           !isalpha(c)) ? 0 : 1;
}

static inline char *rfind(const char *str, int c) {
    char *p, *q;
    if (str == NULL)
        return NULL;
    p = q = strchr(str, c);
    if (p == NULL)
        return NULL;
    do {
        p = q;
        if (*++q)
            q = strchr(q, c);
    } while (q);
    return p;
}

static struct bugz_parsed_url_t *netloc_mixin(struct bugz_parsed_url_t *purl) {
    char *p, *q;
    char username[1024] = {0};
    char password[1024] = {0};
    char hostname[1024] = {0};
    char port[1024] = {0};

    if (NULL == purl || \
        NULL == purl->netloc || \
        NULL == purl->netloc->data)
        return purl;

    strncpy(username, purl->netloc->data, sizeof(username)-1);
    strncpy(hostname, purl->netloc->data, sizeof(hostname)-1);

    p = q = rfind(username, '@');
    if (p != NULL) {
        *p = '\0';
        p = strchr(username, ':');
        if (p != NULL) {
            *p++ = '\0';
            strcpy(password, p);
        }
    }
    else
        username[0] = '\0';

    if (q != NULL) {
        *q++ = '\0';
        strcpy(hostname, q);
    }
    q = strchr(hostname, ':');
    if (q != NULL) {
        int c;
        *q++ = '\0';
        c = atoi(q);
        if (c || (c == 0 && *q == '0'))
            strcpy(port, q);
    }
    for (q=hostname; *q != '\0'; q++)
        *q = tolower(*q);
    
    if (strlen(username))
        purl->username = curl_slist_append(purl->username, username);
    if (strlen(password))
        purl->password = curl_slist_append(purl->password, password);
    if (strlen(hostname))
        purl->hostname = curl_slist_append(purl->hostname, hostname);
    if (strlen(port))
        purl->port = curl_slist_append(purl->port, port);

    return purl;
}

char *bugz_urlunparse(struct bugz_parsed_url_t *purl) {
    static char p[PATH_MAX];
    int len = 0;
    char *scheme, *netloc, *path, *params, *query, *fragment;

    if (purl == NULL)
        return NULL;

    #define _init_attr_(a) a = NULL;                         \
                           if (purl->a) {                    \
                               a = purl->a->data;            \
                               len += strlen(purl->a->data); \
                           }
    _init_attr_(scheme)
    _init_attr_(netloc)
    _init_attr_(path)
    _init_attr_(params)
    _init_attr_(query)
    _init_attr_(fragment)
    
    if (len == 0)
        return NULL;
    len += 7;
    len &= ~0x7;
    memset(p, 0, sizeof(p));

    if (scheme)
        strcat(strcpy(p, scheme), ":");
    if (netloc || \
       (scheme && scheme_use(scheme, uses_netloc) && \
        path && strlen(path) >= 2 && \
       (path[0] != '/' || (path[0] == '/' && path[1] != '/')))) {
        strcat(p, "//");
        if (netloc)
            strcat(p, netloc);
        if (path[0] != '/')
            strcat(p,"/");
        strcat(p, path);
    }
    if (params)
        strcat(strcat(p, ";"), params);
    if (query)
        strcat(strcat(p, "?"), query);
    if (fragment)
        strcat(strcat(p, "#"), fragment);
         
    return p;
}

struct bugz_parsed_url_t *bugz_urlparse(const char *url) {
    char *p, *q;
    char scheme[32]= {0};
    char buf[1024] = {0};
    struct bugz_parsed_url_t *purl;

    if ((purl = malloc(sizeof(struct bugz_parsed_url_t))) == NULL)
        return purl;
    memset(purl, 0, sizeof(struct bugz_parsed_url_t));

    p = q = strchr(url, ':');
    if (p) {
        strncpy(scheme, url, p - url);
        for (p=scheme; *p != '\0'; p++) {
            if (is_scheme_char(*p) == 0) {
                scheme[0] = '\0';
                break;
            }
        }
        for (p=scheme; *p != '\0'; p++)
            *p = tolower(*p);
        if (scheme[0])
            url = q + 1;
    }
    if (scheme_use(scheme, uses_netloc) && \
        url[0] == '/' && url[1] == '/') {
        int c;
        url = url + 2;
        c = strcspn(url, "/?#");
        if (c <= 0)
            c = strlen(url);
        strncpy(buf, url, c);
        purl->netloc = curl_slist_append(NULL, buf);
        url = url + c;
    }
    if (scheme_use(scheme, uses_fragment) && \
        (p = strchr(url, '#'))) {
        purl->fragment = curl_slist_append(NULL, p + 1);
        strncpy(buf, url, p - url);
        url = buf;
    }
    if (scheme_use(scheme, uses_query) && \
        (p = strchr(url, '?'))) {
        char tmp[1024] = {0};
        purl->query = curl_slist_append(NULL, p + 1);
        strncpy(tmp, url, p - url);
        strncpy(buf, tmp, p - url);
        url = buf;
    }
    if (scheme_use(scheme, uses_params) && \
        (p = strchr(url, ';'))) {
        if ((q = rfind(url, '/'))) {
            p = strchr(q, ';');
        }
        if (p) {
            char tmp[1024] = {0};
            purl->params = curl_slist_append(NULL, p + 1);
            strncpy(tmp, url, p - url);
            strncpy(buf, tmp, p - url);
            url = buf;
        }
    }
    if (scheme[0])
        purl->scheme = curl_slist_append(NULL, scheme);
    purl->path = curl_slist_append(NULL, url);
    purl = netloc_mixin(purl);

    return purl;
}

struct bugz_fetch_t {
    char *payload;
    size_t size;
};

static json_object *bugz_fetch_to_json(struct bugz_fetch_t *fetch) {
    json_object *json;
    enum json_tokener_error err;

    if (fetch == NULL)
        return NULL;
    json = json_tokener_parse_verbose(fetch->payload, &err);
    if (err != json_tokener_success) {
        fprintf(stderr, N_("ERROR: failed to parse json string\n"));
        json_object_put(json);
        return NULL;
    }
    return json;
}

static size_t bugz_curl_callback(void *data, size_t size, size_t nmemb, void *userp) {
    char *t;
    size_t realsize = size * nmemb;
    struct bugz_fetch_t *p = (struct bugz_fetch_t *)userp;

    t = (char *)realloc(p->payload, p->size + realsize + 1);
    if (t == NULL) {
        fprintf(stderr, "ERROR: expand payload in bugz_curl_callback failed\n");
        return -1;
    }

    p->payload = t;
    memcpy(&(p->payload[p->size]), data, realsize);
    p->size += realsize;
    p->payload[p->size] = 0;
    return realsize;
}

struct bugz_trace_t {
    char trace_ascii; /* 0 or 1*/
};

static void debug_dump(const char *text, FILE *fp, unsigned char *ptr, size_t size, char nohex) {
    size_t i, c;
    unsigned int width=bugz_arguments.columns/2;
    if (nohex)
        width=bugz_arguments.columns;
    fprintf(fp, " * Debug: %s, %10.10ld bytes (0x%8.8lx)\n", text, 
           (long)size, (long)size);
    for(i=0; i<size; i+= width) {
        if (!nohex) {
            fprintf(fp, " * Debug: %4.4lx: ", (long)i);
            for (c=0; c<width; c++)
                if (i+c < size)
                    fprintf(fp, "%02x", ptr[i+c]);
                else
                    fputs("\x20\x20", fp);
            fputc('\n', fp);
        }
        fprintf(fp, " * Debug: %4.4lx: ", (long)i);
        for (c=0; (c<width)&&(i+c<size); c++) {
            if (nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
                i+=(c+2-width);
                break;
            }
            fprintf(fp, "%c", (ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
            if (nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
                i+=(c+3-width);
                break;
            }
        }
        fputc('\n', fp);
    }
    fflush(fp);
}

static int debug_trace(CURL *curl, curl_infotype type, char *data, size_t size, void *userp) {
    struct bugz_trace_t *config = (struct bugz_trace_t *)userp;
    const char *text;
    (void)curl; /* prevent compiler warning */

    switch (type) {
    case CURLINFO_TEXT :
        fprintf(stderr, " * Debug: %s", data);
    default: /* in case a new one is introduced to shock us */
        return 0;
    case CURLINFO_HEADER_OUT :
        text = "=> send header";
        break;
    case CURLINFO_DATA_OUT :
        text = "=> send data";
        break;
    case CURLINFO_SSL_DATA_OUT :
        text = "=> send SSL data";
        break;
    case CURLINFO_HEADER_IN :
        text = "<= recv header";
        break;
    case CURLINFO_DATA_IN :
        text = "<= recv data";
        break;
    case CURLINFO_SSL_DATA_IN :
        text = "<= recv SSL data";
        break;
    }

    debug_dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
    return 0;
}

static CURLcode bugz_get_fetch(CURL *curl, const char *url, struct bugz_fetch_t *fetch) {
    CURLcode rcode;
    struct bugz_trace_t debug_config;

    if (fetch->payload)
        free(fetch->payload);
    fetch->payload = 0;
    fetch->size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, bugz_curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)fetch);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

    if (bugz_arguments.debug > 1) {
        debug_config.trace_ascii = bugz_arguments.debug > 2 ? 0 : 1;
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &debug_config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    rcode = curl_easy_perform(curl);
    return rcode;
}

CURLcode bugz_get_result(CURL *curl, const char *url, json_object **jsonp) {
    CURLcode rcode;
    json_object *json = NULL;
    struct bugz_fetch_t fetch = {0};

    rcode = bugz_get_fetch(curl, url, &fetch);
    if (rcode != CURLE_OK || fetch.size < 1)
        fprintf(stderr, N_("ERROR: %s\n"), curl_easy_strerror(rcode));
    else
        json = bugz_fetch_to_json(&fetch);

    free(fetch.payload);
    *jsonp = json;

    return rcode;
}

char *bugz_get_base(struct bugz_config_t *config) {
    static char base[PATH_MAX];

    char *ptr;
    struct bugz_parsed_url_t *purl;
    
    if (*base)
        return base;

    *base = '\0';
    if (bugz_arguments.base) {
        purl = bugz_urlparse(bugz_arguments.base);
        if (NULL == purl ||
            NULL == purl->scheme ||
            NULL == purl->netloc ||
            NULL == purl->path) {
            bugz_parsed_url_free(purl);
            fprintf(stderr, N_("ERROR: invalid '-b/--base' option : %s\n"),
                            bugz_arguments.base);
            return NULL;
        }
        ptr = strstr(purl->path->data, "/xmlrpc.cgi");
        if (ptr)
            *ptr = '\0';
        ptr = bugz_urlunparse(purl);
        if (ptr) {
            strcpy(base, ptr);
            ptr = base;
        } /* XXX: internal urlunparse failed */
        bugz_parsed_url_free(purl);
        return ptr;
    }

    if (config) {
        struct curl_slist *last;
        struct bugz_config_t *used = NULL;
        if (bugz_arguments.connection) {
            used = bugz_config_get(config, bugz_arguments.connection);
            if (NULL == used)
                return NULL;
        }
        if (NULL == used ||
            NULL == used->base) {
            used = bugz_config_get_default(config);
            if (used && used->connection) {
                struct bugz_config_t *conn = bugz_config_get(config,
                                             used->connection->data);
                if (conn && conn->base)
                    used = conn;
            }
        }
        if (NULL == used ||
            NULL == used->base) {
            return NULL;
        }
        ptr = NULL;
        last = bugz_slist_get_last(used->base);
        purl = bugz_urlparse(last->data); 
        if (purl && purl->scheme && purl->netloc && purl->path) {
            ptr = strstr(purl->path->data, "/xmlrpc.cgi");
            if (ptr)
                *ptr = '\0';
            ptr = bugz_urlunparse(purl);
            if (ptr) {
                strncpy(base, ptr, sizeof(base));
                ptr = base;
            } /* XXX: internal urlunparse failed */
        }
        bugz_parsed_url_free(purl);
        return ptr;
    }

    return NULL;
}

char *bugz_get_auth(struct bugz_config_t *config, char **pass) {
    static char username[PATH_MAX]; /* might be api_key */
    static char password[PATH_MAX]; /* might be empty */
    char *pu, *pp, *pc;

    if (*password)
        return pass ? *pass=password, username : NULL; 
    if (pass)
        *pass = NULL;
    pu = pp = pc = NULL;
    *username = *password = '\0';
    if (bugz_arguments.key) {
        strncpy(username, bugz_arguments.key, sizeof(username));
        return username;
    }
    if (bugz_arguments.user) {
        strncpy(username, bugz_arguments.user, sizeof(username));
        if (bugz_arguments.password) {
            strncpy(password, bugz_arguments.password, sizeof(password));
            return pass ? *pass=password, username : NULL;
        }
    }
    if (config) {
        struct curl_slist *lasta = NULL;
        struct curl_slist *lastu = NULL;
        struct curl_slist *lastp = NULL;
        struct curl_slist *lastc = NULL;
        struct bugz_config_t *used = NULL;
        if (bugz_arguments.connection)
            used = bugz_config_get(config, bugz_arguments.connection);
        if (used) {
            lasta = bugz_slist_get_last(used->key);
            lastu = bugz_slist_get_last(used->user);
            lastp = bugz_slist_get_last(used->password);
            lastc = bugz_slist_get_last(used->passwordcmd);
        }
        if (lasta == NULL) {
            used = bugz_config_get_default(config);
            if (used) {
                int u, p, c;
                u = p = c = FALSE;
                lasta = bugz_slist_get_last(used->key);
                if (lastu == NULL) {
                    u = TRUE;
                    lastu = bugz_slist_get_last(used->user);
                }
                if (lastp == NULL) {
                    p = TRUE;
                    lastp = bugz_slist_get_last(used->password);
                }
                if (lastc == NULL) {
                    c = TRUE;
                    lastc = bugz_slist_get_last(used->passwordcmd);
                }
                if (used->connection) {
                    struct bugz_config_t *conn = bugz_config_get(config, used->connection->data);
                    if (conn && conn->user) {
                        lastu = lastu == NULL ? bugz_slist_get_last(conn->user) :
                                                u ? bugz_slist_get_last(conn->user) : lastu;
                    }
                    if (conn && conn->password) {
                        lastp = lastp == NULL ? bugz_slist_get_last(conn->password) :
                                                p ? bugz_slist_get_last(conn->password) : lastp;
                    }
                    if (conn && conn->passwordcmd) {
                        lastc = lastc == NULL ? bugz_slist_get_last(conn->passwordcmd) :
                                                c ? bugz_slist_get_last(conn->passwordcmd) : lastc;
                    }
                }
            }
        }
        if (lasta) {
            strncpy(username, lasta->data, sizeof(username));
            return username;
        }
        if (lastu)
            pu = lastu->data;
        if (lastp)
            pp = lastp->data;
        if (lastc)
            pc = lastc->data;
    }
    if (bugz_arguments.user)
        pu = bugz_arguments.user;
    if (bugz_arguments.password)
        pp = bugz_arguments.password;
    if (bugz_arguments.passwordcmd)
        pc = bugz_arguments.passwordcmd;

    if (pu)
        strncpy(username, pu, sizeof(username));
    else {
        do {
            /*fprintf(stderr, N_("* No username given.\n"));*/
            fprintf(stdout, N_("Username:"));
            fflush(stdout);
        } while (fgets(username, sizeof(username), stdin) == NULL);
        username[strlen(username)-1] = '\0';
        pp = pc = NULL;
        /* then password/passwordcmd invalid */
    }
    if (pp) {
        strncpy(password, pp, sizeof(password));
        return pass ? *pass=password, username : NULL;
    }
    if (pc) {
        FILE *fd = popen(pc, "r");
        fgets(password, sizeof(password), fd);
        pclose(fd);
        pc = strchr(password, '\n');
        if (pc)
            *pc = '\0';
    }
    if (*password == '\0') {
        /*fprintf(stderr, N_("* No password given.\n"));*/
        pp = getpass(N_("Password:"));
        if (pp)
            strncpy(password, pp, sizeof(password));
    }
    if (*password)
        return pass ? *pass=password, username : NULL;
   
    *username = '\0';
    return NULL;
}

struct curl_slist *bugz_get_search_statuses(struct bugz_config_t *config) {
    struct curl_slist *search_statuses = NULL;
    if (config) {
        char *p, *q, buf[128] = {0};
        struct curl_slist *last;
        struct bugz_config_t *used = NULL;
        if (bugz_arguments.connection)
            used = bugz_config_get(config, bugz_arguments.connection);
        if (NULL == used ||
            NULL == used->search_statuses) {
            used = bugz_config_get_default(config);
            if (used && used->connection) {
                struct bugz_config_t *conn = bugz_config_get(config, used->connection->data);
                if (conn && conn->search_statuses)
                    used = conn;
            }
        }
        if (NULL == used ||
            NULL == used->search_statuses)
            return NULL;
        last = bugz_slist_get_last(used->search_statuses);
        p = last->data;
        while ((q = strchr(p, ',')) != NULL){
            memcpy(buf, p, q - p);
            buf[q-p+1] = 0;
            search_statuses = curl_slist_append(search_statuses, buf);
            p = q + 1;
        }
        if (strlen(p))
            search_statuses = curl_slist_append(search_statuses, p);
    }
    return search_statuses;
}

static char* word_wrap(char *wrap, char *word, int width) {
    int len, eol, pos, next;

    *wrap = '\0';
    for (pos = 0; pos < strlen(word); pos = next) {
        char *p = strchr(word + pos, '\n');
        eol = p ? p - word : -1;
        if (eol == -1)
            next = eol = pos + strlen(word + pos);
        else
            next = eol + 1;
        if (eol > pos) {
            do {
                len = eol - pos;
                if (len > width) {
                    int i = width;
                    while (i >=0 && !isspace(word[pos + i]))
                        i--;
                    if (i < 0)
                        len = width;
                    else {
                        while (i >= 0 && isspace(word[pos + i]))
                            i--;
                        len = i + 1;
                    } 
                }
                strncat(wrap, word + pos, len);
                strcat(wrap, "\n");
                pos += len;
                while (pos < eol && isspace(word[pos]))
                    pos++;
            } while (eol > pos);
        }
        else
            strcat(wrap, "\n");
    }
    return wrap;
}

/* https://en.wikipedia.org/wiki/Jenkins_hash_function */
static inline uint32_t jenkins_one_at_a_time_hash(char *key, size_t len) {
    uint32_t i, hash;
    for(i = hash = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

void bugz_show_bug_info(json_object *bug, json_object *attachments, json_object *comments) {
    int i;
    json_object_object_foreach(bug,key,val) {
        if (json_object_is_type(val, json_type_null))
            continue;
        if (json_object_is_type(val, json_type_array) &&
            json_object_array_length(val) == 0)
            continue;
        if (json_object_is_type(val, json_type_string) &&
            json_object_get_string_len(val) == 0)
            continue;
        switch (jenkins_one_at_a_time_hash(key,strlen(key))) {
        case 0x70021a42 : /* assigned_to_detail */
        case 0xef259686 : /* is_cc_accessible */
        case 0xf22b1ac9 : /* creator_detail */
        case 0x71e45014 : /* estimated_time */
        case 0x27625455 : /* remaining_time */
        case 0x02751e9b : /* update_token */
        case 0xc95953dd : /* classification */
        case 0x64e45528 : /* cc_detail */
        case 0x1485bb6c : /* is_open */
        case 0x3b214c7a : /* actual_time */
        case 0x1893ebbc : /* is_creator_accessible */
        case 0xd05616fd : /* target_milestone */
        case 0x6af36a3d : /* is_confirmed */
        case 0x1b60404d : /* id */
            break;        /* skip fields */

        #define _farray1_(f) for (i=0; i<json_object_array_length(val); i++) {               \
                                 fprintf(stdout, "%-12s: %s\n", #f,                          \
                                 json_object_get_string(json_object_array_get_idx(val, i))); \
                             }
        #define _farray2_(f) fprintf(stdout, "%-12s: ", #f);                                 \
                             for (i=0; i<json_object_array_length(val)-1; i++) {             \
                                 fprintf(stdout, "%s, ",                                     \
                                 json_object_get_string(json_object_array_get_idx(val, i))); \
                             }                                                               \
                             fprintf(stdout, "%s\n",                                         \
                             json_object_get_string(json_object_array_get_idx(val, i)));
        #define _fstring_(f) fprintf(stdout, "%-12s: %s\n", #f, \
                                     json_object_get_string(val))

        case 0x1470edc0 : /* priority */
            _fstring_(Priority);
            break;
        case 0xb02e4c82 : /* blocks */
            _farray2_(Blocks);
            break;
        case 0xfd2a3542 : /* creator */
            _fstring_(Reporter);
            break;
        case 0x1ef6bc45 : /* last_change_time */
            _fstring_(Updated);
            break;
        case 0xcb093908 : /* keywords */
            _farray2_(Keywords);
            break;
        case 0x82bf95ca : /* cc */
            _farray1_(CC)
            break;
        case 0xbc113f8c : /* url */
            _fstring_(URL);
            break;
        case 0x7f3f554c : /* assigned_to */
            _fstring_(Assignee);
            break;
        case 0x9150e7cc : /* groups */
            _farray2_(Groups);
            break;
        case 0x324627cc : /* see_also */
            _farray1_(See Also);
            break;
        case 0x1bc9514d : /* whiteboard */
            _fstring_(Whiteboard);
            break;
        case 0x23bf264d : /* creation_time */
            _fstring_(Reported);
            break;
        case 0x9746780d : /* qa_contact */
            _fstring_(qaContact);
            break;
        case 0x15adaa4f : /* depends_on */
            _fstring_(dependsOn);
            break;
        case 0xd74c2a53 : /* dupe_of */
            _fstring_(dupeOf);
            break;
        case 0x135bd2dc : /* resolution */
            _fstring_(Resolution);
            break;
        case 0x3e0ac7de : /* alias */
            _farray2_(Alias);
            break;
        case 0x2a6825df : /* op_sys */
            _fstring_(OpSystem);
            break;
        case 0x6609b863 : /* status */
            _fstring_(Status);
            break;
        case 0x2d4e7caa : /* summary */
            _fstring_(Title);
            break;
        case 0x2fbe482d : /* platform */
            _fstring_(Platform);
            break;
        case 0x33a7556e : /* severity */
            _fstring_(Severity);
            break;
        case 0x67d26872 : /* flags */
            _farray2_(Flags);
            break;
        case 0x68c27e33 : /* version */
            _fstring_(Version);
            break;
        case 0x5f6b0634 : /* deadline */
            _fstring_(Deadline);
            break;
        case 0x51d7187a : /* component */
            _fstring_(Component);
            break;
        case 0x05ac6a7d : /* product */
            _fstring_(Product);
            break;
        default :
            fprintf(stdout, "%-12s: %s\n", key,
                    json_object_to_json_string(val));
        }
    }

    if (attachments) {
        json_object *attachment, *id, *when, *desc;
        fprintf(stdout, "%-12s: %d\n", "Attachments",
                json_object_array_length(attachments));
        for (i=0; i<json_object_array_length(attachments); i++) {
            attachment = json_object_array_get_idx(attachments, i);
            json_object_object_get_ex(attachment, "id", &id);
            json_object_object_get_ex(attachment, "summary", &desc);
            json_object_object_get_ex(attachment, "creation_time", &when);
            fprintf(stdout, "[Attachment] [%s] [%s] [%s]\n",
                    json_object_get_string(id),
                    json_object_get_string(when),
                    json_object_get_string(desc));
        }
    }
    
    if (comments) {
        int j, len;
        char *line, *wrap = NULL;
        json_object *comment, *who, *when, *what;
        fprintf(stdout, "%-12s: %d\n\n", "Comments",
                json_object_array_length(comments));
        for (i=0; i<json_object_array_length(comments); i++) {
            comment = json_object_array_get_idx(comments, i);
            json_object_object_get_ex(comment, "creator", &who);
            json_object_object_get_ex(comment, "time", &when);
            json_object_object_get_ex(comment, "text", &what);
            fprintf(stdout, "[Comment #%d] %s : %s\n", i,
                            json_object_get_string(who),
                            json_object_get_string(when));
            for (j=0; j<bugz_arguments.columns; j++) fprintf(stdout, "%c", '-');
            fprintf(stdout, "\n");
            
            len = json_object_get_string_len(what);
            line = (char *)json_object_get_string(what);
            wrap = (char *)realloc(wrap, len * 2 + 1);
            memset(wrap, 0, len * 2 + 1);
            word_wrap(wrap, line, bugz_arguments.columns);
            fprintf(stdout, "%s\n", wrap);
            fflush(stdout);
        }
        free(wrap);
    }
}

const char *bugz_get_content_type(const char *filename) {
    magic_t cookie = magic_open(MAGIC_MIME);
    magic_load(cookie, NULL);
    return magic_file(cookie, filename);
}

char *bugz_base64_encode(FILE *infile) {
    struct stat stat;
    char *p, *encode = NULL;
    unsigned char in[3] = {0};
    unsigned char out[4]= {0};
    int i, len, cur, total = 0;
    const char eb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    if (fstat(fileno(infile), &stat))
        return NULL;
    total = (stat.st_size + 2)/3*4 + 1;
    if ((encode = (char *)malloc(total + 1)) == NULL)
        return NULL;

    cur = 0;
    memset(encode, 0, total+1);
    while (feof(infile) == 0) {
        len = 0;
        for (i=0; i<3; i++) {
            in[i] = (unsigned char)getc(infile);
            if (feof(infile) == 0)
                len++;
            else
                in[i] = (unsigned char)0;
        }
        if (len > 0) {
            out[0] = (unsigned char)eb64[(int)(in[0] >> 2)];
            out[1] = (unsigned char)eb64[(int)(((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4))];
            out[2] = (unsigned char)(len > 1 ? eb64[(int)(((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6))] : '=');
            out[3] = (unsigned char)(len > 2 ? eb64[(int)(in[2] & 0x3f)] : '=');
            if (cur + 4 > total) {
                if ((p = (char *)malloc(total + 5)) == NULL) {
                    free(encode);
                    return NULL;
                }
                total += 4;
                memset(p, 0, total+1);
                memcpy(p, encode, cur);
                free(encode);
                encode = p;
            }
            for (i=0; i<4; i++) {
                encode[cur++] = out[i];
            }
        }
    }
    return encode;
}

char *bugz_base64_decode(const char *decode, FILE *outfile) {
    unsigned char in[4] = {0};
    unsigned char out[3]= {0};
    int v, i, len;
    const char db64[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

    char *p = (char *)decode;
    while (*p) {
        for (len=0, i=0; i<4 && *p; i++) {
            v = 0;
            while (*p && v == 0) {
                v = *p++;
                if (*p) {
                    v = ((v < 43 || v > 122) ? 0 : (int)db64[v-43]);
                    if (v)
                        v = ((v == (int)'$') ? 0 : v-61);
                }
            }
            if (*p) {
                len++;
                if (v)
                    in[i] = (unsigned char)(v-1);
            }
            else
                in[i] = (unsigned char)0;
        }
        if (len > 0) {
            out[0] = (unsigned char)(in[0] << 2 | in[1] >> 4);
            out[1] = (unsigned char)(in[1] << 4 | in[2] >> 2);
            out[2] = (unsigned char)(((in[2] << 6) & 0xc0) | in[3]);
            for (i=0; i<len-1; i++) {
                if (putc((int)out[i], outfile) == EOF) {
                    if (ferror(outfile))
                        return NULL;
                    break;
                }
            }
        }
    }

    return (char *)decode;
}

char *bugz_raw_input(const char *prompt) {
    char *p, *q;
    int i = 0, s = 1024;

    if (prompt)
        fprintf(stderr, "%s", prompt);

    p = q = NULL;
    while (feof(stdin) == 0) {
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
        fgets(p+strlen(p), i - strlen(p), stdin);
    }
    return p;
}

json_object *bugz_slist_to_json_array(struct curl_slist *list, int jtype) {
    struct curl_slist *head = list;
    json_object *jarray = json_object_new_array();
    while (head) {
        json_object *val = NULL;
        switch (jtype) {
        case json_type_int :
            val = json_object_new_int(atoi(head->data));
            break;
        case json_type_double :
            val = json_object_new_double(atof(head->data));
            break;
        case json_type_string :
            val = json_object_new_string(head->data);
        }
        json_object_array_add(jarray, val);
        head = head->next;
    }
    return jarray;
}

json_object *bugz_slist_to_json_string(struct curl_slist *list) {
    json_object *jstring = NULL;
    struct curl_slist *last = bugz_slist_get_last(list);
    if (last)
        jstring = json_object_new_string(last->data);
    return jstring;
}

int bugz_check_result(json_object *json) {
    int retval = FALSE;
    if (json) {
        int j;
        json_object *error;
        j=json_object_object_get_ex(json, "error", &error);
        if (j) {
            json_object *code, *message;
            json_object_object_get_ex(json, "code", &code);
            json_object_object_get_ex(json, "message", &message);
            fprintf(stderr, N_("ERROR: %s: %s\n"),
                            json_object_to_json_string(code),
                            json_object_to_json_string(message));
        }
        else {
             retval = TRUE;
        }
    }
    return retval;
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

char *bugz_urlencode(json_object *json) {
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
