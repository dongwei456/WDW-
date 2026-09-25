// Package wdw 是 WDW 大文本数据压缩 API 的官方开源 Go SDK。
//
// 端点: https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php
//
// 用法:
//   client := wdw.New("你的API_KEY")
//   b85, _ := client.Compress("要压缩的文本")
//   text, _ := client.Decompress(b85)
package wdw

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strings"
	"time"
)

const DefaultEndpoint = "https://www.zhongbaomaoyi.cn/fwq/shujuyasuo/compress_api.php"

// Client WDW API 客户端
type Client struct {
	Key      string
	Endpoint string
	DeviceID string
	Timeout  time.Duration
	HTTPClient *http.Client
}

// New 创建一个客户端
func New(key string) *Client {
	return &Client{
		Key:      key,
		Endpoint: DefaultEndpoint,
		Timeout:  30 * time.Second,
		HTTPClient: &http.Client{Timeout: 30 * time.Second},
	}
}

// Compress 压缩 UTF-8 文本，返回 <~...~> 包装的 base85 字符串
func (c *Client) Compress(text string) (string, error) {
	form := url.Values{}
	form.Set("clwb", text)
	form.Set("lx", "1")
	form.Set("fhlx", "1")
	return c.postForm(form, false)
}

// Decompress 传入 Compress 返回的完整字符串，还原原文
func (c *Client) Decompress(b85 string) (string, error) {
	form := url.Values{}
	form.Set("clwb", b85)
	form.Set("lx", "2")
	form.Set("fhlx", "1")
	return c.postForm(form, false)
}

// CompressBytes 压缩任意二进制，返回裸 .wdw 二进制
func (c *Client) CompressBytes(raw []byte) ([]byte, error) {
	form := url.Values{}
	form.Set("clwb", string(raw))
	form.Set("lx", "1")
	form.Set("fhlx", "2")
	return c.postFormRaw(form, true)
}

// DecompressBytes 解压 .wdw 二进制，返回原文
func (c *Client) DecompressBytes(raw []byte) (string, error) {
	q := url.Values{}
	q.Set("lx", "2")
	q.Set("fhlx", "2")
	q.Set("key", c.Key)
	if c.DeviceID != "" {
		q.Set("device_id", c.DeviceID)
	}
	reqURL := c.Endpoint + "?" + q.Encode()
	req, err := http.NewRequest("POST", reqURL, bytes.NewReader(raw))
	if err != nil {
		return "", err
	}
	req.Header.Set("Content-Type", "application/octet-stream")
	resp, err := c.HTTPClient.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	body, _ := io.ReadAll(resp.Body)
	if err := checkError(body); err != nil {
		return "", err
	}
	return string(body), nil
}

// ---------- 内部 ----------

func (c *Client) baseForm() url.Values {
	f := url.Values{}
	f.Set("key", c.Key)
	if c.DeviceID != "" {
		f.Set("device_id", c.DeviceID)
	}
	return f
}

func (c *Client) postForm(form url.Values, binary bool) (string, error) {
	// 把 base 字段合并进去
	base := c.baseForm()
	for k, vs := range base {
		for _, v := range vs {
			form.Set(k, v)
		}
	}
	resp, err := c.HTTPClient.PostForm(c.Endpoint, form)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	body, _ := io.ReadAll(resp.Body)
	if err := checkError(body); err != nil {
		return "", err
	}
	return string(body), nil
}

func (c *Client) postFormRaw(form url.Values, binary bool) ([]byte, error) {
	base := c.baseForm()
	for k, vs := range base {
		for _, v := range vs {
			form.Set(k, v)
		}
	}
	resp, err := c.HTTPClient.PostForm(c.Endpoint, form)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	body, _ := io.ReadAll(resp.Body)
	if err := checkError(body); err != nil {
		return nil, err
	}
	return body, nil
}

// 失败时服务器返回 {"code":0,"msg":"..."}；成功时是裸文本/裸二进制
func checkError(body []byte) error {
	trimmed := bytes.TrimLeft(body, " \t\r\n")
	if len(trimmed) > 0 && trimmed[0] == '{' {
		var obj struct {
			Code int             `json:"code"`
			Msg  string          `json:"msg"`
			Raw  json.RawMessage `json:"data"`
		}
		if err := json.Unmarshal(trimmed, &obj); err == nil && obj.Msg != "" {
			return fmt.Errorf("wdw: %s", obj.Msg)
		}
	}
	return nil
}

// ---------- 本地 base85 <-> bytes (不消耗额度) ----------

const b85Alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~"

// B85Decode 把 <~...~> base85 文本解码为原始 bytes
func B85Decode(s string) ([]byte, error) {
	s = strings.TrimSpace(s)
	s = strings.TrimPrefix(s, "<~")
	s = strings.TrimSuffix(s, "~>")

	var val [256]byte
	for i := range val {
		val[i] = 255
	}
	for i := 0; i < len(b85Alphabet); i++ {
		val[b85Alphabet[i]] = byte(i)
	}

	var out []byte
	group := uint32(0)
	count := 0
	for i := 0; i < len(s); i++ {
		c := s[i]
		if val[c] == 255 {
			continue
		}
		group = group*85 + uint32(val[c])
		count++
		if count == 5 {
			out = append(out, byte(group>>24), byte(group>>16), byte(group>>8), byte(group))
			group = 0
			count = 0
		}
	}
	if count > 0 {
		for j := 0; j < 5-count; j++ {
			group *= 85
		}
		tmp := [4]byte{
			byte(group >> 24), byte(group >> 16), byte(group >> 8), byte(group),
		}
		for j := 0; j < count-1; j++ {
			out = append(out, tmp[j])
		}
	}
	return out, nil
}

// B85Encode 把任意 bytes 编码成 <~...~> 包装的 base85
func B85Encode(data []byte) string {
	var sb strings.Builder
	sb.WriteString("<~")
	for i := 0; i < len(data); i += 4 {
		chunk := data[i:min(i+4, len(data))]
		pad := 4 - len(chunk)
		var n uint32
		for _, b := range chunk {
			n = (n << 8) | uint32(b)
		}
		for j := len(chunk); j < 4; j++ {
			n <<= 8
		}
		codes := [5]int{}
		for d := 4; d >= 0; d-- {
			codes[d] = int(n % 85)
			n /= 85
		}
		if pad > 0 {
			keep := 5 - pad
			tailZero := true
			for d := keep; d < 5; d++ {
				if codes[d] != 0 { tailZero = false; break }
			}
			if !tailZero {
				pos := keep - 1
				codes[pos]++
				for pos > 0 && codes[pos] >= 85 {
					codes[pos] -= 85
					pos--
					codes[pos]++
				}
			}
			for d := 0; d < keep; d++ {
				sb.WriteByte(b85Alphabet[codes[d]])
			}
		} else {
			for d := 0; d < 5; d++ {
				sb.WriteByte(b85Alphabet[codes[d]])
			}
		}
	}
	sb.WriteString("~>")
	return sb.String()
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
