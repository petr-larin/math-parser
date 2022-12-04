//
// MathParser demo app
//

// link with Common Controls v6

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <unordered_map>
#include <assert.h>
#include "mp.hpp"
#include "mp_resource.h"
#include "mp_rndstr.hpp"

// redefine MS definitions causing C26456

#undef NM_CLICK
#undef NM_DBLCLK
#undef NM_RETURN
#define NM_CLICK 0xFFFFFFFE
#define NM_DBLCLK 0xFFFFFFFD
#define NM_RETURN 0xFFFFFFFC

class MParserDemoApp {

	// All members are static to obviate the need to pass "this" to DlgProcs

	MParserDemoApp() = default;
	~MParserDemoApp() = default;

	friend int WINAPI
		wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int);

	static INT_PTR CALLBACK
		MathParserMainDlgProc(HWND, UINT, WPARAM, LPARAM);
	static INT_PTR CALLBACK
		MathParserVariablesDlgProc(HWND, UINT, WPARAM, LPARAM);
	static INT_PTR CALLBACK
		MathParserNewEditDlgProc(HWND, UINT, WPARAM, LPARAM);

	static void MainDlg_Init(HWND);
	static void MainDlg_Notify_OpenVarsDlg();
	static void MainDlg_Notify_GenRndStr();
	static void MainDlg_Notify_CopyToClipboard(const double&);
	static void MainDlg_Command_CheckForTextChange(WPARAM);
	static void MainDlg_Command_Parse(WPARAM, double&);
	static void MainDlg_Command_ProcessCheckBox();
	static BOOL MainDlg_SetResultText(LPCWSTR = nullptr);
	static void MainDlg_UpdateExecuteButton(bool);

	static void VariablesDlg_Init(HWND);
	static int VariablesDlg_LV_InsertItems(const wchar_t*, double, int);
	static void VariablesDlg_LV_AdjustColumnWidth();
	static LRESULT VariablesDlg_LV_GetSelMark();
	static void VariablesDlg_Notify(LPARAM);
	static void VariablesDlg_Command_AddVar();
	static void VariablesDlg_Command_EditVar();
	static void VariablesDlg_Command_RemoveVar();
	static void VariablesDlg_DebugMsg();

	static INT_PTR NewEditDlg_Init(HWND, wstring&, double&, bool&);
	static void NewEditDlg_Destroy(const bool&);
	static void NewEditDlg_Command_CheckForTextChange(WPARAM, bool&);
	static void NewEditDlg_Command_OK();
	static void NewEditDlg_Command_Cancel(wstring&&, const double&, bool&);

	static const wchar_t* UserMessage(int);
	static void HighlightText(int);
	static void FatalError(int, HWND = NULL);

	static HWND hDlgMain;
	static HWND hDlgVars;
	static HWND hDlgNewEdit;

	static MathParser g_mp;
	static vector<double> arg_list; // argument list for Evaluate and Execute
	
	// new_edit_ind controls the mode of MathParserNewEditDlgProc:
	// 
	// new_edit_ind >= 0  - "edit" mode - editing an existing var, new_edit_ind == its index
	// new_edit_ind == -1  - "new" mode - adding a new var

	static LRESULT new_edit_ind;
};

HWND MParserDemoApp::hDlgMain{ NULL };
HWND MParserDemoApp::hDlgVars{ NULL };
HWND MParserDemoApp::hDlgNewEdit{ NULL };

MathParser MParserDemoApp::g_mp{ true }; // true == case-sensitive parser
vector<double> MParserDemoApp::arg_list{};

LRESULT MParserDemoApp::new_edit_ind{};

int WINAPI wWinMain(_In_ HINSTANCE,	_In_opt_ HINSTANCE,	_In_ LPWSTR, _In_ int)
{
	static MParserDemoApp App{}; // static = on exit() destructor will be called

	INITCOMMONCONTROLSEX icc{};
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS;

	if (!InitCommonControlsEx(&icc)) MParserDemoApp::FatalError(IDS_FE_COMMCTL);

	DialogBoxParamW(
		NULL,
		MAKEINTRESOURCEW(IDD_DIALOG_MAIN),
		NULL,
		DLGPROC(MParserDemoApp::MathParserMainDlgProc),
		0);

	return 0;
};

INT_PTR CALLBACK
MParserDemoApp::MathParserMainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Keeps the result of the most recent successful call to Evaluate/Execute
	static auto str_value{ 0.0 };

	switch (uMsg)
	{
	case WM_INITDIALOG:

		MainDlg_Init(hwndDlg);
		return TRUE;

	case WM_NOTIFY:
	{
		// notifications from SysLinks

		const auto phdr = LPNMHDR(lParam);
		assert(phdr != nullptr);

		const auto code = phdr->code;
		const auto from = phdr->idFrom;

		if (code == NM_CLICK || code == NM_RETURN)
			switch (from)
			{
			case IDC_SYSLINK_VARS:

				// open "set variable values" dialog via a syslink

				MainDlg_Notify_OpenVarsDlg();
				return TRUE;

			case IDC_SYSLINK_RNDSTR:

				// generate a random string

				MainDlg_Notify_GenRndStr();
				return TRUE;

			case IDC_RESULT:

				// copy the result to clipboard

				MainDlg_Notify_CopyToClipboard(str_value);
				return TRUE;
			}

		return FALSE;
	}

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDC_EDIT_MAIN:

			// If the text has changed, invalidate it

			MainDlg_Command_CheckForTextChange(wParam);
			return TRUE;

		case IDEXECUTE:
		case IDPARSE:
		case IDEVALUATE:
		case IDCOMPILE:

			// Main functions

			MainDlg_Command_Parse(wParam, str_value);
			return TRUE;

		case IDCANCEL:

			EndDialog(hwndDlg, 1);
			return TRUE;

		case IDC_CHECK_CASE:

			// Checkbox for case sensitivity

			MainDlg_Command_ProcessCheckBox();
			return TRUE;

		default: return FALSE;
		}

	} // WM_COMMAND

	return FALSE;
}

INT_PTR CALLBACK
MParserDemoApp::MathParserVariablesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:

		VariablesDlg_Init(hwndDlg);
		return TRUE;

	case WM_DESTROY:

		hDlgVars = NULL;
		return TRUE;

	case WM_NOTIFY:

		// notification(s) from list view items

		VariablesDlg_Notify(lParam);
		return TRUE;

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:

			EndDialog(hwndDlg, 1);
			return TRUE;

		case IDADD:

			VariablesDlg_Command_AddVar();
			return TRUE;

		case IDEDIT:

			VariablesDlg_Command_EditVar();
			return TRUE;

		case IDREMOVE:

			VariablesDlg_Command_RemoveVar();
			return TRUE;

		default: return FALSE;
		}

	} // WM_COMMAND

	return FALSE;
}

INT_PTR CALLBACK
MParserDemoApp::MathParserNewEditDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
	// Save the items being edited in case editing is canceled
	static wstring undo_var{};
	static double undo_arg{};

	// Accumulate logic whether to disable Execute button on exit
	static bool have_to_disable_execute{};

	switch (uMsg)
	{
	case WM_INITDIALOG:

		return NewEditDlg_Init(hwndDlg, undo_var, undo_arg, have_to_disable_execute);

	case WM_DESTROY:

		NewEditDlg_Destroy(have_to_disable_execute);
		return TRUE;

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDC_EDIT_VAR:

			NewEditDlg_Command_CheckForTextChange(wParam, have_to_disable_execute);
			return TRUE;

		case IDOK:

			NewEditDlg_Command_OK();
			return TRUE;

		case IDCANCEL:

			NewEditDlg_Command_Cancel(move(undo_var), undo_arg, have_to_disable_execute);
			return TRUE;

		default: return FALSE;
		}

	} // WM_COMMAND

	return FALSE;
}

void MParserDemoApp::MainDlg_Init(HWND hwndDlg)
{
	hDlgMain = hwndDlg;

	MainDlg_SetResultText();

	SendDlgItemMessageW(hwndDlg, IDC_CHECK_CASE, BM_SETCHECK,
		g_mp.IsCaseSensitive() ? BST_CHECKED : BST_UNCHECKED, 0);
}

void MParserDemoApp::MainDlg_Notify_OpenVarsDlg()
{
	MainDlg_SetResultText();

	DialogBoxParamW(
		NULL,
		MAKEINTRESOURCEW(IDD_DIALOG_OPTIONS),
		hDlgMain,
		DLGPROC(MParserDemoApp::MathParserVariablesDlgProc),
		0);
}

void MParserDemoApp::MainDlg_Notify_GenRndStr()
{
	mp_rndstr rndstr{};
	wstring rnd_str{};
	mp_rndstr::main_weights_seq empty_seq{};

	rndstr.shuffle_variables(1);

	rnd_str = rndstr.mp_random_string(empty_seq);

	SetDlgItemTextW(hDlgMain, IDC_EDIT_MAIN, rnd_str.data());

	g_mp.RemoveAllVars();
	arg_list.clear();

	SYSTEMTIME st{};
	GetSystemTime(&st);
	srand(st.wMilliseconds + st.wSecond * 1000 + st.wMinute * 1000 * 60);

	for (size_t it = 0; it < rndstr.vars_just_used_qty(); ++it)
	{
		size_t unused{};
		const wchar_t wch = rndstr.vars_just_used(it);
		wstring wstr(1, wch);

		if (g_mp.CheckAndInsertVar(move(wstr), 0, unused) == MathParser::OK)
			arg_list.insert(arg_list.begin(),
				(double(rand()) / RAND_MAX) * 20.0 - 10.0); // [-10.0, 10.0]
	}
}

void MParserDemoApp::MainDlg_Notify_CopyToClipboard(const double& str_value)
{
	constexpr auto buffer_size{ 30 };
	wchar_t buffer[buffer_size]{};
	swprintf_s(buffer, buffer_size, L"%.17g", str_value);

	if (OpenClipboard(hDlgMain))
	{
		EmptyClipboard();

		const auto hglb = GlobalAlloc(GMEM_MOVEABLE, buffer_size * sizeof(wchar_t));

		if (hglb != NULL)
		{
			const auto pt_str = GlobalLock(hglb);

			if (pt_str != nullptr)
			{
				wcscpy_s((wchar_t*)(pt_str), buffer_size, buffer);

				SetClipboardData(CF_UNICODETEXT, hglb);

				GlobalUnlock(hglb);
			}
		}
		CloseClipboard();
	}
}

void MParserDemoApp::MainDlg_Command_CheckForTextChange(WPARAM wParam)
{
	if (HIWORD(wParam) == EN_CHANGE)
	{
		MainDlg_SetResultText();
	
		if (g_mp.NumberOfStrings() == 1) g_mp.RemoveString(0);

		MainDlg_UpdateExecuteButton(false);
	}
}

void MParserDemoApp::MainDlg_Command_Parse(WPARAM wParam, double& str_value)
{
	auto hEdit = GetDlgItem(hDlgMain, IDC_EDIT_MAIN);

	if (LOWORD(wParam) != IDEXECUTE)
		if (g_mp.NumberOfStrings() == 0)
		{
			const auto str_len = GetWindowTextLengthW(hEdit);

			wstring wstr{};
			wstr.resize(static_cast<size_t>(str_len) + 1);

			GetWindowTextW(hEdit, &(wstr[0]), str_len + 1);

			wstr.erase(str_len, 1); // exclude terminating 0 from size() count

			size_t unused{};
			g_mp.InsertString(move(wstr), 0, unused);
		}

	MathParser::ErrorCodes err_code{};
	size_t err_pos{};

	switch (LOWORD(wParam))
	{
	case IDPARSE:
		err_code = g_mp.Parse(err_pos);
		break;

	case IDEVALUATE:
		err_code = g_mp.Evaluate(err_pos, arg_list, str_value);
		break;

	case IDCOMPILE:
		err_code = g_mp.Compile(err_pos);
		MainDlg_UpdateExecuteButton(true);
		break;

	case IDEXECUTE:
		err_code = g_mp.Execute(err_pos, arg_list, str_value);
		break;
	}

	auto str_id{ 0 };

	switch (err_code)
	{
	case MathParser::OK:
		str_id = IDS_OK;
		break;

	case MathParser::InvalidNumber:
		str_id = IDS_InvalidNumber;
		break;

	case MathParser::UnknownIdentifier:
		str_id = IDS_UnknownIdentifier;
		break;

	case MathParser::InvalidCharacter:
		str_id = IDS_InvalidCharacter;
		break;

	case MathParser::EmptyString:
		str_id = IDS_EmptyString;
		break;

	case MathParser::ExpectedRealFunUnSignLeftPar:
		str_id = IDS_ExpectedRealFunUnSignLeftPar;
		break;

	case MathParser::ExpectedRealFunLeftPar:
		str_id = IDS_ExpectedRealFunLeftPar;
		break;

	case MathParser::ExpectedBiSignRightPar:
		str_id = IDS_ExpectedBiSignRightPar;
		break;

	case MathParser::ExpectedBiSign:
		str_id = IDS_ExpectedBiSign;
		break;

	case MathParser::ExpectedLeftPar:
		str_id = IDS_ExpectedLeftPar;
		break;

	case MathParser::ExpectedRightPar:
		str_id = IDS_ExpectedRightPar;
		break;

	case MathParser::ExtraRightPar:
		str_id = IDS_ExtraRightPar;
		break;

	case MathParser::FloatingPointErrorNaN:
		str_id = IDS_FloatingPointErrorNaN;
		break;

	case MathParser::FloatingPointErrorPosInf:
		str_id = IDS_FloatingPointErrorPosInf;
		break;

	case MathParser::FloatingPointErrorNegInf:
		str_id = IDS_FloatingPointErrorNegInf;
		break;

	default:
		assert(false); // stub
	}

	constexpr auto buffer_size{ 120 };
	wchar_t buffer[buffer_size]{};

	SetFocus(hEdit);

	if (err_code == MathParser::OK)
		if (LOWORD(wParam) == IDEVALUATE || LOWORD(wParam) == IDEXECUTE)
		{
			// print OK + numeric result

			swprintf_s(buffer, buffer_size, L"%s %s, %s: %.15g    [<A>%s</A>]",
				UserMessage(IDS_RESULT), UserMessage(str_id),
				UserMessage(IDS_CVAL), str_value, UserMessage(IDS_COPY));
		}
		else
		{
			// print just OK

			swprintf_s(buffer, buffer_size, L"%s %s",
				UserMessage(IDS_RESULT), UserMessage(str_id));
		}
	else
	{
		// print error message + position

		swprintf_s(buffer, buffer_size, L"%s %s %s %llu",
			UserMessage(IDS_RESULT), UserMessage(str_id),
			UserMessage(IDS_ATPOS), err_pos + 1);

		// move caret to the error position

		SendMessageW(hEdit, EM_SETCARETINDEX, err_pos, 0);
	}

	MainDlg_SetResultText(buffer);
}

void MParserDemoApp::MainDlg_Command_ProcessCheckBox()
{
	g_mp.SetCaseSensitive(
		(SendDlgItemMessageW(hDlgMain,
			IDC_CHECK_CASE, BM_GETCHECK, 0, 0) == BST_CHECKED));

	MainDlg_UpdateExecuteButton(false);
}

BOOL MParserDemoApp::MainDlg_SetResultText(LPCWSTR msg)
{
	assert(hDlgMain != NULL);

	return SetDlgItemTextW(hDlgMain, IDC_RESULT,
				msg == nullptr ?
					UserMessage(IDS_RESULT) // default message
					: msg);
}

void MParserDemoApp::MainDlg_UpdateExecuteButton(bool action)
{
	// enable/disable Execute button
	// action == false: disable the button
	// action == true: enable the button if there is no error token

	EnableWindow(GetDlgItem(hDlgMain, IDEXECUTE), action && g_mp.OKtoExecute(0));
}

void MParserDemoApp::VariablesDlg_Init(HWND hwndDlg)
{
	hDlgVars = hwndDlg;

	VariablesDlg_DebugMsg();

	// Set extended styles of the list view control.
	// The styles should be in line with the WM_NOTIFY codes processed in VariablesDlg_Notify().

	const auto hListView = GetDlgItem(hwndDlg, IDC_LIST_VARS);

	SendMessageW(
		hListView, LVM_SETEXTENDEDLISTVIEWSTYLE,
		LVS_EX_FULLROWSELECT |
		LVS_EX_ONECLICKACTIVATE |
		//LVS_EX_UNDERLINEHOT |
		LVS_EX_GRIDLINES |
		LVS_EX_TRACKSELECT
		, -1
	);

	// Create two list view columns {var name, value}

	LVCOLUMNW lvc{};

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 10;

	auto insert_column = [=, &lvc](int header, WPARAM col_ind)
	{
		lvc.pszText = const_cast<wchar_t*>(UserMessage(header));
		SendMessageW(hListView, LVM_INSERTCOLUMNW, col_ind, LPARAM(&lvc));
	};

	insert_column(IDS_VAR, 0);
	insert_column(IDS_VAL, 1);

	// Fill the columns

	for (size_t ind = 0; ind < g_mp.NumberOfVars(); ++ind)
		VariablesDlg_LV_InsertItems(g_mp.Var(ind), arg_list[ind], static_cast<int>(ind));

	// Ensure the width of columns stays the same

	VariablesDlg_LV_AdjustColumnWidth();
}

int MParserDemoApp::VariablesDlg_LV_InsertItems(const wchar_t* name, double value, int index)
{
	// add a pair {name, value} at [index] to the list view

	LVITEMW lvi{};

	auto insert_item = [=, &lvi](UINT msg)
	{
		return static_cast<int>(
			SendDlgItemMessageW(hDlgVars, IDC_LIST_VARS, msg, 0, LPARAM(&lvi)));
	};

	lvi.mask = LVIF_TEXT;
	lvi.iItem = index;
	if (lvi.iItem < 0) lvi.iItem = 0;

	// Insert a new line with name

	lvi.iSubItem = 0;
	lvi.pszText = const_cast<wchar_t*>(name);
	lvi.iItem = insert_item(LVM_INSERTITEMW);

	assert(lvi.iItem != -1);

	// Add a value to the new line

	lvi.iSubItem = 1;
	constexpr auto lvi_buffer_size{ 30 };
	wchar_t lvi_buffer[lvi_buffer_size]{};
	lvi.pszText = lvi_buffer;

	swprintf_s(lvi_buffer, lvi_buffer_size, L"%.15g", value);

	auto res = insert_item(LVM_SETITEMW);
	assert(res != FALSE);

	return lvi.iItem;
}

void MParserDemoApp::VariablesDlg_LV_AdjustColumnWidth()
{
	if (hDlgVars == NULL) return;

	const auto hListView = GetDlgItem(hDlgVars, IDC_LIST_VARS);

	RECT rc{};
	GetWindowRect(hListView, &rc);

	constexpr auto ratio{ 0.40 }; // == (first column width) / (total listview width)

	SendMessageW(
		hListView, LVM_SETCOLUMNWIDTH, 0,
		static_cast<LPARAM>((rc.right - rc.left) * ratio));

	SendMessageW(
		hListView, LVM_SETCOLUMNWIDTH, 1, LVSCW_AUTOSIZE_USEHEADER);
}

LRESULT MParserDemoApp::VariablesDlg_LV_GetSelMark()
{
	return SendDlgItemMessageW(hDlgVars, IDC_LIST_VARS, LVM_GETSELECTIONMARK, 0, 0);
}

void MParserDemoApp::VariablesDlg_Notify(LPARAM lParam)
{
	const auto phdr = LPNMHDR(lParam);
	assert(phdr != nullptr);

	const auto code = phdr->code;
	const auto from = phdr->idFrom;

	// NM_CLICK and/or NM_DBLCLK should be used in line with the extended list view
	// styles selected in VariablesDlg_Init().
	// Note: NM_RETURN cannot be caught by Dialog Proc.

	if (from == IDC_LIST_VARS && code == NM_CLICK)
	{
		const auto lpnmitem = LPNMITEMACTIVATE(lParam);
		const auto item = lpnmitem->iItem;

		if (item >= 0) SendMessageW(hDlgVars, WM_COMMAND, IDEDIT, 0);
	}
}

void MParserDemoApp::VariablesDlg_Command_AddVar()
{
	new_edit_ind = -1; // "new" mode

	DialogBoxParamW(
		NULL,
		MAKEINTRESOURCEW(IDD_DIALOG_NEWEDIT),
		hDlgVars,
		DLGPROC(MParserDemoApp::MathParserNewEditDlgProc),
		0);

	VariablesDlg_DebugMsg();
}

void MParserDemoApp::VariablesDlg_Command_EditVar()
{
	const auto ind_to_edit = VariablesDlg_LV_GetSelMark();

	if (ind_to_edit < 0) MessageBeep(MB_ICONERROR);
	else
	{
		new_edit_ind = ind_to_edit; // "edit" mode

		DialogBoxParamW(
			NULL,
			MAKEINTRESOURCEW(IDD_DIALOG_NEWEDIT),
			hDlgVars,
			DLGPROC(MParserDemoApp::MathParserNewEditDlgProc),
			0);
	}

	VariablesDlg_DebugMsg();
}

void MParserDemoApp::VariablesDlg_Command_RemoveVar()
{
	const auto ind_to_remove = VariablesDlg_LV_GetSelMark();

	if (ind_to_remove < 0) MessageBeep(MB_ICONERROR);
	else
	{
		SendDlgItemMessageW(hDlgVars, IDC_LIST_VARS, LVM_DELETEITEM, ind_to_remove, 0);
		VariablesDlg_LV_AdjustColumnWidth();

		g_mp.RemoveVar(ind_to_remove);
		arg_list.erase(arg_list.begin() + ind_to_remove);

		MainDlg_UpdateExecuteButton(false);
	}

	VariablesDlg_DebugMsg();
}

void MParserDemoApp::VariablesDlg_DebugMsg() // remove from Release
{
#ifdef _DEBUG
	constexpr auto buf_size{ 32 };
	wchar_t buf[buf_size];

	swprintf_s(
		buf, buf_size, L"%zu : %zu : %zu",
		g_mp.NumberOfStrings(), g_mp.NumberOfVars(), arg_list.size());

	SetDlgItemTextW(hDlgVars, IDC_STATIC_DEBUG, buf);
#endif
}

INT_PTR MParserDemoApp::NewEditDlg_Init(
	HWND hwndDlg, wstring& undo_var, double& undo_arg, bool& have_to_disable_execute)
{
	hDlgNewEdit = hwndDlg;

	VariablesDlg_DebugMsg();

	SetWindowTextW(hwndDlg,
		UserMessage(
			new_edit_ind == -1 ? IDS_CAPTION_ADD : IDS_CAPTION_EDIT));

	if (new_edit_ind != -1)
	{
		assert(new_edit_ind >= 0);

		// In "edit" mode - save the items being edited

		undo_var = wstring(g_mp.Var(new_edit_ind));
		undo_arg = arg_list[new_edit_ind];

		// Load existing values to edit controls

		SetDlgItemTextW(hwndDlg, IDC_EDIT_VAR, undo_var.data());

		if (isfinite(undo_arg)) // avoid loading "INF" or "NAN"
		{
			constexpr auto edt_buffer_size{ 30 };
			wchar_t edt_buffer[edt_buffer_size]{};

			swprintf_s(edt_buffer, edt_buffer_size, L"%.15g", undo_arg);
			SetDlgItemTextW(hwndDlg, IDC_EDIT_EXPR, edt_buffer);
		}

		HighlightText(IDC_EDIT_EXPR);

		// Delete the items being edited from the parser but not from the list view

		g_mp.RemoveVar(new_edit_ind);
		arg_list.erase(arg_list.begin() + new_edit_ind);

		have_to_disable_execute = false; // initial value for the session

		VariablesDlg_DebugMsg();

		return FALSE;  // enable SetFocus from HighlightText
	}

	return FALSE;  // was return TRUE; but no effect on kb focus
}

void MParserDemoApp::NewEditDlg_Destroy(const bool& have_to_disable_execute)
{
	// disable Execute button if flag says so (unless we are in the "new" mode)

	if (have_to_disable_execute && new_edit_ind != -1) MainDlg_UpdateExecuteButton(false);

	hDlgNewEdit = NULL;
}

void MParserDemoApp::NewEditDlg_Command_CheckForTextChange(
	WPARAM wParam, bool& have_to_disable_execute)
{
	if (HIWORD(wParam) == EN_CHANGE)
		have_to_disable_execute = true; // var name potentially changed
}

void MParserDemoApp::NewEditDlg_Command_OK()
{
	// get text from both edit controls

	const auto hEditVar = GetDlgItem(hDlgNewEdit, IDC_EDIT_VAR);
	const auto hEditExpr = GetDlgItem(hDlgNewEdit, IDC_EDIT_EXPR);

	const auto var_len = GetWindowTextLengthW(hEditVar);
	const auto expr_len = GetWindowTextLengthW(hEditExpr);

	constexpr auto var_len_max{ 8 }, expr_len_max{ 32 };

	if (var_len > var_len_max)
	{
		MessageBoxW(hDlgNewEdit,
			UserMessage(IDS_ID_2LONG),
			UserMessage(IDS_MB_SERROR),
			MB_ICONEXCLAMATION);

		HighlightText(IDC_EDIT_VAR);

		return;
	}

	if (expr_len > expr_len_max)
	{
		MessageBoxW(hDlgNewEdit,
			UserMessage(IDS_NUM_2LONG),
			UserMessage(IDS_MB_SERROR),
			MB_ICONEXCLAMATION);

		HighlightText(IDC_EDIT_EXPR);

		return;
	}

	wchar_t var_buf[var_len_max + 1]{}, expr_buf[expr_len_max + 1]{};

	GetWindowTextW(hEditVar, var_buf, var_len_max + 1); // var name
	GetWindowTextW(hEditExpr, expr_buf, expr_len_max + 1); // number expression

	// check if the entered var name is correct

	wstring wstr{ var_buf };
	MathParser::TrimVarName(wstr);
	const auto err_code = g_mp.CheckVar(wstr);

	if (err_code != MathParser::OK)
	{
		MessageBoxW(hDlgNewEdit,
			err_code == MathParser::InvalidIdentifier ?
				UserMessage(IDS_ENT_VALID_ID) : UserMessage(IDS_ID_IN_USE),
			UserMessage(IDS_MB_SERROR),
			MB_ICONEXCLAMATION);

		HighlightText(IDC_EDIT_VAR);

		return;
	}

	// check if the entered number expression is correct using Evaluate

	double expr_value{};
	size_t assigned_index{}, error_position{};

	g_mp.InsertString(wstring(expr_buf), 1, assigned_index);
	const auto expr_ok{
		g_mp.Evaluate(error_position, arg_list, expr_value, assigned_index)
			== MathParser::OK };
	g_mp.RemoveString(assigned_index);

	if (!expr_ok)
	{
		MessageBoxW(hDlgNewEdit,
			UserMessage(IDS_ENT_VALID_NUM),
			UserMessage(IDS_MB_SERROR),
			MB_ICONEXCLAMATION);

		HighlightText(IDC_EDIT_EXPR);

		return;
	}

	// all OK - insert the new item

	auto insert_at = VariablesDlg_LV_GetSelMark();

	if (insert_at < 0) insert_at = g_mp.NumberOfVars(); //no selection - add at the end

	g_mp.CheckAndInsertVar(move(wstr), insert_at, assigned_index);

	if (new_edit_ind != -1)
		SendDlgItemMessageW(
			hDlgVars, IDC_LIST_VARS, LVM_DELETEITEM, new_edit_ind, 0);

	insert_at = VariablesDlg_LV_InsertItems(
		g_mp.Var(insert_at), expr_value, static_cast<int>(insert_at));

	VariablesDlg_LV_AdjustColumnWidth();

	SendDlgItemMessageW(
		hDlgVars, IDC_LIST_VARS, LVM_ENSUREVISIBLE, insert_at, TRUE);

	arg_list.insert(arg_list.begin() + insert_at, expr_value);

	EndDialog(hDlgNewEdit, 1);
}

void MParserDemoApp::NewEditDlg_Command_Cancel(
	wstring&& undo_var, const double& undo_arg, bool& have_to_disable_execute)
{
	if (new_edit_ind != -1)
	{
		// edit mode - roll back

		size_t unused;
		g_mp.CheckAndInsertVar(move(undo_var), new_edit_ind, unused);
		arg_list.insert(arg_list.begin() + new_edit_ind, undo_arg);
	}

	have_to_disable_execute = false; // session canceled - no need to update

	EndDialog(hDlgNewEdit, 1);
}

const wchar_t* MParserDemoApp::UserMessage(int msg_id)
{
	// load and store a string resource and return a pointer to it

	using umap = std::unordered_map<int, wstring>;
	static umap UserMessages{};

	auto it = UserMessages.find(msg_id);

	if (it == UserMessages.end())
	{
		//	ptr_to_str will receive a pointer to the resource string (not 0-terminated)
		//	wchar_count == how many characters

		const wchar_t* ptr_to_str{};
		const wchar_t** const ptr_to_ptr{ &ptr_to_str };

		const auto wchar_count = LoadStringW(0, msg_id, (wchar_t*)(ptr_to_ptr), 0);

		assert(wchar_count > 0);

		wstring usr_msg(ptr_to_str, 0, wchar_count);
		it = (UserMessages.insert(umap::value_type(msg_id, move(usr_msg)))).first;
	}
	return (it->second).data();
}

void MParserDemoApp::HighlightText(int ctrl_id)
{
	SetFocus(GetDlgItem(hDlgNewEdit, ctrl_id));
	SendDlgItemMessageW(hDlgNewEdit, ctrl_id, EM_SETSEL, 0, -1);
}

void MParserDemoApp::FatalError(int error_msg, HWND hwnd)
{
	MessageBoxW(hwnd, UserMessage(error_msg), UserMessage(IDS_FATALERROR_CAP), MB_ICONSTOP);

	exit(error_msg);
}