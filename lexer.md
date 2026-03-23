# Lexer Design (scanner.l and main.cpp)

This project uses **Flex** to generate a scanner that recognizes the tokens of a mini-language. The design is split into three main parts: the definition (`scanner.l`), the entry point (`main.cpp`), and the token definitions (`tokens.h`).

## 1. Scanner Definition (`scanner.l`)

The `scanner.l` file is divided into three sections:

### **Definitions Section (`%{ ... %}`)**

- Contains C++ headers (`iostream`, `vector`, `string`) and `tokens.h`.
- Defines the `emit()` function, which:
  1. Creates a `Token` object.
  2. Appends it to the global `g_tokens` vector.
  3. Prints the token to `stdout` for immediate feedback.

### **Rules Section (`%% ... %%`)**

- Uses **Regular Expressions** to define how different strings should be recognized as tokens.
- **Keywords**: `"int"`, `"if"`, `"else"`, etc., are matched exactly.
- **Identifiers**: `[a-zA-Z_][a-zA-Z0-9_]*` matches any word starting with a letter/underscore.
- **Integers**: `[0-9]+` matches sequences of digits.
- **String Literals**: `\"[^\"]*\"` matches anything between double quotes.
- **Operators**: `"=="`, `"<<"`, `"+"`, etc.
- **Actions**: Each rule calls `emit(T_TYPE, yytext)` to process the matched text.

### **User Code Section**

- Left empty here, but can contain additional C++ helper functions.

---

## 2. Main Controller (`main.cpp`)

The `main.cpp` file manages the overall execution flow:

- **Input Handling**: It checks if a filename was provided as a command-line argument. If so, it opens the file and sets `yyin` to point to it.
- **Scanning**: It calls `yylex()`. This invokes the Flex-generated state machine, which consumes the entire input file and populates the `g_tokens` list.
- **Output Generation**: After scanning is complete, it calls `writeTokensToFile()`. This function iterates through `g_tokens` and writes each token to `tokens.out`.

---

## 3. Token Definitions (`tokens.h`)

This header file defines the **shared interface** between the scanner and any future components (like a parser).

- **`enum TokenType`**: Lists all unique types of tokens recognized (e.g., `T_INT`, `T_ID`, `T_ASSIGN`).
- **`struct Token`**: A simple data structure that pairs a `TokenType` with its **lexeme** (the actual string found in the source code).
- **`tokenTypeName()`**: A utility function that returns the string representation of an enum value (e.g., converting `T_PLUS` to `"T_PLUS"`).

---

## How They Work Together

1.**`main.cpp`** opens the source file and calls **`yylex()`**.
2.**`yylex()`** (inside **`scanner.cpp`**) reads the file character by character.
3.When it finds a match based on the rules in **`scanner.l`**, it calls **`emit()`**.
4.**`emit()`** adds the new token to a global vector called **`g_tokens`**.
5.When the file ends, **`yylex()`** returns to **`main.cpp`**.
6.**`main.cpp`** then writes all the collected tokens from **`g_tokens`** into **`tokens.out`**.

### **Listing of Tokens**

The tokens are listed in two formats:

- **`stdout`**: Printed during scanning for debugging.
- **`tokens.out`**: A structured file where each line contains either a simple token type (e.g., `T_PLUS`) or a type with its value (e.g., `T_ID(x)` or `T_INTEGER(42)`).
