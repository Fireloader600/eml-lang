// Emerald VS Code 扩展主入口
// 提供：编译、运行、打包、诊断、悬停、补全、命令注册

const vscode = require('vscode');
const path = require('path');
const fs = require('fs');
const cp = require('child_process');

/** @type {vscode.DiagnosticCollection} */
let diagnostics;

/** @type {vscode.OutputChannel} */
let output;

/** 内建函数与签名（用于悬停/补全） */
const BUILTIN_FUNCTIONS = {
    print:    { sig: 'print(value)',        doc: '不换行输出内容。' },
    newline:  { sig: 'newline()',           doc: '输出一个换行符。' },
    input:    { sig: 'input(var)',          doc: '读取一行输入到变量。' },
    speak:    { sig: 'speak(text)',         doc: '由讲述人朗读/输出。需要 `import narrator;`' },
    format:   { sig: 'format(var, list)',   doc: '把变量格式化为数组 `[]`。' },
    list:     { sig: 'list a(add, v) | list a(del, n) | list a(clean)', doc: '数组操作。' },
    subcommand: { sig: 'subcommand("1") { ... }', doc: '定义子命令分支。' }
};

const KEYWORDS = [
    'import', 'eml', 'return', 'int', 'string', 'bool',
    'function', 'list', 'format', 'true', 'false',
    'if', 'else', 'while', 'for', 'break', 'continue',
    'print', 'newline', 'input', 'speak', 'subcommand', 'other'
];

const MODULES = ['maineml', 'narrator', 'subcommand'];

/* ============================================================
 * 激活
 * ============================================================ */
function activate(context) {
    output = vscode.window.createOutputChannel('Emerald');
    diagnostics = vscode.languages.createDiagnosticCollection('emerald');
    context.subscriptions.push(output, diagnostics);

    // 命令
    context.subscriptions.push(
        vscode.commands.registerCommand('emerald.compile', cmdCompile),
        vscode.commands.registerCommand('emerald.run', cmdRun),
        vscode.commands.registerCommand('emerald.wrapper', cmdWrapper),
        vscode.commands.registerCommand('emerald.newProject', cmdNewProject),
        vscode.commands.registerCommand('emerald.generateWrapperConfig', cmdGenerateWrapperConfig)
    );

    // 诊断：打开、保存、修改时触发
    context.subscriptions.push(
        vscode.workspace.onDidOpenTextDocument(doc => lintDocument(doc)),
        vscode.workspace.onDidSaveTextDocument(doc => lintDocument(doc)),
        vscode.workspace.onDidChangeTextDocument(ev => {
            if (ev.document.languageId === 'emerald') lintDocument(ev.document);
        }),
        vscode.workspace.onDidCloseTextDocument(doc => diagnostics.delete(doc.uri))
    );

    // 首次打开时检查已打开文档
    vscode.workspace.textDocuments.forEach(doc => {
        if (doc.languageId === 'emerald') lintDocument(doc);
    });

    // 悬停
    context.subscriptions.push(
        vscode.languages.registerHoverProvider('emerald', new EmeraldHoverProvider())
    );

    // 补全
    context.subscriptions.push(
        vscode.languages.registerCompletionItemProvider(
            'emerald',
            new EmeraldCompletionProvider(),
            '.', '"'
        )
    );

    // 状态栏快捷按钮
    const statusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 100);
    statusItem.text = '$(play) Emerald 运行';
    statusItem.tooltip = '运行当前 Emerald 文件';
    statusItem.command = 'emerald.run';
    context.subscriptions.push(statusItem);

    // 只在 emerald 语言下显示
    const updateStatus = editor => {
        if (editor && editor.document.languageId === 'emerald') statusItem.show();
        else statusItem.hide();
    };
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(updateStatus));
    updateStatus(vscode.window.activeTextEditor);
}

function deactivate() {
    if (diagnostics) diagnostics.dispose();
}

/* ============================================================
 * 编译 / 运行 / 打包
 * ============================================================ */
function getCompilerPath() {
    const cfg = vscode.workspace.getConfiguration('emerald');
    return cfg.get('compilerPath') || 'eml';
}

function getActiveEmlFile() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('请先打开一个 Emerald 文件。');
        return null;
    }
    const doc = editor.document;
    if (doc.languageId !== 'emerald' && !/\.(eml|emr|elr)$/i.test(doc.fileName)) {
        vscode.window.showErrorMessage('当前文件不是 Emerald 文件（.eml/.emr/.elr）。');
        return null;
    }
    return doc;
}

async function cmdCompile() {
    const doc = getActiveEmlFile();
    if (!doc) return;
    if (doc.isDirty) await doc.save();

    const compiler = getCompilerPath();
    const file = doc.uri.fsPath;

    output.show(true);
    output.appendLine(`[compile] ${compiler} -c "${file}"`);

    cp.execFile(compiler, ['-c', file], { cwd: path.dirname(file) }, (err, stdout, stderr) => {
        if (stdout) output.appendLine(stdout.trimEnd());
        if (stderr) output.appendLine(stderr.trimEnd());
        if (err) {
            vscode.window.showErrorMessage(`Emerald 编译失败：${err.message}`);
        } else {
            const elr = file.replace(/\.(eml|emr)$/i, '.elr');
            vscode.window.showInformationMessage(`编译成功 → ${path.basename(elr)}`);
        }
    });
}

async function cmdRun() {
    const doc = getActiveEmlFile();
    if (!doc) return;
    if (doc.isDirty) await doc.save();

    const cfg = vscode.workspace.getConfiguration('emerald');
    const autoCompile = cfg.get('autoCompileOnRun');
    const inTerminal = cfg.get('runInTerminal');
    const compiler = getCompilerPath();
    const file = doc.uri.fsPath;
    const base = file.replace(/\.(eml|emr|elr)$/i, '');

    if (autoCompile && /\.(eml|emr)$/i.test(file)) {
        await new Promise(resolve => {
            cp.execFile(compiler, ['-c', file], { cwd: path.dirname(file) }, (err, stdout, stderr) => {
                if (stdout) output.appendLine(stdout.trimEnd());
                if (stderr) output.appendLine(stderr.trimEnd());
                if (err) {
                    vscode.window.showErrorMessage(`编译失败：${err.message}`);
                    resolve(false);
                } else {
                    resolve(true);
                }
            });
        });
    }

    const cmdLine = `${escapeShell(compiler)} run ${escapeShell(base)}`;

    if (inTerminal) {
        let term = vscode.window.terminals.find(t => t.name === 'Emerald');
        if (!term) {
            term = vscode.window.createTerminal('Emerald');
        }
        term.show(true);
        term.sendText(cmdLine, true);
    } else {
        output.show(true);
        output.appendLine(`[run] ${cmdLine}`);
        cp.exec(cmdLine, { cwd: path.dirname(file) }, (err, stdout, stderr) => {
            if (stdout) output.appendLine(stdout.trimEnd());
            if (stderr) output.appendLine(stderr.trimEnd());
            if (err) vscode.window.showErrorMessage(`运行失败：${err.message}`);
        });
    }
}

async function cmdWrapper() {
    const doc = getActiveEmlFile();
    if (!doc) return;
    if (doc.isDirty) await doc.save();

    // 先编译
    const compiler = getCompilerPath();
    const file = doc.uri.fsPath;
    output.show(true);

    cp.execFile(compiler, ['-c', file], { cwd: path.dirname(file) }, (err, stdout, stderr) => {
        if (stdout) output.appendLine(stdout.trimEnd());
        if (stderr) output.appendLine(stderr.trimEnd());
        if (err) {
            vscode.window.showErrorMessage(`编译失败：${err.message}`);
            return;
        }
        cp.execFile(compiler, ['wrapper', file], { cwd: path.dirname(file) }, (err2, stdout2, stderr2) => {
            if (stdout2) output.appendLine(stdout2.trimEnd());
            if (stderr2) output.appendLine(stderr2.trimEnd());
            if (err2) vscode.window.showErrorMessage(`打包失败：${err2.message}`);
            else vscode.window.showInformationMessage('打包完成，已生成 .exe');
        });
    });
}

function escapeShell(s) {
    return `"${String(s).replace(/"/g, '\\"')}"`;
}

/* ============================================================
 * 新建项目
 * ============================================================ */
async function cmdNewProject() {
    const folders = await vscode.window.showOpenDialog({
        canSelectFolders: true,
        canSelectFiles: false,
        canSelectMany: false,
        openLabel: '选择项目文件夹（将在此创建 Emerald 项目）'
    });
    if (!folders || !folders.length) return;

    const root = folders[0].fsPath;
    const name = await vscode.window.showInputBox({
        prompt: '项目名',
        value: path.basename(root) || 'myemerald'
    });
    if (name === undefined) return;

    const srcFile = path.join(root, 'src.eml');
    const wdFile = path.join(root, 'wrapper.ewd');
    const gitignore = path.join(root, '.gitignore');
    const vscodeDir = path.join(root, '.vscode');
    const taskFile = path.join(vscodeDir, 'tasks.json');

    if (!fs.existsSync(vscodeDir)) fs.mkdirSync(vscodeDir, { recursive: true });

    fs.writeFileSync(srcFile,
        `import maineml;

eml(){
    print("Hello, Emerald!");
    newline();
    return 0;
}
`, 'utf8');

    fs.writeFileSync(wdFile,
        `pack_name="${name}";
pack_version="1.0.0";
icon="logo.png"
`, 'utf8');

    fs.writeFileSync(gitignore, `*.elr\n*.o\n*.exe\nbuild/\n`, 'utf8');

    fs.writeFileSync(taskFile, JSON.stringify({
        version: '2.0.0',
        tasks: [
            {
                label: 'Emerald: 编译',
                type: 'shell',
                command: 'eml',
                args: ['-c', '${file}'],
                group: 'build',
                problemMatcher: []
            },
            {
                label: 'Emerald: 运行',
                type: 'shell',
                command: 'eml',
                args: ['run', '${fileBasenameNoExtension}'],
                group: { kind: 'build', isDefault: true },
                problemMatcher: []
            },
            {
                label: 'Emerald: 打包 EXE',
                type: 'shell',
                command: 'eml',
                args: ['wrapper', '${file}'],
                problemMatcher: []
            }
        ]
    }, null, 2), 'utf8');

    const open = await vscode.window.showInformationMessage(
        'Emerald 项目已创建，是否打开？',
        '打开 src.eml', '打开文件夹'
    );
    if (open === '打开 src.eml') {
        const d = await vscode.workspace.openTextDocument(srcFile);
        await vscode.window.showTextDocument(d);
    } else if (open === '打开文件夹') {
        vscode.commands.executeCommand('vscode.openFolder', folders[0]);
    }
}

async function cmdGenerateWrapperConfig() {
    const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    if (!root) {
        vscode.window.showErrorMessage('请先打开一个文件夹。');
        return;
    }
    const wd = path.join(root, 'wrapper.ewd');
    if (fs.existsSync(wd)) {
        vscode.window.showWarningMessage('wrapper.ewd 已存在。');
        return;
    }
    fs.writeFileSync(wd,
        `pack_name="example";
pack_version="1.0.0";
icon="logo.png"
`, 'utf8');
    const d = await vscode.workspace.openTextDocument(wd);
    vscode.window.showTextDocument(d);
}

/* ============================================================
 * 诊断（基础 lint）
 * ============================================================ */
function lintDocument(doc) {
    const cfg = vscode.workspace.getConfiguration('emerald');
    if (!cfg.get('lint.enable', true)) {
        diagnostics.delete(doc.uri);
        return;
    }
    if (doc.languageId !== 'emerald') return;

    const text = doc.getText();
    const lines = text.split(/\r?\n/);
    /** @type {vscode.Diagnostic[]} */
    const diags = [];

    const declared = new Set();
    const declareBeforeUse = cfg.get('lint.declareBeforeUse', true);

    // 跳过的内建名
    const builtinSet = new Set(Object.keys(BUILTIN_FUNCTIONS));

    let depth = 0;
    for (let i = 0; i < lines.length; i++) {
        const raw = lines[i];
        const line = stripCommentAndString(raw).trim();
        if (!line) continue;

        // 括号平衡
        for (const ch of line) {
            if (ch === '{') depth++;
            else if (ch === '}') depth--;
        }
        if (depth < 0) {
            diags.push(makeDiag(i, 0, raw.length,
                '多余的 "}"（括号不匹配）。', vscode.DiagnosticSeverity.Error));
            depth = 0;
        }

        // 声明识别
        let m;
        if ((m = line.match(/^(int|string|bool)\s+([A-Za-z_]\w*)\s*(;|=|\()/))) {
            declared.add(m[2]);
        }

        // 使用未声明变量
        if (declareBeforeUse) {
            const re = /\b([A-Za-z_]\w*)\b/g;
            let t;
            while ((t = re.exec(line)) !== null) {
                const name = t[1];
                if (KEYWORDS.includes(name)) continue;
                if (MODULES.includes(name)) continue;
                if (builtinSet.has(name)) continue;
                if (declared.has(name)) continue;
                // 若是声明行中的名字，跳过
                if (/^(int|string|bool|function)\s+/.test(line) &&
                    line.indexOf(name) >= 0) continue;

                diags.push(makeDiag(i, t.index, name.length,
                    `变量 "${name}" 使用前未声明。`, vscode.DiagnosticSeverity.Warning));
                break; // 每行只报一次
            }
        }

        // 语句要以分号或大括号结尾
        if (/^[A-Za-z_].*[^;{}:,]$/.test(line) &&
            !/\{$/.test(line) &&
            !/^import\b/.test(line) &&
            !/^(eml|subcommand)\b/.test(line) &&
            !/\bfunction\b/.test(line) &&
            !/\bint\s+\w+\s*\(/.test(line) &&
            !/\bstring\s+\w+\s*\(/.test(line) &&
            !/\bbool\s+\w+\s*\(/.test(line)) {
            diags.push(makeDiag(i, raw.length, 0,
                '该语句可能缺少分号 ";"。', vscode.DiagnosticSeverity.Information));
        }
    }

    if (depth !== 0) {
        const last = lines.length - 1;
        diags.push(makeDiag(last, 0, lines[last].length,
            `缺少 ${depth} 个 "}"。`, vscode.DiagnosticSeverity.Error));
    }

    // 入口函数存在性
    if (/\.eml$/i.test(doc.fileName) && !/\beml\s*\(\s*\)\s*\{/.test(text)) {
        diags.push(makeDiag(0, 0, 1,
            '缺少入口函数 `eml(){ ... }`。', vscode.DiagnosticSeverity.Warning));
    }

    diagnostics.set(doc.uri, diags);
}

function stripCommentAndString(line) {
    let out = '';
    let inStr = false;
    for (let i = 0; i < line.length; i++) {
        const c = line[i];
        if (!inStr && c === '/' && line[i + 1] === '/') break;
        if (c === '"' && line[i - 1] !== '\\') inStr = !inStr;
        if (!inStr) out += c;
    }
    return out;
}

function makeDiag(line, startChar, length, message, severity) {
    const range = new vscode.Range(line, startChar, line, startChar + Math.max(length, 1));
    const d = new vscode.Diagnostic(range, message, severity);
    d.source = 'emerald';
    return d;
}

/* ============================================================
 * 悬停
 * ============================================================ */
class EmeraldHoverProvider {
    provideHover(document, position) {
        const range = document.getWordRangeAtPosition(position, /[A-Za-z_]\w*/);
        if (!range) return;
        const word = document.getText(range);

        if (BUILTIN_FUNCTIONS[word]) {
            const info = BUILTIN_FUNCTIONS[word];
            const md = new vscode.MarkdownString();
            md.appendCodeblock(info.sig, 'emerald');
            md.appendMarkdown('\n\n' + info.doc);
            return new vscode.Hover(md, range);
        }

        if (KEYWORDS.includes(word)) {
            const md = new vscode.MarkdownString(`**${word}** — Emerald 关键字`);
            return new vscode.Hover(md, range);
        }

        // 变量类型悬停
        const text = document.getText();
        const re = new RegExp(`\\b(int|string|bool)\\s+${escapeReg(word)}\\b`);
        const m = re.exec(text);
        if (m) {
            const md = new vscode.MarkdownString(`\`\`\`emerald\n${m[1]} ${word};\n\`\`\``);
            return new vscode.Hover(md, range);
        }
    }
}

function escapeReg(s) { return s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'); }

/* ============================================================
 * 补全
 * ============================================================ */
class EmeraldCompletionProvider {
    provideCompletionItems(document, position) {
        const items = [];

        // 关键字
        for (const k of KEYWORDS) {
            const it = new vscode.CompletionItem(k, vscode.CompletionItemKind.Keyword);
            it.insertText = k;
            items.push(it);
        }

        // 内建函数
        for (const [name, info] of Object.entries(BUILTIN_FUNCTIONS)) {
            const it = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
            it.detail = info.sig;
            it.documentation = new vscode.MarkdownString(info.doc);
            if (name === 'print') it.insertText = new vscode.SnippetString('print(${1:"内容"});');
            else if (name === 'newline') it.insertText = 'newline();';
            else if (name === 'input') it.insertText = new vscode.SnippetString('input(${1:a});');
            else if (name === 'speak') it.insertText = new vscode.SnippetString('speak(${1:"内容"});');
            else if (name === 'format') it.insertText = new vscode.SnippetString('format(${1:a}, list);');
            else if (name === 'list') it.insertText = new vscode.SnippetString('list ${1:a}(${2:add}, ${3:"内容"});');
            else if (name === 'subcommand') it.insertText = new vscode.SnippetString('subcommand(${1:"1"}){\n    $0\n}');
            items.push(it);
        }

        // 模块名
        for (const m of MODULES) {
            const it = new vscode.CompletionItem(m, vscode.CompletionItemKind.Module);
            items.push(it);
        }

        // 用户自定义变量/函数
        const text = document.getText();
        const varRe = /\b(int|string|bool)\s+([A-Za-z_]\w*)/g;
        let match;
        const seen = new Set();
        while ((match = varRe.exec(text)) !== null) {
            if (seen.has(match[2])) continue;
            seen.add(match[2]);
            const it = new vscode.CompletionItem(match[2], vscode.CompletionItemKind.Variable);
            it.detail = match[1];
            items.push(it);
        }

        const fnRe = /\b(function|int|string|bool)\s+([A-Za-z_]\w*)\s*\(/g;
        while ((match = fnRe.exec(text)) !== null) {
            const it = new vscode.CompletionItem(match[2], vscode.CompletionItemKind.Function);
            it.detail = `${match[1]} ${match[2]}()`;
            items.push(it);
        }

        return items;
    }
}

module.exports = { activate, deactivate };