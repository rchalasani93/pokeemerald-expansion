"""Minimal C tokenizer, initializer parser and constant-expression evaluator.

Used to read the game's data tables from preprocessed (cpp) sources so the dex
sees exactly what the ROM compiles: config switches, macros and forms resolved.
"""
import re

TOKEN_RE = re.compile(r'''
    (?P<ws>\s+)
  | (?P<linemarker>^\#[^\n]*)
  | (?P<string>"(?:\\.|[^"\\])*")
  | (?P<char>'(?:\\.|[^'\\])')
  | (?P<number>0[xX][0-9a-fA-F]+[uUlL]*|\d+\.\d*(?:[eE][-+]?\d+)?[fF]?|\.\d+[fF]?|\d+[uUlL]*)
  | (?P<ident>[A-Za-z_]\w*)
  | (?P<op><<=|>>=|\.\.\.|->|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^!~<>?:=,;.(){}\[\]#])
''', re.M | re.X)


def tokenize(text):
    out = []
    for m in TOKEN_RE.finditer(text):
        kind = m.lastgroup
        if kind in ('ws', 'linemarker'):
            continue
        out.append((kind, m.group(kind)))
    return out


class Expr:
    """An unparsed expression (list of tokens) inside an initializer."""
    __slots__ = ('tokens',)

    def __init__(self, tokens):
        self.tokens = tokens

    def text(self):
        return ' '.join(t[1] for t in self.tokens)

    def ident(self):
        if len(self.tokens) == 1 and self.tokens[0][0] == 'ident':
            return self.tokens[0][1]
        return None

    def __repr__(self):
        return 'Expr(%s)' % self.text()


class Init:
    """A brace initializer: positional items plus designated fields/indices."""
    __slots__ = ('items', 'fields', 'indexed')

    def __init__(self):
        self.items = []      # positional values
        self.fields = {}     # .name = value
        self.indexed = []    # ([index expr], value)

    def get(self, name, default=None):
        return self.fields.get(name, default)


class Parser:
    def __init__(self, tokens, pos=0):
        self.t = tokens
        self.i = pos

    def peek(self, k=0):
        j = self.i + k
        return self.t[j] if j < len(self.t) else (None, None)

    def take(self):
        tok = self.t[self.i]
        self.i += 1
        return tok

    def value(self):
        # cast + compound literal: (const struct X[]) { ... }
        if self.peek()[1] == '(':
            j, depth = self.i, 0
            while j < len(self.t):
                v = self.t[j][1]
                depth += v == '('
                depth -= v == ')'
                j += 1
                if depth == 0:
                    break
            if j < len(self.t) and self.t[j][1] == '{':
                self.i = j
        if self.peek()[1] == '{':
            return self.brace()
        return self.expr()

    def brace(self):
        assert self.take()[1] == '{'
        init = Init()
        while self.peek()[1] != '}':
            if self.peek()[1] == '.' and self.peek(1)[0] == 'ident' and self.peek(2)[1] == '=':
                self.take()
                name = self.take()[1]
                self.take()
                init.fields[name] = self.value()
            elif self.peek()[1] == '[':
                self.take()
                idx = []
                depth = 1
                while True:
                    tok = self.take()
                    depth += tok[1] == '['
                    depth -= tok[1] == ']'
                    if depth == 0:
                        break
                    idx.append(tok)
                self.take()  # '='
                init.indexed.append((Expr(idx), self.value()))
            else:
                init.items.append(self.value())
            if self.peek()[1] == ',':
                self.take()
        self.take()
        return init

    def expr(self):
        toks, depth = [], 0
        while self.i < len(self.t):
            v = self.peek()[1]
            if depth == 0 and v in (',', '}', ';'):
                break
            depth += v in ('(', '[', '{')
            depth -= v in (')', ']', '}')
            toks.append(self.take())
        return Expr(toks)


def find_array(tokens, name, start=0):
    """Return the initializer of the definition `... name[...] = {...}`."""
    for i in range(start, len(tokens) - 1):
        if tokens[i][1] == name and tokens[i][0] == 'ident':
            j = i + 1
            while j < len(tokens) and tokens[j][1] == '[':
                depth = 0
                while True:
                    depth += tokens[j][1] == '['
                    depth -= tokens[j][1] == ']'
                    j += 1
                    if depth == 0:
                        break
            if j < len(tokens) and tokens[j][1] == '=' and j + 1 < len(tokens) and tokens[j + 1][1] == '{':
                return Parser(tokens, j + 1).brace()
    return None


def find_all_arrays(tokens, name_re):
    """All `name[] = {...}` definitions whose name matches name_re."""
    found = {}
    rx = re.compile(name_re)
    for i in range(len(tokens) - 4):
        if tokens[i][0] == 'ident' and tokens[i + 1][1] == '[' and rx.fullmatch(tokens[i][1]):
            j = i + 1
            while tokens[j][1] != ']':
                j += 1
            if tokens[j + 1][1] == '=' and tokens[j + 2][1] == '{':
                found[tokens[i][1]] = Parser(tokens, j + 2).brace()
    return found


def parse_enums(tokens):
    """Collect enumerator values from every enum definition."""
    values = {}
    i = 0
    while i < len(tokens):
        if tokens[i][1] == 'enum':
            j = i + 1
            # enum [__attribute__((...))] [Tag] [__attribute__((...))] { ... }
            def skip_attribute(j):
                if j < len(tokens) and tokens[j][1] == '__attribute__':
                    j += 1
                    depth = 0
                    while j < len(tokens):
                        depth += tokens[j][1] == '('
                        depth -= tokens[j][1] == ')'
                        j += 1
                        if depth == 0:
                            break
                return j
            j = skip_attribute(j)
            if j < len(tokens) and tokens[j][0] == 'ident' and tokens[j][1] != '__attribute__':
                j += 1
            j = skip_attribute(j)
            if j < len(tokens) and tokens[j][1] == '{':
                j += 1
                current = -1
                while tokens[j][1] != '}':
                    name = tokens[j][1]
                    j += 1
                    if tokens[j][1] == '=':
                        j += 1
                        start = j
                        depth = 0
                        while not (depth == 0 and tokens[j][1] in (',', '}')):
                            depth += tokens[j][1] in ('(', '[')
                            depth -= tokens[j][1] in (')', ']')
                            j += 1
                        v = evaluate(tokens[start:j], values)
                        current = v if isinstance(v, (int, float)) else current + 1
                    else:
                        current += 1
                    values[name] = current
                    if tokens[j][1] == ',':
                        j += 1
                i = j
        i += 1
    return values


class _Eval:
    PREC = [('?',), ('||',), ('&&',), ('|',), ('^',), ('&',), ('==', '!='),
            ('<', '>', '<=', '>='), ('<<', '>>'), ('+', '-'), ('*', '/', '%')]

    def __init__(self, toks, env):
        self.t = toks
        self.i = 0
        self.env = env

    def peek(self):
        return self.t[self.i][1] if self.i < len(self.t) else None

    def parse(self, level=0):
        if level == 0:
            cond = self.parse(1)
            if self.peek() == '?':
                self.i += 1
                a = self.parse(0)
                assert self.peek() == ':'
                self.i += 1
                b = self.parse(0)
                return a if cond else b
            return cond
        if level >= len(self.PREC):
            return self.unary()
        left = self.parse(level + 1)
        while self.peek() in self.PREC[level]:
            op = self.t[self.i][1]
            self.i += 1
            right = self.parse(level + 1)
            left = self.apply(op, left, right)
        return left

    @staticmethod
    def apply(op, a, b):
        if op == '+': return a + b
        if op == '-': return a - b
        if op == '*': return a * b
        if op == '/': return (int(a / b) if isinstance(a, int) and isinstance(b, int) else a / b) if b else 0
        if op == '%': return a % b if b else 0
        if op == '<<': return int(a) << int(b)
        if op == '>>': return int(a) >> int(b)
        if op == '<': return int(a < b)
        if op == '>': return int(a > b)
        if op == '<=': return int(a <= b)
        if op == '>=': return int(a >= b)
        if op == '==': return int(a == b)
        if op == '!=': return int(a != b)
        if op == '&': return int(a) & int(b)
        if op == '|': return int(a) | int(b)
        if op == '^': return int(a) ^ int(b)
        if op == '&&': return int(bool(a) and bool(b))
        if op == '||': return int(bool(a) or bool(b))
        raise ValueError(op)

    def unary(self):
        v = self.peek()
        if v == '-':
            self.i += 1
            return -self.unary()
        if v == '+':
            self.i += 1
            return self.unary()
        if v == '!':
            self.i += 1
            return int(not self.unary())
        if v == '~':
            self.i += 1
            return ~int(self.unary())
        if v == '(':
            # cast like (u8) / (uq4_12_t) / (enum X)
            j = self.i + 1
            while j < len(self.t) and self.t[j][0] == 'ident':
                j += 1
            if j < len(self.t) and self.t[j][1] == ')' and j > self.i + 1 and self.t[self.i + 1][1] not in self.env:
                self.i = j + 1
                val = self.unary()
                name = self.t[j - 1][1]
                return int(val) if name in ('u8', 'u16', 'u32', 's8', 's16', 's32', 'uq4_12_t', 'size_t', 'int') else val
            self.i += 1
            val = self.parse(0)
            assert self.peek() == ')'
            self.i += 1
            return val
        kind, text = self.t[self.i]
        self.i += 1
        if kind == 'number':
            t = text.rstrip('uUlLfF')
            if t.lower().startswith('0x'):
                return int(t, 16)
            return float(t) if '.' in t or 'e' in t.lower() else int(t)
        if kind == 'char':
            return ord(text[1:-1].encode().decode('unicode_escape'))
        if kind == 'ident':
            if text in ('TRUE', 'true'):
                return 1
            if text in ('FALSE', 'false', 'NULL'):
                return 0
            if text in self.env:
                return self.env[text]
            raise KeyError(text)
        raise ValueError(text)


def evaluate(tokens, env):
    """Evaluate a constant C expression; returns None if it can't."""
    if isinstance(tokens, Expr):
        tokens = tokens.tokens
    if not tokens:
        return None
    try:
        e = _Eval(tokens, env)
        v = e.parse(0)
        if e.i != len(tokens):
            return None
        return v
    except Exception:
        return None


def c_string(value):
    """Text of _("..."), COMPOUND_STRING("...") or a plain string literal."""
    if isinstance(value, Init):
        value = value.items[0] if value.items else None
    if not isinstance(value, Expr):
        return None
    parts = []
    for t in value.tokens:
        if t[0] == 'string':
            parts.append(t[1][1:-1])
        elif parts:
            break  # only the first run of adjacent literals (macros may repeat the text)
    if not parts:
        return None
    s = ''.join(parts)
    s = s.replace('\\n', ' ').replace('\\p', ' ').replace('\\l', ' ')
    s = re.sub(r'\{[A-Z_0-9 ]+\}', '', s)  # control codes like {PKMN}
    s = s.replace('\\"', '"').replace("\\'", "'")
    return re.sub(r'\s+', ' ', s).strip()
