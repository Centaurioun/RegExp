#include "pch.h"
#include "detours.h"
#include "ThemeHelper.h"
#include "Theme.h"
#include "CustomEdit.h"
#include "SizeGrip.h"
#include "CustomStatusBar.h"
#include "CustomButton.h"

const Theme* CurrentTheme;

static decltype(::GetSysColor)* OrgGetSysColor = ::GetSysColor;
static decltype(::GetSysColorBrush)* OrgGetSysColorBrush = ::GetSysColorBrush;
static decltype(::GetSystemMetrics)* OrgGetSystemMetrics = ::GetSystemMetrics;

int WINAPI HookedGetSystemMetrics(_In_ int index) {
	switch (index) {
		case SM_CYMENUSIZE: 
			return 30;
	}
	return OrgGetSystemMetrics(index);
}

HBRUSH WINAPI HookedGetSysColorBrush(int index) {
	if (CurrentTheme) {
		auto hBrush = CurrentTheme->GetSysBrush(index);
		if (hBrush)
			return hBrush;
	}
	return OrgGetSysColorBrush(index);
}

COLORREF WINAPI HookedGetSysColor(int index) {
	if (CurrentTheme) {
		auto color = CurrentTheme->GetSysColor(index);
		if (color != CLR_INVALID)
			return color;
	}
	return OrgGetSysColor(index);
}

void HandleCreateWindow(CWPRETSTRUCT* cs) {
	CString name;
	CWindow win(cs->hwnd);
	auto lpcs = (LPCREATESTRUCT)cs->lParam;
	if((lpcs->style & (WS_CAPTION | WS_POPUP)) == 0)
		::SetWindowTheme(cs->hwnd, L" ", L"");

	if (::GetClassName(cs->hwnd, name.GetBufferSetLength(64), 64)) {
		//if (name.CompareNoCase(L"EDIT") == 0 || name.CompareNoCase(L"ATL:EDIT") == 0) {
		//	auto win = new CCustomEdit;
		//	ATLVERIFY(win->SubclassWindow(cs->hwnd));
		//}
		if (name.CompareNoCase(WC_LISTVIEW) == 0) {
			::SetWindowTheme(cs->hwnd, nullptr, nullptr);
		}
		else if (name.CompareNoCase(WC_TREEVIEW) == 0) {
			::SetWindowTheme(cs->hwnd, nullptr, nullptr);
		}
		else if (name.CompareNoCase(WC_HEADER) == 0) {
			//::SetWindowTheme(cs->hwnd, L"Explorer", nullptr);
		}
		else if (name.CompareNoCase(STATUSCLASSNAME) == 0) {
			//::SetWindowTheme(cs->hwnd, nullptr, nullptr);
			auto win = new CCustomStatusBar;
			ATLVERIFY(win->SubclassWindow(cs->hwnd));
		}
		else if (name.CompareNoCase(L"ScrollBar") == 0 && (lpcs->style & (SBS_SIZEBOX | SBS_SIZEGRIP))) {
			//auto win = new CSizeGrip;
			//ATLVERIFY(win->SubclassWindow(cs->hwnd));
		}
		else if (name.CompareNoCase(L"BUTTON") == 0) {
			auto type = lpcs->style & BS_TYPEMASK;
			if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) {
				auto win = new CCustomButtonParent;
				ATLVERIFY(win->SubclassWindow(::GetParent(cs->hwnd)));
			}
		}

	}
}

LRESULT CALLBACK CallWndProc(int action, WPARAM wp, LPARAM lp) {
	if (action == HC_ACTION) {
		auto cs = reinterpret_cast<CWPRETSTRUCT*>(lp);
		switch (cs->message) {
			case WM_CREATE:
				HandleCreateWindow(cs);
				break;

		}
	}

	return ::CallNextHookEx(nullptr, action, wp, lp);
}


bool ThemeHelper::Init(HANDLE hThread) {
	auto hook = ::SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndProc, nullptr, ::GetThreadId(hThread));
	if (!hook)
		return false;

	if (NOERROR != DetourTransactionBegin())
		return false;

	DetourUpdateThread(hThread);
	DetourAttach((PVOID*)&OrgGetSysColor, HookedGetSysColor);
	DetourAttach((PVOID*)&OrgGetSysColorBrush, HookedGetSysColorBrush);
	DetourAttach((PVOID*)&OrgGetSystemMetrics, HookedGetSystemMetrics);
	auto error = DetourTransactionCommit();
	ATLASSERT(error == NOERROR);
	return error == NOERROR;
}

const Theme* ThemeHelper::GetCurrentTheme() {
	return CurrentTheme;
}

void ThemeHelper::SetCurrentTheme(const Theme& theme) {
	CurrentTheme = &theme;
}
