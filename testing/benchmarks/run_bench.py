import subprocess,time,os,sys

WIND_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "build", "wind")
OUTPUT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build/")
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
            [path],
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
        return compile_time, run_time, output

    def process_casetype(casetype, input):
        print("-"*50)
        file = os.path.basename(casetype)
        if casetype.endswith(".w"):
            print("Running WIND benchmark: "+file)
        elif casetype.endswith(".cpp"):
            print("Running C++ benchmark: "+file)
        else:
            raise Exception("Invalid file type")
        compile_time, run_time, output = WBench.run_bench(casetype, input)
        print("Compile time: "+hrtd(compile_time))
        print("Run time: "+hrtd(run_time))
        print("Output: "+output)
        print("-"*50)

    def run_case(case, input: str=""):
        path = CASES_PATH+case
        for file in os.listdir(path):
            path = os.path.join(path, file)
            WBench.process_casetype(path, input)

INPUT = """100\n329404 270217\n224240 493803\n268847 243953\n84230 474836\n137041 458239\n171353 224814\n319366 491382\n20012 27853\n79187 157590\n342076 201019\n339642 108747\n362056 6577\n91792 443044\n290663 97700\n373366 468352\n287537 302480\n337406 82665\n58271 459526\n92104 272599\n115039 85418\n20843 465178\n52631 476848\n100795 278734\n177619 144026\n266307 213135\n218974 16076\n88010 422170\n195700 419518\n49089 421402\n445271 94813\n381577 162459\n132506 356822\n86182 253816\n47083 297154\n499896 404829\n19000 406343\n133617 289786\n356085 141451\n180200 443084\n428605 386613\n255406 299811\n442489 159186\n235204 39384\n334326 195278\n166016 386490\n140056 256382\n263710 145013\n24022 184518\n303809 38571\n309487 230317\n480253 90264\n73384 143855\n295192 485628\n167280 97401\n446629 473043\n227102 34867\n399894 192739\n76931 36127\n121708 269069\n440542 22249\n438186 477626\n347533 100549\n242631 471232\n389590 404087\n99127 44074\n180575 15977\n256927 280412\n75970 371005\n301505 359288\n215978 327105\n258338 340934\n106298 431559\n168456 179766\n440064 81475\n68107 285052\n85899 24981\n279525 161610\n16605 282049\n240744 108180\n377157 415447\n402054 280872\n111435 419544\n286470 6804\n355075 78037\n38593 190004\n479356 471554\n472312 168792\n8291 139616\n436147 19706\n381073 346097\n283370 110453\n378280 412140\n188585 47845\n427184 300100\n353449 220166\n32214 347673\n266029 416623\n56525 341891\n136963 72295\n423103 145649'"""

WBench.run_case("sum_even", INPUT)