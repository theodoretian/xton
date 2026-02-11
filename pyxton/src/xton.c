#include <Python.h>
#include <string.h>
#include <stdlib.h> // For strtod
#include <ctype.h>  // For isdigit
// #include <stdio.h> // For fprintf and stderr - REMOVED DEBUGGING

// Helper function to check if a character is an XTon delimiter for unquoted identifiers
static int is_xton_identifier_delimiter(char c) {
    // Delimiters are '/', '<', '>', '[', ']', '-', '\0', and whitespace
    return (c == '/' || c == '<' || c == '>' || c == '[' || c == ']' || c == '-' || c == '\0' || isspace(c));
}

// Helper function to skip whitespace
static void skip_whitespace(const char** s) {
    while (isspace(**s)) {
        (*s)++;
    }
}

// Forward declaration of the parsing function
static PyObject* parse_value(const char** s);

// Parse a number
static PyObject* parse_number(const char** s) {
    char* endptr;
    double value = strtod(*s, &endptr);
    if (*s == endptr) { // No characters were consumed
        return NULL; // Not a valid number
    }
    *s = endptr;
    return PyFloat_FromDouble(value);
}

// Parse a quoted string (e.g., 'hello')
static PyObject* parse_quoted_string(const char** s) {
    if (**s != '\'') {
        return NULL; // Not a quoted string
    }
    (*s)++; // Skip the leading '\''

    PyObject* py_bytes = PyBytes_FromStringAndSize(NULL, 0); // Start with empty bytes object
    Py_ssize_t current_size = 0;
    Py_ssize_t allocated_size = 64; // Initial allocation size
    if (_PyBytes_Resize(&py_bytes, allocated_size) == -1) return NULL; // Check for allocation failure
    char* buffer = PyBytes_AS_STRING(py_bytes);

    while (**s != '\0' && **s != '\'') { // Stop at end of string or next unescaped single quote
        if (**s == '\\') {
            (*s)++; // Skip the escape character
            char escaped_char = '\0';
            switch (**s) {
                case '-': escaped_char = '-'; break;
                case '<': escaped_char = '<'; break;
                case '>': escaped_char = '>'; break;
                case '[': escaped_char = '['; break;
                case ']': escaped_char = ']'; break;
                case '/': escaped_char = '/'; break;
                case '\'': escaped_char = '\''; break; // Handle escaped single quote
                case '\\': escaped_char = '\\'; break; // Handle escaped backslash
                default:
                    // Unknown escape sequence, treat it as literal
                    escaped_char = **s;
                    break;
            }
            if (current_size >= allocated_size) {
                allocated_size *= 2;
                if (_PyBytes_Resize(&py_bytes, allocated_size) == -1) return NULL;
                buffer = PyBytes_AS_STRING(py_bytes);
            }
            buffer[current_size++] = escaped_char;
            (*s)++;
        } else {
            // Normal character
            if (current_size >= allocated_size) {
                allocated_size *= 2;
                if (_PyBytes_Resize(&py_bytes, allocated_size) == -1) return NULL;
                buffer = PyBytes_AS_STRING(py_bytes);
            }
            buffer[current_size++] = **s;
            (*s)++;
        }
    }
    
    if (**s == '\'') { // Consume the closing single quote
        (*s)++;
    } else {
        PyErr_SetString(PyExc_ValueError, "Unterminated XTon quoted string.");
        Py_XDECREF(py_bytes);
        return NULL;
    }

    // Resize the PyBytes object to the actual length
    if (_PyBytes_Resize(&py_bytes, current_size) == -1) return NULL;
    
    return PyUnicode_FromEncodedObject(py_bytes, "utf-8", "strict");
}

// Parse an unquoted string (identifier)
static PyObject* parse_unquoted_string(const char** s) {
    const char* start = *s;
    while (**s != '\0' && !is_xton_identifier_delimiter(**s)) {
        (*s)++;
    }
    if (*s == start) { // No characters consumed
        return NULL; // Not a valid unquoted string
    }
    return PyUnicode_FromStringAndSize(start, *s - start);
}


// Parse an array
static PyObject* parse_array(const char** s) {
    if (**s != '[') {
        return NULL; // Not an array
    }
    (*s)++; // Skip the leading '['

    PyObject* list = PyList_New(0);
    if (!list) {
        return NULL;
    }

    // Handle empty array case
    skip_whitespace(s);
    if (**s == ']') {
        (*s)++; // Skip the closing ']'
        return list;
    }

    // Loop to parse array elements
    while (**s != '\0') {
        PyObject* value = parse_value(s);
        if (!value) {
            Py_DECREF(list);
            return NULL;
        }
        if (PyList_Append(list, value) == -1) {
            Py_DECREF(list);
            Py_DECREF(value);
            return NULL;
        }
        Py_XDECREF(value); // PyList_Append increases ref count, so we can decrement ours

        skip_whitespace(s);
        if (**s == ']') { // Found closing bracket, end of array
            (*s)++;
            return list;
        } else if (**s == '/') { // Found delimiter, more elements to come
            (*s)++; // Skip the '/' delimiter
            skip_whitespace(s);
        } else {
            PyErr_SetString(PyExc_ValueError, "Expected '/' or ']' in XTon array.");
            Py_DECREF(list);
            return NULL;
        }
    }

    // If loop finishes without finding ']', it's an unterminated array
    PyErr_SetString(PyExc_ValueError, "Unterminated XTon array.");
    Py_DECREF(list);
    return NULL;
}

// Parse an object
static PyObject* parse_object(const char** s) {
    if (**s != '<') {
        return NULL; // Not an object
    }
    (*s)++; // Skip the leading '<'

    PyObject* dict = PyDict_New();
    if (!dict) {
        return NULL;
    }

    // Handle empty object case
    skip_whitespace(s);
    if (**s == '>') {
        (*s)++; // Skip the closing '>'
        return dict;
    }

    // Loop to parse object key-value pairs
    while (**s != '\0') {
        PyObject* key = NULL;
        // Try parsing quoted string as key first
        key = parse_quoted_string(s);
        if (!key) { // If not a quoted string, try unquoted string
            key = parse_unquoted_string(s);
        }
        
        if (!key) { // If neither quoted nor unquoted string, it's an error
            Py_DECREF(dict);
            return NULL;
        }

        skip_whitespace(s);
        if (**s != '-') {
            PyErr_SetString(PyExc_ValueError, "Expected '-' after key in XTon object.");
            Py_DECREF(dict);
            Py_DECREF(key);
            return NULL;
        }
        (*s)++; // Skip the '-'
        skip_whitespace(s);

        PyObject* value = parse_value(s);
        if (!value) {
            Py_DECREF(dict);
            Py_DECREF(key);
            return NULL;
        }

        if (PyDict_SetItem(dict, key, value) == -1) {
            Py_DECREF(dict);
            Py_DECREF(key);
            Py_DECREF(value);
            return NULL;
        }
        Py_XDECREF(key); // PyDict_SetItem increases ref count, so we can decrement ours
        Py_XDECREF(value);

        skip_whitespace(s);
        if (**s == '>') { // Found closing bracket, end of object
            (*s)++;
            return dict;
        } else if (**s == '/') { // Found delimiter, more key-value pairs to come
            (*s)++; // Skip the '/' delimiter
            skip_whitespace(s);
        } else {
            PyErr_SetString(PyExc_ValueError, "Expected '/' or '>' in XTon object.");
            Py_DECREF(dict);
            return NULL;
        }
    }

    // If loop finishes without finding '>', it's an unterminated object
    PyErr_SetString(PyExc_ValueError, "Unterminated XTon object.");
    Py_DECREF(dict);
    return NULL;
}


// Central parsing logic for any XTon value
static PyObject* parse_value(const char** s) {
    skip_whitespace(s);

    // Order matters: try complex structures first, then literals, then numbers, then strings
    // Strings are tricky as they can be quoted or unquoted.

    // Try parsing object
    PyObject* obj = parse_object(s);
    if (obj) {
        return obj;
    }
    
    // Try parsing array
    PyObject* arr = parse_array(s);
    if (arr) {
        return arr;
    }

    // Check for boolean and null literals
    if (strncmp(*s, "\\true", 5) == 0) {
        *s += 5;
        return Py_True;
    }
    if (strncmp(*s, "\\false", 6) == 0) {
        *s += 6;
        return Py_False;
    }
    if (strncmp(*s, "\\none", 5) == 0) {
        *s += 5;
        return Py_None;
    }

    // Check for numbers (must come after literal checks, as numbers might start with '\' which is not the case for XTon)
    if (isdigit(**s) || (**s == '-' && isdigit(*(*s + 1)))) {
        PyObject* num = parse_number(s);
        if (num) {
            return num;
        }
    }

    // Try parsing quoted string
    PyObject* quoted_str = parse_quoted_string(s);
    if (quoted_str) {
        return quoted_str;
    }

    // Finally, try parsing unquoted string (identifier). This should be last to avoid false positives with other types.
    PyObject* unquoted_str = parse_unquoted_string(s);
    if (unquoted_str) {
        return unquoted_str;
    }
    
    PyErr_SetString(PyExc_ValueError, "Invalid XTon format or unsupported type.");
    return NULL;
}

// Main parsing function for an XTon string
static PyObject* xton_loads(PyObject *self, PyObject *args) {
    const char *s_input;
    const char *s_current;
    const char *s_initial_pos; // Store initial position after whitespace

    if (!PyArg_ParseTuple(args, "s", &s_input)) {
        return NULL;
    }

    s_current = s_input;

    skip_whitespace(&s_current);
    s_initial_pos = s_current; // Store position after initial whitespace

    // --- Attempt 1: Parse as a single value ---
    const char *s_temp_for_value_parse = s_initial_pos; // Use a temporary pointer for this attempt
    PyObject* result_as_value = parse_value(&s_temp_for_value_parse);

    if (result_as_value) {
        // Check if the entire string was consumed by parsing as a single value
        const char *s_temp_after_value = s_temp_for_value_parse;
        skip_whitespace(&s_temp_after_value);
        if (*s_temp_after_value == '\0') {
            return result_as_value; // Successfully parsed as a single value, no extra characters
        }
        // Else: Parsed as a single value, but there are extra characters.
        // This 'result_as_value' is likely just a part of a larger structure (e.g., a key)
        // We will discard this 'result_as_value' and proceed to try parsing as a KV pair.
        Py_XDECREF(result_as_value); // Discard this partial result.
        // Clear any error set by parse_value if it encountered "Extra characters"
        PyErr_Clear(); 
    } else {
        // parse_value failed. Clear its error to try KV pair.
        PyErr_Clear();
    }
    
    // --- Attempt 2: Parse as a top-level key-value pair ---
    // Reset pointer for KV pair attempt
    s_current = s_initial_pos; 

    PyObject* key = NULL;
    PyObject* value = NULL;
    PyObject* dict = NULL;

    key = parse_unquoted_string(&s_current);
    if (key) {
        skip_whitespace(&s_current);
        if (*s_current == '-') {
            s_current++; // Skip '-'
            skip_whitespace(&s_current);
            value = parse_value(&s_current);
            if (value) {
                dict = PyDict_New();
                if (!dict) {
                    Py_XDECREF(key); Py_XDECREF(value); return NULL;
                }
                if (PyDict_SetItem(dict, key, value) == -1) {
                    Py_XDECREF(key); Py_XDECREF(value); Py_XDECREF(dict); return NULL;
                }
                Py_XDECREF(key);
                Py_XDECREF(value);

                skip_whitespace(&s_current);
                if (*s_current != '\0') {
                    PyErr_SetString(PyExc_ValueError, "Extra characters after XTon value.");
                    Py_XDECREF(dict); return NULL;
                }
                return dict;
            }
        }
    }

    // If neither attempt succeeded
    Py_XDECREF(key); // Cleanup if key was parsed but not dict was created
    Py_XDECREF(value); // Cleanup if value was parsed but not dict was created
    PyErr_SetString(PyExc_ValueError, "Invalid XTon format or unsupported type."); // Changed to ValueError
    return NULL;
}


// --- DUMPS FUNCTIONALITY ---

// Forward declaration for recursive calls
static PyObject* _serialize_to_xton(PyObject* obj);

// Helper for string serialization, adding quotes and escaping
static PyObject* _serialize_quoted_string(PyObject* obj) {
    Py_ssize_t len;
    const char* s = PyUnicode_AsUTF8AndSize(obj, &len);
    if (!s) return NULL; // Error in conversion

    // Determine required size for escaped string
    Py_ssize_t escaped_len = 0;
    for (Py_ssize_t i = 0; i < len; i++) {
        char c = s[i];
        // Special characters to escape: '-', '<', '>', '[', ']', '/', ''' and '\'
        if (c == '-' || c == '<' || c == '>' || c == '[' || c == ']' || c == '/' || c == '\'' || c == '\\') {
            escaped_len += 2; // for escape char + original char
        } else {
            escaped_len += 1;
        }
    }

    // Allocate buffer for escaped string plus quotes and null terminator
    // +2 for the enclosing single quotes, +1 for null terminator
    char* buffer = (char*)PyMem_Malloc(escaped_len + 3); 
    if (!buffer) {
        PyErr_NoMemory();
        return NULL;
    }

    Py_ssize_t current_pos = 0;
    buffer[current_pos++] = '\''; // Opening single quote

    for (Py_ssize_t i = 0; i < len; i++) {
        char c = s[i];
        // Add '\' to the list of characters that need a preceding '\'
        if (c == '-' || c == '<' || c == '>' || c == '[' || c == ']' || c == '/' || c == '\'' || c == '\\') {
            buffer[current_pos++] = '\\'; // Add escape character
        }
        buffer[current_pos++] = c;
    }

    buffer[current_pos++] = '\''; // Closing single quote
    buffer[current_pos] = '\0'; // Null terminator

    PyObject* result = PyUnicode_FromString(buffer);
    PyMem_Free(buffer);
    return result;
}

// New helper function to get string representation (quoted or unquoted) for array elements and dict keys
static PyObject* _serialize_maybe_unquoted_string(PyObject* obj) {
    Py_ssize_t len;
    const char* s = PyUnicode_AsUTF8AndSize(obj, &len);
    if (!s) return NULL; // Error in conversion

    int needs_quoting = 0;
    if (len == 0) { // Empty string always needs quoting
        needs_quoting = 1;
    } else {
        for (Py_ssize_t i = 0; i < len; i++) {
            char c = s[i];
            // If it's an XTon delimiter or whitespace, or a single quote, or a backslash, it needs quoting.
            if (is_xton_identifier_delimiter(c) || c == '\'' || c == '\\') {
                needs_quoting = 1;
                break;
            }
        }
    }

    if (needs_quoting) {
        return _serialize_quoted_string(obj); // This function adds '' and escapes
    } else {
        // Return a new reference to the string object for consistency with other returns
        Py_INCREF(obj);
        return obj; // Unquoted
    }
}


static PyObject* _serialize_to_xton(PyObject* obj) {
    if (obj == Py_None) {
        return PyUnicode_FromString("\\none");
    }
    if (obj == Py_True) {
        return PyUnicode_FromString("\\true");
    }
    if (obj == Py_False) {
        return PyUnicode_FromString("\\false");
    }

    if (PyLong_Check(obj)) {
        return PyObject_Repr(obj); // Converts int to string representation
    }
    if (PyFloat_Check(obj)) {
        // PyObject_Repr for float might include 'L' or 'j' for complex, so we convert directly
        // to string for more control, or or use repr for simplicity
        return PyObject_Repr(obj); 
    }
    if (PyUnicode_Check(obj)) {
        // Independent string values are always quoted
        return _serialize_quoted_string(obj);
    }
    if (PyList_Check(obj)) {
        Py_ssize_t size = PyList_Size(obj);
        PyObject* parts = PyList_New(0); // List to hold serialized parts
        if (!parts) return NULL;

        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject* item = PyList_GetItem(obj, i);
            PyObject* serialized_item = NULL;
            // Special handling for string elements in lists
            if (PyUnicode_Check(item)) {
                serialized_item = _serialize_maybe_unquoted_string(item);
            } else {
                serialized_item = _serialize_to_xton(item);
            }
            
            if (!serialized_item) {
                Py_DECREF(parts);
                return NULL;
            }
            if (PyList_Append(parts, serialized_item) == -1) {
                Py_DECREF(parts);
                Py_DECREF(serialized_item);
                return NULL;
            }
            Py_XDECREF(serialized_item);
        }

        PyObject* separator = PyUnicode_FromString("/");
        if (!separator) {
            Py_DECREF(parts);
            return NULL;
        }
        PyObject* joined = PyUnicode_Join(separator, parts);
        Py_DECREF(separator);
        Py_DECREF(parts);
        if (!joined) return NULL;

        PyObject* result = PyUnicode_FromFormat("[%U]", joined);
        Py_DECREF(joined);
        return result;
    }
    if (PyDict_Check(obj)) {
        PyObject* items = PyDict_Items(obj);
        if (!items) return NULL;
        Py_ssize_t size = PyList_Size(items);
        PyObject* parts = PyList_New(0); // List to hold serialized key-value parts
        if (!parts) {
            Py_DECREF(items);
            return NULL;
        }

        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject* item_tuple = PyList_GetItem(items, i); // (key, value) tuple
            PyObject* key_obj = PyTuple_GetItem(item_tuple, 0);
            PyObject* value_obj = PyTuple_GetItem(item_tuple, 1);

            // Keys must be strings. Unquoted if possible, else quoted.
            PyObject* serialized_key = NULL;
            if (PyUnicode_Check(key_obj)) {
                serialized_key = _serialize_maybe_unquoted_string(key_obj); // Use new helper
            } else {
                PyErr_SetString(PyExc_TypeError, "XTon dictionary keys must be strings.");
                Py_DECREF(items);
                Py_DECREF(parts);
                return NULL;
            }
            if (!serialized_key) {
                Py_DECREF(items);
                Py_DECREF(parts);
                return NULL;
            }

            PyObject* serialized_value = _serialize_to_xton(value_obj);
            if (!serialized_value) {
                Py_DECREF(items);
                Py_DECREF(parts);
                Py_DECREF(serialized_key);
                return NULL;
            }

            PyObject* kv_pair = PyUnicode_FromFormat("%U-%U", serialized_key, serialized_value);
            Py_XDECREF(serialized_key);
            Py_XDECREF(serialized_value);
            if (!kv_pair) {
                Py_DECREF(items);
                Py_DECREF(parts);
                return NULL;
            }
            if (PyList_Append(parts, kv_pair) == -1) {
                Py_DECREF(items);
                Py_DECREF(parts);
                Py_DECREF(kv_pair);
                return NULL;
            }
            Py_XDECREF(kv_pair);
        }
        Py_DECREF(items);

        PyObject* separator = PyUnicode_FromString("/");
        if (!separator) {
            Py_DECREF(parts);
            return NULL;
        }
        PyObject* joined = PyUnicode_Join(separator, parts);
        Py_DECREF(separator);
        Py_DECREF(parts);
        if (!joined) return NULL;

        PyObject* result = PyUnicode_FromFormat("<%U>", joined);
        Py_DECREF(joined);
        return result;
    }

    PyErr_SetString(PyExc_TypeError, "Unsupported Python type for XTon serialization.");
    return NULL;
}


static PyObject* xton_dumps(PyObject *self, PyObject *args) {
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    // Special handling for top-level dictionaries with a single key-value pair
    if (PyDict_Check(obj)) {
        Py_ssize_t size = PyDict_Size(obj);
        if (size == 1) { // Potentially a top-level key-value pair
            PyObject* key_py = NULL;
            PyObject* value_py = NULL;
            Py_ssize_t pos = 0;
            
            // Get the single key-value pair
            if (PyDict_Next(obj, &pos, &key_py, &value_py)) {
                // Check if key is a string and can be unquoted
                if (PyUnicode_Check(key_py)) {
                    Py_ssize_t key_len;
                    const char* key_s = PyUnicode_AsUTF8AndSize(key_py, &key_len);
                    int needs_quoting = 0;
                    if (key_len == 0) needs_quoting = 1;
                    else {
                        for (Py_ssize_t i = 0; i < key_len; i++) {
                            char c = key_s[i];
                            // If it's an XTon delimiter or whitespace, or a single quote, or a backslash, it needs quoting.
                            if (is_xton_identifier_delimiter(c) || c == '\'' || c == '\\') {
                                needs_quoting = 1;
                                break;
                            }
                        }
                    }

                    if (!needs_quoting) { // It's an unquoted key
                        PyObject* serialized_key = _serialize_maybe_unquoted_string(key_py); // This will return key_py itself (incref'd)
                        PyObject* serialized_value = _serialize_to_xton(value_py);
                        if (!serialized_key || !serialized_value) {
                            Py_XDECREF(serialized_key); // Clean up if one failed
                            Py_XDECREF(serialized_value);
                            return NULL;
                        }
                        PyObject* result = PyUnicode_FromFormat("%U-%U", serialized_key, serialized_value);
                        Py_XDECREF(serialized_key);
                        Py_XDECREF(serialized_value);
                        return result;
                    }
                }
            }
        }
    }
    // Default serialization for other types or dicts that don't fit the single KV top-level rule
    return _serialize_to_xton(obj);
}


// Method definition object
static PyMethodDef XTonMethods[] = {
    {"loads", xton_loads, METH_VARARGS, "Parse an XTon string into a Python object."},
    {"dumps", xton_dumps, METH_VARARGS, "Serialize a Python object into an XTon string."},
    {NULL, NULL, 0, NULL} // Sentinel
};

// Module definition
static struct PyModuleDef xtonmodule = {
    PyModuleDef_HEAD_INIT,
    "xton._c_ext", // name of module
    "A C extension for XTon parsing.", // module documentation, may be NULL
    -1, // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
    XTonMethods
};

// Module initialization function
PyMODINIT_FUNC PyInit__c_ext(void) {
    return PyModule_Create(&xtonmodule);
}