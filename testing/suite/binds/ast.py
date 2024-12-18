
class WAST:
    class Function():
        def __init__(self, fn_name:str, fn_type:str, arg_types:list[str], body:list):
            self.fn_name = fn_name
            self.fn_type = fn_type
            self.arg_types = arg_types
            self.body = body
        def __eq__(self, other_fn):
            isEq = True
            isEq = isEq and self.fn_name == other_fn.fn_name
            isEq = isEq and self.fn_type == other_fn.fn_type
            isEq = isEq and self.arg_types == other_fn.arg_types
            isEq = isEq and self.body == other_fn.body