#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>

#define int long long


char *p, *lp,   // current position in source code
     *data;     // data/bss pointer

int *e, *le,    // current position in emitted code
    *id,        // currently parsed identifier
    *sym,       // symbol table (simple list of identifiers)
    tk,         // current token
    ival,       // current token value
    ty,         // current expression type
    loc,        // local variable offset
    line,       // current line number
    src,        // print source and assembly flag
    debug;      // print executed instructions


// tokens and classes (operators last and in precedence order)
enum
{
    Num = 128, Fun, Sys, Glo, Loc, Id, 
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, 
    Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};
/*
Assign  =
Cond    ?
Lor     ||
Lan     &&
Or      |
Xor     ^
And     &
Eq      ==
Ne      !=
Lt      <
Gt      >
Le      <=
Ge      >=
Shl     <<
Shr     >>
Add     +
Sub     -
Mul     *
Div     /
Mod     %
Inc     ++
Dec     --
Brak    [
*/

// opcodes
enum
{
    LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
    OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
    OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT
};

// types
enum { CHAR, INT, PTR };

// identifier offsets (since we can't create an ident struct)
enum { Tk, Hash, Name, Class, Type, Val, HClass, HType, HVal, Idsz };


// Lexical Analyzer
void next() {
    char *pp;

    // use loop to ignore blank and some characters that it cannot recognize
    while (tk = *p) {
        ++p;
        if (tk == '\n') {
            // if use "-s" option in command line
            if (src) {
                printf("%d: %.*s", line, p - lp, lp);
                lp = p;
                while (le < e) {
                    printf("%8.4s", &"LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
                           "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                           "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,"[*++le * 5]);
                    if (*le <= ADJ)
                        printf(" %d\n", *++le);
                    else
                        printf("\n");
                }
            }
            ++line;
        }
        // ignore line with "#"
        else if (tk == '#') {
            while (*p != 0 && *p != '\n') ++p;
        }
        else if ((tk >= 'a' && tk <= 'z') || (tk >= 'A' && tk <= 'Z') || tk == '_') {
            // pp points to the first initials of the symbol
            pp = p - 1;

            while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_')
                // calculate the hash value of the token
                tk = tk * 147 + *p++;
            
            // hash value plus the length of the symbol
            tk = (tk << 6) + (p - pp);

            // traverse the sym array(identifier table)
            id = sym;
            while (id[Tk]) {
                // find the same one
                if (tk == id[Hash] && !memcmp((char *)id[Name], pp, p - pp)) {
                    tk = id[Tk];
                    return;
                }
                // not found, go ahead
                id = id + Idsz;
            }
            // not found? well we find a new identifier
            id[Name] = (int)pp; 
            id[Hash] = tk;      // hash value
            tk = id[Tk] = Id;   // type of token is identifier
            return;
        }
        // if the first bit is a number, it must be a numerical value
        else if (tk >= '0' && tk <= '9') {
            // dec
            if (ival = tk - '0') {
                while (*p >= '0' && *p <= '9')
                    ival = ival * 10 + *p++ - '0';
            }
            // hex
            else if (*p == 'x' || *p == 'X') {
                while ((tk = *++p) && ((tk >= '0' && tk <= '9') || (tk >= 'a' && tk <= 'f') || (tk >= 'A' && tk <= 'F')))
                    ival = ival * 16 + (tk & 15) + (tk >= 'A' ? 9 : 0);
            }
            // oct
            else { 
                while (*p >= '0' && *p <= '7')
                    ival = ival * 8 + *p++ - '0';
            }

            tk = Num;
            return;
        }
        else if (tk == '/') {
            // annotation
            if (*p == '/') {
                ++p;
                while (*p != 0 && *p != '\n') ++p;
            }
            // division
            else {
                tk = Div;
                return;
            }
        }
        // start with ' or ", it must a `char` or `string`
        else if (tk == '\'' || tk == '"') {
            pp = data;

            while (*p != 0 && *p != tk) {
                if ((ival = *p++) == '\\') {
                    // line feed: '\n'
                    if ((ival = *p++) == 'n')
                        ival = '\n';
                }
                // if is ", means it is a string, then copy characters to data
                if (tk == '"')
                    *data++ = ival;
            }
            ++p;
            if (tk == '"')
                ival = (int)pp;
            else
                tk = Num;

            return;
        }
        else if (tk == '=') { if (*p == '=') { ++p; tk = Eq; } else tk = Assign; return; }
        else if (tk == '+') { if (*p == '+') { ++p; tk = Inc; } else tk = Add; return; }
        else if (tk == '-') { if (*p == '-') { ++p; tk = Dec; } else tk = Sub; return; }
        else if (tk == '!') { if (*p == '=') { ++p; tk = Ne; } return; }
        else if (tk == '<') { if (*p == '=') { ++p; tk = Le; } else if (*p == '<') { ++p; tk = Shl; } else tk = Lt; return; }
        else if (tk == '>') { if (*p == '=') { ++p; tk = Ge; } else if (*p == '>') { ++p; tk = Shr; } else tk = Gt; return; }
        else if (tk == '|') { if (*p == '|') { ++p; tk = Lor; } else tk = Or; return; }
        else if (tk == '&') { if (*p == '&') { ++p; tk = Lan; } else tk = And; return; }
        else if (tk == '^') { tk = Xor; return; }
        else if (tk == '%') { tk = Mod; return; }
        else if (tk == '*') { tk = Mul; return; }
        else if (tk == '[') { tk = Brak; return; }
        else if (tk == '?') { tk = Cond; return; }
        else if (tk == '~' || tk == ';' || tk == '{' || tk == '}' || tk == '(' || tk == ')' || tk == ']' || tk == ',' || tk == ':') return;
    }
}


// Expression Analyzer
void expr(int lev) {
    int t, *d;

    if (!tk) {
        printf("%d: unexpected eof in expression\n", line);
        exit(-1);
    }
    // Take the immediate number directly as the value of the expression
    else if (tk == Num) {
        *++e = IMM;
        *++e = ival;
        next();
        ty = INT;
    }
    // consecutive '"' in C-style multiline text, such as ["abc" "def"]
    else if (tk == '"') {
        *++e = IMM;
        *++e = ival;
        next();
        while (tk == '"') next();
        data = (char *)((int)data + sizeof(int) & -sizeof(int));    // Byte alignment to int
        ty = PTR;
    }
    else if (tk == Sizeof) {
        next();
        if (tk == '(')
            next();
        else {
            printf("%d: open paren expected in sizeof\n", line);
            exit(-1);
        }
        ty = INT;
        if (tk == Int)
            next();
        else if (tk == Char) {
            next();
            ty = CHAR;
        }
        // multi-level pointer
        while (tk == Mul) {
            next();
            ty = ty + PTR;
        }
        if (tk == ')')
            next();
        else {
            printf("%d: close paren expected in sizeof\n", line);
            exit(-1);
        }
        *++e = IMM;
        // the size of multi-level pointer is same as int's, except char
        *++e = (ty == CHAR) ? sizeof(char) : sizeof(int);
        ty = INT;
    }
    // identifier
    else if (tk == Id) {
        d = id;
        next();
        // function
        if (tk == '(') {
            next();
            t = 0;  // the number of arguments

            /* calculate the value of arguments, 
               then push on the stack(pass the arguments) */
            while (tk != ')') {
                expr(Assign);
                *++e = PSH;
                ++t;
                if (tk == ',')
                    next();
            }
            next();
            // syscall, such as `malloc`, `memset`, d[Val] is opcode
            if (d[Class] == Sys)
                *++e = d[Val];
            // user defined function, d[Val] is function entry address
            else if (d[Class] == Fun) {
                *++e = JSR;
                *++e = d[Val];
            }
            else {
                printf("%d: undefined variable\n", line);
                exit(-1);
            }
            *++e = ((ty = d[Type]) == CHAR) ? LC : LI;
        }
    }
    else if (tk == '(') {
        next();
        if (tk == Int || tk == Char) {  // cast
            t = (tk == Int) ? INT : CHAR;
            // pointer
            while (tk == Mul) {
                next();
                t = t + PTR;    
            }
            if (tk == ')')
                next();
            else {
                printf("%d: bad cast\n", line);
                exit(-1);
            }
            expr(Inc);  // higher priority
            ty = t;
        }
        else {  // common grammar brackets
            expr(Assign);
            if (tk == ')')
                next();
            else {
                printf("%d: close paren expected\n", line);
                exit(-1);
            }
        }
    }
    // get the value pointed by the pointer
    else if (tk == Mul) {
        next();
        expr(Inc);  // higher priority
        if (ty > INT)
            ty = ty - PTR;
        else {
            printf("%d: bad dereference\n", line);
            exit(-1);
        }
        *++e = (ty == CHAR) ? LC : LI;
    }
    // &, get address operation
    else if (tk == And) {
        next();
        expr(Inc);
        /*
        According to the above code, 
        when the token is a variable, 
        the address is taken first and then `LI/LC`, 
        so `--e` becomes the address to `a`
        */
        if (*e == LC || *e == LT)
            --e;
        else {
            printf("%d: bad address-of\n", line);
            exit(-1);
        }
        ty = ty + PTR;
    }
    else if (tk == '!') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = 0; *++e = EQ; ty = INT; }   // !x is equivalent to x==0
    else if (tk == '~') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = -1; *++e = XOR; ty = INT; } // ~x is equivalent to x^-1
    else if (tk == Add) { next(); expr(Inc); ty = INT; }
    else if (tk == Sub) {
        next();
        *++e = IMM;
        // if it is a Num, get its negative value
        if (tk == Num) {
            *++e = -ival;
            next();
        }
        // multiplied with `-1`
        else {
            *++e = -1;
            *++e = PSH;
            expr(Inc);
            *++e = MUL;
            ty = INT;
        }
    }
    // processing ++x / --x / x-- / x++
    else if (tk == Inc || tk == Dec) {
        t = tk;
        next();
        expr(Inc);
        // processing ++x, --x
        if (*e == LC) {
            // push it to stack(SC/SI below will use it), then get value
            *e = PSH;
            *++e = LC;
        }
        else if (*e == LI) {
            *e = PSH;
            *++e = LI;
        }
        else {
            printf("%d: bad lvalue in pre-increment\n", line);
            exit(-1);
        }
        // push it to stack
        *++e = PSH;
        *++e = IMM;
        // pointer plus/subtract an Integer or a character
        *++e = (ty > PTR) ? sizeof(int) : sizeof(char);
        *++e = (t == Inc) ? ADD : SUB;
        *++e = (ty == CHAR) ? SC : SI;
    }
    else {
        printf("%d: bad lvalue in pre-increment\n", line);
        exit(-1);
    }

    // "precedence climbing" or "Top Down Operator Precedence" method
    // `tk` is an ASCII code, and it will not greater than Num=128
    while (tk >= lev) {
        t = tk; // make a copy
        // assign value
        if (tk == Assign) {
            next();
            if (*e == LC || *e == LI)
                *e = PSH;
            else {
                printf("%d: bad lvalue in pre-increment\n", line);
                exit(-1);
            }
            expr(Assign);
            *++e = ((ty = t) == CHAR) ? SC : SI;
        }
        // process token " ? : "
        else if (tk == Cond) {
            next();
            *++e = BZ;
            d = ++e;
            expr(Assign);
            if (tk == ':')
                next();
            else {
                printf("%d: conditional missing colon\n", line);
                exit(-1);
            }
            *d = (int)(e + 3);
            *++e = JMP;
            d = ++e;
            expr(Cond);
            *d = (int)(e + 1);
        }

        else if (tk == Lor) {
            next();
            // add opcode `BNZ` to the top of the code array
            *++e = BNZ;         // which means if true, it will jump to other position
            d = ++e;            // `d` points to the next position which stores the address will jump to
            expr(Lan);          // analyze an expression
            *d = (int)(e + 1);  // calculate the address which will jump to (current position plus 1), and put it in `d`
            ty = INT;           // set the type of result INTEGER
        }
        // same as above
        else if (tk == Lan) { next(); *++e = BZ;  d = ++e; expr(Or);  *d = (int)(e + 1); ty = INT; }
        // push current code to the stack, then analyze the next code, finally analyze them(with the top of the stack) together
        else if (tk == Or)  { next(); *++e = PSH; expr(Xor); *++e = OR;  ty = INT; }
        else if (tk == Xor) { next(); *++e = PSH; expr(And); *++e = XOR; ty = INT; }
        else if (tk == And) { next(); *++e = PSH; expr(Eq);  *++e = AND; ty = INT; }
        else if (tk == Eq)  { next(); *++e = PSH; expr(Lt);  *++e = EQ;  ty = INT; }
        else if (tk == Ne)  { next(); *++e = PSH; expr(Lt);  *++e = NE;  ty = INT; }
        else if (tk == Lt)  { next(); *++e = PSH; expr(Shl); *++e = LT;  ty = INT; }
        else if (tk == Gt)  { next(); *++e = PSH; expr(Shl); *++e = GT;  ty = INT; }
        else if (tk == Le)  { next(); *++e = PSH; expr(Shl); *++e = LE;  ty = INT; }
        else if (tk == Ge)  { next(); *++e = PSH; expr(Shl); *++e = GE;  ty = INT; }
        else if (tk == Shl) { next(); *++e = PSH; expr(Add); *++e = SHL; ty = INT; }
        else if (tk == Shr) { next(); *++e = PSH; expr(Add); *++e = SHR; ty = INT; }
    }
}
