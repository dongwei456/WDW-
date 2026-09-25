// WDW SDK Go 演示: go run ./demo
package main

import (
	"fmt"
	"strings"

	wdw "wdw-sdk-go"
)

func main() {
	// 换成你自己的 Key
	client := wdw.New("你的API_KEY")

	original := strings.Repeat("WDW 大文本数据压缩 API 测试。", 50)
	fmt.Printf("原文: %d 字节\n", len(original))

	b85, err := client.Compress(original)
	if err != nil {
		fmt.Println("[API 错误]", err)
		fmt.Println("提示: 若提示未登录/额度用尽，请先在官网注册并把 Key 填到 client.New()。")
		return
	}
	fmt.Printf("压缩后 (%d 字符): %s...\n", len(b85), truncate(b85, 120))

	restored, err := client.Decompress(b85)
	if err != nil {
		fmt.Println("[解压错误]", err)
		return
	}
	fmt.Println("往返一致:", restored == original)

	// 本地 base85 <-> bytes (不消耗额度)
	blob, _ := wdw.B85Decode(b85)
	fmt.Printf("本地解码 base85 -> %d 字节\n", len(blob))
	fmt.Println("本地 re-encode 一致:", wdw.B85Encode(blob) == b85)
}

func truncate(s string, n int) string {
	if len(s) <= n {
		return s
	}
	return s[:n] + "..."
}
