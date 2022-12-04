//
// Stack of lexemes used by MathParser
//

#pragma once

#include <math.h>

class MathLexeme {
    
    friend class MyStack;
    friend class MathParser;

    enum MathLexType {

        Begin       = 0,
        End         = 1,
        Number      = 2,
        Function    = 3,
        Unary       = 4,
        Binary      = 5,
        LeftPar     = 6,
        RightPar    = 7
    };

    enum MathLexNumItem { Constant, Variable };
    enum MathLexBiItem { Plus = 0, Minus = 1, Multiply = 2, Divide = 3, Power = 4 };

    MathLexeme() = default;
    explicit MathLexeme(MathLexType iType);
    MathLexeme(MathLexType iType, int iItem);
    MathLexeme(MathLexType iType, int iItem, double dValue);

    MathLexType iType{};                // type of lexeme
    int         iItem{ 0 };             // sub-type
    double      dValue{ 0.0 };          // value of a variable (for Number/Constant)
    double      (*pFunction)(double) { nullptr };  // pointer to a function (for Function)
    size_t      iPosition{ 0 };         // position in the string (for error reporting)
    size_t      iRunTimeIndex{ 0 };     // index of value stored in RunTimeMemory

    static constexpr size_t MathLexNumberOfFunctions{ 45 };
    static constexpr size_t MathLexNumberOfConstants{ 2 };

    static const bool IsPriorityHigher[5][5];

    static double (*const BinaryOpAddress[5])(double, double);

    static const wchar_t *const FunctionID[MathLexNumberOfFunctions];
    static const bool IsFunctionAllowed[MathLexNumberOfFunctions];
    static double (*const FunctionAddress[MathLexNumberOfFunctions])(double);

    static const wchar_t *const ConstantID[MathLexNumberOfConstants];
    static const bool IsConstantAllowed[MathLexNumberOfConstants];
    static const double ConstantValue[MathLexNumberOfConstants];

    static double mysqrt(double);
    static double myexp(double);
    static double myln(double);
    static double mylg(double);
    static double mylog(double);
    static double mysin(double);
    static double mycos(double);
    static double mysec(double);
    static double mycsc(double);
    static double mytg(double);
    static double myctg(double);
    static double mytan(double);
    static double mycot(double);
    static double myarcsin(double);
    static double myarccos(double);
    static double myarcsec(double);
    static double myarccsc(double);
    static double myarctg(double);
    static double myarcctg(double);
    static double myarctan(double);
    static double myarccot(double);
    static double mysech(double);
    static double mycsch(double);
    static double mysh(double);
    static double mych(double);
    static double myth(double);
    static double mycth(double);
    static double mysinh(double);
    static double mycosh(double);
    static double mytanh(double);
    static double mycoth(double);
    static double myarsh(double);
    static double myarch(double);
    static double myarth(double);
    static double myarcth(double);
    static double myarsech(double);
    static double myarcsch(double);
    static double myarcsinh(double);
    static double myarccosh(double);
    static double myarctanh(double);
    static double myarccoth(double);
    static double myarcsech(double);
    static double myarccsch(double);
    static double myabs(double);
    static double myint(double);
};

inline MathLexeme::MathLexeme(MathLexType iType) : iType(iType)
{
}

inline MathLexeme::MathLexeme(MathLexType iType, int iItem)
    : iType(iType), iItem(iItem)
{
}

inline MathLexeme::MathLexeme(MathLexType iType, int iItem, double dValue)
    : iType(iType), iItem(iItem), dValue(dValue)
{
}

class MyStack { // unsafe but fast
    
    friend class MathParser;

    MyStack() noexcept;
    ~MyStack();
    void Resize(size_t iNewSize);
    operator MathLexeme* () const;
    void Push(const MathLexeme& Lexeme);
    void Pop();
    void Pop(MathLexeme& Lexeme);
    MathLexeme Top();
    MathLexeme SecondFromTop();
    void Reset();

    MathLexeme* pMemory;
    size_t      iTopOfStack;
    size_t      iCurrentStackSize;
};

inline MyStack::MyStack() noexcept
    : pMemory{ nullptr }, iTopOfStack{ 0 }, iCurrentStackSize{ 0 }
{
}

inline MyStack::~MyStack()
{
    delete *this;
}

inline void MyStack::Resize(size_t iNewSize)
{
    delete *this;
    pMemory = new MathLexeme[iNewSize * sizeof(MathLexeme)]{};
    iCurrentStackSize = iNewSize;
}

inline MyStack::operator MathLexeme* () const
{
    return pMemory;
}

inline void MyStack::Push(const MathLexeme& Lexeme)
{
    pMemory[iTopOfStack++] = Lexeme;    
}

inline void MyStack::Pop()
{
    --iTopOfStack;
}

inline void MyStack::Pop(MathLexeme& Lexeme)
{
    Lexeme = pMemory[--iTopOfStack];
}

inline MathLexeme MyStack::Top()
{
    return pMemory[iTopOfStack - 1];
}

inline MathLexeme MyStack::SecondFromTop()
{
    return pMemory[iTopOfStack - 2];
}

inline void MyStack::Reset()
{
    iTopOfStack = 0;
}