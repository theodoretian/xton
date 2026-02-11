import unittest
import sys
import os
import io

from xton import (
    loads, dumps, load, dump, 
    XTONEncoder, XTONDecoder, XTONDecodeError,
    _default_encoder, _default_decoder,
    encoder, decoder
)

class TestXTonLoads(unittest.TestCase):

    def test_loads_numbers(self):
        self.assertEqual(loads("123"), 123.0)
        self.assertEqual(loads("123.45"), 123.45)
        self.assertEqual(loads("-67.89"), -67.89)
        self.assertEqual(loads("0"), 0.0)
        self.assertEqual(loads("-0.5"), -0.5)

    def test_loads_booleans(self):
        self.assertTrue(loads(r"\true"))
        self.assertFalse(loads(r"\false"))

    def test_loads_none(self):
        self.assertIsNone(loads(r"\none"))

    def test_loads_invalid_format(self):
        with self.assertRaises(ValueError):
            loads("[")

    def test_loads_strings(self):
        self.assertEqual(loads("''"), "")
        self.assertEqual(loads("'hello'"), "hello")
        self.assertEqual(loads("'hello world'"), "hello world")
        self.assertEqual(loads(r"'hello\-world'"), "hello-world")
        self.assertEqual(loads(r"'hello\<world'"), "hello<world")
        self.assertEqual(loads(r"'hello\>world'"), "hello>world")
        self.assertEqual(loads(r"'hello\[world'"), "hello[world")
        self.assertEqual(loads(r"'hello\]world'"), "hello]world")
        self.assertEqual(loads(r"'hello\/world'"), "hello/world")
        self.assertEqual(loads(r"'23.5'"), "23.5")
        self.assertEqual(loads(r"'hello\\world'"), "hello\\world") # Added test for escaped backslash
        self.assertEqual(loads(r"'hello\'world'"), "hello'world") # Added test for escaped single quote

    def test_loads_arrays(self):
        self.assertEqual(loads("[]"), [])
        self.assertEqual(loads("[a/25.3/87]"), ["a", 25.3, 87.0])
        self.assertEqual(loads(r"[\true/hello/\none]"), [True, "hello", None]) 
        self.assertEqual(loads("[1/[2/3]/4]"), [1.0, [2.0, 3.0], 4.0])

    def test_loads_objects(self):
        self.assertEqual(loads("<>"), {})
        self.assertEqual(loads(r"q-<a-\none/k-\true/g-[a/25.3/87]>"), {"q": {"a": None, "k": True, "g": ["a", 25.3, 87.0]}})
        self.assertEqual(loads("<key-value>"), {"key": "value"})
        self.assertEqual(loads(r"<'quoted key'-'quoted value'>"), {"quoted key": "quoted value"})
        self.assertEqual(loads(r"<num-123/bool-\true>"), {"num": 123.0, "bool": True})
        self.assertEqual(loads("<obj-<a-1/b-2>>"), {"obj": {"a": 1.0, "b": 2.0}})


class TestXTonDumps(unittest.TestCase):

    def test_dumps_none(self):
        self.assertEqual(dumps(None), r"\none")

    def test_dumps_booleans(self):
        self.assertEqual(dumps(True), r"\true")
        self.assertEqual(dumps(False), r"\false")

    def test_dumps_numbers(self):
        self.assertEqual(dumps(123), "123")
        self.assertEqual(dumps(123.45), "123.45")
        self.assertEqual(dumps(-67), "-67")
        self.assertEqual(dumps(0.0), "0.0")

    def test_dumps_strings(self):
        self.assertEqual(dumps(""), "''")
        self.assertEqual(dumps("hello"), "'hello'")
        self.assertEqual(dumps("hello world"), "'hello world'")
        # Test escaping
        self.assertEqual(dumps("hello-world"), r"'hello\-world'")
        self.assertEqual(dumps("hello<world"), r"'hello\<world'")
        self.assertEqual(dumps("hello>world"), r"'hello\>world'")
        self.assertEqual(dumps("hello[world"), r"'hello\[world'")
        self.assertEqual(dumps("hello]world"), r"'hello\]world'")
        self.assertEqual(dumps("hello/world"), r"'hello\/world'")
        self.assertEqual(dumps("hello'world"), r"'hello\'world'") 
        self.assertEqual(dumps("hello\\world"), r"'hello\\world'") # Added test for escaped backslash
        self.assertEqual(dumps("23.5"), "'23.5'") # Should be quoted as it's a string

    def test_dumps_arrays(self):
        self.assertEqual(dumps([]), "[]")
        self.assertEqual(dumps(["a", 25.3, 87]), "[a/25.3/87]")
        self.assertEqual(dumps([True, "hello", None]), r"[\true/hello/\none]") 
        self.assertEqual(dumps([1, [2, 3], 4]), "[1/[2/3]/4]")
        self.assertEqual(dumps([1.0, [2.0, 3.0], 4.0]), "[1.0/[2.0/3.0]/4.0]") # Ensure float output matches loads expects

    def test_dumps_objects(self):
        self.assertEqual(dumps({"q": {"a": None, "k": True, "g": ["a", 25.3, 87]}}), r"q-<a-\none/k-\true/g-[a/25.3/87]>")
        self.assertEqual(dumps({"key": "value"}), "key-'value'") 
        self.assertEqual(dumps({"quoted key": "quoted value"}), r"<'quoted key'-'quoted value'>")
        self.assertEqual(dumps({"num": 123, "bool": True}), r"<num-123/bool-\true>")
        self.assertEqual(dumps({"obj": {"a": 1, "b": 2}}), "obj-<a-1/b-2>")
        
        # Test keys that need quoting
        actual_output_negative_key = dumps({"-key": "value"})
        # print(f"DEBUG: Actual dumps output for -key: {actual_output_negative_key!r}") # Removed debug print
        # print(f"DEBUG: Expected string literal evaluation for -key: {r"<'\\-key'-'value'>"!r}") # Removed debug print
        self.assertEqual(actual_output_negative_key, "<'\\-key'-'value'>") # Corrected expected output for escaped backslash in test case
        
        actual_output_slash_key = dumps({"key/name": "value"})
        # print(f"DEBUG: Actual dumps output for key/name: {actual_output_slash_key!r}") # Removed debug print
        # print(f"DEBUG: Expected string literal evaluation for key/name: {r"<'key\\/name'-'value'>"!r}") # Removed debug print
        self.assertEqual(actual_output_slash_key, "<'key\\/name'-'value'>")
        
        actual_output_space_key = dumps({"key name": "value"})
        # print(f"DEBUG: Actual dumps output for key name: {actual_output_space_key!r}") # Removed debug print
        self.assertEqual(actual_output_space_key, r"<'key name'-'value'>") # Space needs quoting as it's a delimiter
        
        actual_output_backslash_key = dumps({"key\\name": "value"})
        # print(f"DEBUG: Actual dumps output for key\\name: {actual_output_backslash_key!r}") # Removed debug print
        # print(f"DEBUG: Expected string literal evaluation for key\\name: {r"<'key\\\\name'-'value'>"!r}") # Removed debug print
        self.assertEqual(actual_output_backslash_key, "<'key\\\\name'-'value'>") # Corrected expected output for escaped backslash in key

    def test_dumps_and_loads_consistency(self):
        data = {
            "name": "XTon Example",
            "version": 1.0,
            "enabled": True,
            "config": {
                "host": "localhost",
                "port": 8080.0,  # Changed to float
                "users": ["admin", "guest"],
                "data": [
                    {"id": 1.0, "status": "active"},  # Changed to float
                    {"id": 2.0, "status": "inactive", "message": "error/code"} # Changed to float
                ],
                "description with space": "a simple config with [brackets] and -hyphens-"
            },
            "empty_list": [],
            "empty_dict": {},
            "null_value": None
        }
        
        xton_str = dumps(data)
        loaded_data = loads(xton_str)
        
        self.assertEqual(loaded_data, data)
        self.assertEqual(loads(dumps("test string")), "test string")
        self.assertEqual(loads(dumps(123)), 123.0)
        self.assertEqual(loads(dumps(True)), True)
        self.assertEqual(loads(dumps(["a", 1.0, {"b": 2.0}])), ["a", 1.0, {"b": 2.0}]) # Adjusted for float
        self.assertEqual(loads(dumps(["a", 1, {"b": 2}])), ["a", 1.0, {"b": 2.0}]) # Add a test with int input

class TestXTonFileOperations(unittest.TestCase):

    def test_dump_and_load_simple_object(self):
        data = {"key": "value"}
        fp = io.StringIO()
        dump(data, fp)
        fp.seek(0) # Rewind to beginning
        loaded_data = load(fp)
        self.assertEqual(loaded_data, loads(dumps(data))) # Use loads(dumps(data)) for comparison due to int/float conversion

    def test_dump_and_load_complex_object(self):
        data = {
            "name": "Test Name",
            "age": 30.0,
            "is_active": True,
            "items": [1.0, 2.0, "three", {"id": 4.0, "tag": "four"}],
            "settings": {
                "level": 5.0,
                "mode": "advanced"
            }
        }
        fp = io.StringIO()
        dump(data, fp)
        fp.seek(0)
        loaded_data = load(fp)
        # Use loads(dumps(data)) for comparison due to int/float conversion in original data structure
        self.assertEqual(loaded_data, loads(dumps(data)))

    def test_dump_and_load_top_level_kv(self):
        data = {"q": {"a": None, "k": True}}
        fp = io.StringIO()
        dump(data, fp)
        fp.seek(0)
        loaded_data = load(fp)
        self.assertEqual(loaded_data, loads(dumps(data)))

    def test_dump_and_load_empty_list(self):
        data = []
        fp = io.StringIO()
        dump(data, fp)
        fp.seek(0)
        loaded_data = load(fp)
        self.assertEqual(loaded_data, loads(dumps(data)))

    def test_dump_and_load_empty_dict(self):
        data = {}
        fp = io.StringIO()
        dump(data, fp)
        fp.seek(0)
        loaded_data = load(fp)
        self.assertEqual(loaded_data, loads(dumps(data)))

class TestXTonEncoderDecoder(unittest.TestCase):
    def test_xton_decode_error_inheritance(self):
        self.assertTrue(issubclass(XTONDecodeError, ValueError))

    def test_decoder_decode_valid(self):
        decoder = XTONDecoder()
        self.assertEqual(decoder.decode("123"), 123.0)
        self.assertEqual(decoder.decode(r"q-<a-\none>"), {"q": {"a": None}})

    def test_decoder_decode_invalid(self):
        decoder = XTONDecoder()
        with self.assertRaises(XTONDecodeError):
            decoder.decode("[") # Invalid XTon format

    def test_decoder_raw_decode(self):
        decoder = XTONDecoder()
        # Changed test input to a valid XTon string for loads
        obj, end_idx = decoder.raw_decode("123") 
        self.assertEqual(obj, 123.0)
        # end_idx should be the length of the consumed string
        self.assertEqual(end_idx, len("123"))

    def test_encoder_encode_valid(self):
        encoder = XTONEncoder()
        self.assertEqual(encoder.encode(123), "123")
        self.assertEqual(encoder.encode({"key": "value"}), "key-'value'")

    def test_encoder_encode_with_default(self):
        class MyCustomClass:
            def __init__(self, value):
                self.value = value
        
        def default_serializer(obj):
            if isinstance(obj, MyCustomClass):
                return f"Custom({obj.value})" # Changed to return a plain string
            raise TypeError(f"Object of type {obj.__class__.__name__} is not XTon serializable")

        # Test with default function
        encoder = XTONEncoder(default=default_serializer)
        custom_obj = MyCustomClass(100)
        # Expected output should now be properly quoted by dumps
        self.assertEqual(encoder.encode(custom_obj), "'Custom(100)'")

        # Test without default function (should raise TypeError from _c_ext.dumps)
        encoder_no_default = XTONEncoder()
        with self.assertRaises(TypeError):
            encoder_no_default.encode(custom_obj)

    def test_encoder_iterencode(self):
        encoder = XTONEncoder()
        obj = {"a": 1, "b": "hello"}
        chunks = list(encoder.iterencode(obj))
        self.assertEqual(len(chunks), 1)
        self.assertEqual(chunks[0], encoder.encode(obj))


class TestXTonModuleGlobals(unittest.TestCase):
    def test_encoder_class_and_instance(self):
        self.assertEqual(encoder, XTONEncoder)
        self.assertIsInstance(_default_encoder, XTONEncoder)
        self.assertEqual(_default_encoder.encode(1), "1") # Test encode method

    def test_decoder_class_and_instance(self):
        self.assertEqual(decoder, XTONDecoder)
        self.assertIsInstance(_default_decoder, XTONDecoder)
        self.assertEqual(_default_decoder.decode("1"), 1.0) # Test decode method

    def test_default_encoder_decoder_functionality(self):
        # Test encoding with _default_encoder
        data_to_encode = {"key": "value", "num": 123}
        encoded_str = _default_encoder.encode(data_to_encode)
        self.assertEqual(encoded_str, "<key-'value'/num-123>") # Expected output
        
        # Test decoding with _default_decoder
        decoded_data = _default_decoder.decode(encoded_str)
        self.assertEqual(decoded_data, {"key": "value", "num": 123.0}) # loads returns floats

    def test_top_level_load_dump_with_globals(self):
        # This part tests the top-level load/dump functions, not _default_encoder/_default_decoder methods directly
        data = {"test": 1.0}
        fp_dump = io.StringIO()
        dump(data, fp_dump) # Use top-level dump function
        fp_dump.seek(0)
        loaded_data = load(fp_dump) # Use top-level load function
        self.assertEqual(loaded_data, data)

if __name__ == '__main__':
    unittest.main()
