#include <cmath>
#include "mp_mystack.hpp"

// MathLexeme static members

// Priority [BinaryOp1][BinaryOp2]==true
// if BinaryOp1 has higher priority than BinaryOp2 in this context:
// <X> <BinaryOp1> <Y> <BinaryOp2> <Z>
//
constexpr bool MathLexeme::IsPriorityHigher[5][5] =
{
    {true,  true, false, false, false},
    {true,  true, false, false, false},
    {true,  true,  true,  true, false},
    {true,  true,  true,  true, false},
    {true,  true,  true,  true, false}
};

const wchar_t *const MathLexeme::FunctionID[MathLexNumberOfFunctions] =
{
    L"sqrt", L"exp", L"ln", L"lg", L"log",
    L"sin", L"cos", L"sec", L"csc", L"tg", L"ctg", L"tan", L"cot",
    L"arcsin", L"arccos", L"arcsec", L"arccsc", L"arctg", L"arcctg", L"arctan", L"arccot",
    L"sech", L"csch", L"sh", L"ch", L"th", L"cth", L"sinh", L"cosh", L"tanh", L"coth",
    L"arsh", L"arch", L"arth", L"arcth", L"arsech", L"arcsch",
    L"arcsinh", L"arccosh", L"arctanh", L"arccoth", L"arcsech", L"arccsch",
    L"abs", L"int"
};

constexpr bool MathLexeme::IsFunctionAllowed[MathLexNumberOfFunctions] =
{
    true, true, true, true, true, 
    true, true, true, true, true, true, true, true, 
    true, true, true, true, true, true, true, true, 
    true, true, true, true, true, true, true, true, true, true, 
    true, true, true, true, true, true, 
    true, true, true, true, true, true, 
    true, true        
};

constexpr double(*const MathLexeme::FunctionAddress[MathLexNumberOfFunctions])(double) =
{
    MathLexeme::mysqrt, MathLexeme::myexp, MathLexeme::myln, MathLexeme::mylg, 
    MathLexeme::mylog, MathLexeme::mysin, MathLexeme::mycos, MathLexeme::mysec,
    MathLexeme::mycsc, MathLexeme::mytg, MathLexeme::myctg, MathLexeme::mytan,
    MathLexeme::mycot, MathLexeme::myarcsin, MathLexeme::myarccos, MathLexeme::myarcsec,
    MathLexeme::myarccsc, MathLexeme::myarctg, MathLexeme::myarcctg, MathLexeme::myarctan,
    MathLexeme::myarccot, MathLexeme::mysech, MathLexeme::mycsch, MathLexeme::mysh,
    MathLexeme::mych, MathLexeme::myth, MathLexeme::mycth, MathLexeme::mysinh,
    MathLexeme::mycosh, MathLexeme::mytanh, MathLexeme::mycoth, MathLexeme::myarsh,
    MathLexeme::myarch, MathLexeme::myarth, MathLexeme::myarcth, MathLexeme::myarsech,
    MathLexeme::myarcsch, MathLexeme::myarcsinh, MathLexeme::myarccosh, MathLexeme::myarctanh,
    MathLexeme::myarccoth, MathLexeme::myarcsech, MathLexeme::myarccsch, MathLexeme::myabs,
    MathLexeme::myint
};

const wchar_t *const MathLexeme::ConstantID[MathLexNumberOfConstants]=
{
    L"pi", L"e"
};

constexpr bool MathLexeme::IsConstantAllowed[MathLexNumberOfConstants]=
{
    true, true
};

constexpr double MathLexeme::ConstantValue[MathLexNumberOfConstants]=
{
    3.14159265358979323846, 2.7182818284590452354
};

double MathLexeme::mysqrt (double x)
{
    return sqrt(x);
}

double MathLexeme::myexp (double x)
{
    return exp(x);
}

double MathLexeme::myln (double x)
{
    return log(x);
}

double MathLexeme::mylg (double x)
{
    return log10(x);
}

double MathLexeme::mylog (double x)
{
    return log10(x);
}

double MathLexeme::mysin (double x)
{
    return sin(x);
}

double MathLexeme::mycos (double x)
{
    return cos(x);
}

double MathLexeme::mysec (double x)
{
    return 1.0/cos(x);
}

double MathLexeme::mycsc (double x)
{
    return 1.0/sin(x);
}

double MathLexeme::mytg (double x)
{
    return tan(x);
}

double MathLexeme::myctg (double x)
{
    return 1.0/tan(x);
}

double MathLexeme::mytan (double x)
{
    return tan(x);
}

double MathLexeme::mycot (double x)
{
    return 1.0/tan(x);
}

double MathLexeme::myarcsin (double x)
{
    return asin(x);
}

double MathLexeme::myarccos (double x)
{
    return acos(x);
}

double MathLexeme::myarcsec (double x)
{
    return acos(1.0/x);
}

double MathLexeme::myarccsc (double x)
{
    return asin(1.0/x);
}

double MathLexeme::myarctg (double x)
{
    return asin(x/sqrt(1.0 + x*x));
}

double MathLexeme::myarcctg (double x)
{
    return acos(x/sqrt(1.0 + x*x));
}

double MathLexeme::myarctan (double x)
{
    return asin(x/sqrt(1.0 + x*x));
}

double MathLexeme::myarccot (double x)
{
    return acos(x/sqrt(1.0 + x*x));
}

double MathLexeme::mysech (double x)
{
    return 1.0/cosh(x);
}

double MathLexeme::mycsch (double x)
{
    return 1.0/sinh(x);
}

double MathLexeme::mysh (double x)
{
    return sinh(x);
}

double MathLexeme::mych (double x)
{
    return cosh(x);
}

double MathLexeme::myth (double x)
{
    return tanh(x);
}

double MathLexeme::mycth (double x)
{
    return 1.0/tanh(x);
}

double MathLexeme::mysinh (double x)
{
    return sinh(x);
}

double MathLexeme::mycosh (double x)
{
    return cosh(x);
}

double MathLexeme::mytanh (double x)
{
    return tanh(x);
}

double MathLexeme::mycoth (double x)
{
    return 1.0/tanh(x);
}

double MathLexeme::myarsh (double x)
{
    return log(x + sqrt(x*x + 1.0));
}

double MathLexeme::myarch (double x)
{
    return log(x + sqrt(x*x - 1.0));
}

double MathLexeme::myarth (double x)
{
    return log((1.0 + x)/(1.0 - x))/2.0;
}

double MathLexeme::myarcth (double x)
{
    return log((x + 1.0)/(x - 1.0))/2.0;
}

double MathLexeme::myarsech (double x)
{
    return log(1.0/x + sqrt(1.0/(x*x) - 1.0));
}

double MathLexeme::myarcsch (double x)
{
    return log(1.0/x + sqrt(1.0/(x*x) + 1.0));
}

double MathLexeme::myarcsinh (double x)
{
    return log(x + sqrt(x*x + 1.0));
}

double MathLexeme::myarccosh (double x)
{
    return log(x + sqrt(x*x - 1.0));
}

double MathLexeme::myarctanh (double x)
{
    return log((1.0 + x)/(1.0 - x))/2.0;
}

double MathLexeme::myarccoth (double x)
{
    return log((x + 1.0)/(x - 1.0))/2.0;
}

double MathLexeme::myarcsech (double x)
{
    return log(1.0/x + sqrt(1.0/(x*x) - 1.0));
}

double MathLexeme::myarccsch (double x)
{
    return log(1.0/x + sqrt(1.0/(x*x) + 1.0));
}

double MathLexeme::myabs (double x)
{
    return fabs(x);
}

double MathLexeme::myint (double x)
{
    return floor(x);
}
