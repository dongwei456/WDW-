/*
 * WDW SDK 实现: 基于 libcurl。
 * 编译 (Windows MSVC):
 *   cl /I path\to\curl\include demo.c wdw.c path\to\libcurl.lib
 * 编译 (gcc / MinGW):
 *   gcc demo.c wdw.c -o wdw_demo -lcurl
 */
#include "wdw.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static char g_err[512] = {0};

const char *wdw_last_error(void) { return g_err; }

static void set_err(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(g_err, sizeof(g_err), fmt, ap);
    va_end(ap);
}

void wdw_client_init(wdw_client_t *c, const char *key, const char *endpoint, const char *device_id) {
    memset(c, 0, sizeof(*c));
    c->key = key;
    c->endpoint = endpoint ? endpoint : WDW_DEFAULT_ENDPOINT;
    c->device_id = device_id;
}

/* ---------- URL 编码 ---------- */
char *wdw_urlencode(const char *s) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;
    char *out = curl_easy_escape(curl, s, 0);
    curl_easy_cleanup(curl);
    return out;
}

/* ---------- HTTP 响应 buffer ---------- */
typedef struct {
    char *buf;
    size_t len;
} resp_t;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    resp_t *r = (resp_t *)userdata;
    char *nb = realloc(r->buf, r->len + total + 1);
    if (!nb) return 0;
    r->buf = nb;
    memcpy(r->buf + r->len, ptr, total);
    r->len += total;
    r->buf[r->len] = 0;
    return total;
}

/* 失败时服务器返回 {"code":0,"msg":"..."}；成功时是裸文本。
 * 返回 0 = 成功 (out 已填好); 返回 -1 = 错误, g_err 已设置。 */
static int parse_response(resp_t *r) {
    if (r->len > 0 && r->buf[0] == '{') {
        /* 找 "msg":"..." 提取错误信息 */
        const char *p = strstr(r->buf, "\"msg\"");
        if (p) {
            p = strchr(p, ':');
            if (p) {
                p++;
                while (*p == ' ' || *p == '"') p++;
                char buf[256] = {0};
                size_t i = 0;
                while (*p && *p != '"' && i < sizeof(buf) - 1) buf[i++] = *p++;
                set_err("%s", buf);
                free(r->buf); r->buf = NULL; r->len = 0;
                return -1;
            }
        }
        set_err("服务器返回未知 JSON: %s", r->buf);
        free(r->buf); r->buf = NULL; r->len = 0;
        return -1;
    }
    return 0;
}

/* 通用 POST: 表单字段 */
static char *post_form(const wdw_client_t *c, const char *clwb, int lx, int fhlx) {
    char *enc_clwb = wdw_urlencode(clwb);
    char *enc_key = wdw_urlencode(c->key);
    if (!enc_clwb || !enc_key) { set_err("urlencode 失败"); free(enc_clwb); free(enc_key); return NULL; }

    size_t cap = strlen(enc_clwb) + strlen(enc_key) + 128;
    char *post = malloc(cap);
    snprintf(post, cap, "clwb=%s&lx=%d&fhlx=%d&key=%s", enc_clwb, lx, fhlx, enc_key);
    if (c->device_id) {
        char *enc_dev = wdw_urlencode(c->device_id);
        size_t add = strlen(enc_dev) + 16;
        post = realloc(post, cap + add);
        strcat(post, "&device_id=");
        strcat(post, enc_dev);
        free(enc_dev);
    }
    free(enc_clwb); free(enc_key);

    CURL *curl = curl_easy_init();
    if (!curl) { set_err("curl_easy_init 失败"); free(post); return NULL; }

    resp_t r = {0};
    curl_easy_setopt(curl, CURLOPT_URL, c->endpoint);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    free(post);

    if (rc != CURLE_OK) {
        set_err("网络错误: %s", curl_easy_strerror(rc));
        free(r.buf);
        return NULL;
    }
    if (parse_response(&r) != 0) return NULL;
    return r.buf; /* 调用方 free */
}

char *wdw_compress(const wdw_client_t *c, const char *text) {
    if (!c || !text) { set_err("参数为空"); return NULL; }
    return post_form(c, text, 1, 1);
}

char *wdw_decompress(const wdw_client_t *c, const char *b85) {
    if (!c || !b85) { set_err("参数为空"); return NULL; }
    return post_form(c, b85, 2, 1);
}

/* ---------- base85 (RFC 1924 字母表, <~...~> 包装) ---------- */

static const char B85_ALPHA[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~";

uint8_t *wdw_b85_decode(const char *b85, size_t *out_len) {
    if (!b85 || !out_len) return NULL;
    *out_len = 0;

    const char *s = b85;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    size_t L = strlen(s);
    if (L >= 2 && s[0] == '<' && s[1] == '~') { s += 2; L -= 2; }
    while (L >= 2 && s[L-2] == '~' && s[L-1] == '>') L -= 2;

    static uint8_t map[256];
    static int map_inited = 0;
    if (!map_inited) {
        memset(map, 255, sizeof(map));
        for (int i = 0; i < 85; i++) map[(unsigned char)B85_ALPHA[i]] = (uint8_t)i;
        map_inited = 1;
    }

    uint8_t *out = malloc(L); /* 上界: 每5字符4字节 */
    size_t cap = L; (void)cap;
    size_t n = 0;
    unsigned long group = 0;
    int count = 0;
    for (size_t i = 0; i < L; i++) {
        unsigned char ch = (unsigned char)s[i];
        if (map[ch] == 255) continue;
        group = group * 85 + map[ch];
        count++;
        if (count == 5) {
            out[n++] = (group >> 24) & 0xFF;
            out[n++] = (group >> 16) & 0xFF;
            out[n++] = (group >> 8) & 0xFF;
            out[n++] = group & 0xFF;
            group = 0; count = 0;
        }
    }
    if (count > 0) {
        for (int j = 0; j < 5 - count; j++) group *= 85;
        uint8_t tmp[4];
        tmp[0] = (group >> 24) & 0xFF;
        tmp[1] = (group >> 16) & 0xFF;
        tmp[2] = (group >> 8) & 0xFF;
        tmp[3] = group & 0xFF;
        for (int j = 0; j < count - 1; j++) out[n++] = tmp[j];
    }
    *out_len = n;
    return out;
}

char *wdw_b85_encode(const uint8_t *data, size_t len) {
    size_t cap = len * 2 + 8;
    char *out = malloc(cap);
    size_t o = 0;
    out[o++] = '<'; out[o++] = '~';
    for (size_t i = 0; i < len; i += 4) {
        size_t chunk = len - i; if (chunk > 4) chunk = 4;
        int pad = 4 - (int)chunk;
        unsigned long n = 0;
        for (size_t j = 0; j < chunk; j++) n = (n << 8) | data[i + j];
        for (int j = 0; j < pad; j++) n <<= 8;
        int codes[5];
        for (int d = 4; d >= 0; d--) { codes[d] = (int)(n % 85); n /= 85; }
        if (pad > 0) {
            int keep = 5 - pad;
            int tailZero = 1;
            for (int d = keep; d < 5; d++) if (codes[d] != 0) { tailZero = 0; break; }
            if (!tailZero) {
                int pos = keep - 1;
                codes[pos]++;
                while (pos > 0 && codes[pos] >= 85) { codes[pos] -= 85; codes[--pos]++; }
            }
            for (int d = 0; d < keep; d++) out[o++] = B85_ALPHA[codes[d]];
        } else {
            for (int d = 0; d < 5; d++) out[o++] = B85_ALPHA[codes[d]];
        }
    }
    out[o++] = '~'; out[o++] = '>'; out[o] = 0;
    return out;
}
