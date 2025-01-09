import subprocess,time,os,sys

WIND_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "build", "windc")
OUTPUT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build/")
if (not os.path.exists(OUTPUT_PATH)):
    os.makedirs(OUTPUT_PATH)
CASES_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cases/")

WIND_FLAGS = []
CPP_FLAGS = []


def hrtd(diff):
    if diff < 1e-6:
        # Nanoseconds
        return f"{diff * 1e9:.3f} ns"
    elif diff < 1e-3:
        # Microseconds
        return f"{diff * 1e6:.3f} µs"
    elif diff < 1:
        # Milliseconds
        return f"{diff * 1e3:.3f} ms"
    else:
        # Seconds
        return f"{diff:.6f} s"

class WBench:
    def compile(path):
        assert(os.path.exists(path))
        if (path.endswith(".w")):
            flags = WIND_FLAGS
            exect = WIND_PATH
            output = "benchWIND"
        elif (path.endswith(".cpp")):
            flags = CPP_FLAGS
            exect = "g++"
            output = "benchCPP"
        else:
            raise Exception("Invalid file type")
        
        start_time = time.time()
        res = subprocess.run(
            [exect, path]+flags+["-o", OUTPUT_PATH+output],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        end_time = time.time()
        compile_time = end_time - start_time
        return compile_time, res.stdout, res.stderr, output

    def run(path, input):
        assert(os.path.exists(path))
        start_time = time.time()
        res = subprocess.run(
            ["stdbuf", "-oL", path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            input=input,
            text=True
        )
        end_time = time.time()
        run_time = end_time - start_time
        return run_time, res.stdout, res.stderr

    def run_bench(path, input):
        compile_time, compile_out, compile_err, output = WBench.compile(path)
        if compile_err:
            print("Compilation error: "+compile_err)
        run_time, run_out, run_err = WBench.run(OUTPUT_PATH+output, input)
        return compile_time, run_time, output, run_out, run_err

    def process_casetype(casetype, input):
        if (casetype.endswith(".txt")): return
        print("-"*50)
        file = os.path.basename(casetype)
        if casetype.endswith(".w"):
            print("Running WIND benchmark: "+file)  
        elif casetype.endswith(".cpp"):
            print("Running C++ benchmark: "+file)
        else:
            raise Exception("Invalid file type")
        compile_time, run_time, output, stdout, stderr = WBench.run_bench(casetype, input)
        print("Compile time: "+hrtd(compile_time))
        print("Run time: "+hrtd(run_time))
        print("Output file: "+output)
        print("stdout: " +stdout)
        print("stderr: " +stderr)
        print("-"*50)

    def run_case(case, input: str=""):
        path = CASES_PATH+case
        for file in os.listdir(path):
            pathx = os.path.join(path, file)
            WBench.process_casetype(pathx, input)

if (len(sys.argv) < 2) :
    print("Usage: python run_bench.py <case>")
    sys.exit(1)
case = sys.argv[1]
if (not os.path.exists(CASES_PATH+case)):
    print("Case not found")
    sys.exit(1)
if (not os.path.exists(CASES_PATH+case+"/input.txt")):
    print("Input file not found")
    sys.exit(1)
with open(CASES_PATH+case+"/input.txt", "r") as f:
    INPUT = f.read()
    WBench.run_case(case, INPUT)