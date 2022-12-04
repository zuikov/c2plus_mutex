#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;

#define BUFSIZE 4096 

//HANDLE *hChildEvent = (HANDLE*)malloc(sizeof(HANDLE) * 1);
HANDLE hChildEvent;
std::string lpChildEventName;

int main(int argc, char* argv[])
{
	CHAR chBuf[BUFSIZE];
	DWORD dwRead, dwWritten;
	HANDLE hStdin, hStdout;
	BOOL bSuccess;

	std::string stringified_index = argv[0];
	//int int_index = stoi(stringified_index);

	lpChildEventName = "hChildEvent" + stringified_index;

	char c;
	hChildEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, lpChildEventName.c_str());
	if (hChildEvent == NULL)
	{
		cout << "Open Child event failed." << endl;
		cout << "Input any char to exit." << endl;
		cin >> c;
		return GetLastError();
	}
	cout << "Input any char as a message from Child ";
	cout << stringified_index;
	cout << " process: ";
	cin >> c;
	//-------------------------------------------------------------------------------------
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (
		(hStdout == INVALID_HANDLE_VALUE) ||
		(hStdin == INVALID_HANDLE_VALUE)
		)
		ExitProcess(1);

	// Send something to this process's stdout using printf.
	printf("\n ** This is a message from the child process. ** \n");

	// This simple algorithm uses the existence of the pipes to control execution.
	// It relies on the pipe buffers to ensure that no data is lost.
	// Larger applications would use more advanced process control.

	for (;;)
	{
		// Read from standard input and stop on error or no data.
		bSuccess = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL);

		if (!bSuccess || dwRead == 0)
			break;

		// Write to standard output and stop on error.
		bSuccess = WriteFile(hStdout, chBuf, dwRead, &dwWritten, NULL);

		if (!bSuccess)
			break;
	}

	// ------------------------------------------------------------------------------------
	// Setup event that message was added
	SetEvent(hChildEvent);
	// Close event descriptor in current process
	CloseHandle(hChildEvent);
	cout << "Now input any char to exit from the process: ";
	cin >> c;
	return 0;
}