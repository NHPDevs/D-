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
program HelloWorld;
  output "Hello, D++ World!" endline;
endprogram;
```

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
```d++
program LoopExample;
  var counter: integer = 0;
  
  loop counter from 0 to 10;
    if counter % 2 == 0;
      output "Even: " counter endline;
    else;
      output "Odd: " counter endline;
    endif;
  endloop;
endprogram;
```

### Functions & Pointers
```d++
program PointerExample;
  function add(a: integer, b: integer) returns integer;
    return a + b;
  endfunction;
  
  var result: integer = add(5, 3);
  var ptr: pointer = &result;
  
  output "Result: " result endline;
  output "Memory Address: " ptr endline;
endprogram;
```

### Memory & Hardware Access
```d++
program MemoryExample;
  var data: integer = 100;
  var address: pointer = &data;
  
  output "Value: " data endline;
  output "Address: " address endline;
  memory_write(address, 200);
  output "New Value: " memory_read(address) endline;
endprogram;
```

### Calculator Operations
```d++
program Calculator;
  function calculate(x: integer, y: integer, op: string) returns integer;
    if op == "+";
      return x + y;
    elif op == "-";
      return x - y;
    elif op == "*";
      return x * y;
    elif op == "/";
      return x / y;
    endif;
  endfunction;
  
  var result: integer = calculate(10, 5, "*");
  output "10 * 5 = " result endline;
endprogram;
```

---

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