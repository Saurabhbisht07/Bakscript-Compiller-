const sampleSelect = document.getElementById("sampleSelect");
const loadSampleBtn = document.getElementById("loadSampleBtn");
const runBtn = document.getElementById("runBtn");
const visualizeBtn = document.getElementById("visualizeBtn");
const clearBtn = document.getElementById("clearBtn");
const codeEditor = document.getElementById("codeEditor");
const output = document.getElementById("output");
const visualizer = document.getElementById("visualizer");

/** Human-readable token kind for pipeline chips (no emoji). */
const TOKEN_LABELS = {
  TOKEN_NUM: { label: "keyword: num" },
  TOKEN_STR: { label: "keyword: str" },
  TOKEN_TRUE: { label: "true" },
  TOKEN_FALSE: { label: "false" },
  TOKEN_PLUS: { label: "+" },
  TOKEN_MINUS: { label: "−" },
  TOKEN_MULTIPLY: { label: "×" },
  TOKEN_DIVIDE: { label: "÷" },
  TOKEN_NOT: { label: "!" },
  TOKEN_LESS: { label: "<" },
  TOKEN_GREATER: { label: ">" },
  TOKEN_LESS_EQ: { label: "<=" },
  TOKEN_GREATER_EQ: { label: ">=" },
  TOKEN_EQ_EQ: { label: "==" },
  TOKEN_NOT_EQ: { label: "!=" },
  TOKEN_AND_AND: { label: "&&" },
  TOKEN_OR_OR: { label: "||" },
  TOKEN_SHOW: { label: "show" },
  TOKEN_WHEN: { label: "when" },
  TOKEN_OTHERWISE: { label: "otherwise" },
  TOKEN_REPEAT: { label: "repeat" },
  TOKEN_WHILE: { label: "while" },
  TOKEN_FUNCTION: { label: "function" },
  TOKEN_RETURN: { label: "return" },
  TOKEN_ASK: { label: "ask" },
  TOKEN_IDENTIFIER: { label: "identifier" },
  TOKEN_NUMBER_LITERAL: { label: "number literal" },
  TOKEN_STRING_LITERAL: { label: "string literal" },
  TOKEN_COMMA: { label: "," },
  TOKEN_EQUALS: { label: "=" },
  TOKEN_SEMICOLON: { label: ";" },
  TOKEN_LPAREN: { label: "(" },
  TOKEN_RPAREN: { label: ")" },
  TOKEN_LBRACKET: { label: "[" },
  TOKEN_RBRACKET: { label: "]" },
  TOKEN_LBRACE: { label: "{" },
  TOKEN_RBRACE: { label: "}" },
  TOKEN_EOF: { label: "EOF" },
};

function simplifyAstLine(line) {
  const t = line.trimEnd();
  let m;
  if (/^Program:$/.test(t)) return "Program (root)";
  if (/^Block:$/.test(t)) return "Block";
  if ((m = t.match(/^Number: (-?\d+)$/))) return `Number literal: ${m[1]}`;
  if ((m = t.match(/^String: "(.*)"$/))) return `String literal: "${m[1]}"`;
  if ((m = t.match(/^Identifier: (.+)$/))) return `Identifier: ${m[1]}`;
  if (t === "Bool: true") return "Boolean: true";
  if (t === "Bool: false") return "Boolean: false";
  if ((m = t.match(/^VarDecl: (\w+) (\w+)$/)))
    return `Variable declaration: ${m[2]} (${m[1]})`;
  if (t === "IfStatement:") return "If statement";
  if (t === "Condition:") return "Condition";
  if (t === "Then:") return "Then branch";
  if (t === "Else:") return "Else branch";
  if (t === "ForLoop:") return "For loop";
  if (t === "WhileLoop:") return "While loop";
  if (t === "Init:") return "Loop initializer";
  if (t === "Increment:") return "Loop increment";
  if (t === "Body:") return "Loop body";
  if ((m = t.match(/^FunctionDecl: (\w+)\((.*)\)$/))) {
    const args = m[2].trim() || "";
    return args ? `Function declaration: ${m[1]}(${args})` : `Function declaration: ${m[1]}()`;
  }
  if ((m = t.match(/^FunctionCall: (.+)$/))) return `Call: ${m[1]}`;
  if (t === "Return:") return "Return";
  if (/^UnaryOp: \d+$/.test(t)) return "Unary expression";
  if (/^BinaryOp: \d+$/.test(t)) return "Binary expression";
  if ((m = t.match(/^ArrayLiteral\[(\d+)\]$/))) return `Array literal (${m[1]} element(s))`;
  if (t === "IndexAccess:") return "Index access";
  return null;
}

function setOutput(text) {
  output.textContent = text;
}

function appendOutput(text) {
  output.textContent += text;
}

function clearVisualizer() {
  visualizer.replaceChildren();
}

function extractSection(text, startMarker, endMarker) {
  const t = text.replace(/\r\n/g, "\n");
  const start = t.indexOf(startMarker);
  if (start === -1) return "";
  const from = start + startMarker.length;
  const end = t.indexOf(endMarker, from);
  if (end === -1) return t.slice(from).trim();
  return t.slice(from, end).trim();
}

/** Build nested tree from indented AST dump (two spaces per level). */
function parseAstDumpToTree(text) {
  const synthetic = { label: "AST", raw: "", children: [] };
  const lines = text.replace(/\r\n/g, "\n").split("\n");
  const stack = [{ node: synthetic, depth: -1 }];
  for (const line of lines) {
    if (!line.trim()) continue;
    const depth = indentLevel(line);
    const raw = line.trim();
    const label = simplifyAstLine(line) || raw;
    const node = { label, raw, children: [] };
    while (stack.length > 1 && stack[stack.length - 1].depth >= depth) {
      stack.pop();
    }
    stack[stack.length - 1].node.children.push(node);
    stack.push({ node, depth });
  }
  return synthetic;
}

function renderAstTreeDom(n, depth = 0) {
  const li = document.createElement("li");
  li.className = "ast-tree-item";

  const title = el("span", "ast-tree-label", n.label);
  if (n.raw && n.label !== n.raw) title.title = n.raw;

  if (n.children.length === 0) {
    li.appendChild(title);
    return li;
  }

  const details = document.createElement("details");
  details.className = "ast-tree-details";
  // Keep the top of the tree expanded; deeper nodes can be opened as needed.
  if (depth <= 1) details.open = true;

  const summary = document.createElement("summary");
  summary.className = "ast-tree-summary";
  summary.appendChild(title);
  details.appendChild(summary);

  const ul = el("ul", "ast-tree-children");
  for (const c of n.children) {
    ul.appendChild(renderAstTreeDom(c, depth + 1));
  }
  details.appendChild(ul);
  li.appendChild(details);
  return li;
}

/** When the driver cannot emit === SYMBOL TABLE === (old binary or parse failure). */
function fallbackSymbolTableFromAst(astText) {
  if (!astText || !astText.trim()) return "";
  const rows = [];
  rows.push(
    "Symbol table (recovered from AST dump — rebuild bakscript.exe for full semantic output)"
  );
  rows.push("--------------------------------------------------------");
  rows.push(
    `${padField("Name", 20)} ${padField("Type", 10)} ${padField("DataType", 10)} ${padField("Scope", 10)} Initialized`
  );
  rows.push("--------------------------------------------------------");
  let any = false;
  const varRe = /^VarDecl:\s+(\w+)\s+(\w+)/gm;
  let m;
  while ((m = varRe.exec(astText)) !== null) {
    any = true;
    rows.push(
      `${padField(m[2], 20)} ${padField("Variable", 10)} ${padField(m[1], 10)} ${padField("?", 10)} ?`
    );
  }
  const fnRe = /^FunctionDecl:\s+(\w+)\s*\(/gm;
  while ((m = fnRe.exec(astText)) !== null) {
    any = true;
    rows.push(
      `${padField(m[1], 20)} ${padField("Function", 10)} ${padField("void", 10)} ${padField("?", 10)} n/a`
    );
  }
  rows.push("--------------------------------------------------------");
  return any ? rows.join("\n") : "";
}

function padField(s, w) {
  const t = String(s);
  return t.length >= w ? t.slice(0, w) : t + " ".repeat(w - t.length);
}

function parseTokenLine(line) {
  const m = line.match(
    /^Token\(type=(\w+), value='(.*)', line=(\d+), column=(\d+)\)\s*$/
  );
  if (!m) return null;
  return { type: m[1], value: m[2], line: m[3], column: m[4] };
}

function indentLevel(line) {
  let n = 0;
  while (n < line.length && line[n] === " ") n++;
  return Math.floor(n / 2);
}

const TRACE_NODE_LABEL = {
  Number: "number",
  Bool: "boolean",
  String: "string",
  Identifier: "identifier",
  ArrayLiteral: "array literal",
  IndexAccess: "index access",
  BinaryOp: "binary operation",
  UnaryOp: "unary operation",
  FunctionCall: "function call",
  VarDecl: "variable declaration",
  IfStatement: "if statement",
  ForLoop: "for loop",
  WhileLoop: "while loop",
  FunctionDecl: "function declaration",
  Return: "return",
  Block: "block",
  Program: "program",
  UnknownNode: "unknown node",
};

function humanizeTraceKind(kind) {
  return TRACE_NODE_LABEL[kind] || kind;
}

function parseTraceAfter(line) {
  const m = line.match(/^\[TRACE (\d+)\] (\w+) -> (ok|error)$/);
  if (!m) return null;
  return { step: m[1], kind: m[2], ok: m[3] === "ok" };
}

function parseTraceBefore(line) {
  const m = line.match(/^\[TRACE (\d+)\] (\w+)$/);
  if (!m) return null;
  return { step: m[1], kind: m[2] };
}

function parseStateLine(line) {
  const m = line.match(/^state \{ (.*) \}\s*$/);
  return m ? m[1] : null;
}

function el(tag, className, text) {
  const n = document.createElement(tag);
  if (className) n.className = className;
  if (text !== undefined && text !== "") n.textContent = text;
  return n;
}

function renderPipelineVisualizer(stdout, stderr, exitCode) {
  clearVisualizer();

  const tokens = extractSection(stdout, "=== TOKENS ===", "=== END TOKENS ===");
  const ast = extractSection(stdout, "=== AST ===", "=== END AST ===");
  let symbols = extractSection(stdout, "=== SYMBOL TABLE ===", "=== END SYMBOL TABLE ===");
  let symbolsFromFallback = false;
  const tac = extractSection(stdout, "=== TAC ===", "=== END TAC ===");
  const execution = extractSection(stdout, "=== EXECUTION ===", "=== END EXECUTION ===");
  const trace = extractSection(stdout, "=== TRACE ===", "=== END TRACE ===");

  if (!symbols.trim()) {
    const fb = fallbackSymbolTableFromAst(ast);
    if (fb) {
      symbols = fb;
      symbolsFromFallback = true;
    }
  }

  const banner = el("div", "viz-banner");
  if (exitCode === 0) {
    banner.classList.add("viz-banner--ok");
    banner.appendChild(el("span", "viz-banner-lead", "Completed successfully."));
    banner.appendChild(document.createTextNode(" Exit code 0."));
  } else {
    banner.classList.add("viz-banner--warn");
    banner.appendChild(el("span", "viz-banner-lead", "Finished with errors."));
    banner.appendChild(document.createTextNode(" See stderr below."));
  }
  visualizer.appendChild(banner);

  // Step 1 — tokens
  const step1 = el("article", "viz-step");
  step1.appendChild(el("div", "viz-step-num", "1"));
  const s1h = el("div", "viz-step-body");
  s1h.appendChild(el("h3", "viz-step-title", "Lexing"));
  s1h.appendChild(
    el(
      "p",
      "viz-step-blurb",
      "Stream of tokens produced by the lexer (literals, identifiers, punctuation, keywords)."
    )
  );
  const strip = el("div", "token-strip");
  let tokenCount = 0;
  if (tokens) {
    const lines = tokens.split("\n");
    for (const line of lines) {
      const tok = parseTokenLine(line);
      if (!tok) continue;
      if (tok.type === "TOKEN_EOF") {
        const chip = el("div", "token-chip token-chip--end");
        chip.appendChild(el("span", "token-chip-label", "EOF"));
        strip.appendChild(chip);
        continue;
      }
      tokenCount++;
      const info = TOKEN_LABELS[tok.type] || { label: tok.type.replace(/^TOKEN_/, "") };
      const chip = el("div", "token-chip");
      chip.title = `${info.label}${tok.value ? ` · ${tok.value}` : ""}`;
      const cap = el("div", "token-cap");
      cap.appendChild(el("span", "token-chip-label", info.label));
      if (tok.value && tok.value.length > 0) {
        cap.appendChild(el("span", "token-chip-value", tok.value.length > 18 ? tok.value.slice(0, 17) + "…" : tok.value));
      }
      chip.appendChild(cap);
      strip.appendChild(chip);
    }
  }
  if (strip.childNodes.length === 0) {
    strip.appendChild(
      el("p", "viz-empty", "No token stream returned. Add source and run Visualize pipeline.")
    );
  }
  s1h.appendChild(strip);
  const meta1 = el("p", "viz-step-meta");
  meta1.textContent = `Token count: ${tokenCount} (plus EOF).`;
  s1h.appendChild(meta1);
  step1.appendChild(s1h);
  visualizer.appendChild(step1);

  // Step 2 — tree
  const step2 = el("article", "viz-step");
  step2.appendChild(el("div", "viz-step-num", "2"));
  const s2h = el("div", "viz-step-body");
  s2h.appendChild(el("h3", "viz-step-title", "Parsing (AST)"));
  s2h.appendChild(
    el(
      "p",
      "viz-step-blurb",
      "Abstract syntax tree: nested list built from the indented AST dump (two spaces per nesting level)."
    )
  );
  const treeBox = el("div", "ast-tree");
  if (ast && ast.trim()) {
    const root = parseAstDumpToTree(ast);
    const wrap = el("div", "ast-tree-wrap");
    const ul = el("ul", "ast-tree-root");
    if (root.children.length === 0) {
      ul.appendChild(renderAstTreeDom({ label: "(empty)", raw: "", children: [] }));
    } else {
      for (const c of root.children) {
        ul.appendChild(renderAstTreeDom(c));
      }
    }
    wrap.appendChild(ul);
    treeBox.appendChild(wrap);
  } else {
    treeBox.appendChild(el("p", "viz-empty", "No AST section (parse may have failed before emit)."));
  }
  s2h.appendChild(treeBox);
  const metaAst = el("p", "viz-step-meta");
  metaAst.textContent =
    "Hover a node to see the original dump line. Friendly labels are shown when the line matches a known AST pattern.";
  s2h.appendChild(metaAst);
  step2.appendChild(s2h);
  visualizer.appendChild(step2);

  // Step 3 — symbol table (semantic analysis)
  const stepSym = el("article", "viz-step");
  stepSym.appendChild(el("div", "viz-step-num", "3"));
  const sSym = el("div", "viz-step-body");
  sSym.appendChild(el("h3", "viz-step-title", "Symbol table"));
  sSym.appendChild(
    el(
      "p",
      "viz-step-blurb",
      "Bindings recorded during semantic analysis: names, kinds, types, scope depth, initialization."
    )
  );
  const symPanel = el("div", "viz-output-panel");
  if (symbols && symbols.trim()) {
    symPanel.appendChild(el("pre", "viz-output-pre", symbols.trim()));
  } else {
    symPanel.appendChild(
      el(
        "p",
        "viz-muted",
        "No symbol data. Check stderr for semantic errors, or rebuild bakscript.exe with --dump-symbols. Run Visualize after a successful parse."
      )
    );
  }
  sSym.appendChild(symPanel);
  if (symbolsFromFallback) {
    sSym.appendChild(
      el(
        "p",
        "viz-step-meta",
        "Recovered from AST text (not the full semantic symbol table). Rebuild the driver for real scope and initialization columns."
      )
    );
  }
  stepSym.appendChild(sSym);
  visualizer.appendChild(stepSym);

  // Step 4 — TAC / IR
  const stepTac = el("article", "viz-step");
  stepTac.appendChild(el("div", "viz-step-num", "4"));
  const sTac = el("div", "viz-step-body");
  sTac.appendChild(el("h3", "viz-step-title", "Three-address code (IR)"));
  sTac.appendChild(
    el(
      "p",
      "viz-step-blurb",
      "Linear intermediate representation: temporaries, labels, and control-flow–style ops used before native codegen."
    )
  );
  const tacPanel = el("div", "viz-output-panel");
  if (tac && tac.trim()) {
    tacPanel.appendChild(el("pre", "viz-output-pre", tac.trim()));
  } else {
    tacPanel.appendChild(
      el(
        "p",
        "viz-muted",
        "No TAC in stdout. Rebuild bakscript / bakscript.exe from current sources (main.c with --dump-tac) and ensure Visualize pipeline runs the new binary."
      )
    );
  }
  sTac.appendChild(tacPanel);
  stepTac.appendChild(sTac);
  visualizer.appendChild(stepTac);

  // Step 5 — run output
  const step3 = el("article", "viz-step");
  step3.appendChild(el("div", "viz-step-num", "5"));
  const s3h = el("div", "viz-step-body");
  s3h.appendChild(el("h3", "viz-step-title", "Interpreter output"));
  s3h.appendChild(
    el(
      "p",
      "viz-step-blurb",
      "Program stdout during interpretation (e.g. show(...), ask(...)), delimited by the debug execution markers."
    )
  );
  const bubble = el("div", "viz-output-panel");
  if (execution && execution.trim()) {
    bubble.appendChild(el("pre", "viz-output-pre", execution.trim()));
  } else {
    bubble.appendChild(el("p", "viz-muted", "No stdout for this run."));
  }
  s3h.appendChild(bubble);
  step3.appendChild(s3h);
  visualizer.appendChild(step3);

  // Step 6 — trace
  const step4 = el("article", "viz-step");
  step4.appendChild(el("div", "viz-step-num", "6"));
  const s4h = el("div", "viz-step-body");
  s4h.appendChild(el("h3", "viz-step-title", "Execution trace"));
  s4h.appendChild(
    el(
      "p",
      "viz-step-blurb",
      "Statement-level trace with environment snapshots after each step (--trace)."
    )
  );
  const traceBox = el("div", "trace-story");
  if (trace) {
    const lines = trace.split("\n");
    let pendingBefore = null;
    for (const line of lines) {
      const before = parseTraceBefore(line);
      if (before) {
        pendingBefore = before;
        continue;
      }
      const after = parseTraceAfter(line);
      if (after && pendingBefore && after.step === pendingBefore.step) {
        const card = el("div", "trace-card");
        const title = el("div", "trace-card-title");
        title.textContent = `Step ${after.step}: ${humanizeTraceKind(after.kind)}`;
        card.appendChild(title);
        const status = el("div", after.ok ? "trace-status trace-status--ok" : "trace-status trace-status--bad");
        status.textContent = after.ok ? "Status: ok" : "Status: error";
        card.appendChild(status);
        pendingBefore = null;
        traceBox.appendChild(card);
        continue;
      }
      const state = parseStateLine(line);
      if (state !== null) {
        const mem = el("div", "trace-memory");
        const label = el("span", "trace-memory-label", "Environment:");
        mem.appendChild(label);
        const val = el("span", "trace-memory-val", state.trim() ? state : "{ }");
        mem.appendChild(val);
        if (traceBox.lastChild) traceBox.lastChild.appendChild(mem);
        continue;
      }
      if (line.trim()) {
        traceBox.appendChild(el("div", "trace-raw", line.trim()));
      }
    }
  }
  if (traceBox.childNodes.length === 0) {
    traceBox.appendChild(
      el("p", "viz-empty", "No trace data. The server runs with --trace when you use Visualize pipeline.")
    );
  }
  s4h.appendChild(traceBox);
  step4.appendChild(s4h);
  visualizer.appendChild(step4);

  if (stderr && stderr.trim()) {
    const errBox = el("div", "viz-error-box");
    errBox.appendChild(el("h4", "viz-error-title", "Standard error (stderr)"));
    errBox.appendChild(el("pre", "viz-error-pre", stderr.trim()));
    visualizer.appendChild(errBox);
  }
}

async function loadSamples() {
  const res = await fetch("/samples");
  const data = await res.json();
  sampleSelect.innerHTML = "";
  for (const name of data.samples || []) {
    const opt = document.createElement("option");
    opt.value = name;
    opt.textContent = name;
    sampleSelect.appendChild(opt);
  }
}

async function loadSelectedSample() {
  const file = sampleSelect.value;
  if (!file) return;
  const res = await fetch(`/sample/${encodeURIComponent(file)}`);
  const data = await res.json();
  if (data.code !== undefined) {
    codeEditor.value = data.code;
    setOutput(`Loaded sample: ${data.name}\n`);
  } else {
    setOutput(`Error: ${data.error || "Failed to load sample"}\n`);
  }
}

async function runCode() {
  setOutput("Running...\n");
  try {
    const res = await fetch("/run", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ code: codeEditor.value }),
    });
    const data = await res.json();

    setOutput("");
    if (data.error) {
      appendOutput(`Error: ${data.error}\n`);
      return;
    }

    appendOutput(`Exit code: ${data.exit_code}\n\n`);
    if (data.stdout) {
      appendOutput("STDOUT:\n");
      appendOutput(data.stdout + "\n");
    }
    if (data.stderr) {
      appendOutput("STDERR:\n");
      appendOutput(data.stderr + "\n");
    }
  } catch (err) {
    setOutput(`Request failed: ${err}\n`);
  }
}

async function visualizeCode() {
  clearVisualizer();
  visualizer.appendChild(el("p", "viz-loading", "Running pipeline visualization…"));
  try {
    const res = await fetch("/visualize", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ code: codeEditor.value }),
    });
    const data = await res.json();
    if (data.error) {
      clearVisualizer();
      const err = el("div", "viz-error-box");
      err.appendChild(el("p", "viz-error-title", "Error"));
      err.appendChild(el("pre", "viz-error-pre", data.error));
      visualizer.appendChild(err);
      return;
    }
    const stdout = data.stdout || "";
    const stderr = data.stderr || "";
    renderPipelineVisualizer(stdout, stderr, data.exit_code);
  } catch (err) {
    clearVisualizer();
    const errBox = el("div", "viz-error-box");
    errBox.appendChild(el("p", "viz-error-title", "Request failed"));
    errBox.appendChild(el("pre", "viz-error-pre", err.toString()));
    visualizer.appendChild(errBox);
  }
}

loadSampleBtn.addEventListener("click", loadSelectedSample);
runBtn.addEventListener("click", runCode);
visualizeBtn.addEventListener("click", visualizeCode);
clearBtn.addEventListener("click", () => {
  setOutput("");
  clearVisualizer();
});

window.addEventListener("DOMContentLoaded", async () => {
  await loadSamples();
  if (sampleSelect.options.length > 0) {
    await loadSelectedSample();
  }
});
