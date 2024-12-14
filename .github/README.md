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
  var m: long = malloc(64);
  asm {
    mov rax, ?m;
    mov byte ptr [rax], 0x48;
    mov byte ptr [rax+1], 0x65;
  }
  printf("Sum: %s\n" m);
  return 0;
}
```

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
|   `--ej`   | Compile as object |
|   `--sa`   |     Show AST      |
|   `--si`   |      Show IR      |

<h3 align="center">
Example
</h3>

Run [std compilation](#troubleshooting) before running the following commands.

```sh
wind <file> -o <output>

### EXECUTABLE + SHOW AST + SHOW IR

wind main.w -o main -sa -si

### OBJECT FILE

wind main.w -o main.o --ej
```

## Troubleshooting

> [!WARNING]  
> `ld: cannot find raw_std/wind_lib.o: No such file or directory`

---

<details>
<summary> Why is this happening? </summary>

[❗] The link command when compiling into executable is linking emitted object with the standard library object file. If the standard library object file is not found, the error will be thrown.

</details>

---

### Solution

```sh
wind raw_std/stack.w raw_std/start.w -ej -o raw_std/wind_lib.o
```
