# pyxton Python 库

`pyxton` 是一个高性能的 Python 库，用于解析和序列化 XTon (Xiao Tian Object Notation) 数据格式。它通过 C 语言扩展实现了核心逻辑，旨在提供与 Python 标准 `json` 模块兼容的 API 接口，同时保证了卓越的性能。

XTon 是一种轻量级的数据格式，旨在通过简化语法，提供比 JSON 更简洁、高效的数据表示和传输方式。

## 特性

*   **高性能**：核心解析和序列化逻辑由 C 语言实现。
*   **API 兼容性**：提供与 `json` 模块类似的 `loads`, `dumps`, `load`, `dump` 函数。
*   **灵活的编码器/解码器**：支持自定义 `XTONEncoder` 和 `XTONDecoder`。
*   **完整的 XTon 支持**：支持 XTon 格式的所有数据类型和转义规则。

## 安装

`pyxton` 库需要从源代码编译 C 扩展。请确保您的系统已安装 C 编译器（如 `gcc` 或 `clang`）和 Python 开发头文件。

1.  **克隆或下载 `xton` 仓库**：
    ```bash
    git clone https://github.com/theodoretian/xton.git
    cd xton
    ```
2.  **导航到 `pyxton` 目录**：
    ```bash
    cd pyxton
    ```
3.  **编译并安装 C 扩展**：
    ```bash
    python3 setup.py install
    ```
    或者，如果您只是想在当前目录中使用：
    ```bash
    python3 setup.py build_ext --inplace
    ```

## 使用方法

`pyxton` 提供了与 `json` 模块高度相似的 API，方便您进行 XTon 数据的处理。

### `xton.loads(s)`

从 XTon 格式的字符串 `s` 中解析出 Python 对象。

```python
import xton

xton_str = "q-<a-\\none/k-\\true/g-[a/25.3/87]>"
data = xton.loads(xton_str)
print(data) 
# Output: {'q': {'a': None, 'k': True, 'g': ['a', 25.3, 87.0]}}
```

### `xton.dumps(obj)`

将 Python 对象序列化为 XTon 格式的字符串。

```python
import xton

python_obj = {'name': 'XTon Example', 'version': 1.0, 'enabled': True}
xton_str = xton.dumps(python_obj)
print(xton_str)
# Output: name-'XTon Example'/version-1.0/enabled-\true
```

### `xton.load(fp)`

从文件对象 `fp` 中读取 XTon 文档并解析为 Python 对象。

```python
import xton
import io

xton_file_content = "key-'value'"
fp = io.StringIO(xton_file_content)
data = xton.load(fp)
print(data) 
# Output: {'key': 'value'}
```

### `xton.dump(obj, fp)`

将 Python 对象序列化为 XTon 格式，并写入文件对象 `fp`。

```python
import xton
import io

python_obj = {'item': 'example', 'count': 5}
fp = io.StringIO()
xton.dump(python_obj, fp)
print(fp.getvalue())
# Output: item-'example'/count-5
```

## XTon 数据格式简介

`XTon` 是一种为轻量级场景设计的数据交换格式。它支持多种基本数据类型（数字、字符串、布尔值、空值），以及复杂的数组和对象结构。`XTon` 的设计哲学是减少冗余符号，提高可读性和解析效率。

**转义规则**：在 `XTon` 字符串中，`-`, `<`, `>`, `[`, `]`, `/`, `\`, `'` 等字符需要使用 `\` 进行转义。

有关 XTon 数据格式的详细规范和所有转义规则，请参阅[此处](README.md)。

## 致谢

感谢 @flora.lee 对 XTon 数据格式的贡献和 `pyxton` 项目的指导。
