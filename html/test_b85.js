// 验证 wdw.js 的 base85 编解码 round-trip 正确
const { wdwB85Encode, wdwB85Decode } = require('./wdw.js');

let fail = 0;
function eq(a, b, label) {
    const ok = JSON.stringify(a) === JSON.stringify(b);
    if (!ok) {
        fail++;
        console.log('FAIL  ' + label);
        console.log('  expected:', JSON.stringify(b));
        console.log('  got     :', JSON.stringify(a));
    } else {
        console.log('PASS  ' + label);
    }
}

// 空输入
eq(wdwB85Encode(new Uint8Array(0)), '<~~>', 'encode(empty)');

// 各种长度 (1..20) 全字节模式 round-trip
for (let len = 1; len <= 20; len++) {
    const data = new Uint8Array(len);
    for (let i = 0; i < len; i++) data[i] = (i * 37 + 11) & 0xff;
    const s = wdwB85Encode(data);
    const back = wdwB85Decode(s);
    eq(Array.from(back), Array.from(data), `roundtrip len=${len}`);
}

// 暴力：所有 1 字节值 0..255
for (let v = 0; v < 256; v++) {
    const data = new Uint8Array([v]);
    const s = wdwB85Encode(data);
    const back = wdwB85Decode(s);
    eq(Array.from(back), [v], `byte 0x${v.toString(16)}`);
}

// 2 字节所有边界
for (let v = 0; v < 256; v += 17) {
    const data = new Uint8Array([v, 255 - v]);
    const s = wdwB85Encode(data);
    const back = wdwB85Decode(s);
    eq(Array.from(back), [v, 255 - v], `2byte [${v},${255-v}]`);
}

// 大尺寸
const big = new Uint8Array(5000);
for (let i = 0; i < big.length; i++) big[i] = i & 0xff;
eq(Array.from(wdwB85Decode(wdwB85Encode(big))), Array.from(big), 'roundtrip 5000 bytes');

// UTF-8 中文
const zh = Buffer.from('WDW 大文本数据压缩 API 测试，重复重复重复。', 'utf8');
eq(Array.from(wdwB85Decode(wdwB85Encode(new Uint8Array(zh)))), Array.from(zh), 'UTF-8 Chinese');

// 全 0 字节边界
for (let len = 1; len <= 8; len++) {
    const data = new Uint8Array(len);
    eq(Array.from(wdwB85Decode(wdwB85Encode(data))), Array.from(data), `zero bytes len=${len}`);
}

console.log(fail === 0 ? '\n全部通过 ✓' : `\n${fail} 个失败 ✗`);
process.exit(fail === 0 ? 0 : 1);
