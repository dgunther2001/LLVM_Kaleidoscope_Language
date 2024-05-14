#include "../include/kaleidoscope/lexer.h"

std::string IdentifierStr;
double NumVal;

// the entire implementation of the lexer...
// TODO => figure out how to take file input as opposed to just standard input...
int gettok() {
    static int LastChar = ' '; // sets the last character to an empty character

    while (isspace(LastChar)) { // SKIPS WHITESPACE
        // GET CHAR IS A DEFAULT C function that reads the next character from the standard input...
        LastChar = getchar();  // while the current character is whitespace (initialized like that) go to the next character
    }

    // all alphanumberic combinations in any order with as many as we want... => IDENTIFIERS
    if (isalpha(LastChar)) { // looking for identifiers now.. => gets more complex in here if we want string data types too...
        IdentifierStr = LastChar; // set the identifier string to the character brought in by the input stream...
        while (isalnum(LastChar = getchar())) { // while we iterate over the character stream, and it is still an alphanumeric...
            IdentifierStr += LastChar; // append the most recently read character onto the current Identifier
        }

        // after the Identifier has been read, check for special alphanumeric keywords...
        if (IdentifierStr == "def") { // if we are defining a function, return a tok_def
            return tok_def;
        }
        if  (IdentifierStr == "decl") { // if we are declaring a function, return a tok_decl
            return tok_decl;
        }
        if (IdentifierStr == "if") {
            return tok_if; // if we are statrting conditional control flow...
        }
        if (IdentifierStr == "then") {
            return tok_then; // if we are within conditional control flow
        }
        if (IdentifierStr == "else") {
            return tok_else; // if we are at the tail end of a condtional statement
        }
        if (IdentifierStr == "for") {
            return tok_for; // if we see a for statement return that token
        }
        if (IdentifierStr == "in") {
            return tok_in; // return a tok_in if the lexer catches that keyword
        }

        // if we have an alphanumeric stream and it's not a keyword, it must be an identifier, so return the appropriate token
        return tok_identifier;
    }

    // if its a digit (OUR ONLY DATA TYPE...)
    if (isdigit(LastChar) || LastChar == '.') { 
        std::string NumStr; // declare a temporary input string for the number to be stored in...
        //bool has_decimal_pt = false; // declares a boolean that indicates whether a decimal point already exists in the token...
        do {
            //if (LastChar == '.' && has_decimal_pt == false) { // if the current character is a '.' and we have not found one in our token yet...
                //has_decimal_pt = true; // inddicate that a decimal point has been included
            //} 
            // if (LastChar == '.' && has_decimal_pt == true) { // if we find a second decimal point...
            //     throw std::runtime_error("Invalid input: More than one '.' in a number"); // throw a runtime error
            // } 


            NumStr += LastChar; // append the last character to the input stream string
            LastChar = getchar(); // get the next character
        } while (isdigit(LastChar) || LastChar == '.'); // so long as the new character is a digit, or a '.', keep looping

        NumVal = strtod(NumStr.c_str(), nullptr); // converts the NumStr to a double precision float, the 0 indicates that we don't need to tell it where to stop parsing, as the string is finite...
        return tok_number; // return a tok_number, as that is the type we have read in
    }

    // if we see the character #, we ignore the rest of the line until a '\n' character
    if (LastChar == '#') { // if we hit a '#'
        do {
            LastChar = getchar(); // keep chugging through input until...
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r'); // we hit the end of the file, a newline, or a reset

        if (LastChar != EOF) { // if we're not at the end of the file...
            return gettok(); // calls the function again at the next line
        }
    }

    // this deals with finding the end of the file...
    if (LastChar == EOF) { // if the current stream inputs an EOF character...
        return tok_eof; // return a tok_eof, which indicates we are at the end of the program
    }

    // if the character matchs none of our tokens just spit out it's ASCII value
    int ThisChar = LastChar; // get the ASCII value of the character
    LastChar = getchar(); // get the next character
    return ThisChar; // return the ASCII value of the character

}