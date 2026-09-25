/*
 * WDW 大文本数据压缩 API SDK (浏览器 / Node.js 通用)
 *
 * 用法 (浏览器):
 *   <script src="wdw.js"></script>
 *   <script>
 *     const c = new WdwClient("你的API_KEY");
 *     const b85 = await c.compress("要压缩的文本");
 *     const text = await c.decompress(b85);
 *   </script>
 *
 * 用法 (Node.js):
 *   const { WdwClient } = require('./wdw.js');
 *   const c = new WdwClient("你的API_KEY");
 *   const b85 = await c.compress("hello");
 */
(function (root, factory) {
    if (typeof module === 'object' && module.exports) {
        module.exports = factory();
    } else {
        root.WdwClient = factory().WdwClient;
        root.wdwB85Decode = factory().wdwB85Decode;
        root.wdwB85Encode = factory().wdwB85Encode;
    }
})(typeof self !== 'undefined' ? self : this, function () {
    const ENDPOINT = 'https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php';

    class WdwError extends Error {
        constructor(msg) { super(msg); this.name = 'WdwError'; }
    }

    class WdwClient {
        /**
         * @param {string} key       API Key
         * @param {object} opts      { endpoint, deviceId }
         */
        constructor(key, opts = {}) {
            this.key = key;
            this.endpoint = opts.endpoint || ENDPOINT;
            this.deviceId = opts.deviceId || '';
        }

        async _postForm(fields) {
            const fd = new FormData();
            for (const k in fields) fd.append(k, fields[k]);
            if (this.deviceId) fd.append('device_id', this.deviceId);
            const resp = await fetch(this.endpoint, { method: 'POST', body: fd });
            const text = await resp.text();
            // 失败时服务器返回 {"code":0,"msg":"..."}；成功时是裸 base85 文本
            const t = text.trimStart();
            if (t.startsWith('{')) {
                try {
                    const obj = JSON.parse(text);
                    if (obj && obj.msg) throw new WdwError(obj.msg);
                } catch (e) {
                    if (e instanceof WdwError) throw e;
                }
            }
            return text;
        }

        /** 压缩 UTF-8 文本，返回 <~...~> base85 字符串 */
        async compress(text) {
            return this._postForm({ clwb: text, lx: 1, fhlx: 1, key: this.key });
        }

        /** 解压 compress() 返回的完整字符串 */
        async decompress(b85) {
            return this._postForm({ clwb: b85, lx: 2, fhlx: 1, key: this.key });
        }
    }

    /* ---------- 本地 base85 <-> bytes (不消耗额度) ---------- */

    const ALPHA = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~';

    /** <~...~> base85 -> Uint8Array */
    function wdwB85Decode(b85) {
        let s = String(b85).trim();
        if (s.startsWith('<~')) s = s.slice(2);
        if (s.endsWith('~>')) s = s.slice(0, -2);
        const map = {};
        for (let i = 0; i < ALPHA.length; i++) map[ALPHA[i]] = i;
        const bytes = [];
        let group = 0, count = 0;
        for (let i = 0; i < s.length; i++) {
            const v = map[s[i]];
            if (v === undefined) continue;
            group = group * 85 + v;
            count++;
            if (count === 5) {
                bytes.push((group >>> 24) & 255, (group >>> 16) & 255, (group >>> 8) & 255, group & 255);
                group = 0; count = 0;
            }
        }
        if (count > 0) {
            for (let j = 0; j < 5 - count; j++) group *= 85;
            const tmp = [0, 0, 0, 0];
            for (let j = 3; j >= 0; j--) { tmp[j] = group & 255; group = Math.floor(group / 256); }
            for (let j = 0; j < count - 1; j++) bytes.push(tmp[j]);
        }
        return new Uint8Array(bytes);
    }

    /** Uint8Array -> <~...~> base85 */
    function wdwB85Encode(data) {
        let out = '<~';
        for (let i = 0; i < data.length; i += 4) {
            const chunk = data.subarray(i, i + 4);
            const m = chunk.length;
            const pad = 4 - m;
            let n = 0;
            for (let j = 0; j < m; j++) n = (n << 8) | chunk[j];
            for (let j = 0; j < pad; j++) n = (n << 8) >>> 0;
            // 5 个 base85 数字，c[0] 为最高位
            const c = [0, 0, 0, 0, 0];
            let t = n >>> 0;
            for (let d = 4; d >= 0; d--) { c[d] = t % 85; t = Math.floor(t / 85); }
            if (pad > 0) {
                const keep = m + 1;
                // 被截断的尾部若非零，最后一个输出字符要进位（标准 Ascii85 padding）
                let tailZero = true;
                for (let d = keep; d < 5; d++) { if (c[d] !== 0) { tailZero = false; break; } }
                if (!tailZero) {
                    let pos = keep - 1;
                    c[pos]++;
                    while (pos > 0 && c[pos] >= 85) { c[pos] -= 85; c[--pos]++; }
                }
                for (let d = 0; d < keep; d++) out += ALPHA[c[d]];
            } else {
                for (let d = 0; d < 5; d++) out += ALPHA[c[d]];
            }
        }
        return out + '~>';
    }

    return { WdwClient, WdwError, wdwB85Decode, wdwB85Encode };
});
