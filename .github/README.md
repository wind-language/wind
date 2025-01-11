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
    Example code:
  </summary>

```rs
global maxRes:int64=-1;
func occrec(N: int, last: int64, M: int64, i: int): void {
    branch[
        N==i: return;
    ]
    var digit: int=3;
    loop[digit<=9] {
        branch[
            digit == (last%10): {}
            else : {
                var newLast: int64 = (last*10)+digit;
                var modc: int64 = newLast%M;
                branch [
                    modc > maxRes: {
                        maxRes = modc;
                    }
                ]
                occrec(N, newLast, M, i+1);
            }
        ]
        digit+=3;
    }
}
```
<sub>

**Problem from Training Olinfo: _[ois_cabala](https://training.olinfo.it/task/ois_cabala/)_**

</sub>

</details>

</td>
</tr>
</table>
<div align="center">
  <h3>Try benchmarking it!</h3>
</div>

```sh
python testing/benchmarks/run_bench.py cabala
```

<div align="center">

## [Documentation](https://utcq.github.io/wind/)

</div>