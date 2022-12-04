#include <windows.h>
#include <iostream>
#include <string>

using namespace std;

//HANDLE *hParentEvent = (HANDLE*)malloc(sizeof(HANDLE) * 1);
HANDLE hParentEvent;
std::string lpParentEventName;

int main(int argc, char* argv[])
{
	std::string stringified_index = argv[0];
	//int int_index = stoi(stringified_index);

	lpParentEventName = "hParentEvent" + stringified_index;

	char c;
	hParentEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, lpParentEventName.c_str());
	if (hParentEvent == NULL)
	{
		cout << "Open Parent event failed." << endl;
		cout << "Input any char to exit." << endl;
		cin >> c;
		return GetLastError();
	}
	cout << "Input any char as a message from Parent ";
	cout << stringified_index;
	cout << " process: ";
	cin >> c;
	// Setup event that message was added
	SetEvent(hParentEvent);
	// Close event descriptor in current process
	CloseHandle(hParentEvent);
	cout << "Now input any char to exit from the process: ";
	cin >> c;
	return 0;
}
