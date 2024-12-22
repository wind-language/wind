import sys, os, _io
sys.path.append(
    os.path.join(
        os.path.realpath(os.path.dirname(__file__)),
        'lib/'
    )
)
import libpyapi
from suite.binds import *

class WTest:
    def __init__(self):
        libpyapi.initLib()
    
    def newRun(self):
        libpyapi.newRun()
    
    def lexTest(self, f:any, tokens:list) -> dict:
        self.newRun()
        if type(f) == _io.TextIOWrapper:
            retToks = WAPI.file2tok(f.name)
        else:
            assert type(f) == str, "f must be a string or file object"
            retToks = WAPI.lit2tok(f)
        status = {
            "success": True,
            "failures": []
        }
        for i in range(len(retToks)):
            if i >= len(tokens):
                status["success"] = False
                status["failures"].append(f"Token stream is longer than expected, got {retToks[i].value}")
                continue
            elif retToks[i].value != tokens[i].value:
                status["success"] = False
                status["failures"].append(f"Expected {tokens[i].value} but got {retToks[i].value}")
            elif retToks[i].type != tokens[i].type:
                status["success"] = False
                status["failures"].append(f"Expected {tokens[i].type} but got {retToks[i].type}")
        if len(retToks) < len(tokens):
            status["success"] = False
            status["failures"].append(f"Token stream is shorter than expected, expected {tokens[len(retToks)].value}")
        return status

    def parseTest(self, f:any, ast:list) -> dict:
        self.newRun()
        if type(f) == _io.TextIOWrapper:
            retAst = WAPI.parseFile(f.name)
        else:
            assert type(f) == str, "f must be a string or file object"
            retAst = WAPI.parseLiteral(f)

    def runTests(
        self,
        parent:str,
        tests:list[dict]
    ):
        print ("  [T]>",parent)
        for test in tests:
            print("    *",test["name"] + ": ", end="")
            args = test["args"]
            if (test["type"].lower() == "lexer"):
                result = WTest().lexTest(args[0], args[1])
            if result["success"]:
                print("✅")
            else:
                print("❌")
                for failure in result["failures"][0:4]:
                    print("      [X]", failure)