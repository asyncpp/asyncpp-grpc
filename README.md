# Async++ GRPC library
This library provides a c++20 coroutine wrapper around grpc generated functions.
It is an addition to [async++](https://github.com/Thalhammer/asyncpp) which provides general coroutine tasks and support classes.

Tested and supported compilers:
* Clang 12
* Clang 14

### Provided classes
* `call` is a wrapper around a client side grpc stub
* `task` allows for using coroutines to implement a grpc server side method