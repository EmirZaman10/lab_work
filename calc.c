// Emir Zaman 251ADB048 - emir zaman lab
// github.com/EmirZaman10/calc-lab-home
// Compile with gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: C man pages; Gemini for power function: https://share.gemini.google/aSuLEVdusaDN


#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define MY_NAME     "emir"
#define MY_LASTNAME "zaman"
#define MY_ID       "251ADB048"

#define NUM    1
#define PLUS   2
#define MINUS  3
#define MUL    4
#define DIV    5
#define POW    6
#define LPAR   7
#define RPAR   8
#define END    9

struct Number {
    int isFloat;
    long long i;
    double d;
};

char *text;
long lineEnd;      
long pos;          
int tok;           
long tokStart;     
struct Number tokNum;
int error;         
long errorPos;     

char *out;
long outLen;
long outCap;

// realloc doubling make big files fast and save memory without slow down.
void addOutput(char *s)
{
    long n = strlen(s);
    while (outLen + n + 1 > outCap) {
        if (outCap == 0)
            outCap = 1024;
        else
            outCap = outCap * 2;
        out = realloc(out, outCap);
        if (out == NULL) {
            printf("out of memory\n");
            exit(1);
        }
    }
    strcpy(out + outLen, s);
    outLen = outLen + n;
}

// error pos show start of bad token so test can match corectly.
void setError(long index)
{
    if (error == 0) {
        error = 1;
        errorPos = index + 1; 
    }
}

// tokenizer track pos and start to find exact place with spaces and lines.
void nextToken(void)
{
    while (pos < lineEnd && (text[pos] == ' ' || text[pos] == '\t'))
        pos++;

    tokStart = pos;

    if (pos >= lineEnd) {
        tok = END;
        return;
    }

    char c = text[pos];

    if (isdigit((unsigned char)c)) {
        long start = pos;
        while (pos < lineEnd && isdigit((unsigned char)text[pos]))
            pos++;

        if (pos + 1 < lineEnd && text[pos] == '.' && isdigit((unsigned char)text[pos + 1])) {
            pos++;
            while (pos < lineEnd && isdigit((unsigned char)text[pos]))
                pos++;

            long n = pos - start;
            char small[100];
            char *tmp = small;
            if (n >= 100)
                tmp = malloc(n + 1);
            memcpy(tmp, text + start, n);
            tmp[n] = '\0';
            tokNum.isFloat = 1;
            tokNum.d = strtod(tmp, NULL);
            tokNum.i = 0;
            if (tmp != small)
                free(tmp);
        } else {
            long long value = 0;
            for (long k = start; k < pos; k++)
                value = value * 10 + (text[k] - '0');
            tokNum.isFloat = 0;
            tokNum.i = value;
            tokNum.d = 0;
        }
        tok = NUM;
        return;
    }

    pos++;
    if (c == '+') tok = PLUS;
    else if (c == '-') tok = MINUS;
    else if (c == '/') tok = DIV;
    else if (c == '(') tok = LPAR;
    else if (c == ')') tok = RPAR;
    else if (c == '*') {
        if (pos < lineEnd && text[pos] == '*') {
            pos++;
            tok = POW;
        } else {
            tok = MUL;
        }
    } else {
        tok = END;
        setError(tokStart);
    }
}

long long intPower(long long a, long long b)
{
    unsigned long long base = a;
    unsigned long long result = 1;
    unsigned long long e = b;
    while (e > 0) {
        if (e % 2 == 1)
            result = result * base;
        base = base * base;
        e = e / 2;
    }
    return (long long)result;
}

// if one number is float we use double math otherwise we keep integer.
struct Number calculate(struct Number a, int op, struct Number b, long opIndex, long rightIndex)
{
    struct Number r;
    r.isFloat = 0;
    r.i = 0;
    r.d = 0;

    if (a.isFloat == 0 && b.isFloat == 0) {
        if (op == PLUS) r.i = a.i + b.i;
        if (op == MINUS) r.i = a.i - b.i;
        if (op == MUL) r.i = a.i * b.i;
        if (op == DIV) {
            if (b.i == 0) {
                setError(rightIndex);
                return r;
            }
            r.i = a.i / b.i;
        }
        if (op == POW) {
            if (b.i < 0) {
                setError(rightIndex);
                return r;
            }
            r.i = intPower(a.i, b.i);
        }
        return r;
    }

    double x, y, res = 0;
    if (a.isFloat) x = a.d; else x = (double)a.i;
    if (b.isFloat) y = b.d; else y = (double)b.i;

    if (op == PLUS) res = x + y;
    if (op == MINUS) res = x - y;
    if (op == MUL) res = x * y;
    if (op == DIV) {
        if (y == 0.0) {
            setError(rightIndex);
            return r;
        }
        res = x / y;
    }
    if (op == POW) res = pow(x, y);

    if (!isfinite(res)) {
        setError(opIndex);
        return r;
    }
    r.isFloat = 1;
    r.d = res;
    return r;
}

struct Number expr(void);

struct Number primary(void)
{
    struct Number v;
    v.isFloat = 0;
    v.i = 0;
    v.d = 0;

    if (tok == NUM) {
        v = tokNum;
        nextToken();
        return v;
    }

    if (tok == LPAR) {
        nextToken();
        if (error) return v;
        v = expr();
        if (error) return v;
        if (tok != RPAR) {
            setError(tokStart);
            return v;
        }
        nextToken();
        return v;
    }

    setError(tokStart);
    return v;
}

// i used gemini to make power right associative so it call itself for exponent.
struct Number power(void)
{
    struct Number base = primary();
    if (error || tok != POW)
        return base;

    long opIndex = tokStart;
    nextToken();
    if (error) return base;
    long rightIndex = tokStart;

    struct Number e = power(); 
    if (error) return base;

    return calculate(base, POW, e, opIndex, rightIndex);
}

// grammar use functions to make multiply and divide tighter than plus and minus.
struct Number term(void)
{
    struct Number v = power();
    while (error == 0 && (tok == MUL || tok == DIV)) {
        int op = tok;
        long opIndex = tokStart;
        nextToken();
        if (error) break;
        long rightIndex = tokStart;
        struct Number r = power();
        if (error) break;
        v = calculate(v, op, r, opIndex, rightIndex);
    }
    return v;
}

struct Number expr(void)
{
    struct Number v = term();
    while (error == 0 && (tok == PLUS || tok == MINUS)) {
        int op = tok;
        long opIndex = tokStart;
        nextToken();
        if (error) break;
        long rightIndex = tokStart;
        struct Number r = term();
        if (error) break;
        v = calculate(v, op, r, opIndex, rightIndex);
    }
    return v;
}

void evaluateText(long len)
{
    long i = 0;
    outLen = 0;
    error = 0;

    while (i < len) {
        long e = i;
        while (e < len && text[e] != '\n')
            e++;

        long k = i;
        while (k < e && (text[k] == ' ' || text[k] == '\t'))
            k++;

        if (k < e && text[k] != '#') {
            lineEnd = e;
            pos = k;
            nextToken();

            struct Number v;
            v.isFloat = 0;
            v.i = 0;
            v.d = 0;
            if (error == 0)
                v = expr();

            if (error == 0 && tok != END)
                setError(tokStart);

            char line[100];
            if (error) {
                outLen = 0;
                sprintf(line, "ERROR:%ld\n", errorPos);
                addOutput(line);
                return;
            }
            if (v.isFloat)
                sprintf(line, "%.15g\n", v.d);
            else
                sprintf(line, "%lld\n", v.i);
            addOutput(line);
        }
        i = e + 1;
    }
}

int doFile(char *inPath, char *outDir)
{
    struct stat st;
    if (stat(inPath, &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "cannot open input file %s\n", inPath);
        return 0;
    }

    FILE *f = fopen(inPath, "rb");
    if (f == NULL) {
        fprintf(stderr, "cannot open input file %s\n", inPath);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len < 0) {
        fprintf(stderr, "cannot read input file %s\n", inPath);
        fclose(f);
        return 0;
    }
    text = malloc(len + 1);
    if (text == NULL) {
        fprintf(stderr, "out of memory\n");
        fclose(f);
        return 0;
    }
    len = fread(text, 1, len, f);
    if (ferror(f)) {
        fprintf(stderr, "cannot read input file %s\n", inPath);
        fclose(f);
        free(text);
        return 0;
    }
    fclose(f);

    evaluateText(len);
    free(text);

    if (mkdir(outDir, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "cannot create folder %s\n", outDir);
        return 0;
    }

    char *name = strrchr(inPath, '/');
    if (name == NULL)
        name = inPath;
    else
        name = name + 1;
    long n = strlen(name);
    if (n >= 4 && strcmp(name + n - 4, ".txt") == 0)
        n = n - 4;

    char *outPath = malloc(strlen(outDir) + n + 100);
    if (outPath == NULL) {
        fprintf(stderr, "out of memory\n");
        return 0;
    }
    sprintf(outPath, "%s/%.*s_%s_%s_%s.txt", outDir, (int)n, name, MY_NAME, MY_LASTNAME, MY_ID);

    FILE *o = fopen(outPath, "wb");
    if (o == NULL) {
        fprintf(stderr, "cannot write %s\n", outPath);
        free(outPath);
        return 0;
    }
    if (outLen > 0)
        fwrite(out, 1, outLen, o);
    int ok = 1;
    if (fclose(o) != 0) {
        fprintf(stderr, "cannot write %s\n", outPath);
        ok = 0;
    }
    free(outPath);
    return ok;
}

int doFolder(char *dir, char *outDir)
{
    DIR *d = opendir(dir);
    if (d == NULL) {
        fprintf(stderr, "cannot open folder %s\n", dir);
        return 0;
    }

    char **files = NULL;
    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        long n = strlen(ent->d_name);
        if (n < 4 || strcmp(ent->d_name + n - 4, ".txt") != 0)
            continue;

        char *path = malloc(strlen(dir) + n + 2);
        if (path == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        sprintf(path, "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
            free(path);
            continue;
        }
        files = realloc(files, (count + 1) * sizeof(char *));
        if (files == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        files[count] = path;
        count++;
    }
    closedir(d);

    int ok = 1;
    for (int k = 0; k < count; k++) {
        if (doFile(files[k], outDir) == 0)
            ok = 0;
        free(files[k]);
    }
    free(files);
    return ok;
}

int main(int argc, char *argv[])
{
    char *dir = NULL;
    char *outDir = NULL;
    char *input = NULL;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "-d") == 0 || strcmp(argv[k], "--dir") == 0) {
            if (k + 1 >= argc) {
                fprintf(stderr, "missing folder after %s\n", argv[k]);
                return 1;
            }
            k++;
            dir = argv[k];
        } else if (strcmp(argv[k], "-o") == 0 || strcmp(argv[k], "--output-dir") == 0) {
            if (k + 1 >= argc) {
                fprintf(stderr, "missing folder after %s\n", argv[k]);
                return 1;
            }
            k++;
            outDir = argv[k];
        } else if (argv[k][0] == '-') {
            fprintf(stderr, "unknown option %s\n", argv[k]);
            return 1;
        } else {
            if (input != NULL) {
                fprintf(stderr, "only one input file please\n");
                return 1;
            }
            input = argv[k];
        }
    }

    if (dir == NULL && input == NULL) {
        fprintf(stderr, "usage: calc [-d DIR] [-o OUTDIR] [input.txt]\n");
        return 1;
    }
    if (dir != NULL && input != NULL) {
        fprintf(stderr, "use -d or an input file, not both\n");
        return 1;
    }

    char defaultOut[1100];
    if (outDir == NULL) {
        char *src;
        if (dir != NULL)
            src = dir;
        else
            src = input;

        char base[1000];
        snprintf(base, sizeof(base), "%s", src);
        long n = strlen(base);
        while (n > 1 && base[n - 1] == '/') {
            base[n - 1] = '\0';
            n--;
        }
        char *b = strrchr(base, '/');
        if (b == NULL)
            b = base;
        else
            b = b + 1;
        n = strlen(b);
        if (dir == NULL && n >= 4 && strcmp(b + n - 4, ".txt") == 0)
            b[n - 4] = '\0';

        snprintf(defaultOut, sizeof(defaultOut), "%s_%s_%s", b, MY_NAME, MY_ID);
        outDir = defaultOut;
    }

    int ok;
    if (dir != NULL)
        ok = doFolder(dir, outDir);
    else
        ok = doFile(input, outDir);

    free(out);
    if (ok)
        return 0;
    return 1;
}