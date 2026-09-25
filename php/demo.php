<?php
/** WDW SDK PHP 演示: php demo.php */
require __DIR__ . '/WdwClient.php';

// 换成你自己的 Key
$KEY = '你的API_KEY';

try {
    $client = new WdwClient($KEY);
    $original = str_repeat("WDW 大文本数据压缩 API 测试。", 50);
    echo "原文: " . strlen($original) . " 字节\n";

    $b85 = $client->compress($original);
    echo "压缩后 (" . strlen($b85) . " 字符): " . substr($b85, 0, 120) . "...\n";

    $restored = $client->decompress($b85);
    echo "往返一致: " . ($restored === $original ? 'YES' : 'NO') . "\n";

    // 本地 base85 <-> 二进制 (不消耗额度)
    $blob = wdw_b85_decode($b85);
    echo "本地解码 base85 -> " . strlen($blob) . " 字节\n";
    echo "本地 re-encode 一致: " . (wdw_b85_encode($blob) === $b85 ? 'YES' : 'NO') . "\n";
} catch (WdwException $e) {
    echo "[API 错误] " . $e->getMessage() . "\n";
    echo "提示: 若提示未登录/额度用尽，请先在官网注册并把 Key 填到 \$KEY 变量。\n";
}
