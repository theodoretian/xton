"""
XTon parser, with a json-like interface.
"""
try:
    from . import _c_ext
except ImportError:
    # This is a fallback in case the C extension is not built
    _c_ext = None

# Define XTONDecodeError
class XTONDecodeError(ValueError):
    """Raised when an XTon document is invalid."""
    pass

def loads(s):
    """
    Parse an XTon string and return a Python object.
    """
    if _c_ext is None:
        raise RuntimeError("XTon C extension not built. Please build the library first.")
    try:
        return _c_ext.loads(s)
    except ValueError as e:
        raise XTONDecodeError(e) from e

def dumps(obj):
    """
    Serialize a Python object into an XTon string.
    """
    if _c_ext is None:
        raise RuntimeError("XTon C extension not built. Please build the library first.")
    return _c_ext.dumps(obj)

def load(fp):
    """
    Deserialize 'fp' (a .read()-supporting file-like object containing an XTon document)
    to a Python object.
    """
    if _c_ext is None:
        raise RuntimeError("XTon C extension not built. Please build the library first.")
    try:
        return _c_ext.loads(fp.read())
    except ValueError as e:
        raise XTONDecodeError(e) from e

def dump(obj, fp):
    """
    Serialize 'obj' to an XTon formatted stream to 'fp' (a .write()-supporting file-like object).
    """
    if _c_ext is None:
        raise RuntimeError("XTon C extension not built. Please build the library first.")
    fp.write(_c_ext.dumps(obj))

# XTONEncoder and XTONDecoder classes
class XTONDecoder:
    """Simple XTON decoder class that mimics json.JSONDecoder."""
    def __init__(self, encoding=None, **kw):
        # For compatibility with json.JSONDecoder, but not strictly used by XTon C extension
        pass

    def decode(self, s):
        """
        Decode an XTon string to a Python object.
        """
        if _c_ext is None:
            raise RuntimeError("XTon C extension not built. Please build the library first.")
        try:
            return _c_ext.loads(s)
        except ValueError as e:
            raise XTONDecodeError(e) from e

    def raw_decode(self, s):
        """
        Decode an XTon string and return a two-tuple of (object, idx),
        where idx is the beginning of the string not consumed by the parser.
        This is not directly supported by current _c_ext.loads, so it's a simplified version.
        """
        # Our loads function expects the entire string to be consumed,
        # so raw_decode will behave similarly to decode for now.
        obj = self.decode(s)
        return obj, len(s)


class XTONEncoder:
    """Simple XTON encoder class that mimics json.JSONEncoder."""
    def __init__(self, skipkeys=False, ensure_ascii=True, check_circular=True, allow_nan=True, indent=None, separators=None, default=None):
        # XTon C extension handles these internally, so mostly for compatibility
        self.default = default # default function to call for objects that dumps cannot serialize
        self.indent = indent # XTon is compact, so indent is not strictly supported yet

    def encode(self, obj):
        """
        Encode a Python object into an XTon string.
        """
        if _c_ext is None:
            raise RuntimeError("XTon C extension not built. Please build the library first.")
        
        # If a default function is provided, try to use it for unsupported types
        if self.default:
            try:
                # This is a simplification; a real encoder might need to iterate
                # through obj and apply default to unsupported parts recursively.
                # For now, we assume _c_ext.dumps will raise TypeError directly
                # and if default can handle it, it will return a serializable type.
                # If _c_ext.dumps raises TypeError, we catch it and try calling default.
                return _c_ext.dumps(obj)
            except TypeError as e:
                try:
                    serializable_obj = self.default(obj)
                    return _c_ext.dumps(serializable_obj)
                except Exception:
                    raise e # Re-raise original TypeError if default also fails
        
        return _c_ext.dumps(obj)

    def iterencode(self, obj):
        """
        Encode the given object and yield each string chunk.
        This is a simplification for XTon, as dumps produces a single string.
        """
        yield self.encode(obj)

# Default encoder and decoder instances
_default_encoder = XTONEncoder()
_default_decoder = XTONDecoder()

# Encoder and decoder modules (classes)
encoder = XTONEncoder
decoder = XTONDecoder
