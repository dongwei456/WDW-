/* WDW SDK C 演示 */
#include "wdw.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    /* 换成你自己的 Key */
    wdw_client_t c;
    wdw_client_init(&c, "你的API_KEY", NULL, NULL);

    /* 构造一段较长文本 */
    char original[4096];
    const char *piece = "WDW 大文本数据压缩 API 测试。";
    size_t o = 0;
    for (int i = 0; i < 50; i++) {
        size_t n = strlen(piece);
        memcpy(original + o, piece, n);
        o += n;
    }
    original[o] = 0;
    printf("原文: %zu 字节\n", o);

    char *b85 = wdw_compress(&c, original);
    if (!b85) {
        fprintf(stderr, "[API 错误] %s\n", wdw_last_error());
        fprintf(stderr, "提示: 若提示未登录/额度用尽，请先在官网注册并把 Key 填到 main()。\n");
        return 1;
    }
    printf("压缩后 (%zu 字符): %.120s...\n", strlen(b85), b85);

    char *text = wdw_decompress(&c, b85);
    if (!text) {
        fprintf(stderr, "[解压错误] %s\n", wdw_last_error());
        free(b85);
        return 1;
    }
    printf("往返一致: %s\n", strcmp(text, original) == 0 ? "YES" : "NO");

    /* 本地 base85 <-> bytes */
    size_t blen = 0;
    uint8_t *blob = wdw_b85_decode(b85, &blen);
    printf("本地解码 base85 -> %zu 字节\n", blen);
    char *again = wdw_b85_encode(blob, blen);
    printf("本地 re-encode 一致: %s\n", strcmp(again, b85) == 0 ? "YES" : "NO");

    free(again); free(blob); free(text); free(b85);
    return 0;
}
