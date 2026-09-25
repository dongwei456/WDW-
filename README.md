# WDW 大文本数据压缩 API・开源多语言 SDK

一套对接 **WDW 大文本数据压缩 API** 的开源 SDK，覆盖 **C / PHP / Python / HTML(JS) / Go** 五种语言。



* 官方端点：`https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php`

* 注册 / 获取 Key：[https://www.zhongbaomaoyi.cn/](https://www.zhongbaomaoyi.cn/)

* 协议：HTTPS `POST application/x-www-form-urlencoded`

* 压缩扣 1 次额度，解压免费。



***

## 零、产品能力与压缩率参考

WDW 是一款服务端大文本压缩 API，**无需指定数据类型，服务端自动识别**并选用最优压缩算法。官方参考压缩率如下（实际随内容分布波动）：

| 数据类型 | 压缩后剩余体积 | 压缩率 |
|---|---|---|
| HTML / XML 标记文本 | ~24% | **76%** |
| 中文文章 / Markdown 长文 | ~37% | **63%** |
| 英文文章 / 自然语言 | ~40% | **60%** |

**内置适配的文本类型（共 27 种，SDK 调用时自动识别，无需传参）：**

* **编程语言源码（21）**：html、php、css、asp、js、Go、Rust、Java、Lua、Swift、Kotlin、Dart、Julia、VB、R、Solidity、Move、汇编、SQL、Unity3D 脚本、TypeScript/JavaScript
* **自然语言（2）**：中文文章、English
* **结构化数据（4）**：binary01（二进制串）、coords（坐标数据）、pure_num（纯数字）、pure_alpha（纯字母）

**典型使用场景：**

1. **AI 大模型上下文压缩**：长文档 / 训练语料本地分包压缩存储，需要时按需解压读取，降低向量库与对象存储成本。
2. **代码仓库 / 日志传输**：源码、JSON 日志、HTML 报告在上传 CDN 或跨机同步前压缩，下载端解压。
3. **带宽与存储降本**：按 60–76% 的压缩率，可显著减少出口流量费与磁盘占用（实际节省取决于原始数据熵值与套餐计费方式）。



***

## 一、API 契约（所有语言共用）



| 参数          | 类型     | 必填 | 说明                                          |
| ----------- | ------ | -- | ------------------------------------------- |
| `clwb`      | string | 是  | 压缩时填原文；解压时填压缩返回的完整 `<~...~>` 字符串            |
| `lx`        | int    | 是  | `1` = 压缩，`2` = 解压                           |
| `key`       | string | 是  | API Key                                     |
| `fhlx`      | int    | 否  | `1` = 返回 base85 文本（推荐，跨语言）；`2` = 返回二进制流（默认） |
| `device_id` | string | 否  | 游客模式设备 ID；登录用户可忽略                           |

**响应约定（重要）**：



* 成功：HTTP 200，body 是**裸文本**（fhlx=1 时为 `<~...~>` base85）或**裸二进制**（fhlx=2）。

* 失败：HTTP 200，body 是 JSON `{"code":0,"msg":"错误原因"}`。

* SDK 内部会自动识别 JSON 错误体并抛出异常，调用方无需自己判断。

**base85 字母表**（RFC 1924）：



```
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#\$%&()\*+-;<=>?@^\_\`{|}\~
```

输出包裹 `<~` 前缀、`~>` 后缀；体积比 Base64 小约 25%。



***

## 二、目录结构



```
wdw-compress-sdk/

├── README.md              ← 本文件

├── python/

│   ├── wdw.py             ← SDK (仅标准库，零依赖)

│   └── demo.py

├── php/

│   ├── WdwClient.php      ← SDK (依赖 curl 扩展，自带 base85)

│   └── demo.php

├── go/

│   ├── go.mod

│   ├── wdw.go             ← SDK (仅标准库 net/http)

│   └── demo/main.go

├── c/

│   ├── wdw.h / wdw.c      ← SDK (依赖 libcurl，自带 base85)

│   ├── demo.c

│   └── build.bat          ← Windows MSVC 编译脚本

└── html/

&#x20;   ├── wdw.js             ← 浏览器 / Node.js 通用 SDK

&#x20;   └── demo.html          ← 双击即可在浏览器打开的演示页
```



***

## 三、各语言快速上手

### Python（≥ 3.7）



```
cd python

python demo.py
```



```
from wdw import WdwClient

c = WdwClient(key="你的KEY")

b85 = c.compress("任意长文本")   # -> "<\~....\~>"

text = c.decompress(b85)         # 还原
```

### PHP（≥ 7.1，需 curl 扩展）



```
cd php

php demo.php
```



```
require 'WdwClient.php';

\$c = new WdwClient('你的KEY');

\$b85 = \$c->compress('任意长文本');

echo \$c->decompress(\$b85);
```

### Go（≥ 1.18）



```
cd go

go run ./demo
```



```
client := wdw.New("你的KEY")

b85, \_ := client.Compress("任意长文本")

text, \_ := client.Decompress(b85)
```

### C（依赖 libcurl）



```
\# Windows (MSVC)

build.bat

\# Linux / macOS

gcc demo.c wdw.c -o wdw\_demo -lcurl && ./wdw\_demo
```



```
\#include "wdw.h"

wdw\_client\_t c; wdw\_client\_init(\&c, "你的KEY", NULL, NULL);

char \*b85 = wdw\_compress(\&c, "任意长文本");

char \*text = wdw\_decompress(\&c, b85);
```

### HTML / JavaScript（浏览器或 Node）

直接用浏览器打开 `html/demo.html`；或在自己的网页里：



```
\<script src="wdw.js">\</script>

\<script>

&#x20; const c = new WdwClient("你的KEY");

&#x20; const b85 = await c.compress("任意长文本");

&#x20; const text = await c.decompress(b85);

\</script>
```



***

## 四、错误处理

所有语言的 SDK 都会把服务器返回的 `{"code":0,"msg":"..."}` 转成异常 / 错误：



| 语言     | 异常类型                               |
| ------ | ---------------------------------- |
| Python | `wdw.WdwError`                     |
| PHP    | `WdwException`                     |
| Go     | `error`（字符串形如 `wdw: ...`）          |
| C      | 返回 `NULL`，用 `wdw_last_error()` 取原因 |
| JS     | `WdwError`                         |

常见错误：`未登录，请先登录或注册` / `本设备免费次数已用完，请登录或注册` / `解压失败：输入不是有效的85进制文本...`。



***

## 五、License

MIT — 可自由用于个人与商业项目。