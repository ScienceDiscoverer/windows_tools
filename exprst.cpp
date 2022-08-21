// NLAUNCH
#include <windows.h>
#include <conio.h>
#include <psapi.h>

#pragma comment( lib, "shell32" )
#pragma comment( lib, "user32" )

// TODO IMPOVE PATH GEATHERING MECHANISM
/* IntPtr MyHwnd = FindWindow(null, "Directory");
var t = Type.GetTypeFromProgID("Shell.Application");
dynamic o = Activator.CreateInstance(t);
try
{
	var ws = o.Windows();
	for (int i = 0; i < ws.Count; i++)
	{
		var ie = ws.Item(i);
		if (ie == null || ie.hwnd != (long)MyHwnd) continue;
		var path = System.IO.Path.GetFileName((string)ie.FullName);
		if (path.ToLower() == "explorer.exe")
		{
			var explorepath = ie.document.focuseditem.path;
		}
	}
}
finally
{
	Marshal.FinalReleaseComObject(o);
} */
// I have found var explorepath = new Uri(ie.LocationURL).LocalPath; to also work when there's no selected item.
// https://docs.microsoft.com/en-us/windows/win32/shell/shellwindows?redirectedfrom=MSDN
// https://stackoverflow.com/questions/20960316/get-folder-path-from-explorer-window
//C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.32.31326\bin\Hostx64\x|||TRUNCED!!



using namespace std;

//#define DEBUG
#ifdef DEBUG
#include <iostream>
void conon()
{
	///////////////////////////////////////////
	//SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	
	AllocConsole();
	FILE* s = freopen("CONIN$", "r", stdin);
	s = freopen("CONOUT$", "w", stdout);
	s = freopen("CONOUT$", "w", stderr);
	
	HANDLE chand = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO inf;
	inf.dwSize = 100;
	inf.bVisible = FALSE;
	SetConsoleCursorInfo(chand, &inf);
	
	system("chcp 65001");
	
	/////////////////////////////////////
}
#endif

typedef struct _dw_list_item
{
	DWORD dw;
	DWORD reserved;
	struct _dw_list_item *next;
} DW_LIST_ITEM;

void dwAttachUq(DW_LIST_ITEM **root, DWORD dw)
{
	DW_LIST_ITEM *head = *root;
	
	if(head != NULL)
	{
		while(head->next != NULL)
		{
			if(head->dw == dw)
			{
				return;
			}
			
			head = head->next;
		}
		
		if(head->dw == dw) // Check last element also
		{
			return;
		}
	}
	
	DW_LIST_ITEM tmp;
	tmp.dw = dw;
	tmp.next = NULL;
	static HANDLE heap = GetProcessHeap();
	LPVOID ih = HeapAlloc(heap, 0, sizeof(DW_LIST_ITEM));
	CopyMemory(ih, &tmp, sizeof(DW_LIST_ITEM));
	
	if(head == NULL)
	{
		*root = (DW_LIST_ITEM *)ih;
	}
	else
	{
		head->next = (DW_LIST_ITEM *)ih; // I ACCESSED NULLPTR HERE, BAKA ME!
	}
	//head == NULL ? *root = (STR_LIST_ITEM *)ih : head->next = (STR_LIST_ITEM *)ih; // LEGAL!
}

void dwDestoryList(DW_LIST_ITEM **root)
{
	DW_LIST_ITEM *cur = *root;
	while(cur != NULL)
	{
		DW_LIST_ITEM *tmp = cur;
		static HANDLE heap = GetProcessHeap();
		HeapFree(heap, 0, cur);
		cur = tmp->next;
	}
	*root = NULL;
}

typedef struct _str_list_item
{
	LPWSTR str;
	struct _str_list_item *next;
} STR_LIST_ITEM;

void slAttach(STR_LIST_ITEM **root, LPCWSTR str)
{
	STR_LIST_ITEM *head = *root;
	
	if(head != NULL)
	{
		while(head->next != NULL)
		{
			head = head->next;
		}
	}
	
	SIZE_T l = wcslen(str) + 1;
	static HANDLE heap = GetProcessHeap();
	LPVOID sh = HeapAlloc(heap, 0, l * sizeof(wchar_t));
	CopyMemory(sh, str, l * sizeof(wchar_t));
	
	STR_LIST_ITEM tmp;
	tmp.str = (LPWSTR)sh;
	tmp.next = NULL;
	LPVOID ih = HeapAlloc(heap, 0, sizeof(STR_LIST_ITEM));
	CopyMemory(ih, &tmp, sizeof(STR_LIST_ITEM));
	
	if(head == NULL)
	{
		*root = (STR_LIST_ITEM *)ih;
	}
	else
	{
		head->next = (STR_LIST_ITEM *)ih; // I ACCESSED NULLPTR HERE, BAKA ME!
	}
	//head == NULL ? *root = (STR_LIST_ITEM *)ih : head->next = (STR_LIST_ITEM *)ih; // LEGAL!
}

void slDestoryList(STR_LIST_ITEM **root)
{
	STR_LIST_ITEM *cur = *root;
	while(cur != NULL)
	{
		STR_LIST_ITEM *tmp = cur;
		static HANDLE heap = GetProcessHeap();
		HeapFree(heap, 0, cur->str);
		HeapFree(heap, 0, cur);
		cur = tmp->next;
	}
	*root = NULL;
}

bool pathOfExe(LPCWSTR path, LPCWSTR name)
{
	SIZE_T el = wcslen(name); // EXE length
	SIZE_T pl = wcslen(path); // Path length
	
	//wcout << path+pl-el-1 << endl;
	
	return !wcscmp(path+pl-el, name);
}

STR_LIST_ITEM *exp_wnds; // EXPLORER WINDOWS
DW_LIST_ITEM *exp_pids; // EXPLORER PID-S

#pragma warning( suppress : 4100 )  // 'lp': unreferenced formal parameter
BOOL __declspec(nothrow) CALLBACK ewGetExpPidsAndWnds(HWND hwnd, LPARAM lp)
{
	static wchar_t buff[500];
	
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	
	HANDLE hproc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	
	GetModuleFileNameExW(hproc, NULL, buff, 500);
	
	if(pathOfExe(buff, L"explorer.exe"))
	{
		dwAttachUq(&exp_pids, pid);
		
		SendMessageW(hwnd, WM_GETTEXT, 500, (LPARAM)buff);
		if(buff[0] != 0 && buff[1] == L':' && buff[2] == L'\\')
		{
			slAttach(&exp_wnds, buff);
		}
	}
	
	return TRUE;
}

int spawnProc(LPCWSTR app, LPCWSTR cmd)
{
	// NEVER QUOTE lpApplicationName IT SCREWS UP PARSING RESULTING IN ACESS_DENIED (ERR5)
	LPWSTR cmd_out = NULL;
	if(cmd != NULL)
	{
		static wchar_t cmd_out_buff[500];
		SIZE_T appl = wcslen(app);
		//SIZE_T cmdl = wcslen(cmd);
		cmd_out_buff[0] = L'\"';
		wcscpy(cmd_out_buff+1, app);
		cmd_out_buff[appl+1] = L'\"';
		cmd_out_buff[appl+2] = L' ';
		wcscpy(cmd_out_buff+appl+3, cmd);
		cmd_out = cmd_out_buff;
	}

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNORMAL;
	memset(&pi, 0, sizeof(pi));

	//stealFground();

	// Create Process ============================================================================
	BOOL res = CreateProcessW(
		app,		//  [I|O]  Name of the module to be executed, that's it
		cmd_out,	// [IO|O]  Command line to be exectued, searches PATH, adds extention
		NULL,		//  [I|O]  Sec. Attrib. for inhereting new process by child processes
		NULL,		//  [I|O]  Sec. Attrib. for inhereting new thread by child processes
		FALSE,		//    [I]  New proc. inherits each inheritable handle
		0,			//    [I]  Process creation flags
		NULL,		//  [I|O]  Ptr to environment block of new process (inherit if NULL)
		NULL,		//  [I|O]  Full path to current directory for the process
		&si,		//    [I]  Ptr to STARTUPINFO struct, if dwFlags = 0, def. values used
		&pi);		//    [O]  Ptr to PROCESS_INFORMATION struct with new proc identification info
	// ===========================================================================================

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return res == 0;
}

int wmain()
{
#ifdef DEBUG
	conon();
#endif
	
	EnumWindows(ewGetExpPidsAndWnds, 0);
	
	//system("pause");
	
	// EXTERMINATE ALL DA EXPLORERS!
	DW_LIST_ITEM *pid = exp_pids;
	while(pid != NULL)
	{
		HANDLE exp_proc = OpenProcess(PROCESS_TERMINATE, FALSE, pid->dw);
		TerminateProcess(exp_proc, 1);
		WaitForSingleObject(exp_proc, INFINITE);
		CloseHandle(exp_proc);
		
		pid = pid->next;
	}
	
	
	//system("pause");
	
	
	// REVIVE EXPLORER!
	spawnProc(L"C:\\Windows\\explorer.exe", NULL); // Spawn Shell
	
	
	//system("pause");
	Sleep(2000);
	
	
	STR_LIST_ITEM *wnd = exp_wnds;
	while(wnd != NULL)  // Spawn Windows
	{
		// Launches Explorer as separate thread within main shell explorer.exe process
		// Let The Shell do all the hard work =========================================================
		ShellExecuteW(
			NULL,			//  [I|O]  Handle to parent window that will show error or UI messages
			L"explore",		//  [I|O]  Verb string -> open|edit|explore|find|open|print|runas|NULL(def)
			wnd->str,		//    [I]  File or object that the verb is beeing executed on
			NULL,			//  [I|O]  Cmd arguments to pass to the file, if it is exe file
			NULL,			//  [I|O]  Default working directory of the action NULL -> current dir
			SW_SHOWNORMAL);	//    [I]  Parameter that sets default nCmdShow status of the window
		// ============================================================================================
		
		//std::wcout << wnd->str << std::endl;
		//spawnProc(L"C:\\Windows\\explorer.exe", wnd->str);
		
		wnd = wnd->next;
	}
	
	slDestoryList(&exp_wnds);
	dwDestoryList(&exp_pids);
	
	//system("pause");
	
	return 0;
}