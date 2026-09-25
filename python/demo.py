# -*- coding: utf-8 -*-
"""WDW SDK Python 演示：压缩 -> 打印 -> 解压还原。"""
from wdw import WdwClient, WdwError, b85_to_bytes, bytes_to_b85

# 把下面换成你自己的 Key（登录 https://www.zhongbaomaoyi.cn 控制台获取）
KEY = "你的API_KEY"


def main():
    client = WdwClient(key=KEY)

    original = "WDW 大文本数据压缩 API 测试。" * 50
    print(f"原文长度: {len(original)} 字符 ({len(original.encode('utf-8'))} 字节)")

    try:
        # 1) 压缩成 base85 文本
        b85 = client.compress(original)
        print(f"\n压缩结果 (base85, 长度 {len(b85)}):")
        print(b85[:120] + ("..." if len(b85) > 120 else ""))

        # 2) 解压还原
        restored = client.decompress(b85)
        print(f"\n解压后长度: {len(restored)} 字符")
        print("往返一致:", restored == original)

        # 3) 本地 base85 <-> bytes 互转（不消耗额度）
        blob = b85_to_bytes(b85)
        print(f"\n本地解码 base85 -> {len(blob)} 字节 (.wdw)")
        again = bytes_to_b85(blob)
        print("本地 re-encode 与服务器输出一致:", again == b85)

    except WdwError as e:
        print(f"\n[API 错误] {e}")
        print("提示: 若提示未登录/额度用尽，请先在官网注册并把 Key 填到顶部 KEY 变量。")


if __name__ == "__main__":
    main()
