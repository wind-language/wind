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

```rs
@extern func malloc(size: int) : long;
@extern func puts(str: long) : int;
@extern func printf(str: long, ...) : int;

func add(a: int, b: int) : short {
  return a + b;
}

func main() : int {
  printf("Sum: %s\n", add(1, 2));
  return 0;
}
```

This code runs **67% faster than C++** and **44% faster than Rust** (`println`).

This code compiles **46% faster than C++** and **120% faster than Rust**

<p align="right">
<sub>(Preview)</sub>
</p>

</td>
</tr>
</table>

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

Run [runtime compilation](#troubleshooting) before running the following commands.

```sh
wind <file> -o <output>

### EXECUTABLE + SHOW AST + SHOW IR

wind main.w -o main -sa -si

### OBJECT FILE

wind main.w -o main.o --ej
```

## Troubleshooting

> [!WARNING] > `ld: cannot find wind_runtime.o: No such file or directory`

---

<details>
<summary> Why is this happening? </summary>

[❗] Wind has a runtime utility that is required to jump to main function and run stack checks. Just compile it before linking executables.

</details>

---

### Solution

```sh
wind src/runtime/stack.w src/runtime/start.w -ej -o src/runtime/wind_runtime.o
```
