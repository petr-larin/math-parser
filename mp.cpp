#include "mp.hpp"

// MathParser static member initialization
// Expected[Lexeme1][Lexeme2]=
//    = true if Lexeme2 may follow Lexeme1,
//    = false otherwise

constexpr bool MathParser::Expected[8][8]=
{
    {false,
#ifdef MATH_PARSER_EMPTY_STRING_ALLOWED            
             true,
#else //MATH_PARSER_EMPTY_STRING_ALLOWED
            false,
#endif //MATH_PARSER_EMPTY_STRING_ALLOWED
                    true,  true,  true, false,  true, false},
    {false, false, false, false, false, false, false, false},
    {false,  true, false, false, false,  true, false,  true},
    {false, false, false, false, false, false,  true, false},
    {false, false,  true,  true, false, false,  true, false},
    {false, false,  true,  true, false, false,  true, false},
    {false, false,  true,  true,  true, false,  true, false},
    {false,  true, false, false, false,  true, false,  true}
};

MathParser::MathParser(bool case_sensitive) :
    input_strings{}, user_vars{}, Stack{}, lexer_buffer{},
    case_sensitive{ case_sensitive }, compiled_code{}, runtime_mem{},
    iCommandCounter{ 0 }, iMemoryCounter{ 0 }, pCompiledCode{ nullptr }
{
}

MathParser::ErrorCodes MathParser::Parse(size_t& iErrorPosition, size_t iIndex)
{
    size_t iFirstSymbol{ 0 };

    try
    {
        if (iIndex >= input_strings.size()) throw WrongIndex;

        const auto (*const pString) { input_strings[iIndex].data() };
        auto (*const pBuffer) { lexer_buffer.data() };
        size_t iCurrentPosition{ 0 }, iBufferPosition{};
        MathLexeme CurrentLexeme{ MathLexeme::Begin }, PreviousLexeme{};
        long long int iParBalance{ 0 };

        do
            GetLexCheckSyntax(
                ParseMode,
                iFirstSymbol,
                pString,
                pBuffer,
                iCurrentPosition,
                iBufferPosition,
                CurrentLexeme,
                PreviousLexeme,
                iParBalance,
                nullptr);

        while (CurrentLexeme.iType != MathLexeme::End);

        // Check if there've been enough right parentheses

        if (iParBalance > 0) throw ExpectedRightPar;

        // syntax OK!

        iErrorPosition = iCurrentPosition;
        return OK;

    }//try

    catch (const MathParser::ErrorCodes& iCaught)
    {
        iErrorPosition = iFirstSymbol;
        return iCaught;
    }
}

MathParser::ErrorCodes MathParser::Evaluate(
    size_t& iErrorPosition, const vector<double>& args,
	double& dValue, size_t iIndex)
{
    size_t iFirstSymbol{ 0 };

    try
    {
        if (iIndex >= input_strings.size()) throw WrongIndex;
    
        const auto (*const pString) { input_strings[iIndex].data() };
        auto (*const pBuffer) { lexer_buffer.data() };
        size_t iCurrentPosition{ 0 }, iBufferPosition{};
        MathLexeme CurrentLexeme{ MathLexeme::Begin }, PreviousLexeme{};
        long long int iParBalance{ 0 };
        const double* rgdArguments{ args.data() };

        Stack.Push (CurrentLexeme);

        do
        {
            GetLexCheckSyntax(
                EvaluateMode,
                iFirstSymbol,
                pString,
                pBuffer,
                iCurrentPosition,
                iBufferPosition,
                CurrentLexeme,
                PreviousLexeme,
                iParBalance,
                rgdArguments);

	        // Check if there've been enough right parentheses

			if (CurrentLexeme.iType == MathLexeme::End)
				if (iParBalance > 0) throw ExpectedRightPar;

            // Now evaluate

            switch (CurrentLexeme.iType)
            {
            case MathLexeme::Number:
            case MathLexeme::LeftPar:
            case MathLexeme::Function:
                Stack.Push(CurrentLexeme);
                break;

            case MathLexeme::Binary:
            {
                bool fMore{ true };

                do
                {
                    const auto SecondFromTop = Stack.SecondFromTop();

                    switch (SecondFromTop.iType)
                    {
                    case MathLexeme::Begin:
                    case MathLexeme::LeftPar:
                        fMore = false;
                        break;

                    default:
                    // case MathLexeme::Binary:
                        if (MathLexeme::IsPriorityHigher
                            [SecondFromTop.iItem]
                            [CurrentLexeme.iItem]) 
                            {
                                iFirstSymbol = Stack.SecondFromTop().iPosition;
                                EvaluateBinaryOp();
                            }
                        else fMore = false;

                    } // switch

                } while (fMore);

                Stack.Push(CurrentLexeme);

            } // case

            break;

            case MathLexeme::RightPar:
            {
                while (Stack.SecondFromTop().iType == MathLexeme::Binary)
                {
                    iFirstSymbol = Stack.SecondFromTop().iPosition;
                    EvaluateBinaryOp();
                }

                MathLexeme Tmp;
                Stack.Pop(Tmp);
                Stack.Pop(); // remove '('

                if (Stack.Top().iType == MathLexeme::Function)
                {
                    Tmp.dValue = Stack.Top().pFunction(Tmp.dValue);
                    
                    iFirstSymbol = Stack.Top().iPosition;

                    CheckForFloatingPointError(Tmp.dValue);

                    Stack.Pop(); // remove function
                }
                Stack.Push(Tmp);
            }
            break;

            case MathLexeme::End:
                while (Stack.SecondFromTop().iType == MathLexeme::Binary)
                {
                    iFirstSymbol = Stack.SecondFromTop().iPosition;
                    EvaluateBinaryOp();
                }
            break;

            default:
            // case MathLexeme::Unary:
                if (CurrentLexeme.iItem == MathLexeme::Minus)
                {
                    Stack.Push(MathLexeme(MathLexeme::Number, MathLexeme::Constant, 0.0));
                    Stack.Push(MathLexeme(MathLexeme::Binary, MathLexeme::Minus));
                }

            } // switch

        } // do
        while (CurrentLexeme.iType != MathLexeme::End);

	    // syntax OK!
        iErrorPosition = iCurrentPosition;
        dValue = Stack.Top().dValue;
        Stack.Reset();
        return OK;

    }//try

    catch (const MathParser::ErrorCodes& iCaught)
    {
        iErrorPosition = iFirstSymbol;
        Stack.Reset();
        return iCaught;
    }
}

MathParser::ErrorCodes MathParser::Compile(size_t& iErrorPosition, size_t iIndex)
{
    size_t iFirstSymbol{ 0 };

    try
    {
        if (iIndex >= input_strings.size()) throw WrongIndex;

        const auto (*const pString) { input_strings[iIndex].data() };
        auto (*const pBuffer) { lexer_buffer.data() };
        size_t iCurrentPosition{ 0 }, iBufferPosition{};
        MathLexeme CurrentLexeme{ MathLexeme::Begin }, PreviousLexeme{};
        long long int iParBalance{ 0 };

        pCompiledCode = compiled_code[iIndex].data();
		iCommandCounter = 0; // will count commands produced by compiler

		// Will count runtime memory used for storage of intermediate values.
		// Values of user variables passed as arguments will be stored in the beginning.
		// Memory won't be reused.
		iMemoryCounter = NumberOfVars();

        Stack.Push(CurrentLexeme);

        do
        {
            GetLexCheckSyntax(
                CompileMode,
                iFirstSymbol,
                pString,
                pBuffer,
                iCurrentPosition,
                iBufferPosition,
                CurrentLexeme,
                PreviousLexeme,
                iParBalance,
                nullptr);

			// Check if there've been enough right parentheses
			
            if (CurrentLexeme.iType == MathLexeme::End)
				if (iParBalance > 0) throw ExpectedRightPar;

            // Now compile

            switch (CurrentLexeme.iType)
            {
            case MathLexeme::Number:
            case MathLexeme::LeftPar:
            case MathLexeme::Function:
                Stack.Push(CurrentLexeme);
            break;

            case MathLexeme::Binary:
            {
                bool fMore{ true };

                do // unwind stack
                {
                    const auto SecondFromTop = Stack.SecondFromTop();

                    switch (SecondFromTop.iType)
                    {
                    case MathLexeme::Begin:
                    case MathLexeme::LeftPar:
                        fMore = false;
                    break;

                    default:
                    // case MathLexeme::Binary:
                        if (MathLexeme::IsPriorityHigher
                            [SecondFromTop.iItem]
                            [CurrentLexeme.iItem])
                            {
                                iFirstSymbol = Stack.SecondFromTop().iPosition;
                                CompileBinaryOp();
                            }
                        else fMore = false;

                    } // switch

                } while (fMore);

                Stack.Push(CurrentLexeme);

            } // case MathLexeme::Binary

            break;

            case MathLexeme::RightPar:
            {
                while (Stack.SecondFromTop().iType == MathLexeme::Binary)
                {
                    iFirstSymbol = Stack.SecondFromTop().iPosition;
                    CompileBinaryOp();
                }

                MathLexeme Tmp;
                Stack.Pop(Tmp);
                Stack.Pop(); // remove '('
                
                if (Stack.Top().iType == MathLexeme::Function)
                {
                    if (Tmp.iItem == MathLexeme::Constant)
                    {
                        // argument of the function is known, no need to compile

                        Tmp.dValue = Stack.Top().pFunction(Tmp.dValue);

                        iFirstSymbol = Stack.Top().iPosition;

                        CheckForFloatingPointError(Tmp.dValue);
                    }
                    else
                    {
                        // argument of the function not known

                        pCompiledCode[iCommandCounter++].
                            CompiledCommand::CompiledCommand(Tmp.iRunTimeIndex, iMemoryCounter,
                                Stack.Top().pFunction, Stack.Top().iPosition);

                        Tmp.iRunTimeIndex = iMemoryCounter++;
                    }
					Stack.Pop(); // remove function
                }
                Stack.Push(Tmp);
            } //case MathLexeme::RightPar:

            break;

            case MathLexeme::End:
                while (Stack.SecondFromTop().iType == MathLexeme::Binary)
                {
                    iFirstSymbol = Stack.SecondFromTop().iPosition;
                    CompileBinaryOp();
                }
            break;

            default:
            // case MathLexeme::Unary:
                if (CurrentLexeme.iItem == MathLexeme::Minus)
                {
                    Stack.Push(MathLexeme(MathLexeme::Number, MathLexeme::Constant, 0.0));
                    Stack.Push(MathLexeme(MathLexeme::Binary, MathLexeme::Minus));
                }

            } // switch
        
        } // do
        while (CurrentLexeme.iType != MathLexeme::End);

        // syntax OK!
        iErrorPosition = iCurrentPosition;

        if (Stack.Top().iItem == MathLexeme::Constant)
            pCompiledCode[iCommandCounter++].
                CompiledCommand::CompiledCommand(Stack.Top().dValue);
        else
            pCompiledCode[iCommandCounter++].
                CompiledCommand::CompiledCommand(Stack.Top().iRunTimeIndex);

        Stack.Reset();
        return OK;

    }//try

    catch (const MathParser::ErrorCodes& iCaught)
    {
        iErrorPosition = iFirstSymbol;
        Stack.Reset();
        InvalidateCompiledCode(iIndex);
        return iCaught;
    }
}

MathParser::ErrorCodes MathParser::Execute(
    size_t& iErrorPosition, const vector<double>& args, double& dValue, size_t iIndex)
{
    auto (*pCmdPtr) { compiled_code[iIndex].data() };
    auto (*const pMemPtr) { runtime_mem.data() };
    memcpy(pMemPtr, args.data(), sizeof(double) * NumberOfVars());

loop:

    if (pCmdPtr->fFlag1)
    {
        // mem+const or const+mem

        auto iBinaryOp{ pCmdPtr->iBinaryOp };

        if (pCmdPtr->fFlag2)
        {
            // mem+const

            switch (iBinaryOp)
            {
            case MathLexeme::Plus:
                pMemPtr[pCmdPtr->iResult] = pMemPtr[pCmdPtr->iFirstOperand] + pCmdPtr->dValue;
                break;
            case MathLexeme::Minus:
                pMemPtr[pCmdPtr->iResult] = pMemPtr[pCmdPtr->iFirstOperand] - pCmdPtr->dValue;
                break;
            case MathLexeme::Multiply:
                pMemPtr[pCmdPtr->iResult] = pMemPtr[pCmdPtr->iFirstOperand] * pCmdPtr->dValue;
                break;
            case MathLexeme::Divide:
                pMemPtr[pCmdPtr->iResult] = pMemPtr[pCmdPtr->iFirstOperand] / pCmdPtr->dValue;
                break;
            default:
                //case MathLexeme::Power:
                pMemPtr[pCmdPtr->iResult] =
                    pow(pMemPtr[pCmdPtr->iFirstOperand], pCmdPtr->dValue);
            }
        }
        else // fFlag2
        {
            // const+mem

            switch (iBinaryOp)
            {
            case MathLexeme::Plus:
                pMemPtr[pCmdPtr->iResult] = pCmdPtr->dValue + pMemPtr[pCmdPtr->iSecondOperand];
                break;
            case MathLexeme::Minus:
                pMemPtr[pCmdPtr->iResult] = pCmdPtr->dValue - pMemPtr[pCmdPtr->iSecondOperand];
                break;
            case MathLexeme::Multiply:
                pMemPtr[pCmdPtr->iResult] = pCmdPtr->dValue * pMemPtr[pCmdPtr->iSecondOperand];
                break;
            case MathLexeme::Divide:
                pMemPtr[pCmdPtr->iResult] = pCmdPtr->dValue / pMemPtr[pCmdPtr->iSecondOperand];
                break;
            default:
                //case MathLexeme::Power:
                pMemPtr[pCmdPtr->iResult] =
                    pow(pCmdPtr->dValue, pMemPtr[pCmdPtr->iSecondOperand]);
            }
        }
    }
    else // fFlag1
    {
        if (pCmdPtr->fFlag2)
        {
            // mem+mem

            auto iBinaryOp{ pCmdPtr->iBinaryOp };
            switch (iBinaryOp)
            {
            case MathLexeme::Plus:
                pMemPtr[pCmdPtr->iResult] =
                    pMemPtr[pCmdPtr->iFirstOperand] + pMemPtr[pCmdPtr->iSecondOperand];
                break;
            case MathLexeme::Minus:
                pMemPtr[pCmdPtr->iResult] =
                    pMemPtr[pCmdPtr->iFirstOperand] - pMemPtr[pCmdPtr->iSecondOperand];
                break;
            case MathLexeme::Multiply:
                pMemPtr[pCmdPtr->iResult] =
                    pMemPtr[pCmdPtr->iFirstOperand] * pMemPtr[pCmdPtr->iSecondOperand];
                break;
            case MathLexeme::Divide:
                pMemPtr[pCmdPtr->iResult] =
                    pMemPtr[pCmdPtr->iFirstOperand] / pMemPtr[pCmdPtr->iSecondOperand];
                break;
            default:
                //case MathLexeme::Power:
                pMemPtr[pCmdPtr->iResult] =
                    pow(pMemPtr[pCmdPtr->iFirstOperand], pMemPtr[pCmdPtr->iSecondOperand]);
            }
        }
        else // fFlag2
        {
            if (pCmdPtr->fFlag3)
            {
                // function call
                pMemPtr[pCmdPtr->iResult] =
                    pCmdPtr->pFunction(pMemPtr[pCmdPtr->iFirstOperand]);
            }
            else // fFlag3
            {
                // termination

                if (pCmdPtr->fResultInMemory)
                {
                    dValue = pMemPtr[pCmdPtr->iFirstOperand];
                    return OK;
                }
                else
                {
                    dValue = pCmdPtr->dValue;
                    return OK;
                }
            }
        }
    }

#ifdef MATH_PARSER_CHECK_FOR_FLOATING_POINT_ERRORS

    if (!isfinite(pMemPtr[pCmdPtr->iResult]))
    {
        iErrorPosition = pCmdPtr->iErrorPosition;

        if (isnan(pMemPtr[pCmdPtr->iResult]))
            return FloatingPointErrorNaN;
        else
            if (pMemPtr[pCmdPtr->iResult] > 0)
                return FloatingPointErrorPosInf;
            else
                return FloatingPointErrorNegInf;
    }

#endif //MATH_PARSER_CHECK_FOR_FLOATING_POINT_ERRORS

    ++pCmdPtr;
    goto loop;
}

void MathParser::InsertString(
    wstring&& wstr, size_t requested_index, size_t& assigned_index)
{
    if (requested_index >= input_strings.size()) requested_index = input_strings.size();
    assigned_index = requested_index;

    const auto str_len = wstr.size();

    input_strings.insert(input_strings.begin() + requested_index, move(wstr));

    // allocate memory for lexer
    // the buffer is created, and allocates initial memory, when InsertString is called
    // for the first time.

    constexpr size_t min_buf_len = 128; // 1 == test value, to be increased
    size_t required_buf_len{ str_len + 1 };

    if (required_buf_len < min_buf_len) required_buf_len = min_buf_len;

    if (lexer_buffer.size() < required_buf_len)
    {
        constexpr size_t buf_len_extra{ 128 }; // 0 == test value, to be increased
        const auto current_buf_len = required_buf_len + buf_len_extra;

        lexer_buffer.resize(current_buf_len);
    }

    // the stack is created when InsertString is called for the first time
    // + adjust stack size when necessary

    auto required_stack_size = str_len + 2; // was 2*iNewStringLength + 2

    if (Stack.iCurrentStackSize < required_stack_size)
    {
        constexpr size_t stack_size_extra{ 128 }; // 0 == test value, to be increased
        required_stack_size += stack_size_extra;

        Stack.Resize(required_stack_size);
    }

    // allocate memory for compiled code

    compiled_code.insert(
        compiled_code.begin() + requested_index,
        vector<CompiledCommand>(str_len + 2)
    );

    InvalidateCompiledCode(assigned_index);

    // allocate/adjust memory for run-time data (arguments and intermediate storage)

    AdjustRunTimeMem();
}

void MathParser::RemoveString(size_t iIndex)
{
    input_strings.erase(input_strings.begin() + iIndex);
    compiled_code.erase(compiled_code.begin() + iIndex);
}

size_t MathParser::TrimVarName(wstring& wstr)
{
    auto str_len{ wstr.size() };
    size_t it{ 0 };

    for (; it < str_len; ++it)
        if (wstr[it] != ' ' && wstr[it] != '\t')
            break;

    if (it > 0)
    {
        wstr.erase(0, it);
        str_len = wstr.size();
    }

    if (str_len > 0)
    {
        it = str_len - 1;

        while (wstr[it] == ' ' || wstr[it] == '\t') --it;

        if (it < str_len - 1)
        {
            wstr.erase(it + 1);
            str_len = wstr.size();
        }
    }
    return str_len;
}

MathParser::ErrorCodes MathParser::CheckVar(const wstring& wstr) const
{
    // check if wstr is a valid ID
    if (!IsVarNameValid(wstr)) return InvalidIdentifier;

    // check if in use
    if (VarNameInUse(wstr.data())) return IdentifierInUse;

    return OK;
}

MathParser::ErrorCodes MathParser::CheckAndInsertVar(
    wstring&& wstr, size_t requested_index, size_t& assigned_index)
{
    const auto err_code{ CheckVar(wstr) };

    if (err_code != OK) return err_code;

    if (requested_index >= user_vars.size())
        requested_index = user_vars.size();

    assigned_index = requested_index;

    user_vars.insert(user_vars.begin() + requested_index, move(wstr));

    AdjustRunTimeMem();

    return OK;
}

void MathParser::RemoveVar(size_t iIndex)
{
    user_vars.erase(user_vars.begin() + iIndex);
}

void MathParser::RemoveAllVars()
{
    user_vars.clear();
}

bool MathParser::OKtoExecute(size_t iIndex) const
{
    if (iIndex >= compiled_code.size()) return false;

    const auto code = compiled_code[iIndex].data();

    if (code == nullptr) return false; // never happens?

    // check for error token
    return !code->fFlag1 || !code->fFlag3;
}

bool MathParser::IsCaseSensitive() const { return case_sensitive; }

void MathParser::SetCaseSensitive(bool case_sensitive)
{
    this->case_sensitive = case_sensitive;
}

bool MathParser::FunctionExists(const wchar_t* name, size_t* index) const
{
    for (size_t it = 0; it < MathLexeme::MathLexNumberOfFunctions; ++it)
        if (MathLexeme::IsFunctionAllowed[it])
            if (mp_str_cmp(name, MathLexeme::FunctionID[it]) == 0)
            {
                if (index != nullptr) *index = it;
                return true;
            }

    return false;
}

bool MathParser::ConstantExists(const wchar_t* name, size_t* index) const
{
    for (size_t it = 0; it < MathLexeme::MathLexNumberOfConstants; ++it)
        if (MathLexeme::IsConstantAllowed[it])
            if (mp_str_cmp(name, MathLexeme::ConstantID[it]) == 0)
            {
                if (index != nullptr) *index = it;
                return true;
            }

    return false;
}

bool MathParser::VariableExists(const wchar_t* name, size_t* index) const
{
    for (size_t it = 0; it < user_vars.size(); ++it)
        if (mp_str_cmp(name, user_vars[it].data()) == 0)
        {
            if (index != nullptr) *index = it;
            return true;
        }

    return false;
}

bool MathParser::VarNameInUse(const wchar_t* name) const
{
    return FunctionExists(name) || ConstantExists(name) || VariableExists(name);
}

bool MathParser::IsVarNameValid(const wstring& wstr)
{
    const auto str_len{ wstr.size() };

    if (str_len == 0) return false;

    if (!IsFirstChar(wstr[0]))
        return false;
    else
        for (size_t it = 1; it < str_len; ++it)
            if (!IsNextChar(wstr[it]))
                return false;

    return true;
}

void MathParser::EvaluateBinaryOp()
{
    MathLexeme Op1{}, Op2{}, Sign{};
    Stack.Pop(Op2);
    Stack.Pop(Sign);
    Stack.Pop(Op1);

    switch (Sign.iItem)
    {
    case MathLexeme::Plus:
        Op1.dValue += Op2.dValue;
        break;

    case MathLexeme::Minus:
        Op1.dValue -= Op2.dValue;
        break;

    case MathLexeme::Multiply:
        Op1.dValue *= Op2.dValue;
        break;

    case MathLexeme::Divide:
        Op1.dValue /= Op2.dValue;
        break;

    default:
        // case MathLexeme :: Power
        Op1.dValue = pow(Op1.dValue, Op2.dValue);
        break;
    }

    CheckForFloatingPointError(Op1.dValue);

    Stack.Push(Op1);
}

void MathParser::CheckForFloatingPointError(double value)
{
#ifdef MATH_PARSER_CHECK_FOR_FLOATING_POINT_ERRORS

    if (!isfinite(value))
        if (isnan(value))
            throw FloatingPointErrorNaN;
        else
            if (value > 0)
                throw FloatingPointErrorPosInf;
            else
                throw FloatingPointErrorNegInf;

#endif //MATH_PARSER_CHECK_FOR_FLOATING_POINT_ERRORS
}

void MathParser::GetLexCheckSyntax(
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
)
{
    PreviousLexeme = CurrentLexeme;

    // skip blanks

    size_t iFirstSpace = iCurrentPosition;
    while (pString[iCurrentPosition] == ' ' ||
        pString[iCurrentPosition] == '\t') ++iCurrentPosition;

    iFirstSymbol = iCurrentPosition;
    wchar_t cCurrentChar(pString[iCurrentPosition]);

    switch (cCurrentChar)
    {

    // end of line

    case '\0':
        CurrentLexeme.iType = MathLexeme::End;
        iFirstSymbol = iFirstSpace;
        //--iCurrentPosition;

        if (LexerMode != ParseMode)
            if (PreviousLexeme.iType == MathLexeme::Begin)
                Stack.Push(MathLexeme(MathLexeme::Number, MathLexeme::Constant, 0.0));
        break;

    // signs (assume now they are all binary; check for unary later)

    case '+':
        CurrentLexeme.iType = MathLexeme::Binary;
        CurrentLexeme.iItem = MathLexeme::Plus;
        CurrentLexeme.iPosition = iFirstSymbol;
        break;

    case '-':
        CurrentLexeme.iType = MathLexeme::Binary;
        CurrentLexeme.iItem = MathLexeme::Minus;
        CurrentLexeme.iPosition = iFirstSymbol;
        break;

    case '*':
        CurrentLexeme.iType = MathLexeme::Binary;
        CurrentLexeme.iItem = MathLexeme::Multiply;
        CurrentLexeme.iPosition = iFirstSymbol;

#ifdef MATH_PARSER_DOUBLE_ASTERISK_ALLOWED

        if (pString[++iCurrentPosition] == '*')
            CurrentLexeme.iItem = MathLexeme::Power;
        else --iCurrentPosition;

#endif //MATH_PARSER_DOUBLE_ASTERISK_ALLOWED

        break;

    case '/':

#ifdef MATH_PARSER_COLON_ALLOWED

    case ':':

#endif //MATH_PARSER_COLON_ALLOWED

        CurrentLexeme.iType = MathLexeme::Binary;
        CurrentLexeme.iItem = MathLexeme::Divide;
        CurrentLexeme.iPosition = iFirstSymbol;
        break;

    case '^':
        CurrentLexeme.iType = MathLexeme::Binary;
        CurrentLexeme.iItem = MathLexeme::Power;
        CurrentLexeme.iPosition = iFirstSymbol;
        break;

    // parentheses

    case '(':
        CurrentLexeme.iType = MathLexeme::LeftPar;
        ++iParBalance;
        break;

    case ')':
        // check balance
        if (iParBalance <= 0) throw ExtraRightPar;

        CurrentLexeme.iType = MathLexeme::RightPar;
        --iParBalance;
        break;

    // possibly number, variable or function

    default:
        if (cCurrentChar == '0' ||
            (cCurrentChar >= '1' && cCurrentChar <= '9') ||
            cCurrentChar == '.')
        {
            // possibly number
            iBufferPosition = 0;

            auto fExpFound{ false }, fPointFound{ false }, fSignFound{ false };
            for ( ; ; )
            {
                cCurrentChar = pString[iCurrentPosition];

                if ((cCurrentChar >= '1' && cCurrentChar <= '9') || cCurrentChar == '0')
                    pBuffer[iBufferPosition++] = pString[iCurrentPosition++];
                else
                    if (cCurrentChar == 'e' || cCurrentChar == 'E')
                    {
                        if (fExpFound)
                        {
                            --iCurrentPosition;
                            break;
                        }
                        else
                        {
                            pBuffer[iBufferPosition++] = pString[iCurrentPosition++];
                            fExpFound = true;
                        }
                    }
                    else
                        if (cCurrentChar == '.')
                        {
                            if (fPointFound)
                            {
                                --iCurrentPosition;
                                break;
                            }
                            else
                            {
                                pBuffer[iBufferPosition++] = pString[iCurrentPosition++];
                                fPointFound = true;
                            }
                        }
                        else
                            if (cCurrentChar == '+' || cCurrentChar == '-')
                            {

                                // check whether +/- belongs to the exponent or next lexeme

                                if (pString[iCurrentPosition - 1] == 'e' ||
                                    pString[iCurrentPosition - 1] == 'E')
                                {
                                    if (fSignFound)
                                    {
                                        --iCurrentPosition;
                                        break;
                                    }
                                    else
                                    {
                                        pBuffer[iBufferPosition++] = pString[iCurrentPosition++];
                                        fSignFound = true;
                                    }
                                }
                                else
                                {
                                    --iCurrentPosition;
                                    break;
                                }
                            }
                            else
                            {
                                --iCurrentPosition;
                                break;
                            }

            } // for

            // possibly got a number in pBuffer, check it

            pBuffer[iBufferPosition] = '\0';
            if (swscanf_s(pBuffer, L"%lf", &CurrentLexeme.dValue) == 1)
            {
                CurrentLexeme.iType = MathLexeme::Number;
                CurrentLexeme.iItem = MathLexeme::Constant;
                CurrentLexeme.iPosition = iFirstSymbol;//is this needed? - can be inf/NaN
                // check here for inf/nan and throw floating point error

                if (LexerMode != ParseMode)
                    CheckForFloatingPointError(CurrentLexeme.dValue);
            }
            else throw MathParser::InvalidNumber;

        } // if

        else // not a number, possibly function, constant or variable

            if (IsFirstChar(cCurrentChar))
            {
                iBufferPosition = 0;

                for (; ; )
                {
                    cCurrentChar = pString[iCurrentPosition];

                    if (IsNextChar(cCurrentChar))
                        pBuffer[iBufferPosition++] = pString[iCurrentPosition++];
                    else
                    {
                        --iCurrentPosition;
                        break;
                    }

                } // for

                // check for matching names (functions, constants, variables)

                pBuffer[iBufferPosition] = '\0';
                size_t iIndex{ 0 };

                if (FunctionExists(pBuffer, &iIndex)) // check for matching functions
                {
                    CurrentLexeme.iType = MathLexeme::Function;
                    CurrentLexeme.pFunction = MathLexeme::FunctionAddress[iIndex];
                    CurrentLexeme.iPosition = iFirstSymbol; // is this needed?
                }
                else // no function matches, check for built-in constants
                    if (ConstantExists(pBuffer, &iIndex))
                    {
                        CurrentLexeme.iType = MathLexeme::Number;
                        CurrentLexeme.iItem = MathLexeme::Constant;
                        CurrentLexeme.dValue = MathLexeme::ConstantValue[iIndex];
                    }
                    else // no constant matches, check for user-defined variables (no check for Parse)
                        if (LexerMode == ParseMode || VariableExists(pBuffer, &iIndex))
                        {
                            CurrentLexeme.iType = MathLexeme::Number;
                            CurrentLexeme.iItem = MathLexeme::Variable;
                            CurrentLexeme.iRunTimeIndex = iIndex;

                            if (LexerMode == EvaluateMode)
                                CurrentLexeme.dValue = rgdArguments[iIndex];
                        }
                        else throw UnknownIdentifier; // no matches

            } // if IsFirstChar

            else // invalid character
                throw InvalidCharacter;

    } // switch (cCurrentChar)

    ++iCurrentPosition;

    // got a lexeme, now check syntax

    // Check if a binary sign is actually unary

    if (CurrentLexeme.iType == MathLexeme::Binary)
        if (CurrentLexeme.iItem == MathLexeme::Plus ||
            CurrentLexeme.iItem == MathLexeme::Minus)
            if (Expected[PreviousLexeme.iType][MathLexeme::Unary])
                CurrentLexeme.iType = MathLexeme::Unary;

    // Check syntax

    if (!Expected[PreviousLexeme.iType][CurrentLexeme.iType])
        switch (PreviousLexeme.iType)
        {
        case MathLexeme::Begin:

#ifndef MATH_PARSER_EMPTY_STRING_ALLOWED

            if (CurrentLexeme.iType == MathLexeme::End)
                throw EmptyString;
            else

#endif //MATH_PARSER_EMPTY_STRING_ALLOWED

                throw ExpectedRealFunUnSignLeftPar;

        case MathLexeme::LeftPar:
            throw ExpectedRealFunUnSignLeftPar;

        case MathLexeme::Unary:
        case MathLexeme::Binary:
            throw ExpectedRealFunLeftPar;

        case MathLexeme::Number:
        case MathLexeme::RightPar:
            if (CurrentLexeme.iType == MathLexeme::LeftPar)
                throw iParBalance > 1 ? ExpectedBiSignRightPar : ExpectedBiSign;
            else
                throw iParBalance > 0 ? ExpectedBiSignRightPar : ExpectedBiSign;

        default:
            // case MathLexeme::Function:
            throw ExpectedLeftPar;
        }
}

// Check and increase if necessary the runtime memory used by Execute
// based off the longest string and the number of user variables
//
void MathParser::AdjustRunTimeMem()
{
    constexpr size_t min_runtime_mem{ 128 }; // 1 == test value, to be increased
    auto required_runtime_mem{ min_runtime_mem };

    for (size_t ind = 0; ind < NumberOfStrings(); ++ind)
    {
        const auto str_len = input_strings[ind].size() + 1;
        if (required_runtime_mem < str_len) required_runtime_mem = str_len;
    }

    required_runtime_mem += NumberOfVars();

    if (runtime_mem.size() < required_runtime_mem)
    {
        constexpr size_t runtime_mem_extra{ 128 }; // 0 == test value, to be increased
        const auto current_runtime_mem = required_runtime_mem + runtime_mem_extra;

        runtime_mem.resize(current_runtime_mem);
    }
}

void MathParser::CompileBinaryOp()
{
    MathLexeme Op1{}, Op2{}, Sign{};
    Stack.Pop(Op2);
    Stack.Pop(Sign);
    Stack.Pop(Op1);

    if (Op1.iItem == MathLexeme::Constant)
    {
        if (Op2.iItem == MathLexeme::Constant)
        {
            // both operands' values are known at the time of compilation,
            // therefore just evaluate the operation, nothing to compile

            switch (Sign.iItem)
            {
            case MathLexeme::Plus:
                Op1.dValue += Op2.dValue;
                break;

            case MathLexeme::Minus:
                Op1.dValue -= Op2.dValue;
                break;

            case MathLexeme::Multiply:
                Op1.dValue *= Op2.dValue;
                break;

            case MathLexeme::Divide:
                Op1.dValue /= Op2.dValue;
                break;

            default:
                // case MathLexeme::Power
                Op1.dValue = pow(Op1.dValue, Op2.dValue);
            }

            CheckForFloatingPointError(Op1.dValue);

            Stack.Push(Op1);

        } // if both are constants
        else
        {
            // Op2's value is not known now
            // produce const+mem operation

            pCompiledCode[iCommandCounter++].
                CompiledCommand::CompiledCommand(Op1.dValue, Op2.iRunTimeIndex,
                    iMemoryCounter, Sign.iItem, Sign.iPosition);

            Op2.iRunTimeIndex = iMemoryCounter++;
            Stack.Push(Op2);
        }
    }
    else
    {
        if (Op2.iItem == MathLexeme::Constant)
        {
            // Op1's value is not known now
            // produce mem+const operation

            pCompiledCode[iCommandCounter++].
                CompiledCommand::CompiledCommand(Op1.iRunTimeIndex, Op2.dValue,
                    iMemoryCounter, Sign.iItem, Sign.iPosition);

            Op1.iRunTimeIndex = iMemoryCounter++;
            Stack.Push(Op1);
        }
        else
        {
            // neither value is known
            // produce mem+mem operation

            pCompiledCode[iCommandCounter++].
                CompiledCommand::CompiledCommand(Op1.iRunTimeIndex, Op2.iRunTimeIndex,
                    iMemoryCounter, Sign.iItem, Sign.iPosition);

            Op1.iRunTimeIndex = iMemoryCounter++;
            Stack.Push(Op1);
        }
    }
}

void MathParser::InvalidateCompiledCode(size_t ind)
{
    compiled_code[ind].data()->CompiledCommand::CompiledCommand(bool{});
}