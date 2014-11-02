#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

using namespace std;
int main() {
	OSVERSIONINFO osinfo;
	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osinfo);
	if (osinfo.dwMajorVersion > 5 || (osinfo.dwMajorVersion == 5 && osinfo.dwMinorVersion >= 1)) { // is Windows Vista or greater
		PIP_ADAPTER_ADDRESSES pAdapterAddr = NULL;
		DWORD adapterBufSize = 0;
		if (ERROR_BUFFER_OVERFLOW == GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAdapterAddr, &adapterBufSize)) { // updates adapterBufSize with correct size
			if (ERROR_BUFFER_OVERFLOW ==
		} else {
			cout << "Call to GetAdaptersAddresses() failed." << endl;
			exit(1);
		}

	} else {
		cout << "Your version of Windows is not supported." << endl;
	}
}
