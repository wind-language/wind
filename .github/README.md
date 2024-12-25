<h1 align="center">
  <br>
  Wind
</h1>
<h2 align="center">

![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/utcq/wind?style=for-the-badge) <span>&nbsp;</span>
![CodeFactor Grade](https://img.shields.io/codefactor/grade/github/utcq/wind?style=for-the-badge&color=blue)

</h2>

<table>
<tr>
<td>
  
_The programming language that you (don't actually) need._ &nbsp; <sub>(WIP)</sub> &nbsp;
Modernized C with a touch of Rust and a sprinkle of Go.


<details>
  <summary>
    This code:
  </summary>

```rs
func occrec(N: int, last: long, M: long, i: int): long {
    branch[
        N==i: return 0;
    ]
    var maxRes: long=-1;
    var digit: int=3;
    loop[digit<=9] {
        branch[
            digit == (last%10): {}
            else : {
                var newLast: long = (last*10)+digit;
                var modc: long = newLast%M;
                branch [
                    modc > maxRes: {
                        maxRes = modc;
                    }
                ]
                var modr: long = occrec(N, newLast, M, i+1);
                branch[
                    modr > maxRes: {
                        maxRes = modr;
                    }
                ]
            }
        ]
        digit=digit+3;
    }
    return maxRes;
}
```
<sub>

**Problem from Training Olinfo: _[ois_cabala](https://training.olinfo.it/task/ois_cabala/)_**

</sub>

</details>

Runs **~2.5x times** faster than **C++**

Compiles **3x times** faster than **C++**

</td>
</tr>
</table>
<div align="center">
  <h3>Try benchmarking it yourself!</h3>
</div>

```sh
cd testing/benchmarks/
python run_bench.py cabala
```

## Command line Table

| ⚙️ Command |  📜 Description   |
| :--------: | :---------------: |
|    `-o`    |    Output file    |
|    `-h`    |     Show help     |
|   `-ej`    | Compile as object |
|   `-sa`    |     Show AST      |
|   `-si`    |      Show IR      |
|   `-ss`    |     Show ASM      |

<h3 align="center">
Example
</h3>

```sh
wind <file> -o <output>

### EXECUTABLE + SHOW AST + SHOW IR

wind main.w -o main -sa -si

### OBJECT FILE

wind main.w -o main.o --ej
```

## Troubleshooting

> [!WARNING]
> `ld: cannot find wind_runtime.o: No such file or directory`

---

<details>
<summary> Why is this happening? </summary>

[❗] Wind has a runtime utility that is required to jump to main function and run stack checks. Just compile it if it hasn't been done by cmake.

</details>

---

### Solution

```sh
wind src/runtime/stack.w src/runtime/start.w -ej -o src/runtime/wind_runtime.o
```
