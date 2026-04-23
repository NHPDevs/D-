# 🚀 D++ Programming Language

![D++ Language Icon](./docs/icon.png)

## What is D++?

**D++** is a powerful, high-performance programming language built with **C**, **Assembly**, and **Machine Code**. Designed for speed and efficiency, D++ is optimized for hardware and OS development, server-side applications, and systems programming.

### Key Features
- ⚡ **Ultra-Fast**: Runs efficiently on servers and hardware
- 🔧 **Hardware Optimized**: Perfect for OS and embedded systems development
- 📦 **Built-in Functionality**: 
  - Mathematics & Variables
  - Arrays & Data Types
  - Loop Control & Conditionals
  - Pointers & Memory Addressing
  - Advanced Calculator Operations
  - Full Memory Management System

---

## 📥 Installation & Download

### Download the D++ Compiler

**Latest Version**: v1.0.0

- **Windows**: [Download Compiler (Windows)](https://github.com/NHPDevs/D-/releases/download/v1.0.0/D-compiler-windows.exe)
- **Linux**: [Download Compiler (Linux)](https://github.com/NHPDevs/D-/releases/download/v1.0.0/D-compiler-linux.tar.gz)
- **macOS**: [Download Compiler (macOS)](https://github.com/NHPDevs/D-/releases/download/v1.0.0/D-compiler-macos.dmg)

### Quick Install
```bash
# Extract the compiler
tar -xzf D-compiler-linux.tar.gz

# Add to PATH
export PATH=$PATH:$(pwd)/D-compiler/bin

# Verify installation
d++ --version
```

---

## 💻 D++ Programming Syntax

### Hello World
```d++
#include <ios++>

int main(){

printel("hello! \n");
return 0;
}

### Variables & Data Types
```d++
program VarExample;
  var x: integer = 10;
  var y: float = 3.14;
  var name: string = "D++ Language";
  
  output "Integer: " x endline;
  output "Float: " y endline;
  output "String: " name endline;
endprogram;
```

### Arrays
```d++
program ArrayExample;
  array numbers: integer[5] = [1, 2, 3, 4, 5];
  
  loop i from 0 to 4;
    output numbers[i] endline;
  endloop;
endprogram;
```

### Loops & Conditionals
#include <ios++>
#include <std::>

int main(){

for mp*)) "nl//"
printel("%+*));

for ((%d+%d = %d));
printel(" I++", plus);
>>(int i ≥ any number) 
return 0;
;
### Functions & Datatypes
Void
Goto
Int
Float
long
long double
double
very long double
char
Void{starter(Bundle)){
RET

### Memory & Hardware Access

we will make soon

### Calculator Operations

#include <ios++>

int main() {

int num;
int num1;

printel("enter num1: \n") ;
scanel("%d", &num);

printel("enter num2: \n");
scanel("%d", &num1);

printel("Answer is: %d\n", num + num1) 
return 0;
}

## 🎯 Getting Started

1. **Download** the compiler from the links above
2. **Install** by extracting and adding to your system PATH
3. **Create** a new `.d++` file with your code
4. **Compile** using: `d++ compile yourfile.d++`
5. **Run** using: `./yourfile`

---

## 📋 D++ Language Reference

| Feature | Example |
|---------|---------|
| Output | `output "text" endline;` |
| Variables | `var name: type = value;` |
| Arrays | `array arr: type[size] = [...]` |
| Loops | `loop var from start to end;` |
| Conditionals | `if condition; ... endif;` |
| Functions | `function name(...) returns type;` |
| Pointers | `var ptr: pointer = &variable;` |
| Memory | `memory_read/write()` |

---

## 🛠️ Use Cases

- **Server Development** - Fast, efficient backend systems
- **Operating Systems** - Low-level hardware control
- **Embedded Systems** - Optimized for resource-constrained environments
- **System Programming** - Direct hardware access and memory management
- **Real-time Applications** - High-performance computing

---

## 📄 License

This project is licensed under the **MIT License** - see the LICENSE file for details.

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bug reports and feature requests.

---

## 📞 Support

For questions, issues, or feedback, please visit our [GitHub Issues](https://github.com/NHPDevs/D-/issues)

---

**Made with ❤️ by NHPDevs | D++ Programming Language**
