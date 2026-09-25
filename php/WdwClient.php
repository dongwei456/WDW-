<?php
/**
 * WDW 大文本数据压缩 API SDK (PHP)
 *
 * 官方端点: https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php
 *
 * 用法:
 *   require_once 'WdwClient.php';
 *   $c = new WdwClient('你的API_KEY');
 *   $b85 = $c->compress('要压缩的文本');
 *   echo $c->decompress($b85);
 *
 * 依赖: PHP >= 7.1，curl 扩展（几乎所有环境默认开启）。
 */

class WdwException extends Exception {}

class WdwClient
{
    const ENDPOINT = 'https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php';

    private $key;
    private $endpoint;
    private $deviceId;
    private $timeout;

    public function __construct(string $key, string $endpoint = self::ENDPOINT, ?string $deviceId = null, float $timeout = 30.0)
    {
        $this->key = $key;
        $this->endpoint = $endpoint;
        $this->deviceId = $deviceId;
        $this->timeout = $timeout;
    }

    /* ---------------- 文本模式 (fhlx=1) ---------------- */

    /** 压缩 UTF-8 文本，返回带 <~...~> 包装的 base85 字符串 */
    public function compress(string $text): string
    {
        return $this->postForm(['clwb' => $text, 'lx' => 1, 'fhlx' => 1], false);
    }

    /** 传入 compress() 返回的完整字符串，还原原文 */
    public function decompress(string $b85): string
    {
        return $this->postForm(['clwb' => $b85, 'lx' => 2, 'fhlx' => 1], false);
    }

    /* ---------------- 二进制模式 (fhlx=2) ---------------- */

    /** 压缩任意二进制，返回裸二进制 (.wdw) */
    public function compressBytes(string $raw): string
    {
        // clwb 走表单；二进制用 chunk 避免 latin-1 概念混淆
        return $this->postForm(['clwb' => $raw, 'lx' => 1, 'fhlx' => 2], true);
    }

    /** 解压 .wdw 二进制，返回原文 */
    public function decompressBytes(string $raw): string
    {
        $query = http_build_query([
            'lx' => 2, 'fhlx' => 2, 'key' => $this->key,
        ] + ($this->deviceId ? ['device_id' => $this->deviceId] : []));
        $ch = curl_init($this->endpoint . '?' . $query);
        curl_setopt_array($ch, [
            CURLOPT_POST => true,
            CURLOPT_POSTFIELDS => $raw,
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_TIMEOUT => $this->timeout,
            CURLOPT_HTTPHEADER => ['Content-Type: application/octet-stream'],
        ]);
        $body = curl_exec($ch);
        $err = curl_error($ch);
        curl_close($ch);
        if ($body === false) {
            throw new WdwException('网络错误: ' . $err);
        }
        $this->assertOk($body);
        return $body;
    }

    /* ---------------- 内部 ---------------- */

    private function postForm(array $fields, bool $binaryReturn): string
    {
        $fields['key'] = $this->key;
        if ($this->deviceId) {
            $fields['device_id'] = $this->deviceId;
        }
        $ch = curl_init($this->endpoint);
        curl_setopt_array($ch, [
            CURLOPT_POST => true,
            CURLOPT_POSTFIELDS => http_build_query($fields),
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_TIMEOUT => $this->timeout,
        ]);
        $body = curl_exec($ch);
        $err = curl_error($ch);
        curl_close($ch);
        if ($body === false) {
            throw new WdwException('网络错误: ' . $err);
        }
        $this->assertOk($body);
        return $body;
    }

    /** 服务器失败时返回 {"code":0,"msg":"..."}；成功时是裸文本/裸二进制 */
    private function assertOk(string $body): void
    {
        $s = ltrim($body);
        if (isset($s[0]) && $s[0] === '{') {
            $obj = json_decode($body, true);
            if (is_array($obj) && isset($obj['msg'])) {
                throw new WdwException($obj['msg']);
            }
        }
    }
}

/* ---------------- 本地 base85 <-> 二进制 工具函数 (不消耗额度) ---------------- */

if (!defined('WDW_B85_ALPHABET')) {
    define('WDW_B85_ALPHABET', '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~');
}

/** 把 <~...~> base85 字符串解码成原始二进制 */
function wdw_b85_decode(string $b85): string
{
    $s = trim($b85);
    if (strncmp($s, '<~', 2) === 0) $s = substr($s, 2);
    if (substr($s, -2) === '~>') $s = substr($s, 0, -2);

    $map = array_flip(str_split(WDW_B85_ALPHABET));
    $out = '';
    $group = 0; $count = 0;
    $len = strlen($s);
    for ($i = 0; $i < $len; $i++) {
        $ch = $s[$i];
        if (!isset($map[$ch])) continue;
        $group = $group * 85 + $map[$ch];
        $count++;
        if ($count === 5) {
            $out .= chr(($group >> 24) & 0xFF) . chr(($group >> 16) & 0xFF)
                  . chr(($group >> 8) & 0xFF) . chr($group & 0xFF);
            $group = 0; $count = 0;
        }
    }
    // 末尾不足 5 字符
    if ($count > 0) {
        for ($j = 0; $j < 5 - $count; $j++) $group *= 85;
        $tmp = [];
        for ($j = 3; $j >= 0; $j--) { $tmp[$j] = $group & 0xFF; $group = intdiv($group, 256); }
        for ($j = 0; $j < $count - 1; $j++) $out .= chr($tmp[$j]);
    }
    return $out;
}

/** 把任意二进制编码成带 <~...~> 包装的 base85 字符串 */
function wdw_b85_encode(string $data): string
{
    $alpha = WDW_B85_ALPHABET;
    $out = '<~';
    $len = strlen($data);
    for ($i = 0; $i < $len; $i += 4) {
        $chunk = substr($data, $i, 4);
        $pad = 4 - strlen($chunk);           // 末尾补的 0 字节数
        $n = 0;
        for ($j = 0; $j < 4; $j++) {
            $n = ($n << 8) | (isset($chunk[$j]) ? ord($chunk[$j]) : 0);
        }
        // 5 个 base85 数字，$codes[0] 为最高位
        $codes = [];
        for ($d = 4; $d >= 0; $d--) {
            $codes[$d] = $n % 85;
            $n = intdiv($n, 85);
        }
        if ($pad > 0) {
            $keep = 5 - $pad;
            // 被截断的尾部若非零，最后一个输出字符要进位（标准 Ascii85 padding）
            $tailZero = true;
            for ($d = $keep; $d < 5; $d++) {
                if ($codes[$d] !== 0) { $tailZero = false; break; }
            }
            if (!$tailZero) {
                $pos = $keep - 1;
                $codes[$pos]++;
                while ($pos > 0 && $codes[$pos] >= 85) {
                    $codes[$pos] -= 85;
                    $codes[--$pos]++;
                }
            }
            for ($d = 0; $d < $keep; $d++) $out .= $alpha[$codes[$d]];
        } else {
            for ($d = 0; $d < 5; $d++) $out .= $alpha[$codes[$d]];
        }
    }
    return $out . '~>';
}
