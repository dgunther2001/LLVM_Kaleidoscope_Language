#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <istream>
#include <fstream>

extern std::istream* input;

enum Token { // defines the different types of tokens the lexer can return as an enumerated value
    tok_eof = -1, // the end of file, or end of token stream token

    // commands for functuon declaration and definition
    tok_def = -2, // function definitions
    tok_decl = -3, // function declarations

    // literals and identifiers
    tok_identifier = -4, // variable identifiers
    tok_number = -5, // numbers => 64-bit floating pt, the only type supported by kaleidoscope...

    // conditionals
    tok_if = -6, // if statements
    tok_then = -7, // then, which should be followed by an else
    tok_else = -8, // the else statement

    // for loop tokens
    tok_for = -9,
    tok_in = 10,

    // user defined operator tokens
    tok_binary = -11, // for binary operators
    tok_unary = -12, // for unary operators
    // ADD MORE HERE LIKE STRINGS, ETC...
}; // returns unknown tokens as their ASCII values

extern std::string IdentifierStr; // utilized if we get an identifier (ALWAYS A STRING) => when tok_identifier is used, this is where we store the data
extern double NumVal; // utilized for the value stored in a particular identifier => tok_number in the case of kaleidoscope, but is expandable

int gettok(); // declares the tokenizer function

#endif

