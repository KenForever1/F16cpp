## 简介

F16.hpp实现了IEEE754-2008标准的Float16类型表示, 以及实现了Float32和Float16类型的互相转换。F16类型的直接比较大小方法。

主要函数：
+ f32_to_f16_fallback：将F16类型转换成F32类型
+ f16_to_f32_fallback：将F32类型转换成F16类型
+ 重载了比较大小的operator函数

F16cpp inspired by rust [half](https://docs.rs/half/latest/src/half/binary16/arch.rs.html) crate.
## 使用
```c++
#include "F16.hpp"

int main(){
    F16 a;
    a.from_f32(0);

    F16 b;
    b.from_f32(-0);
    std::cout << "a = " << a.to_f32() << "\n";
    std::cout << "b = " << b.to_f32() << "\n";

    std::cout << "a == b: " << (a == b) << "\n";
    std::cout << "a < b: " << (a < b) << "\n";
    std::cout << "a > b: " << (a > b) << "\n";
}
```