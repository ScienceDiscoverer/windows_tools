// nCONSOLE nDBG
//#include <sddb>
#include <windows.h>
#include <txt>
#include <wtxt>

#define TRASH_COLLECT

#ifndef TRASH_COLLECT
#include <stdp>
#endif

#ifdef TRASH_COLLECT
#pragma comment( lib, "shell32" )

ui64 coll_after_hrs = 24; // Files deleted this many hours ago will be garbage-collected
#endif

// 99% of the time for 1 user PC its this folder: 
const wchar_t *trash = L"C:\\$Recycle.Bin\\S-1-5-21-1780296672-2431193663-1691526161-1001\\$I*";

#ifdef TRASH_COLLECT
void collGarb(const wtxt &fn);
#else
void printDelMeta(const wtxt &fn);
bool fileExists(const wtxt &fn);
#endif

int wmain(int argc, wchar_t **argv) // Full path to user's RB folder
{
	if(argc >= 2)
	{
		trash = argv[1];
#ifdef TRASH_COLLECT
		if(argc >= 3)
		{
			coll_after_hrs = t2i(wtxt(argv[2]));
		}
#endif
	}
	
	WIN32_FIND_DATA fd;
	HANDLE f = FindFirstFile(trash, &fd);
	if(f == INVALID_HANDLE_VALUE)
	{
		return 1;
	}
	
	wtxt fn, fn_base = trash;
	txtd(fn_base, ~fn_base - 3, 3);
	do
	{
		fn = fn_base + fd.cFileName;
#ifdef TRASH_COLLECT
		collGarb(fn);
#else
		printDelMeta(fn);
#endif
	}
	while(FindNextFile(f, &fd));
	FindClose(f);
	
#ifndef TRASH_COLLECT
	p|TB|P;
#endif
	return 0;
}

#ifdef TRASH_COLLECT

void collGarb(const wtxt &fn)
{
	// Create or open File or Device =================================================================
	HANDLE fh = CreateFile(
		fn,							//   [I]  Name of the file or device to create/open
		GENERIC_READ,				//   [I]  Requested access GENERIC_READ|GENERIC_WRITE|0
		FILE_SHARE_READ,			//   [I]  Sharing mode FILE_SHARE_READ|WRITE|DELETE|0
		NULL,						// [I|O]  SECURITY_ATTRIBUTES for file, handle inheritability
		OPEN_EXISTING,				//   [I]  Action to take if file/device exist or not
		FILE_FLAG_SEQUENTIAL_SCAN,	//   [I]  Attributes and flags for file/device
		NULL);						// [I|O]  Handle to template file to copy attributes from
	// ===============================================================================================

	if(fh == INVALID_HANDLE_VALUE)
	{
		return;
	}
	
	txt f;
	txt buff = 0x400;
	while(ReadFile(fh, buff, (DWORD)!buff, *buff, NULL) && ~buff)
	{
		f += buff;
	}
	CloseHandle(fh);
	
	wtxt fn_data = fn;
	txto(fn_data, (L"$I" << fn_data) + 1, 'R');
	
	for(ui64 i = 0; i < ~f; ++i)
	{
		char *d = (char *)f + i;
		if(*((ui64 *)d) == 0x2)
		{
			SYSTEMTIME cur_st;
			GetSystemTime(&cur_st);
			FILETIME cur_ft;
			SystemTimeToFileTime(&cur_st, &cur_ft);
			
			ui64 del_ft = *((ui64 *)(d + 0x10));
			
			// 10000000ull - 100 nanoseconds in 1 second
			if(*((ui64 *)(&cur_ft)) - del_ft <= coll_after_hrs * 3600ull * 10000000ull)
			{
				return;
			}
			
			break;
		}
	}
	
	DWORD attr = GetFileAttributes(fn_data);
	if(attr == INVALID_FILE_ATTRIBUTES) // Data does not exist
	{
		return;
	}
	
	if(attr & FILE_ATTRIBUTE_DIRECTORY)
	{
		/* typedef struct _SHFILEOPSTRUCTW {
		  HWND         hwnd;
		  UINT         wFunc;
		  PCZZWSTR     pFrom;
		  PCZZWSTR     pTo;
		  FILEOP_FLAGS fFlags;
		  BOOL         fAnyOperationsAborted;
		  LPVOID       hNameMappings;
		  PCWSTR       lpszProgressTitle;
		} SHFILEOPSTRUCTW, *LPSHFILEOPSTRUCTW; */
		
		SHFILEOPSTRUCT shfo;
		memset(&shfo, 0x0, sizeof(SHFILEOPSTRUCT));
		shfo.wFunc = FO_DELETE;
		shfo.pFrom = fn_data += '\0';
		shfo.fFlags = FOF_NO_UI;
		
		SHFileOperation(&shfo);
	}
	else
	{
		DeleteFile(fn_data);
	}
}

#else

void printDelMeta(const wtxt &fn)
{
	// Create or open File or Device =================================================================
	HANDLE fh = CreateFile(
		fn,							//   [I]  Name of the file or device to create/open
		GENERIC_READ,				//   [I]  Requested access GENERIC_READ|GENERIC_WRITE|0
		FILE_SHARE_READ,			//   [I]  Sharing mode FILE_SHARE_READ|WRITE|DELETE|0
		NULL,						// [I|O]  SECURITY_ATTRIBUTES for file, handle inheritability
		OPEN_EXISTING,				//   [I]  Action to take if file/device exist or not
		FILE_FLAG_SEQUENTIAL_SCAN,	//   [I]  Attributes and flags for file/device
		NULL);						// [I|O]  Handle to template file to copy attributes from
	// ===============================================================================================

	if(fh == INVALID_HANDLE_VALUE)
	{
		return;
	}
	
	txt f;
	txt buff = 0x400;
	while(ReadFile(fh, buff, (DWORD)!buff, *buff, NULL) && ~buff)
	{
		f += buff;
	}
	CloseHandle(fh);
	
	wtxt fn_np = fn;
	txtd(fn_np, 0, L"$I" << fn_np);
	wtxt fn_data = fn;
	txto(fn_data, (L"$I" << fn_data) + 1, 'R');
	
	for(ui64 i = 0; i < ~f; ++i)
	{
		char *d = (char *)f + i;
		if(*((ui64 *)d) == 0x2)
		{
			p|"-----------------------------------------"|N;
			p|"Name: "|fn_np|S, (fileExists(fn_data) ? p|G|"DATA FOUND!" : p|R|"DATA MISSING!")|D|N;
			p|"Size: "|PN|*((ui64 *)(d + 0x8))|" B"|N;
			
			ui64 ft = *((ui64 *)(d + 0x10));
			SYSTEMTIME st;
			FileTimeToSystemTime((FILETIME *)(&ft), &st);
			
			p|"Date: ";
			p|st.wYear|'.'|SP(2, '0');
			p|st.wMonth|SP(0)|'.'|SP(2)|st.wDay|S|st.wHour|SP(0)|':'|SP(2);
			p|st.wMinute|SP(0)|':'|SP(2)|st.wSecond|SP(0, ' ')|'\''|st.wMilliseconds|N;
			p|"Path: "|(wchar_t *)(d + 0x1C)|N;
			break;
		}
	}
}

bool fileExists(const wtxt &fn)
{
	DWORD attr = GetFileAttributes(fn);
	if(attr == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	
	if(attr & FILE_ATTRIBUTE_DIRECTORY)
	{
		p|Y|"DIR ";
		return true;
	}
	
	return true;
}

#endif