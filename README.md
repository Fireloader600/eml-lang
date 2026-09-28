# Emerald（绿宝石）语言教程

> 一门简洁、易学、可编译成独立 EXE 的编程语言。  
> 文件后缀：`.eml`  
> 编译产物：`.elr`  
> 项目文件：`.flp`

---

## 目录

1. [语言简介](#1-语言简介)
2. [安装](#2-安装)
3. [第一个程序：Hello World](#3-第一个程序hello-world)
4. [变量](#4-变量)
5. [输入与输出](#5-输入与输出)
6. [列表（数组）](#6-列表数组)
7. [讲述人](#7-讲述人)
8. [函数](#8-函数)
9. [子命令](#9-子命令)
10. [注释](#10-注释)
11. [编译与运行](#11-编译与运行)
12. [打包为独立 EXE](#12-打包为独立-exe)
13. [Emerald IDE](#13-emerald-ide)
14. [安装程序](#14-安装程序)
15. [语法速查表](#15-语法速查表)
16. [常见问题](#16-常见问题)

---

## 1. 语言简介

Emerald 是一门以**可读性**和**零配置**为目标的编程语言。它的设计目标：

- **语法接近自然语言**——看着像英文，写着像伪代码
- **一个文件跑一个程序**——`eml()` 是程序入口
- **能直接编译成独立 exe**——不依赖运行环境
- **自带 IDE**——写代码、编译、运行一站式

一个最小的 Emerald 程序长这样：

```eml
import maineml;

eml(){
    print("HelloWorld!");
    newline();
    return 0;
}
```

---

## 2. 安装

### 方式一：一键安装包（推荐）

1. 下载 `EmeraldSetup.exe`
2. 双击运行，若弹出 UAC 提示请点"是"
3. 安装向导会询问：
   - **安装位置**：默认 `C:\Program Files\Emerald`
   - **是否加入系统 PATH**：建议勾选
   - **是否创建桌面快捷方式**：可选
4. 点击"安装"，几秒后完成

安装完成后会包含：

| 文件 | 说明 |
|---|---|
| `eml.exe` | 编译器 / 解释器 |
| `EmeraldIDE.exe` | 集成开发环境 |
| `uninstall.exe` | 卸载程序 |

### 方式二：手动配置

把 `eml.exe` 放到任意目录，然后把该目录加入系统 PATH：

1. `Win + R` → `sysdm.cpl` → 高级 → 环境变量
2. 在"系统变量"里找到 `Path`，编辑
3. 新增一行 `D:\your\path\to\eml`
4. 确定，重启终端

验证：

```cmd
eml --help
```

看到用法说明即安装成功。

### 方式三：从源码编译

```bash
gcc eml.c -o eml.exe -std=c99 -lshell32
```

把 `eml.exe` 放到 PATH 里的任意目录即可。

---

## 3. 第一个程序：Hello World

新建文件 `hello.eml`：

```eml
import maineml;

eml(){
    print("HelloWorld!");
    newline();
    return 0;
}
```

逐行解释：

| 行 | 含义 |
|---|---|
| `import maineml;` | 导入主模块（每个文件都写） |
| `eml(){ ... }` | 程序入口函数，运行时自动调用 |
| `print("HelloWorld!");` | 输出字符串，**不换行** |
| `newline();` | 输出一个换行符 |
| `return 0;` | 返回 0（惯例，表示正常退出） |

编译并运行：

```cmd
eml -c hello.eml
eml run hello
```

输出：

```
HelloWorld!
```

---

## 4. 变量

Emerald 有三种基本类型：

| 类型 | 关键字 | 说明 |
|---|---|---|
| 整数 | `int` | 如 `1`、`100`、`-5` |
| 字符串 | `string` | 用双引号括起来 |
| 布尔值 | `bool` | `true` 或 `false` |

### 声明变量

```eml
import maineml;

eml(){
    int a;
    string b;
    bool c;
    return 0;
}
```

### 声明并赋值

```eml
eml(){
    int score = 100;
    string name = "绿宝石";
    bool win = true;

    print(name);
    newline();
    print(score);
    newline();
    print(win);
    newline();
    return 0;
}
```

输出：

```
绿宝石
100
true
```

### 先声明后赋值

```eml
eml(){
    int a;
    a = 42;
    print(a);
    newline();
    return 0;
}
```

---

## 5. 输入与输出

### print 与 newline

`print` 输出**不换行**，`newline` 换行。两者配合可以灵活排版：

```eml
eml(){
    print("姓名：");
    print("小明");
    newline();

    print("年龄：");
    print(18);
    newline();
    return 0;
}
```

输出：

```
姓名：小明
年龄：18
```

### input 读取输入

```eml
eml(){
    string name;

    print("请输入你的名字：");
    input(name);

    print("你好，");
    print(name);
    newline();
    return 0;
}
```

运行效果：

```
请输入你的名字：小明
你好，小明
```

> 提示：`input` 会把**整行**读进变量，包括空格。

---

## 6. 列表（数组）

Emerald 的列表用 `format` 初始化，用 `list` 操作。

### 初始化

```eml
eml(){
    int a;
    format(a, list);   // 把 a 格式化成数组，内容是 []
    return 0;
}
```

### 添加元素

```eml
eml(){
    int a;
    format(a, list);

    list a(add, "苹果");
    list a(add, "香蕉");
    list a(add, "橙子");

    print(a);      // 输出 [苹果, 香蕉, 橙子]
    newline();
    return 0;
}
```

### 删除元素

按**序号**删除（从 1 开始计数）：

```eml
list a(del, 2);    // 删除第 2 项
```

### 清空列表

```eml
list a(clean);
```

### 完整示例

```eml
import maineml;

eml(){
    int items;
    format(items, list);

    list items(add, "剑");
    list items(add, "盾");
    list items(add, "药水");

    print("背包：");
    print(items);
    newline();

    list items(del, 3);
    print("丢掉药水后：");
    print(items);
    newline();

    list items(clean);
    print("清空后：");
    print(items);
    newline();

    return 0;
}
```

输出：

```
背包：[剑, 盾, 药水]
丢掉药水后：[剑, 盾]
清空后：[]
```

---

## 7. 讲述人

`speak` 用于输出"讲述人"风格的文本，常用于游戏旁白、故事讲解。

```eml
import maineml;
import narrator;

eml(){
    speak("HelloWorld!");
    speak("欢迎使用 Emerald 语言");
    return 0;
}
```

输出：

```
[讲述人] HelloWorld!
[讲述人] 欢迎使用 Emerald 语言
```

> 使用 `speak` 需要 `import narrator;`。

---

## 8. 函数

### 无返回值函数

```eml
eml(){
    function greet(){
        print("你好！");
        newline();
    }

    greet();     // 调用
    return 0;
}
```

### 带返回值的函数

在函数名前写返回类型：

```eml
eml(){
    int getNumber(){
        return 42;
    }

    bool isReady(){
        return true;
    }

    int n;
    n = getNumber();
    print(n);          // 42
    newline();

    bool ok;
    ok = isReady();
    print(ok);         // true
    newline();

    return 0;
}
```

### 完整示例

```eml
import maineml;

eml(){
    function showTitle(){
        print(">>> Emerald RPG <<<");
        newline();
    }

    int getMaxScore(){
        return 9999;
    }

    showTitle();

    int maxScore;
    maxScore = getMaxScore();
    print("最高分：");
    print(maxScore);
    newline();
    return 0;
}
```

---

## 9. 子命令

`subcommand` 用于根据不同的子命令走不同的分支，类似命令行参数分发。

```eml
import maineml;
import subcommand;

eml(){
    subcommand("1"){
        print("Hello!");
        newline();
    }

    subcommand("2"){
        print("World!");
        newline();
    }

    subcommand(other){
        print("HelloWorld!");
        newline();
    }

    return 0;
}
```

| 分支 | 触发条件 |
|---|---|
| `subcommand("1")` | 子命令是 `1` |
| `subcommand("2")` | 子命令是 `2` |
| `subcommand(other)` | 其他所有情况 |

> 当前解释器默认走 `other` 分支，输出 `HelloWorld!`。

---

## 10. 注释

单行注释：

```eml
// 这是注释，不会被执行
int a;   // 也可以写在语句后面
```

块注释：

```eml
/*
   这是块注释
   可以跨多行
*/
```

---

## 11. 编译与运行

Emerald 有三种运行方式：

### 11.1 编译为字节码

```cmd
eml -c hello.eml
```

会在同目录生成 `hello.elr`。

### 11.2 运行

```cmd
eml run hello
```

> 运行时**不用写扩展名**。解释器会自动寻找 `hello.elr` 或 `hello.eml`。

也可以直接跑源码：

```cmd
eml hello.eml
```

### 11.3 打包为独立 EXE

```cmd
eml wrapper hello.eml
```

会在同目录生成 `hello.exe`——**独立可执行文件**，不需要 `eml.exe` 在旁边，也不需要 `.elr` 文件。

双击 `hello.exe` 即可运行。

### 11.4 自定义打包属性（可选）

新建 `wrapper.ewd`：

```
pack_name="mygame";
pack_version="2.0.0";
icon="logo.png"
```

字段说明：

| 字段 | 说明 |
|---|---|
| `pack_name` | 生成的 exe 名字，默认取 `.eml` 文件名 |
| `pack_version` | 版本号（仅作元数据） |
| `icon` | 图标路径（当前只读作元数据） |

再次运行：

```cmd
eml wrapper hello.eml
```

会生成 `mygame.exe`（而不是 `hello.exe`）。

### 11.5 启动 IDE

```cmd
eml ide
```

会自动查找并启动 Emerald IDE。

---

## 12. 打包为独立 EXE

Emerald 采用**自解压式打包**：把解释器本身 + 你的源码 + 一个 footer 拼在一起，形成一个新 exe。

打包后的 exe 结构：

```
[ eml.exe 的完整二进制 ]
[ 你的 .eml 源码      ]
[ "EMLWRAP1" ][ 长度 ][ "EMLWRAP1" ]
```

运行时，解释器会检查自身末尾是否带有 footer：

- **有** → 直接执行嵌入的源码
- **没有** → 走正常命令行逻辑

所以打包后的 exe **完全独立**，可以在任何 Windows 机器上双击运行。

### 打包大小

`exe 大小 ≈ eml.exe 大小 + 源码大小 + 20 字节`

通常一个打包后的程序在 100–200 KB 左右。

---

## 13. Emerald IDE

Emerald IDE 是 Emerald 语言的**专属集成开发环境**，用 Node.js + Electron 编写。

### 13.1 启动

```cmd
eml ide
```

或直接双击 `EmeraldIDE.exe`。

### 13.2 新建项目

1. 菜单 **文件 → 新建 → 项目**
2. 输入项目名称，点"确定"
3. 选择 `<name>.flp` 的保存位置
4. 该 `.flp` 文件所在目录成为**项目根目录**

IDE 会自动创建：

| 文件 | 说明 |
|---|---|
| `<name>.flp` | 项目文件 |
| `src.eml` | 入口源码（Hello 模板） |
| `wrapper.ewd` | 打包配置 |

`.flp` 文件内容：

```
ide.project(){
    name("ProjectName");
}
```

### 13.3 导入项目

菜单 **文件 → 导入项目**，选择已有的 `.flp` 文件即可。

### 13.4 编辑代码

右侧是代码编辑区，支持：

- 语法高亮（关键字、类型、字符串、注释、函数名……）
- 括号自动配对
- 自动缩进
- 多标签页
- `Ctrl+F` 搜索（高亮所有匹配，↑/↓ 跳转）

### 13.5 构建按钮

工具栏上有四个按钮：

| 按钮 | 命令 | 说明 |
|---|---|---|
| 编译 | `eml -c file.eml` | 生成 `.elr` |
| 运行 | `eml run file` | 运行编译产物或源码 |
| 编译并运行 | 先编译再运行 | 一步到位 |
| 重新编译 | 删除旧 `.elr` 后编译 | 清理后重建 |
| 打包 | `eml wrapper file.eml` | 生成独立 exe |

所有命令都通过**真实终端**执行（`node-pty` + `xterm.js`），输出会流式显示。

### 13.6 终端

底部是一个真正的终端：

- 能跑任何 shell 命令（`dir`、`type`、`cd`……）
- 支持 eml 程序的 `input()`——程序运行时能真正等你输入
- 支持 `Ctrl+C` 中断
- 命令历史（↑/↓）

### 13.7 快捷键

| 快捷键 | 功能 |
|---|---|
| `Ctrl+N` | 新建文件 |
| `Ctrl+Shift+N` | 新建项目 |
| `Ctrl+O` | 导入项目 |
| `Ctrl+S` | 保存 |
| `Ctrl+F` | 搜索 |
| `F5` | 运行 |
| `F7` | 编译 |
| `Ctrl+F5` | 编译并运行 |
| `Ctrl+Shift+F7` | 重新编译 |
| `Ctrl+\`` | 切换终端 |

---

## 14. 安装程序

Emerald 自带一个 C 语言写的图形安装程序。

### 14.1 安装

双击 `EmeraldSetup.exe`：

1. UAC 提示 → 点"是"
2. 图形界面出现：
   - **安装位置**：默认 `C:\Program Files\Emerald`
   - ☑ **将 Emerald 加入系统 PATH**
   - ☐ **创建桌面快捷方式**
3. 点"安装"
4. 几秒后提示"安装完成"，可选择立即启动 IDE

安装程序会自动：

- 复制 `eml.exe` 和 `EmeraldIDE.exe` 到安装目录
- 复制自身为 `uninstall.exe`
- 把安装目录加入系统 PATH 并广播 `WM_SETTINGCHANGE`
- 创建开始菜单项（IDE、命令行、卸载）
- 写注册表卸载信息
- 可选创建桌面快捷方式

### 14.2 卸载

三种方式：

1. **开始菜单** → Emerald → 卸载 Emerald
2. **控制面板** → 程序和功能 → Emerald → 卸载
3. **手动**：

   ```cmd
   "C:\Program Files\Emerald\uninstall.exe" --uninstall
   ```

卸载时会：

- 确认对话框
- 从系统 PATH 移除该目录
- 删除开始菜单、桌面快捷方式
- 删除安装目录里的文件
- 删除注册表项
- 标记 `uninstall.exe` 自身在下次重启时删除

---

## 15. 语法速查表

| 功能 | 语法 |
|---|---|
| 导入模块 | `import maineml;` |
| 入口函数 | `eml(){ ... }` |
| 整数变量 | `int a;` |
| 字符串变量 | `string b;` |
| 布尔变量 | `bool c;` |
| 赋值 | `a = 10;` |
| 打印 | `print(值);` |
| 换行 | `newline();` |
| 输入 | `input(a);` |
| 讲述人 | `speak("内容");` |
| 数组初始化 | `format(a, list);` |
| 数组添加 | `list a(add, 值);` |
| 数组删除 | `list a(del, 序号);` |
| 数组清空 | `list a(clean);` |
| 无返回值函数 | `function f(){ ... }` |
| 整数函数 | `int f(){ return 0; }` |
| 布尔函数 | `bool f(){ return true; }` |
| 子命令 | `subcommand("1"){ ... }` |
| 默认子命令 | `subcommand(other){ ... }` |
| 返回 | `return 值;` |
| 单行注释 | `// 内容` |
| 块注释 | `/* 内容 */` |

### 命令行速查

| 命令 | 说明 |
|---|---|
| `eml --help` | 显示帮助 |
| `eml -c <file.eml>` | 编译为 `.elr` |
| `eml run <file>` | 运行（不用写扩展名） |
| `eml wrapper <file.eml>` | 打包为独立 exe |
| `eml ide` | 启动 Emerald IDE |
| `eml <file>` | 直接运行该文件 |

---

## 16. 常见问题

### Q1: `eml` 不是内部或外部命令

PATH 没配好。检查：

1. `eml.exe` 所在目录是否加入了 PATH
2. 修改 PATH 后是否重开了终端

### Q2: 编译报错 `Cannot open xxx.eml`

路径写错了，或者当前工作目录不对。可以先 `cd` 到文件所在目录再运行：

```cmd
cd /d D:\projects\mygame
eml -c src.eml
```

### Q3: 运行没有输出

检查以下几点：

1. 语句结尾是否有分号 `;`
2. `eml()` 函数是否用全角大括号 `｛｝`——建议统一用半角 `{}`
3. 是否在 `eml()` 外面写了 `print`（不会执行）

### Q4: `input()` 直接跳过，没等我输入

说明在非交互环境（比如 IDE 的终端被收起、输出被重定向）运行。打开终端面板即可正常输入。

### Q5: 打包后 exe 打不开

确认打包前 `eml.exe` 自身可正常运行。打包时 `eml.exe` 的位置不对，或 `wrapper.ewd` 里 `pack_name` 写错了。删除 `wrapper.ewd` 后重试：

```cmd
eml wrapper hello.eml
```

### Q6: IDE 提示"找不到 EmeraldIDE.exe"

`eml ide` 会在以下位置找：

1. `eml.exe` 同目录
2. `..\emerald-ide-dist\win-unpacked\`
3. `.\emerald-ide-dist\win-unpacked\`
4. `..\eml-ide-dist\win-unpacked\`
5. `..\eml-ide\dist\win-unpacked\`
6. `..\emerald-ide\dist\win-unpacked\`

把 `EmeraldIDE.exe` 放到其中之一，或用安装程序安装到 `C:\Program Files\Emerald\`（IDE 和 eml 同级）。

### Q7: 中文乱码

保存 `.eml` 文件时，编码选 **UTF-8**（不要带 BOM）。Windows 终端可能需要先执行：

```cmd
chcp 65001
```

---

## 示例项目

一个完整的 RPG 小示例 `demo.eml`：

```eml
import maineml;
import narrator;
import subcommand;

eml(){
    // ===== 输出 =====
    print("===== Emerald 综合示例 =====");
    newline();

    // ===== 变量 =====
    int score = 100;
    string player = "绿宝石玩家";
    bool win = true;

    print("玩家：");   print(player);   newline();
    print("分数：");   print(score);    newline();
    print("通关：");   print(win);      newline();

    // ===== 列表 =====
    int items;
    format(items, list);
    list items(add, "剑");
    list items(add, "盾");
    list items(add, "药水");

    print("背包：");   print(items);    newline();

    list items(del, 3);
    print("丢掉药水后：");
    print(items);      newline();

    // ===== 函数 =====
    function showTitle(){
        print(">>> Emerald RPG <<<");
        newline();
    }

    int getMaxScore(){
        return 9999;
    }

    showTitle();

    int maxScore;
    maxScore = getMaxScore();
    print("最高分：");  print(maxScore); newline();

    // ===== 讲述人 =====
    speak("游戏结束，感谢游玩！");

    // ===== 子命令 =====
    subcommand(other){
        print("没有子命令，显示默认信息。");
        newline();
    }

    return 0;
}
```

编译运行：

```cmd
eml -c demo.eml
eml run demo
```

输出：

```
===== Emerald 综合示例 =====
玩家：绿宝石玩家
分数：100
通关：true
背包：[剑, 盾, 药水]
丢掉药水后：[剑, 盾]
>>> Emerald RPG <<<
最高分：9999
[讲述人] 游戏结束，感谢游玩！
没有子命令，显示默认信息。
```

---

## 结语

Emerald 语言目前是一个**轻量、可扩展**的原型，覆盖了：

- ✅ 变量、列表、输入输出
- ✅ 函数、返回值、子命令
- ✅ 编译、运行、打包
- ✅ 图形 IDE、安装程序

它适合：

- 学编程的入门语言
- 写小脚本、小游戏
- 学习编译器 / 解释器原理

祝你玩得开心。有问题随时反馈。

---

**Emerald（绿宝石）语言 v1.0.0**  
MIT License