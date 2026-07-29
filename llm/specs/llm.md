# Spec: LLM 子系统

## 职责
LLM 子系统是 siliconbuddy 与模型提供方（Provider）之间的协议边界。它负责：
- 把内部 `Conversation` / `ModelRequestOptions` / `tool_defs` 规整为可发送的请求负载（**Protocol Adapter**）；
- 把提供方返回的裸文本解码为 `ChatResponse`（含内容、结束原因、token 用量、工具调用）；
- 管理 `Tool` 注册表，使 agent 可插拔地贡献能力；
- 管理 `Provider` 注册表，按 id 选择具体提供方实现。

所有对象关系通过 `silicon::di` 装配，构造参数一律使用接口形式。具体实现可替换（测试用 `ScriptedProvider`，生产用 `HttpProvider`）。

## 核心类型
- `Message`：`{ role, content, tool_call_id }`，role ∈ {user, assistant, system, tool}
- `Conversation`：`std::vector<Message>`
- `ModelRequestOptions`：模型名、temperature、max_tokens、extra 键值
- `ChatResponse`：`{ content, finish_reason, prompt_tokens, completion_tokens }`
- `ToolCall`：`{ id, name, arguments(JSON string) }`
- `ToolOutput`：`{ content, truncated, managed_output_path }`
- `LLMError`：`{ message }`
- `Result<T>`：`std::variant<T, LLMError>` 的轻量结果包装

## 接口
```cpp
// 提供方：一次对话补全
class IProvider {
  virtual ~IProvider() = default;
  virtual Result<ChatResponse> chat(const Conversation&,
                                    const ModelRequestOptions&) = 0;
};

// 协议适配器：内部规整 <-> 提供方线路格式
class IProtocolAdapter {
  virtual ~IProtocolAdapter() = default;
  virtual std::string encode_request(const Conversation&,
                                     const ModelRequestOptions&,
                                     const std::vector<std::string>& tool_defs) const = 0;
  virtual Result<ChatResponse> decode_response(std::string_view raw) const = 0;
};

// 工具
class ITool {
  virtual ~ITool() = default;
  virtual std::string_view name() const = 0;
  virtual std::string_view description() const = 0;
  virtual ToolOutput execute(const ToolCall&) = 0;
};

class IToolRegistry {
  virtual ~IToolRegistry() = default;
  virtual bool register_tool(std::unique_ptr<ITool>) = 0;
  virtual ITool* get_tool(std::string_view name) const = 0;
  virtual std::size_t tool_count() const = 0;
};

// 提供方注册表
class IProviderRegistry {
  virtual ~IProviderRegistry() = default;
  virtual bool register_provider(std::string id, std::unique_ptr<IProvider>) = 0;
  virtual IProvider* get_provider(std::string_view id) const = 0;
  virtual std::vector<std::string> list_providers() const = 0;
};
```

## 具体实现
- `ToolRegistry : public IToolRegistry` —— 内存注册表，重复 name 注册返回 false（覆盖式由调用方决定，默认拒绝重复）。
- `ProviderRegistry : public IProviderRegistry` —— 内存注册表，重复 id 注册返回 false。
- `JsonProtocolAdapter : public IProtocolAdapter` —— 编码为 OpenAI 风格 JSON（`messages`/`model`/`temperature`/`max_tokens`/`tools`）；解码从 `choices[0].message.content` 与 `finish_reason`、`usage` 字段还原 `ChatResponse`。
- `ScriptedProvider : public IProvider` —— 持有 `ChatResponse` 队列，按 `chat()` 调用顺序弹出；队列耗尽返回 `LLMError{ "no scripted response" }`。用于确定性 TDD。

## 不变式
1. `ToolRegistry::register_tool` 遇重复 name 返回 false，不替换既有工具。
2. `ProviderRegistry::register_provider` 遇重复 id 返回 false。
3. `JsonProtocolAdapter::encode_request` 产出合法请求 JSON（含 `model`/`messages`/`tools`）；`decode_response` 能从响应形态 JSON（含 `choices[0].message.content`/`finish_reason`/`usage`）正确还原 `content`、`finish_reason` 与 token 用量。
4. `ScriptedProvider::chat` 严格 FIFO；空队列返回 `LLMError`，不抛异常。
5. 所有具体类仅依赖接口，可被 `silicon::di` 以 `scope<shared>` 装配并递归注入。
