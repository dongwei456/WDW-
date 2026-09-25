/*
 * WDW 大文本数据压缩 API SDK (C / C++ 通用)
 *
 * 官方端点: https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php
 *
 * 依赖: libcurl (https://curl.se/libcurl/)
 *
 * 用法:
 *   wdw_client_t c;
 *   wdw_client_init(&c, "你的API_KEY", NULL, NULL);
 *   char *b85 = wdw_compress(&c, "要压缩的文本");
 *   if (b85) {
 *       printf("压缩结果: %s\n", b85);
 *       char *text = wdw_decompress(&c, b85);
 *       printf("还原: %s\n", text);
 *       free(text); free(b85);
 *   } else {
 *       fprintf(stderr, "错误: %s\n", wdw_last_error());
 *   }
 *   wdw_client_cleanup(&c);
 */
#ifndef WDW_SDK_H
#define WDW_SDK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WDW_DEFAULT_ENDPOINT "https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php"

typedef struct {
    const char *key;
    const char *endpoint;   /* 传 NULL 用默认 */
    const char *device_id;  /* 游客模式可传 NULL */
} wdw_client_t;

/* 初始化客户端 (不拷贝字符串，调用方需保证字符串生命周期) */
void wdw_client_init(wdw_client_t *c, const char *key, const char *endpoint, const char *device_id);

/*
 * 压缩 UTF-8 文本，返回 <~...~> base85 字符串。
 * 返回值由 malloc 分配，调用方 free()。失败返回 NULL，用 wdw_last_error() 取原因。
 */
char *wdw_compress(const wdw_client_t *c, const char *text);

/*
 * 解压: 传入 wdw_compress() 返回的完整字符串，返回原文 (malloc)。
 * 失败返回 NULL。
 */
char *wdw_decompress(const wdw_client_t *c, const char *b85);

/* 最近一次错误信息 (线程不安全，单线程够用) */
const char *wdw_last_error(void);

/* ---------- 本地 base85 <-> bytes (不消耗额度) ---------- */

/* 解码 <~...~> base85 字符串；返回 malloc 缓冲区，*out_len 写入长度，调用方 free() */
uint8_t *wdw_b85_decode(const char *b85, size_t *out_len);

/* 编码任意 bytes 为 <~...~> base85；返回 malloc 字符串，调用方 free() */
char *wdw_b85_encode(const uint8_t *data, size_t len);

/* URL 编码 (内部用，也可给用户用) */
char *wdw_urlencode(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* WDW_SDK_H */
