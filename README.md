# Async++ GRPC library
This library provides a c++20 coroutine wrapper around grpc generated functions.
It is an addition to [async++](https://github.com/asyncpp/asyncpp) which provides general coroutine tasks and support classes.

Tested and supported compilers:
| Linux                                                                 |
|-----------------------------------------------------------------------|
| [![ubuntu-2004_clang-10][img_ubuntu-2004_clang-10]][Compiler-Support] | 
| [![ubuntu-2004_clang-11][img_ubuntu-2004_clang-11]][Compiler-Support] |
| [![ubuntu-2004_clang-12][img_ubuntu-2004_clang-12]][Compiler-Support] |
| [![ubuntu-2004_gcc-10][img_ubuntu-2004_gcc-10]][Compiler-Support]     |
| [![ubuntu-2204_clang-12][img_ubuntu-2204_clang-12]][Compiler-Support] |
| [![ubuntu-2204_clang-13][img_ubuntu-2204_clang-13]][Compiler-Support] |
| [![ubuntu-2204_clang-14][img_ubuntu-2204_clang-14]][Compiler-Support] |
| [![ubuntu-2204_gcc-10][img_ubuntu-2204_gcc-10]][Compiler-Support]     |
| [![ubuntu-2204_gcc-11][img_ubuntu-2204_gcc-11]][Compiler-Support]     |

[img_ubuntu-2004_clang-10]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2004_clang-10/shields.json
[img_ubuntu-2004_clang-11]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2004_clang-11/shields.json
[img_ubuntu-2004_clang-12]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2004_clang-12/shields.json
[img_ubuntu-2004_gcc-10]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2004_gcc-10/shields.json
[img_ubuntu-2204_clang-12]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_clang-12/shields.json
[img_ubuntu-2204_clang-13]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_clang-13/shields.json
[img_ubuntu-2204_clang-14]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_clang-14/shields.json
[img_ubuntu-2204_gcc-10]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_gcc-10/shields.json
[img_ubuntu-2204_gcc-11]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_gcc-11/shields.json
[Compiler-Support]: https://github.com/asyncpp/asyncpp-curl/actions/workflows/compiler-support.yml

### Provided classes
* `call` is a wrapper around a client side grpc stub
* `task` allows for using coroutines to implement a grpc server side method
