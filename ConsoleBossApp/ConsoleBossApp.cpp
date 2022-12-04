#include <windows.h>
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>

#include <iostream>
#include <thread>
#include <cassert>
#include <string>

#define BUFSIZE 4096 

using namespace std;

std::string lpParentEventName;
std::string lpChildEventName;
//enum { BossRead, BossWrite, ParentRead, ParentWrite, ChildWrite, ChildRead, NumPipeTypes };

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

//void CreateChildProcess(void);
void WriteToPipe(void);
void ReadFromPipe(void);
void ErrorExit(PTSTR);

int main(int argc, TCHAR* argv[])
{
	SECURITY_ATTRIBUTES saAttr;
	BOOL bSuccess = FALSE;
	HANDLE hBossMutex;

	int dwWaitParentResult, dwWaitChildResult;
	char szParentAppName[] = "F:\\ConsoleParentApp.exe";
	char szChildAppName[] = "F:\\ConsoleChildApp.exe";

	printf("\n->Start of BOSS execution.\n");

	// Set the bInheritHandle flag so pipe handles are inherited. 

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 

	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		ErrorExit(const_cast<LPTSTR>("StdoutRd CreatePipe"));

	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		ErrorExit(const_cast<LPTSTR>("Stdout SetHandleInformation"));

	// Create a pipe for the child process's STDIN. 

	if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
		ErrorExit(const_cast<LPTSTR>("Stdin CreatePipe"));

	// Ensure the write handle to the pipe for STDIN is not inherited. 

	if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
		ErrorExit(const_cast<LPTSTR>("Stdin SetHandleInformation"));

	// Parent and Child processes counter dialog
	int parentProcessesCount = 1;
	int childProcessesCount = 1;
	cout << "How many Parrent processes do you want to launch? ";
	cin >> parentProcessesCount;
	cout << "Parrent processes count: " << parentProcessesCount;
	cout << endl;
	cout << "How many Child processes do you want to launch? ";
	cin >> childProcessesCount;
	cout << "Child processes count: " << childProcessesCount;
	cout << endl;

	HANDLE *hParentEvent = (HANDLE*)malloc(sizeof(HANDLE) * parentProcessesCount);
	HANDLE *hChildEvent = (HANDLE*)malloc(sizeof(HANDLE) * childProcessesCount);
	STARTUPINFO* siParent = (STARTUPINFO*)malloc(sizeof(STARTUPINFO) * parentProcessesCount);
	STARTUPINFO* siChild = (STARTUPINFO*)malloc(sizeof(STARTUPINFO) * childProcessesCount);
	PROCESS_INFORMATION* piParent = (PROCESS_INFORMATION*)malloc(sizeof(PROCESS_INFORMATION) * parentProcessesCount);
	PROCESS_INFORMATION* piChild = (PROCESS_INFORMATION*)malloc(sizeof(PROCESS_INFORMATION) * childProcessesCount);
	//SECURITY_ATTRIBUTES saBoss;
	//saBoss.nLength = sizeof(saBoss);
	//saBoss.bInheritHandle = TRUE;
	//saBoss.lpSecurityDescriptor = nullptr;
	//SECURITY_ATTRIBUTES* saParent = (SECURITY_ATTRIBUTES*)malloc(sizeof(SECURITY_ATTRIBUTES) * parentProcessesCount);
	//SECURITY_ATTRIBUTES* saChild = (SECURITY_ATTRIBUTES*)malloc(sizeof(SECURITY_ATTRIBUTES) * childProcessesCount);

	// Create Mutex
	hBossMutex = CreateMutex(NULL, FALSE, "BossMutex");

	// Start required number of processes that wait for Parrent message input
	for (int i = 1; i <= parentProcessesCount; i++)
	{
		// Create Parent message events
		lpParentEventName = "hParentEvent" + std::to_string(i);
		hParentEvent[i] = CreateEvent(NULL, TRUE, FALSE, lpParentEventName.c_str());

		if (hParentEvent[i] == NULL)
			return GetLastError();

		ZeroMemory(&siParent[i], sizeof(siParent[i]));
		siParent[i].cb = sizeof(siParent[i]);

		if (!CreateProcess(szParentAppName, const_cast<char*>(std::to_string(i).c_str()), NULL, NULL, FALSE,
			CREATE_NEW_CONSOLE, NULL, NULL, &siParent[i], &piParent[i]))
		{
			cout << "Can't create Parent ";
			cout << i;
			cout << " process";
			cout << endl;
			return 0;
		}
	}

	// Start required number of processes that wait for Child message input
	for (int j = 1; j <= childProcessesCount; j++)
	{
		// Create Child message events
		lpChildEventName = "hChildEvent" + std::to_string(j);
		hChildEvent[j] = CreateEvent(NULL, TRUE, FALSE, lpChildEventName.c_str());

		if (hChildEvent[j] == NULL)
			return GetLastError();

		// Set up members of the PROCESS_INFORMATION structure. 

		ZeroMemory(&piChild[j], sizeof(PROCESS_INFORMATION));

		// Set up members of the STARTUPINFO structure. 
		// This structure specifies the STDIN and STDOUT handles for redirection.

		ZeroMemory(&siChild[j], sizeof(siChild[j]));
		//ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		siChild[j].cb = sizeof(STARTUPINFO);
		siChild[j].hStdError = g_hChildStd_OUT_Wr;
		siChild[j].hStdOutput = g_hChildStd_OUT_Wr;
		siChild[j].hStdInput = g_hChildStd_IN_Rd;
		siChild[j].dwFlags |= STARTF_USESTDHANDLES;

		// Create the child process. 

		if (!CreateProcess(szChildAppName,
			const_cast<char*>(std::to_string(j).c_str()),     // command line 
			NULL,                                             // process security attributes 
			NULL,                                             // primary thread security attributes 
			TRUE,                                             // handles are inherited 
			CREATE_NEW_CONSOLE,                               // creation flags 
			NULL,                                             // use parent's environment 
			NULL,                                             // use parent's current directory 
			&siChild[j],                                      // STARTUPINFO pointer 
			&piChild[j]))                                     // receives PROCESS_INFORMATION 
		{
			cout << "Can't create Child ";
			cout << j;
			cout << " process";
			cout << endl;
			return 0;
		}

		// Close handles to the stdin and stdout pipes no longer needed by the child process.
		// If they are not explicitly closed, there is no way to recognize that the child process has ended.

		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}

	// Get a handle to an input file for the Boss. 
	// This app assumes a plain text file and uses string output to verify data flow. 

	char szPipeBufferFile[] = "F:\\PipeBuffer.txt";

	g_hInputFile = CreateFile(
		szPipeBufferFile,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY,
		NULL);

	if (g_hInputFile == INVALID_HANDLE_VALUE)
		ErrorExit(const_cast<LPTSTR>("CreateFile"));

	// Write to the pipe that is the standard input for a child process. 
	// Data is written to the pipe's buffers, so it is not necessary to wait
	// until the child process is running before writing data.

	WriteToPipe();
	printf("\n->Contents of %S written to child STDIN pipe.\n", szPipeBufferFile);

	// Read from pipe that is the standard output for child process. 

	printf("\n->Contents of child process STDOUT:\n\n");
	ReadFromPipe();

	printf("\n->End of BOSS execution.\n");
	cout << endl;

	// The remaining open handles are cleaned up when this process terminates. 
	// To avoid resource leaks in a larger application, close handles explicitly
	

	// Launch Parent process event listeners
	for (int i = 1; i <= parentProcessesCount; i++)
	{
		// Wait notification about event from Parrent process
		dwWaitParentResult = WaitForSingleObject(piParent[i].hProcess, INFINITE);
		if (dwWaitParentResult != WAIT_OBJECT_0)
		{
			cout << "Wait for single object from Parent ";
			cout << i;
			cout << " failed";
			cout << endl;
			cout << "Press any key to exit." << endl;
			//return dwWaitParentResult;
		}
			
		cout << "Message from Parent ";
		cout << i;
		cout << " process";
		cout << endl;
		CloseHandle(hParentEvent[i]);

		// Close Parent process descriptor
		CloseHandle(piParent[i].hThread);
		CloseHandle(piParent[i].hProcess);
	}

	// Launch Child process event listeners
	for (int j = 1; j <= childProcessesCount; j++)
	{
		// Wait notification about event from Child process
		dwWaitChildResult = WaitForSingleObject(piChild[j].hProcess, INFINITE);
		if (dwWaitChildResult != WAIT_OBJECT_0)
		{
			cout << "Wait for single object from Child ";
			cout << j;
			cout << " failed";
			cout << endl;
			cout << "Press any key to exit" << endl;
			//return dwWaitChildResult;
		}
			
		cout << "Message from Child ";
		cout << j;
		cout << " process";
		cout << endl;
		CloseHandle(hChildEvent[j]);

		// Close Child process descriptor
		CloseHandle(piChild[j].hThread);
		CloseHandle(piChild[j].hProcess);
	}

	cout << "Press any key to exit: ";
	cin.get();
	return 0;
}



void WriteToPipe(void)

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
{
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE];
	BOOL bSuccess = FALSE;

	for (;;)
	{
		bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
		if (!bSuccess || dwRead == 0) break;

		bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, NULL);
		if (!bSuccess) break;
	}

	// Close the pipe handle so the child process stops reading. 

	if (!CloseHandle(g_hChildStd_IN_Wr))
		ErrorExit(const_cast<LPTSTR>("StdInWr CloseHandle"));
}

void ReadFromPipe(void)

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
{
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE];
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;)
	{
		bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
		if (!bSuccess || dwRead == 0) break;

		bSuccess = WriteFile(hParentStdOut, chBuf,
			dwRead, &dwWritten, NULL);
		if (!bSuccess) break;
	}
}

void ErrorExit(PTSTR lpszFunction)

// Format a readable error message, display a message box, 
// and exit from the application.
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(1);
}