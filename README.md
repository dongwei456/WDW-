# WDW 大文本数据压缩 API・开源多语言 SDK

一套对接 **WDW 大文本数据压缩 API** 的开源 SDK，覆盖 **C / PHP / Python / HTML(JS) / Go** 五种语言。



* 官方端点：`https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php`

* 注册 / 获取 Key：[https://xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.com.cn/](https://www.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.com.cn/)

* 协议：HTTPS `POST application/x-www-form-urlencoded`

* 压缩扣 1 次额度，解压免费。



***

## 零、产品能力与压缩率参考

WDW 是一款服务端大文本压缩 API，**无需指定数据类型，服务端自动识别**并选用最优压缩算法。官方参考压缩率如下（实际随内容分布波动）：



| 数据类型               | 压缩后剩余体积 | 压缩率     |
| ------------------ | ------- | ------- |
| HTML / XML 标记文本    | \~24%   | **76%** |
| 中文文章 / Markdown 长文 | \~37%   | **63%** |
| 英文文章 / 自然语言        | \~40%   | **60%** |

**内置适配的文本类型（共 27 种，SDK 调用时自动识别，无需传参）：**



* **编程语言源码（21）**：html、php、css、asp、js、Go、Rust、Java、Lua、Swift、Kotlin、Dart、Julia、VB、R、Solidity、Move、汇编、SQL、Unity3D 脚本、TypeScript/JavaScript

* **自然语言（2）**：中文文章、English

* **结构化数据（4）**：binary01（二进制串）、coords（坐标数据）、pure\_num（纯数字）、pure\_alpha（纯字母）

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
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#\\\$%&()\\\*+-;<=>?@^\\\_\\\`{|}\\\~
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

\&#x20;   ├── wdw.js             ← 浏览器 / Node.js 通用 SDK

\&#x20;   └── demo.html          ← 双击即可在浏览器打开的演示页
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

b85 = c.compress("任意长文本")   # -> "<\\\~....\\\~>"

text = c.decompress(b85)         # 还原
```

### PHP（≥ 7.1，需 curl 扩展）



```
cd php

php demo.php
```



```
require 'WdwClient.php';

\\\$c = new WdwClient('你的KEY');

\\\$b85 = \\\$c->compress('任意长文本');

echo \\\$c->decompress(\\\$b85);
```

### Go（≥ 1.18）



```
cd go

go run ./demo
```



```
client := wdw.New("你的KEY")

b85, \\\_ := client.Compress("任意长文本")

text, \\\_ := client.Decompress(b85)
```

### C（依赖 libcurl）



```
\\# Windows (MSVC)

build.bat

\\# Linux / macOS

gcc demo.c wdw.c -o wdw\\\_demo -lcurl && ./wdw\\\_demo
```



```
\\#include "wdw.h"

wdw\\\_client\\\_t c; wdw\\\_client\\\_init(\\\&c, "你的KEY", NULL, NULL);

char \\\*b85 = wdw\\\_compress(\\\&c, "任意长文本");

char \\\*text = wdw\\\_decompress(\\\&c, b85);
```

### HTML / JavaScript（浏览器或 Node）

直接用浏览器打开 `html/demo.html`；或在自己的网页里：



```
\\\<script src="wdw.js">\\\</script>

\\\<script>

\&#x20; const c = new WdwClient("你的KEY");

\&#x20; const b85 = await c.compress("任意长文本");

\&#x20; const text = await c.decompress(b85);

\\\</script>
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



***

# English Version

# WDW Large-Text Compression API · Open-Source Multi-Language SDK

An open-source SDK for the **WDW Large-Text Compression API**, covering **C / PHP / Python / HTML(JS) / Go** across five languages.



* Official endpoint: `https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php`

* Sign up / get a Key: [https://www.zhongbaomaoyi.cn/](https://www.zhongbaomaoyi.cn/)

* Protocol: HTTPS `POST application/x-www-form-urlencoded`

* Compression costs 1 quota; decompression is free.



***

## 0. Capabilities & Compression Ratio

WDW is a server-side large-text compression API. **You do not need to specify the data type — the server auto-detects it** and picks the best algorithm. Reference ratios (actual values vary with content entropy):



| Data type                           | Remaining size | Compression ratio |
| ----------------------------------- | -------------- | ----------------- |
| HTML / XML markup                   | \~24%          | **76%**           |
| Chinese articles / long Markdown    | \~37%          | **63%**           |
| English articles / natural language | \~40%          | **60%**           |

**Built-in adapters for 27 text types (auto-detected, no parameter needed):**



* **Programming source code (21)**: html, php, css, asp, js, Go, Rust, Java, Lua, Swift, Kotlin, Dart, Julia, VB, R, Solidity, Move, Assembly, SQL, Unity3D scripts, TypeScript/JavaScript

* **Natural language (2)**: Chinese, English

* **Structured data (4)**: binary01 (binary string), coords (coordinate data), pure\_num (digits only), pure\_alpha (letters only)

**Typical use cases:**



1. **LLM context compression** — chunk long documents / training corpora into compressed packages on disk, decompress on demand to cut vector-DB and object-storage costs.

2. **Repo / log transfer** — compress source, JSON logs, HTML reports before CDN upload or cross-machine sync; decompress on the receiving end.

3. **Bandwidth & storage cost reduction** — 60–76% compression significantly lowers egress traffic and disk usage (actual savings depend on data entropy and billing plan).



***

## 1. API Contract (shared by all languages)



| Parameter   | Type   | Required | Description                                                                                  |
| ----------- | ------ | -------- | -------------------------------------------------------------------------------------------- |
| `clwb`      | string | yes      | Text to compress; on decompress, the full `<~...~>` string returned by compress              |
| `lx`        | int    | yes      | `1` = compress, `2` = decompress                                                             |
| `key`       | string | yes      | Your API Key                                                                                 |
| `fhlx`      | int    | no       | `1` = return base85 text (recommended, cross-language); `2` = return binary stream (default) |
| `device_id` | string | no       | Guest-mode device ID; ignored for logged-in users                                            |

**Response contract (important):**



* Success: HTTP 200, body is **raw text** (`<~...~>` base85 when fhlx=1) or **raw binary** (fhlx=2).

* Failure: HTTP 200, body is JSON `{"code":0,"msg":"error message"}`.

* The SDK auto-detects JSON error bodies and throws — you don't have to.

**base85 alphabet** (RFC 1924):



```
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#\$%&()\*+-;<=>?@^\_\`{|}\~
```

Outputs are wrapped with `<~` prefix and `~>` suffix; \~25% smaller than Base64.



***

## 2. Repository Layout



```
wdw-compress-sdk/

├── README.md              ← this file

├── python/

│   ├── wdw.py             ← SDK (stdlib only, zero deps)

│   └── demo.py

├── php/

│   ├── WdwClient.php      ← SDK (curl ext, built-in base85)

│   └── demo.php

├── go/

│   ├── go.mod

│   ├── wdw.go             ← SDK (stdlib net/http only)

│   └── demo/main.go

├── c/

│   ├── wdw.h / wdw.c      ← SDK (libcurl, built-in base85)

│   ├── demo.c

│   └── build.bat          ← Windows MSVC build script

└── html/

&#x20;   ├── wdw.js             ← Browser / Node.js universal SDK

&#x20;   └── demo.html          ← open directly in a browser
```



***

## 3. Quick Start per Language

### Python (≥ 3.7)



```
cd python

python demo.py
```



```
from wdw import WdwClient

c = WdwClient(key="YOUR\_KEY")

b85 = c.compress("any long text")   # -> "<\~....\~>"

text = c.decompress(b85)            # restored
```

### PHP (≥ 7.1, curl ext required)



```
cd php

php demo.php
```



```
require 'WdwClient.php';

\$c = new WdwClient('YOUR\_KEY');

\$b85 = \$c->compress('any long text');

echo \$c->decompress(\$b85);
```

### Go (≥ 1.18)



```
cd go

go run ./demo
```



```
client := wdw.New("YOUR\_KEY")

b85, \_ := client.Compress("any long text")

text, \_ := client.Decompress(b85)
```

### C (requires libcurl)



```
\# Windows (MSVC)

build.bat

\# Linux / macOS

gcc demo.c wdw.c -o wdw\_demo -lcurl && ./wdw\_demo
```



```
\#include "wdw.h"

wdw\_client\_t c; wdw\_client\_init(\&c, "YOUR\_KEY", NULL, NULL);

char \*b85 = wdw\_compress(\&c, "any long text");

char \*text = wdw\_decompress(\&c, b85);
```

### HTML / JavaScript (browser or Node)

Open `html/demo.html` directly in a browser, or embed in your own page:



```
\<script src="wdw.js">\</script>

\<script>

&#x20; const c = new WdwClient("YOUR\_KEY");

&#x20; const b85 = await c.compress("any long text");

&#x20; const text = await c.decompress(b85);

\</script>
```



***

## 4. Error Handling

Every language SDK converts server responses `{"code":0,"msg":"..."}` into an exception / error:



| Language | Error type                                         |
| -------- | -------------------------------------------------- |
| Python   | `wdw.WdwError`                                     |
| PHP      | `WdwException`                                     |
| Go       | `error` (string like `wdw: ...`)                   |
| C        | returns `NULL`, call `wdw_last_error()` for reason |
| JS       | `WdwError`                                         |

Common errors: `Not logged in` / `Guest quota exhausted` / `Decompress failed: invalid base85 input`.



***

## 5. License

MIT — free for personal and commercial use.
