# Linux user space memory maps

| Architecture |              Start | End                | Address space bits | Page size | Translation tables |
| :----------: | -----------------: | :----------------- | :----------------: | :-------: | :----------------: |
| `x86_64`     | `0000000000000000` | `00007fffffffffff` |         47         |    4 KB   |         4          |
| `x86_64`     | `0000000000000000` | `00ffffffffffffff` |         56         |    4 KB   |         5          |
| `arm64`      | `0000000000000000` | `0000007fffffffff` |         39         |    4 KB   |         3          |
| `arm64`      | `0000000000000000` | `0000ffffffffffff` |         48         |    4 KB   |         4          |
| `arm64`      | `0000000000000000` | `000003ffffffffff` |         42         |   64 KB   |         2          |
| `arm64`      | `0000000000000000` | `000fffffffffffff` |         52         |   64 KB   |         3          |

# References

 - [`x86_64`][x86_64]
 - [`arm64`][arm64]

[x86_64]: https://www.kernel.org/doc/html/latest/arch/x86/x86_64/mm.html
[arm64]: https://www.kernel.org/doc/html/latest/arch/arm64/memory.html
