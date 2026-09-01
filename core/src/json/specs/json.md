# Spec: silicon.json — JSON 基础库

## 职责
为 siliconcode 提供零外部依赖的 JSON 能力：JSON 值模型、解析（文本 → 值）、序列化（值 → 文本），以及 `IJsonCodec` 编解码接口（供 System Context 的 JSON codec、Schema 载体复用）。

## 值模型（JsonValue）
- `null`：`std::nullptr_t`
- `boolean`：`bool`
- `integer`：`int64_t`
- `number`：`double`
- `string`：`std::string`
- `array`：`std::vector<JsonValue>`
- `object`：`std::map<std::string, JsonValue, std::less<>>`（按键有序，便于确定性渲染）

## 接口契约
- `parse(std::string_view) -> std::expected<JsonValue, ParseError>`：失败时返回位置与消息，不抛异常。
- `serialize(const JsonValue&, bool pretty=false) -> std::string`：确定性输出；object 按键升序。
- `IJsonCodec`：`encode(const JsonValue&) -> std::string` 与 `decode(std::string_view) -> std::expected<JsonValue, ParseError>`，作为可注入的编解码抽象（System Context 用其比较/存储值）。

## 不变式
- 解析严格遵循 RFC 8259（允许 UTF-8 文本、标准空白、转义）。
- 重复 object key 后者覆盖前者。
- 序列化对 `<`/`>`/`&`/`"`/`'`/`\` 做最小化转义；字符串统一用双引号。
- `parse` 对非法输入返回 `ParseError{pos, message}`，禁止抛异常穿透模块边界。

## 平台
纯标准库，无平台相关文件。
