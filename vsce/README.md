# Emerald Language Support

Emerald（绿宝石）语言的 VS Code 官方扩展。

## 功能

- ✅ 语法高亮（`.eml` / `.emr` / `.elr`）
- ✅ 括号配对、自动闭合、代码折叠
- ✅ 代码片段（`eml`、`hello`、`print`、`listadd`、`func` 等）
- ✅ 悬停显示函数签名与变量类型
- ✅ 自动补全：关键字、内建函数、变量、函数名
- ✅ 实时语法诊断（括号匹配、变量未声明、缺少分号、缺少入口点）
- ✅ 命令：
    - `Emerald: 编译当前文件` → `eml -c file.eml`
    - `Emerald: 运行当前文件` → `eml run file`
    - `Emerald: 打包为 EXE` → `eml wrapper file.eml`
    - `Emerald: 新建项目` → 生成 `src.eml`、`wrapper.ewd`、`.vscode/tasks.json`
    - `Emerald: 生成 wrapper.ewd 模板`

## 配置项

| 配置 | 默认 | 说明 |
|---|---|---|
| `emerald.compilerPath` | `eml` | 编译器路径 |
| `emerald.autoCompileOnRun` | `true` | 运行前自动编译 |
| `emerald.runInTerminal` | `true` | 在集成终端运行 |
| `emerald.lint.enable` | `true` | 启用实时诊断 |
| `emerald.lint.declareBeforeUse` | `true` | 使用前必须声明 |

## 快捷操作

- 编辑器标题栏右上角 ▶ 按钮 = 运行
- 底部状态栏 `$(play) Emerald 运行`
- 右键菜单：编译 / 运行 / 打包

## 系统要求

- VS Code ≥ 1.75
- 已在 `PATH` 中安装 `eml`（或在设置里指定 `emerald.compilerPath`）