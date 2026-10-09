package utils;

/**
 * Convierte una expresión matemática del usuario a LaTeX.
 *
 * En particular, la división se representa como fracción y los paréntesis
 * "estructurales" desaparecen (los aporta \frac o los añade solo donde hacen
 * falta por precedencia):
 *
 *   1/(sin(x))    -> \frac{1}{\sin\left(x\right)}
 *   (a+b)/(c+d)   -> \frac{a + b}{c + d}
 *   x^2           -> x^{2}
 */
public final class Latex {

    private Latex() {}

    public static String toLatex(String expr) {
        if (expr == null || expr.isBlank()) return "";
        try {
            return new Parser(expr).parse().tex();
        } catch (RuntimeException e) {
            // Si no se puede interpretar, se devuelve tal cual.
            return expr.trim();
        }
    }

    // ------------------------------------------------------------------ AST
    private interface Node {
        String tex();
        int prec();
    }

    private static final int ADD = 1, MUL = 2, UNARY = 3, POW = 4, ATOM = 5;

    private static String wrap(Node n, int min) {
        String s = n.tex();
        return n.prec() < min ? "\\left(" + s + "\\right)" : s;
    }

    private static final class Num implements Node {
        private final String v;
        Num(String v) { this.v = v; }
        public String tex() { return v; }
        public int prec() { return ATOM; }
    }

    private static final class Var implements Node {
        private final String v;
        Var(String v) { this.v = v; }
        public String tex() {
            switch (v) {
                case "pi": case "PI": case "Pi": return "\\pi";
                default: return v;
            }
        }
        public int prec() { return ATOM; }
    }

    private static final class Func implements Node {
        private final String name;
        private final Node arg;
        Func(String name, Node arg) { this.name = name; this.arg = arg; }
        public String tex() {
            String a = arg.tex();
            switch (name) {
                case "sin": case "sen": return "\\sin\\left(" + a + "\\right)";
                case "cos": return "\\cos\\left(" + a + "\\right)";
                case "tan": return "\\tan\\left(" + a + "\\right)";
                case "sinh": return "\\sinh\\left(" + a + "\\right)";
                case "cosh": return "\\cosh\\left(" + a + "\\right)";
                case "tanh": return "\\tanh\\left(" + a + "\\right)";
                case "ln": return "\\ln\\left(" + a + "\\right)";
                case "log": return "\\log\\left(" + a + "\\right)";
                case "arcsin": return "\\arcsin\\left(" + a + "\\right)";
                case "arccos": return "\\arccos\\left(" + a + "\\right)";
                case "arctan": return "\\arctan\\left(" + a + "\\right)";
                case "abs": return "\\left|" + a + "\\right|";
                case "sqrt": case "r": return "\\sqrt{" + a + "}";
                case "cbrt": case "c": return "\\sqrt[3]{" + a + "}";
                case "exp": case "E": return "e^{" + a + "}";
                default:
                    if (name.length() == 1) return name + "\\left(" + a + "\\right)";
                    return "\\operatorname{" + name + "}\\left(" + a + "\\right)";
            }
        }
        public int prec() { return ATOM; }
    }

    private static final class Unary implements Node {
        private final String op;
        private final Node a;
        Unary(String op, Node a) { this.op = op; this.a = a; }
        public String tex() { return op + wrap(a, UNARY); }
        public int prec() { return UNARY; }
    }

    private static final class Bin implements Node {
        private final String op;
        private final Node l, r;
        Bin(String op, Node l, Node r) { this.op = op; this.l = l; this.r = r; }

        public String tex() {
            switch (op) {
                case "/":
                    // Los paréntesis del numerador/denominador los aporta \frac.
                    return "\\frac{" + l.tex() + "}{" + r.tex() + "}";
                case "^":
                    return wrap(l, POW) + "^{" + r.tex() + "}";
                case "*":
                    return wrap(l, MUL) + " \\cdot " + wrap(r, MUL);
                case "-":
                    return wrap(l, ADD) + " - " + wrap(r, ADD + 1);
                default: // "+"
                    return wrap(l, ADD) + " + " + wrap(r, ADD);
            }
        }

        public int prec() {
            switch (op) {
                case "+": case "-": return ADD;
                case "*": case "/": return MUL;
                case "^": return POW;
                default: return ATOM;
            }
        }
    }

    // --------------------------------------------------------------- Parser
    private static final class Parser {
        private final String s;
        private int i;

        Parser(String s) { this.s = s; }

        Node parse() {
            Node n = expr();
            if (peek() != '\0') throw new RuntimeException("token inesperado");
            return n;
        }

        private char peek() {
            while (i < s.length() && Character.isWhitespace(s.charAt(i))) i++;
            return i < s.length() ? s.charAt(i) : '\0';
        }

        private boolean eat(char c) {
            if (peek() == c) { i++; return true; }
            return false;
        }

        private Node expr() {
            Node n = term();
            while (true) {
                char c = peek();
                if (c == '+' || c == '-') { i++; n = new Bin(String.valueOf(c), n, term()); }
                else return n;
            }
        }

        private Node term() {
            Node n = factor();
            while (true) {
                char c = peek();
                if (c == '*' || c == '/') {
                    i++;
                    n = new Bin(String.valueOf(c), n, factor());
                } else if (c == '(' || Character.isLetter(c) || Character.isDigit(c) || c == '.') {
                    // Multiplicación implícita: 2x, 2(x+1), x sin(x)
                    n = new Bin("*", n, factor());
                } else {
                    return n;
                }
            }
        }

        private Node factor() {
            Node n = unary();
            if (peek() == '^') { i++; n = new Bin("^", n, factor()); }
            return n;
        }

        private Node unary() {
            char c = peek();
            if (c == '-') { i++; return new Unary("-", unary()); }
            if (c == '+') { i++; return unary(); }
            return primary();
        }

        private Node primary() {
            char c = peek();
            if (c == '(') {
                i++;
                Node n = expr();
                if (!eat(')')) throw new RuntimeException("falta )");
                return n;
            }
            if (Character.isDigit(c) || c == '.') {
                int st = i;
                while (i < s.length() && (Character.isDigit(s.charAt(i)) || s.charAt(i) == '.')) i++;
                return new Num(s.substring(st, i));
            }
            if (Character.isLetter(c)) {
                int st = i;
                while (i < s.length() && Character.isLetter(s.charAt(i))) i++;
                String name = s.substring(st, i);
                if (peek() == '(') {
                    // nombre(...) -> aplicación de función (f(x), sin(x), ...)
                    i++;
                    Node arg = expr();
                    if (!eat(')')) throw new RuntimeException("falta ) en " + name);
                    return new Func(name, arg);
                }
                return new Var(name);
            }
            throw new RuntimeException("token inesperado: " + c);
        }
    }
}
