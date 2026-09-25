# -*- coding: utf-8 -*-
"""
WDW 大文本数据压缩 API SDK (Python)
====================================

官方端点: https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php

用法:
    from wdw import WdwClient
    c = WdwClient(key="你的API Key")
    b85 = c.compress("要压缩的文本")          # 返回 <~...~> 形式的 base85 文本
    text = c.decompress(b85)                 # 还原原文

    # 二进制模式 (体积最小)
    raw = c.compress_bytes(b"二进制数据")     # 返回 bytes
    back = c.decompress_bytes(raw)

依赖: 仅 Python 标准库 (urllib)，无需 pip install。
"""

from __future__ import annotations

import base64
import json
import urllib.parse
import urllib.request
from typing import Optional


class WdwError(Exception):
    """服务器返回的业务错误 (code != 1 / HTTP body 是 JSON 错误体)。"""

    def __init__(self, msg: str, http_status: int = 200):
        super().__init__(msg)
        self.http_status = http_status


class WdwClient:
    DEFAULT_ENDPOINT = "https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php"

    def __init__(
        self,
        key: str,
        endpoint: str = DEFAULT_ENDPOINT,
        device_id: Optional[str] = None,
        timeout: float = 30.0,
    ):
        """
        :param key:        API Key (登录后在控制台获取；游客模式可留空但额度极小)
        :param endpoint:   接口地址，一般不用改
        :param device_id:  游客模式设备 ID；登录用户可忽略
        :param timeout:    超时秒数
        """
        self.key = key
        self.endpoint = endpoint
        self.device_id = device_id
        self.timeout = timeout

    # ---------- 内部：统一请求 ----------
    def _post(self, form: dict, expect_binary: bool) -> object:
        data = urllib.parse.urlencode(form).encode("utf-8")
        req = urllib.request.Request(
            self.endpoint,
            data=data,
            headers={
                "Content-Type": "application/x-www-form-urlencoded; charset=utf-8",
                "User-Agent": "wdw-sdk-python/1.0",
            },
            method="POST",
        )
        try:
            with urllib.request.urlopen(req, timeout=self.timeout) as resp:
                body = resp.read()
                status = resp.status
        except urllib.error.HTTPError as e:
            body = e.read()
            status = e.code

        # 服务器错误时返回 JSON: {"code":0,"msg":"..."}；成功时是裸文本或裸二进制
        stripped = body.lstrip()
        if stripped[:1] == b"{":
            try:
                obj = json.loads(body.decode("utf-8", errors="replace"))
                if isinstance(obj, dict) and "msg" in obj:
                    raise WdwError(obj.get("msg", "未知错误"), status)
            except json.JSONDecodeError:
                pass
        if expect_binary:
            return body
        return body.decode("utf-8", errors="replace")

    def _base_form(self, lx: int, fhlx: int) -> dict:
        form = {"lx": str(lx), "fhlx": str(fhlx), "key": self.key}
        if self.device_id:
            form["device_id"] = self.device_id
        return form

    # ---------- 对外：文本模式 (fhlx=1) ----------
    def compress(self, text: str) -> str:
        """压缩一段 UTF-8 文本，返回带 <~...~> 包装的 base85 字符串。"""
        form = self._base_form(lx=1, fhlx=1)
        form["clwb"] = text
        return self._post(form, expect_binary=False)

    def decompress(self, b85: str) -> str:
        """传入 compress() 返回的完整 <~...~> 字符串，还原原文。"""
        form = self._base_form(lx=2, fhlx=1)
        form["clwb"] = b85
        return self._post(form, expect_binary=False)

    # ---------- 对外：二进制模式 (fhlx=2) ----------
    def compress_bytes(self, raw: bytes) -> bytes:
        """压缩 bytes，返回压缩后的裸二进制 (.wdw)。"""
        # fhlx=2 压缩时，clwb 仍走表单 (文本)；这里用 latin-1 透传任意字节
        form = self._base_form(lx=1, fhlx=2)
        form["clwb"] = raw.decode("latin-1")
        return self._post(form, expect_binary=True)

    def decompress_bytes(self, raw: bytes) -> str:
        """解压 .wdw 二进制，返回原文 (str)。"""
        # fhlx=2 解压：原始二进制 body 直接 POST，参数走 query string
        qs = urllib.parse.urlencode({
            "lx": 2, "fhlx": 2, "key": self.key,
            **({"device_id": self.device_id} if self.device_id else {}),
        })
        req = urllib.request.Request(
            f"{self.endpoint}?{qs}",
            data=raw,
            headers={
                "Content-Type": "application/octet-stream",
                "User-Agent": "wdw-sdk-python/1.0",
            },
            method="POST",
        )
        try:
            with urllib.request.urlopen(req, timeout=self.timeout) as resp:
                body = resp.read()
                status = resp.status
        except urllib.error.HTTPError as e:
            body = e.read()
            status = e.code
        if body[:1] == b"{":
            try:
                obj = json.loads(body.decode("utf-8", errors="replace"))
                raise WdwError(obj.get("msg", "未知错误"), status)
            except json.JSONDecodeError:
                pass
        return body.decode("utf-8", errors="replace")


# ---------- 本地 base85 <-> bytes 工具 (可选，不用调服务器) ----------
# 官方字母表 (RFC 1924)，与 <~...~> 包装配套
def b85_to_bytes(b85: str) -> bytes:
    """把服务器返回的 <~...~> base85 文本解码成原始 bytes。"""
    s = b85.strip()
    if s.startswith("<~"):
        s = s[2:]
    if s.endswith("~>"):
        s = s[:-2]
    return base64.b85decode(s)


def bytes_to_b85(data: bytes) -> str:
    """把任意 bytes 编码成带 <~...~> 包装的 base85 文本。"""
    return "<~" + base64.b85encode(data).decode("ascii") + "~>"


# ---------- CLI 入口 ----------
if __name__ == "__main__":
    import sys

    if len(sys.argv) < 3:
        print("用法: python wdw.py <API_KEY> <compress|decompress> [输入文件]")
        sys.exit(1)
    key, op = sys.argv[1], sys.argv[2]
    cli = WdwClient(key=key)
    if op == "compress":
        text = sys.stdin.read() if len(sys.argv) < 4 else open(sys.argv[4], encoding="utf-8").read()
        print(cli.compress(text))
    elif op == "decompress":
        b85 = sys.stdin.read() if len(sys.argv) < 4 else open(sys.argv[4], encoding="utf-8").read()
        print(cli.decompress(b85.strip()))
    else:
        print("未知操作:", op)
        sys.exit(1)
