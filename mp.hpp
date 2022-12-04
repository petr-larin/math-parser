//
// MathParser class declaration
//

#pragma once

#include <vector>
#include <string>
#include "mp_mystack.hpp"

using std::vector;
using std::wstring;

struct CompiledCommand;

//
//
// MathParser class can parse, evaluate and compile math expressions such as
//     (sin(x1)^2 + cos(y1)^2)^(-1) + z
// 
// It also serves as a container for the expressions, user variable names and
// compiled code.
// 
// More details below.
//
//

// Allow/disallow empty strings (spaces/tabs) as grammatically correct
// (they'll evaluate to 0)
#define MATH_PARSER_EMPTY_STRING_ALLOWED

// "**" as a power sign allowed/not allowed (the regular power sign '^' is always allowed)
#define MATH_PARSER_DOUBLE_ASTERISK_ALLOWED

// ':' as a division sign allowed/not allowed (the regular division sign '/' is always allowed)
#define MATH_PARSER_COLON_ALLOWED

// Makes the parser check for floating point errors including constants
#define MATH_PARSER_CHECK_FOR_FLOATING_POINT_ERRORS

class MathParser {

public:

    // Codes returned by Parse, Evaluate, Compile, Execute, CheckVar
    // and CheckAndInsertVar

    enum ErrorCodes {                   // used by:

        OK,                             // Pa, Ev, Co, Ex, CV, CIV
        WrongIndex,                     // Pa, Ev, Co
        InvalidNumber,                  // Pa, Ev, Co
        UnknownIdentifier,              // Pa, Ev, Co
        InvalidIdentifier,              // CV, CIV
        InvalidCharacter,               // Pa, Ev, Co
        EmptyString,                    // Pa, Ev, Co
        ExpectedRealFunUnSignLeftPar,   // Pa, Ev, Co
        ExpectedRealFunLeftPar,         // Pa, Ev, Co
        ExpectedBiSignRightPar,         // Pa, Ev, Co
        ExpectedBiSign,                 // Pa, Ev, Co
        ExpectedLeftPar,                // Pa, Ev, Co
        ExpectedRightPar,               // Pa, Ev, Co
        ExtraRightPar,                  // Pa, Ev, Co
        FloatingPointErrorPosInf,       // Ev, Co, Ex
        FloatingPointErrorNegInf,       // Ev, Co, Ex
        FloatingPointErrorNaN,          // Ev, Co, Ex
        IdentifierInUse                 // CV, CIV
    };

    explicit MathParser(bool case_sensitive);
    MathParser() = delete;
    ~MathParser() = default;

    MathParser(const MathParser&) = delete;
    MathParser(MathParser&&) = delete;
    MathParser& operator = (const MathParser&) = delete;
    MathParser& operator = (MathParser&&) = delete;

    //
    //
    // Main functionality of the class: Parse/Evaluate/Compile/Execute.
    // iIndex refers to the nth string stored in the MathParser object.
    // In case of error, its position is stored in iErrorPosition.
    //
    // Compile/Execute are meant to be used together, and the result is 
    // identical to Evaluate. You can Compile a string once and then Execute
    // it multiple times with different argument sets, which is more efficient
    // than calling Execute for each argument set.
    //
    //

    // Check the syntax of a string with the specified index without evaluating it.
    // Parse does not require user variables to be inserted.
    // Returns OK if the syntax is OK, otherwise -- one of the error codes above.
    //
    ErrorCodes Parse(size_t& iErrorPosition, size_t iIndex = 0);

    // Evaluate the string with the specified index.
    // The values of the user-defined arguments need supplied in args.
    // On success returns OK and stores the result in dValue.
    // Otherwise, returns one of the error codes above.
    //
    ErrorCodes Evaluate(
        size_t& iErrorPosition, const vector<double>& args,
        double& dValue, size_t iIndex = 0);

    // Compile the string with the specified index into internal code to be used
    // by Execute. On success returns OK, otherwise returns one of the
    // error codes above. 
    //
    ErrorCodes Compile(size_t& iErrorPosition, size_t iIndex = 0);

    // Execute the internal code produced by Compile with the same index.
    // The values of the user-defined arguments need supplied in args.
    // Client should check that Compile has returned OK.
    // On success returns OK and stores the result in dValue.
    // Otherwise, returns FloatingPointError if this option is enabled.
    // No checks on validity of pointers - use IsExecutionAllowed prior to.
    //
    ErrorCodes Execute(
        size_t& iErrorPosition, const vector<double>& args,
        double& dValue, size_t iIndex = 0);

    //
    // 
    // Before strings can be Parsed/Evaluated/Compiled/Executed, they need inserted
    // into the MathParser object. These functions handle manipulations with strings.
    //
    //

    // Insert a string at the position designated by iRequestedIndex.
    // The internal semantics is a list of strings, therefore, iRequestedIndex
    // should be in the range [0, NumberOfStrings() - 1]. If its value is higher
    // then the string will be inserted at the position NumberofStrings().
    // The actual assigned index is returned in iAssignedIndex.
    //
    void InsertString(
        wstring&& wstr, size_t iRequestedIndex, size_t& iAssignedIndex);

    // Return nth stored string using vector's at().
    // Should be 0 <= iIndex < NumberOfStrings(),
    // otherwise vector throws an exception.
    //
    const wchar_t* String(size_t iIndex) const;

    // Return nth stored string using vector's operator[]
    // should be 0 <= iIndex < NumberOfStrings(),
    // otherwise behavior undefined.
    //
    const wchar_t* operator[](size_t iIndex) const;
    
    // Remove nth string.
    // Should be 0 <= iIndex < NumberOfStrings()
    // otherwise throws an exception.
    //
    void RemoveString(size_t iIndex);

    // The current number of stored strings
    //
    size_t NumberOfStrings() const;

    //
    //
    // Strings can contain user-defined variables, such as x1 in "sin(x1)^2".
    // Such user-defined variables need inserted into the MathParser object.
    // User-defined variable names can contain letters, digits and underscore, and
    // cannot begin with a digit.
    //
    //

    // A helper to remove leading and trailing blanks (' ' and '\t').
    // Returns the length of the resulting string.
    //
    static size_t TrimVarName(wstring&);

    // Check if this is a valid variable name.
    // Return value =
    //    = InvalidIdentifier if wstr does not contain a valid variable name
    //    = IdentifierInUse if wstr contains a valid name but it is already in use
    //    = otherwise it is OK
    //
    ErrorCodes CheckVar(const wstring&) const;

    // Check if this is a valid variable name, and if so, insert it.
    // Return value =
    //    = InvalidIdentifier if wstr does not contain a valid variable name
    //    = IdentifierInUse if wstr contains a valid name but it is already in use
    //    = otherwise it is OK
    //
    // If OK then the variable is inserted.
    //
    ErrorCodes CheckAndInsertVar(
        wstring&& wstr, size_t iRequestedIndex, size_t& iAssignedIndex);

    // Return nth stored variable name
    //
    const wchar_t* Var(size_t iIndex) const;

    // Remove nth variable.
    // Should be 0 <= iIndex < NumberOfStrings()
    // otherwise throws an exception.
    //
    void RemoveVar(size_t iIndex);

    // Remove all variables.
    //
    void RemoveAllVars();

    // The current number of stored strings
    //
    size_t NumberOfVars() const;
    
    //
    //
    // Other functions
    //
    //
    
    // Check if nth string can be Executed (internal representation contains valid
    // code). No need to do this check if the last call to Compile on
    // the same string returned OK.
    //
    bool OKtoExecute(size_t iIndex = 0) const;

    // Case sensitivity is set in the constructor, and can be checked/changed at runtime.
    //
    bool IsCaseSensitive() const;
    void SetCaseSensitive(bool case_sensitive);

private:

    int mp_str_cmp(const wchar_t*, const wchar_t*) const;
    static inline bool IsFirstChar(wchar_t);
    static inline bool IsNextChar(wchar_t);
    bool FunctionExists(const wchar_t*, size_t* = nullptr) const;
    bool ConstantExists(const wchar_t*, size_t* = nullptr) const;
    bool VariableExists(const wchar_t*, size_t* = nullptr) const;
    bool VarNameInUse(const wchar_t*) const;
    static bool IsVarNameValid(const wstring&);
    void EvaluateBinaryOp();
    static inline void CheckForFloatingPointError(double);

    enum LexerMode {ParseMode, EvaluateMode, CompileMode};
    void GetLexCheckSyntax(
        MathParser::LexerMode LexerMode,
        size_t& iFirstSymbol,
        const wchar_t* pString,
        wchar_t* pBuffer,
        size_t& iCurrentPosition,
        size_t& iBufferPosition,
        MathLexeme& CurrentLexeme,
        MathLexeme& PreviousLexeme,
        long long int& iParBalance,
        const double* rgdArguments
    );

    void AdjustRunTimeMem();
    void CompileBinaryOp();
    void InvalidateCompiledCode(size_t);

    vector<wstring> input_strings;
    vector<wstring> user_vars;
    MyStack         Stack;
    vector<wchar_t> lexer_buffer;
    bool            case_sensitive;

    static const bool Expected[8][8];

    vector<vector<CompiledCommand>> compiled_code;
    vector<double>  runtime_mem;
    size_t          iCommandCounter; // counter of produced commands
    size_t          iMemoryCounter;  // counter of used runtime memory
    CompiledCommand*pCompiledCode;   // shortcut to the member of CompiledCode being used
};

// Internal representation of commands used by Compile/Execute
//
struct CompiledCommand {

    friend class MathParser;

    CompiledCommand() = default; // used by std::vector allocators

private:

    // different signatures are used to produce different types of commands
    //
    CompiledCommand(
        size_t iFirstOperand, double dValue, size_t iResult,
        size_t iBinaryOp, size_t iErrorPosition);
    CompiledCommand(
        double dValue, size_t iSecondOperand, size_t iResult,
        size_t iBinaryOp, size_t iErrorPosition);
    CompiledCommand(
        size_t iFirstOperand, size_t iSecondOperand, size_t iResult,
        size_t iBinaryOp, size_t iErrorPosition);
    CompiledCommand(
        size_t iFirstOperand, size_t iResult, double (*pFunction)(double),
        size_t iErrorPosition);
    CompiledCommand(size_t iFirstOperand);
    CompiledCommand(double dValue);
    CompiledCommand(bool);

    // 3 flags indicate the type of the command
    // mem+const:       f1==true, f2==true, (f3==false)
    // const+mem:       f1==true, f2==false, (f3==false)
    // mem+mem:         f2==false, f2==true, f3 not used
    // function call:   f1==false, f2==false, f3==true
    // end:             f1==false, f2==false, f3==false
    // 
    // f3 is not used by MathParser::Execute in the case of mem+const
    // or const+mem command, but it should be set to false for these
    // commands, because the "command" with the following combination:
    //
    // error:			f1==true, f3==true
    //
    // will be output in case of unsuccessful compilation.
    // OKtoExecute will check for this combination.

    const bool      fFlag1{}, fFlag2{}, fFlag3{};

    const size_t    iFirstOperand{}, iSecondOperand{}, iResult{};
    const double    dValue{};
    const size_t    iBinaryOp{};
    double          (*const pFunction)(double) {};
    const bool      fResultInMemory{}; // == true if the result of Execute should be taken from memory
    const size_t    iErrorPosition{};
};

inline const wchar_t* MathParser::String(size_t iIndex) const
{
    return input_strings.at(iIndex).data();
}

inline const wchar_t* MathParser::operator[](size_t iIndex) const
{
    return input_strings[iIndex].data();
}

inline size_t MathParser::NumberOfStrings() const
{
    return input_strings.size();
}

inline const wchar_t* MathParser::Var(size_t iIndex) const
{
    return user_vars[iIndex].data();
}

inline size_t MathParser::NumberOfVars() const
{
    return user_vars.size();
}

inline int MathParser::mp_str_cmp(const wchar_t* str1, const wchar_t* str2) const
{
    return case_sensitive ? wcscmp(str1, str2) : _wcsicmp(str1, str2);
}

inline bool MathParser::IsFirstChar(wchar_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool MathParser::IsNextChar(wchar_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '1' && c <= '9') || c == '0' || c == '_';
}

// will produce mem+const command
inline CompiledCommand::CompiledCommand(
    size_t iFirstOperand, double dValue, size_t iResult,
    size_t iBinaryOp, size_t iErrorPosition)
    : iFirstOperand(iFirstOperand), dValue(dValue), iResult(iResult),
    iBinaryOp(iBinaryOp), fFlag1(true), fFlag2(true), fFlag3(false),
    iErrorPosition(iErrorPosition)
{
}

// will produce const+mem command
inline CompiledCommand::CompiledCommand(
    double dValue, size_t iSecondOperand, size_t iResult,
    size_t iBinaryOp, size_t iErrorPosition)
    : dValue(dValue), iSecondOperand(iSecondOperand), iResult(iResult),
    iBinaryOp(iBinaryOp), fFlag1(true), fFlag2(false), fFlag3(false),
    iErrorPosition(iErrorPosition)
{
}

// will produce mem+mem command
inline CompiledCommand::CompiledCommand(
    size_t iFirstOperand, size_t iSecondOperand, size_t iResult,
    size_t iBinaryOp, size_t iErrorPosition)
    : iFirstOperand(iFirstOperand), iSecondOperand(iSecondOperand), iResult(iResult),
    iBinaryOp(iBinaryOp), fFlag1(false), fFlag2(true),
    iErrorPosition(iErrorPosition)
{
}

// will produce function call command
inline CompiledCommand::CompiledCommand(
    size_t iFirstOperand, size_t iResult, double (*pFunction)(double), size_t iErrorPosition)
    : iFirstOperand(iFirstOperand), iResult(iResult), pFunction(pFunction),
    fFlag1(false), fFlag2(false), fFlag3(true), iErrorPosition(iErrorPosition)
{
}

// will produce end command, final result taken from memory
inline CompiledCommand::CompiledCommand(size_t iFirstOperand)
    : iFirstOperand(iFirstOperand), fFlag1(false), fFlag2(false), fFlag3(false),
    fResultInMemory(true)
{
}

// will produce end command, final result taken is known at the time of compilation
inline CompiledCommand::CompiledCommand(double dValue)
    : dValue(dValue), fFlag1(false), fFlag2(false), fFlag3(false),
    fResultInMemory(false)
{
}

// will produce error token
inline CompiledCommand::CompiledCommand(bool) : fFlag1(true), fFlag3(true)
{
}