# pyxton Python Library

## \[[中](pyxton/README-E.md)|English\]

`pyxton` is a high-performance Python library for parsing and serializing the XTon (Xiao Tian Object Notation) data format. It implements its core logic via a C extension, aiming to provide an API interface compatible with Python's standard `json` module, while ensuring excellent performance.

XTon is a lightweight data format designed to offer a more concise and efficient way to represent and transmit data compared to JSON, by simplifying its syntax.

## Features

*   **High Performance**: Core parsing and serialization logic implemented in C.
*   **API Compatibility**: Provides functions similar to the `json` module: `loads`, `dumps`, `load`, `dump`.
*   **Flexible Encoder/Decoder**: Supports custom `XTONEncoder` and `XTONDecoder`.
*   **Full XTon Support**: Supports all XTon data types and escape rules.

## Installation

The `pyxton` library requires compiling a C extension from source. Please ensure you have a C compiler (like `gcc` or `clang`) and Python development headers installed on your system.

1.  **Clone or Download the `xton` Repository**:
    ```bash
    git clone https://github.com/theodoretian/xton.git
    cd xton
    ```
2.  **Navigate to the `pyxton` Directory**:
    ```bash
    cd pyxton
    ```
3.  **Compile and Install the C Extension**:
    ```bash
    python3 setup.py install
    ```
    Alternatively, if you only want to use it in the current directory:
    ```bash
    python3 setup.py build_ext --inplace
    ```

## Usage

`pyxton` offers an API highly similar to the `json` module, making it convenient for handling XTon data.

### `xton.loads(s)`

Parses an XTon format string `s` into a Python object.

```python
import xton

xton_str = "q-<a-\\none/k-\\true/g-[a/25.3/87]>"
data = xton.loads(xton_str)
print(data)
# Output: {'q': {'a': None, 'k': True, 'g': ['a', 25.3, 87.0]}}
```

### `xton.dumps(obj)`

Serializes a Python object into an XTon format string.

```python
import xton

python_obj = {'name': 'XTon Example', 'version': 1.0, 'enabled': True}
xton_str = xton.dumps(python_obj)
print(xton_str)
# Output: name-'XTon Example'/version-1.0/enabled-\true
```

### `xton.load(fp)`

Reads an XTon document from a file object `fp` and parses it into a Python object.

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

Serializes a Python object into XTon format and writes it to a file object `fp`.

```python
import xton
import io

python_obj = {'item': 'example', 'count': 5}
fp = io.StringIO()
xton.dump(python_obj, fp)
print(fp.getvalue())
# Output: item-'example'/count-5
```

## Introduction to XTon Data Format

`XTon` is a data exchange format designed for lightweight scenarios. It supports various basic data types (numbers, strings, booleans, null) as well as complex array and object structures. The design philosophy of `XTon` is to reduce redundant symbols and improve readability and parsing efficiency.

**Escape Rules**: In XTon strings, characters such as `-`, `<`, `>`, `[`, `]`, `/`, `\`, `'` need to be escaped using a `\`.

For a detailed specification of the XTon data format and all escape rules, please refer to [here](README.md).

## Acknowledgements

Thanks to @flora.lee for their contribution to the XTon data format and guidance on the `pyxton` project.
```
