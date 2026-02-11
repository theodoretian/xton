
# XTon Data Format Documentation

## \[[中](README.md)|English\]

## Introduction

`XTon` is a lightweight data format designed to provide a simpler and more concise alternative to JSON.  
Its full name is **Xiao Tian Object Notation**. Similar to JSON, `XTon` supports multiple data types such as strings, numbers, booleans, arrays, and objects.

The design goal of `XTon` is to:

-   Simplify data representation
    
-   Reduce unnecessary symbols
    
-   Improve readability and usability
    

Unlike JSON, `XTon` avoids the heavy use of braces, quotation marks, and commas by using a more compact symbolic syntax.

XTon 数据格式文档

----------

## Data Types

`XTon` supports the following data types:

Type

Description

Example

String

Prefixed with `'`

`'text`

Number

Written directly

`23.5`

Boolean

`\true` or `\false`

`\true`

Null

`\none`

`\none`

Array

Enclosed in `[]`

`[a/25.3/87]`

Object

Enclosed in `< >`

`<key-value>`

## Escape Rules

In `XTon`, certain characters must be escaped because they are used as syntax symbols.

Character

Escape form

`-`

`\-`

`<`

`\<`

`>`

`\>`

`[`

`\[`

`]`

`\]`

`/`

`\/`

Escaping ensures these characters are treated as part of the string rather than structural syntax.

## Examples

### String

**XTon**

`'23.5` 

**Meaning**

`"23.5"` 

### Number

**XTon**

`23.5` 

**Meaning**

`23.5` 

### Boolean

**XTon**

`\true` 

**Meaning**

`true` 

### Null

**XTon**

`\none` 

**Meaning**

`null` 

### Array

**XTon**

`[a/25.3/87]` 

**Meaning**

`["a",  25.3,  87]` 

### Object

**XTon**

`q-<a-\none/k-\true/g-[a/25.3/87]>` 

**Meaning**

`{  "q":  {  "a":  null,  "k":  true,  "g":  ["a",  25.3,  87]  }  }` 

## Use Cases

`XTon` is suitable for scenarios where **data size and transmission efficiency** are important.

### Embedded Systems

For devices with limited memory or storage, `XTon` provides a more compact alternative to JSON.

### Device Communication

Smaller payload sizes improve transmission efficiency between devices.

### Lightweight Data Storage

Ideal for applications that require a simpler and more compact data representation.

## Syntax Summary

1.  **String**  
    Prefix with `'`  
    Example:
    
    `'text` 
    
2.  **Number**  
    Written directly
    
    `23.5` 
    
3.  **Boolean**
    
    `\true \false` 
    
4.  **Null**
    
    `\none` 
    
5.  **Array**  
    Use `[]`, elements separated by `/`
    
    `[a/25.3/87]` 
    
6.  **Object**  
    Use `< >`, key-value pairs separated by `/`
    
    `q-<a-\none/k-\true/g-[a/25.3/87]>` 

## Conclusion

`XTon` provides a concise and efficient way to represent and transmit data.  
By reducing redundant symbols and introducing a simple escaping system, it offers a more compact and readable alternative to JSON.

If you are looking for a lightweight data format, `XTon` may be a strong candidate.

For questions, improvements, or discussions about the format, feel free to reach out.
