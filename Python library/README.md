# pyxton

`pyxton` 是一个用于解析和序列化 XTon (Xiao Tian Object Notation) 数据格式的 Python 库。它使用 C 语言实现核心逻辑，旨在提供类似于 Python 标准库 `json` 模块的性能和易用性。

XTon 是一种轻量级级的数据格式，旨在通过简化语法，提供比 JSON 更简洁的方式来表示和传输数据。它支持字符串、数字、布尔值、空值、数组和对象等多种数据类型。

## 安装

目前，`pyxton` 库需要从源代码编译安装 C 扩展。

1. **确保您有 C 编译器** (如 `gcc` 或 `clang`) 和 Python 开发头文件。

* 在 macOS 上，您可能需要安装 Xcode Command Line Tools: `xcode-select --install`
* 在 Linux 上，您可能需要安装 `build-essential` 和 `python3-dev` (或等效包)。

1. **克隆或下载 `pyxton` 源代码**。

```bash
git clone https://github.com/theodoretian/xton/pyxton
```

1. **导航到 `pyxton` 目录**：

```bash
cd pyxton
```

1. **编译并安装 C 扩展**：

```bash
python3 setup.py install
```

或者，如果您只是想在当前目录中使用，而不进行系统安装：

```bash
python3 setup.py build_ext --inplace
```

## 使用方法

`pyxton` 库提供了 `loads`, `dumps`, `load`, `dump` 四个主要函数，与 `json` 模块的接口类似。

### `loads(s)`

从一个 XTon 格式的字符串 `s` 中解析出一个 Python 对象。

```python
import xton

# 解析数字
data = xton.loads("123.45")
print(data) # Output: 123.45

# 解析布尔值
data = xton.loads("\\true")
print(data) # Output: True

# 解析空值
data = xton.loads("\\none")
print(data) # Output: None

# 解析字符串
data = xton.loads("'hello world'")
print(data) # Output: hello world

# 解析数组
data = xton.loads("[a/25.3/87]")
print(data) # Output: ['a', 25.3, 87.0]

# 解析对象
data = xton.loads("<key-value>")
print(data) # Output: {'key': 'value'}

# 解析顶层单键值对（XTon 特有形式）
data = xton.loads("q-<a-\\none/k-\\true/g-[a/25.3/87]>")
print(data) # Output: {'q': {'a': None, 'k': True, 'g': ['a', 25.3, 87.0]}}
```

### `dumps(obj)`

将一个 Python 对象序列化为 XTon 格式的字符串。

```python
import xton

# 序列化基本类型
print(xton.dumps(None))          # Output: \\none
print(xton.dumps(True))          # Output: \\true
print(xton.dumps(123))           # Output: 123
print(xton.dumps("hello world")) # Output: 'hello world'

# 序列化包含特殊字符的字符串（会自动转义和引用）
print(xton.dumps("hello/world")) # Output: 'hello\/world'
print(xton.dumps("hello-world")) # Output: 'hello\-world'

# 序列化列表
print(xton.dumps([]))                      # Output: []
print(xton.dumps(["a", 25.3, 87]))        # Output: [a/25.3/87]
print(xton.dumps([True, "hello", None]))  # Output: [\\true/hello/\\none]

# 序列化字典
print(xton.dumps({}))                                                       # Output: <>
print(xton.dumps({"key": "value"}))                                         # Output: key-'value' (顶层单键值对)
print(xton.dumps({"obj": {"a": 1, "b": 2}}))                                # Output: obj-<a-1/b-2> (顶层单键值对)
print(xton.dumps({"num": 123, "bool": True}))                               # Output: <num-123/bool-\\true> (多键值对字典)
print(xton.dumps({"q": {"a": None, "k": True, "g": ["a", 25.3, 87]}})) # Output: q-<a-\\none/k-\\true/g-[a/25.3/87]>
```

### `load(fp)`

从一个文件对象 `fp` 中读取 XTon 文档，并解析为一个 Python 对象。`fp` 必须是一个支持 `.read()` 方法的文件类对象。

```python
import xton
import io

# 假设 xon_data.xt 是一个 XTon 文件，内容为 "key-'value'"
# with open("xon_data.xt", "r") as f:
#     data = xton.load(f)
# print(data) # Output: {'key': 'value'}

# 使用 StringIO 模拟文件
xton_string = "q-<a-\\none/k-\\true>"
fp = io.StringIO(xton_string)
data = xton.load(fp)
print(data) # Output: {'q': {'a': None, 'k': True}}
```

### `dump(obj, fp)`

将一个 Python 对象序列化为 XTon 格式，并写入一个文件对象 `fp`。`fp` 必须是一个支持 `.write()` 方法的文件类对象。

```python
import xton
import io

data = {
    "name": "Test",
    "value": 123.0,
    "items": ["one", "two"]
}

fp = io.StringIO()
xton.dump(data, fp)
print(fp.getvalue()) # Output: name-'Test'/value-123.0/items-[one/two] (根据实际序列化输出，这里可能是 "<name-'Test'/value-123.0/items-[one/two]>" 或其他形式)

# 验证 dump 和 load
fp_read = io.StringIO(fp.getvalue())
loaded_data = xton.load(fp_read)
print(loaded_data) # Output: {'name': 'Test', 'value': 123.0, 'items': ['one', 'two']}
```

## XTon 数据格式概述

XTon 格式具有以下特点：

* **数据类型**：支持字符串、数字、布尔值 (`` `\\true` ``, `` `\\false` ``)、空值 (`` `\\none` ``)、数组 (`[]`) 和对象 (`<>`)。
* **字符串**：通过反引号 `'` 包裹，内部特殊字符需转义。作为键或数组元素时，如果为有效标识符且不含特殊字符，则可不引用。
* **数字**：直接表示，如 `123`, `23.5`, `-0.5`。
* **数组**：使用中括号 `[]`，元素通过 `/` 分隔。
* **对象**：使用尖括号 `< >` 包裹，键值对通过 `/` 分隔，键和值之间通过 `-` 分隔。
* **顶层单键值对**：如果顶层数据是一个仅包含一个键值对的字典，且键为非引用字符串，则可简化为 `key-value` 形式，而不是 `<key-value>`。

**转义字符**：
在字符串中，以下字符需要通过 `` `` `` 来转义：

* `-` → `\-`
* `<` → `\<`
* `>` → `\>`
* `[` → `\[`
* `]` → `\]`
* `/` → `\/`
* `\` → `\\`
* `'` → `\'`
* `'` → `'`

有关 XTon 格式的更详细说明，请参阅 [XTon 数据格式文档](file:///Users/flora.lee/Downloads/XTon%20数据格式文档.md)。

## 致谢

感谢用户 [@flora.lee](https://github.com/theodoretian) 提供了 XTon 规范以及这个有趣的项目。
