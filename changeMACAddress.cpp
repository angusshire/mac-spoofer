#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <wbemidl.h>
#include <string>
using namespace std;

// macro for checking os type
#if defined(_WIN32) || defined(_WIN64)
#define isWindows() true
#else
#define isWindows() false
#endif

// struct for storing output to getIWbemClassObjectField()
typedef struct VariantStruct {
	enum {
		VT_BSTR,
		VT_I4,
		VT_DISPATCH,
		VT_I2,
		VT_NULL,
		VT_BOOL
	} type;
	union {
		LONG lVal;
		IDispatch* pdispVal;
		BSTR bstrVal;
		SHORT iVal;
		VARIANT_BOOL boolVal;
	} value;
} VariantType;

// declarations
EXTERN_C const CLSID CLSID_WbemLocator = { 0x4590F811,0x1D3A,0x11D0,{ 0x89,0x1F,0x00,0xAA,0x00,0x4B,0x2E,0x24 } }; // for some reason this wasn't defined in my linked library...
const BYTE* generateMACAddress();
char hexify(int);
VariantType* getIWbemClassObjectField(IWbemClassObject*, const wchar_t*);
void resetNIC(IWbemServices*, wstring);
void printVariantType(VariantType*);
char* ConvertBSTRToString(BSTR); // my comutil is corrupted, so redefined here...
bool valid_address(char*);
void setup_wmi_api(IWbemLocator**, IWbemServices**);
wchar_t* get_device_id(IWbemLocator*, IWbemServices*, char*);

// gets the named device's device id
wchar_t* get_device_id(IWbemLocator* pLoc, IWbemServices* pSvc, char* name) {
	unsigned int name_len = strlen(name)+1;
	wchar_t* wname = new wchar_t[name_len];
	mbstowcs(wname, name, name_len);
    // sets up enumeration object
    IEnumWbemClassObject* pEnum = NULL;
    wstring query(L"SELECT * FROM Win32_NetworkAdapter WHERE Name = \"");
    query.append(wname);
    query.append(L"\"");
    BSTR langArg = ::SysAllocString(L"WQL");
    BSTR queryArg = ::SysAllocString(query.c_str());
    DWORD hr = pSvc->ExecQuery(langArg, queryArg, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
    ::SysFreeString(langArg);
    ::SysFreeString(queryArg);
    if (FAILED(hr)) {
        printf("Unable to retrieve network adapters. Error code = %x.\n", hr);
        pLoc->Release();
        pSvc->Release();
        exit(1);
    }

    // loops through enums
    IWbemClassObject* obj = NULL;
    ULONG numElm;
    hr = pEnum->Next(WBEM_INFINITE, 1, &obj, &numElm);
    if (SUCCEEDED(hr) && obj != NULL) {
        VariantType* pDeviceID = getIWbemClassObjectField(obj, L"DeviceID");
        DWORD device_id_len = ::SysStringLen(pDeviceID->value.bstrVal);
        wchar_t* device_id = new wchar_t[device_id_len];
        wcscpy(device_id, pDeviceID->value.bstrVal);
        obj->Release();
    	pEnum->Release();
        delete[] wname;
        return device_id;
    } else {
        printf("No adapter was found with name \"%s\".\n", name);
        pLoc->Release();
        pSvc->Release();
        exit(1);
    }
}


// sets up wmi api and sets pointer to service and pointer to locator
void setup_wmi_api(IWbemLocator** ppLoc, IWbemServices** ppSvc) {
	// launch COM
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		printf("Failed to initialize COM library. Error code = %x.\n", hr);
		exit(1);
	}
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0);

	hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) ppLoc);
	if (FAILED(hr)) {
		printf("Failed to create IWbemLocator object. Error code = %x.\n", hr);
		CoUninitialize();
		exit(1);
	}

    // Connect to the root\CIMV2 namespace with the current user.
    hr = (*ppLoc)->ConnectServer(
            BSTR(L"ROOT\\CIMV2"),  //namespace
            NULL, NULL, NULL, 0, NULL, NULL, ppSvc);
    if (FAILED(hr)) {
        printf("Unable to connect to root\\cimv2. Error code = %x.\n", hr);
        (*ppLoc)->Release();
        CoUninitialize();
        exit(1);
    }

    // Set proxy so impersonation of the client occurs.
    hr = CoSetProxyBlanket(*ppSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr)) {
        printf("Could not set proxy blanket. Error code = %x.\n", hr);
       	(*ppSvc)->Release();
      	(*ppLoc)->Release();
        CoUninitialize();
        exit(1);
    }
}

// predicate for validity of mac address
bool valid_address(char* mac) {
	if (strlen(mac) == 12) {
		for (int i = 0; i < 12; i++) {
			if ((mac[i] >= '0' && mac[i] <= '9') ||
				((mac[i] >= 'A' && mac[i] <= 'F')) ||
				((mac[i] >= 'a' && mac[i] <= 'f'))) {
				continue;
			} else {
				return false;
			}
		}
		if (mac[1] == '2' || mac[1] == '6' || mac[1] == 'A' || mac[1] == 'E') {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

// converts bstr to cstring
char* ConvertBSTRToString(BSTR s) {
	char* rtn = new char[::SysStringLen(s) + 1];
	wcstombs(rtn, s, ::SysStringLen(s) +1);
	return rtn;
}

// Gets specified field of input IWbemClassObject accordingly; caller has responsibility of freeing returned VariantType
VariantType* getIWbemClassObjectField(IWbemClassObject* obj, const wchar_t* field) {
    VARIANT vRet;
	VariantInit(&vRet);
	if (SUCCEEDED(obj->Get(field, 0, &vRet, NULL, NULL))) {
		VariantType* rtn = new VariantType;
		if (rtn == NULL) {
			printf("Unable to allocate memory inside getIWbemClassObjectField()\n");
			exit(1);
		}

		if (vRet.vt == VT_BSTR) {
		// prints string using multibyte representation
			wstring temp(vRet.bstrVal, ::SysStringLen(vRet.bstrVal));
			rtn->value.bstrVal = ::SysAllocString(vRet.bstrVal);
			rtn->type = rtn->VT_BSTR;
		} else if (vRet.vt == VT_I4) {
			rtn->value.lVal = vRet.lVal;
			rtn->type = rtn->VT_I4;
		} else if (vRet.vt == VT_DISPATCH) {
			rtn->value.pdispVal = (IDispatch*) malloc(sizeof(IDispatch)); // new doesn't work on abstract types
			rtn->value.pdispVal = vRet.pdispVal;
			rtn->type = rtn->VT_DISPATCH;
		} else if (vRet.vt == VT_I2) {
			rtn->value.iVal = vRet.iVal;
			rtn->type = rtn->VT_I2;
		} else if (VT_NULL == vRet.vt) {
			rtn->type = rtn->VT_NULL;
		} else if (VT_BOOL == vRet.vt) {
			rtn->value.boolVal = vRet.boolVal;
			rtn->type = rtn->VT_BOOL;
		} else {
			printf("Unexpected variant type.\n");
			VariantClear(&vRet);
			exit(1);
		}
		VariantClear(&vRet);
		return rtn;
	} else {
		printf("Return code: %d\n", obj->Get(field, 0, &vRet, NULL, NULL));
		printf("Unable to get IWbemClassObject's %ls field.\n", field);
		exit(1);
	}
}

// the main script
int main(int argc, char* argv[]) {
	if (isWindows()) {
		OSVERSIONINFO osinfo;
		osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osinfo);
		if (osinfo.dwMajorVersion <= 5) { // is NOT Windows Vista or later
			printf("Your version of Windows is not supported.\n");
			exit(0);
		}
	} else {
		printf("Your operating system is not supported.\n");
		exit(0);
	}

	const BYTE* mac;
	bool MAC_SET = false;
	wstring where_cond;
	if (argc == 1) {
    	where_cond.append(L"NetEnabled=TRUE"); // by default sets adapter currently connected to net
    } else if (argc == 3 && (strcmp(argv[1], "-n") == 0)) {
    	wchar_t* dest = new wchar_t[strlen(argv[2])+1]();
    	mbstowcs(dest, argv[2], strlen(argv[2])+1);
    	where_cond.append(L"Name=\"");
    	where_cond.append(dest);
    	where_cond.append(L"\"");
    	delete[] dest;
    } else if (argc == 3 && (strcmp(argv[1], "-r") == 0)) {
		IWbemLocator* pLoc = NULL;
    	IWbemServices* pSvc = NULL;
    	setup_wmi_api(&pLoc, &pSvc);
    	wchar_t* device_id = get_device_id(pLoc, pSvc, argv[2]);
	    wstring device = L"Win32_NetworkAdapter.DeviceID=\"";
		device += device_id;
		device += L"\"";
    	resetNIC(pSvc, device);
    	pLoc->Release();
    	pSvc->Release();
    	return 0;
    } else if (argc == 3 && (strcmp(argv[1], "-s") == 0)) {
    	if (valid_address(argv[2])) {
    		where_cond.append(L"NetEnabled=TRUE");
    		mac = new BYTE[strlen(argv[2])+1];
    		strcpy((char*) mac, argv[2]);
    		MAC_SET = true;
    	} else {
    		printf("Invalid specified MAC address.\n");
    		exit(1);
    	}
    } else if (argc == 5 && (strcmp(argv[1], "-n") == 0) && (strcmp(argv[3], "-s") == 0)) {
    	// boilerplate
    	wchar_t* dest = new wchar_t[strlen(argv[2])+1]();
    	mbstowcs(dest, argv[2], strlen(argv[2])+1);
    	where_cond.append(L"Name=\"");
    	where_cond.append(dest);
    	where_cond.append(L"\"");
    	delete[] dest;

    	if (valid_address(argv[4])) {
    		mac = new BYTE[strlen(argv[4])+1];
    		strcpy((char*) mac, argv[4]);
    		MAC_SET = true;
    	} else {
    		printf("Invalid specified MAC address.\n");
    		exit(1);
    	}
    } else {
    	if (argc == 2 && strcmp(argv[1], "-help") == 0) {
    		printf("Description: By default, changeMACAddress generates random MAC address for adapter currently connected to the internet.\n");
    		printf("changeMACAddress options:\n");
    		printf("-n NAME\t generate random MAC address for adapter with NAME\n");
    		printf("-r NAME\t reset adapter with NAME\n");
    		printf("-s ADDRESS\t set adapter's MAC address to ADDRESS instead of randomly generating it\n");
    	} else {
			printf("Usage: changeMACAddress [-s address | -n name [-s address] | -r name]\nSpecify '-help' for full options.\n");
    	}
    	exit(1);
    }

	printf("Executing script...\n");

	// sets up wmi api
	IWbemLocator* pLoc = NULL;
	IWbemServices* pSvc = NULL;
    setup_wmi_api(&pLoc, &pSvc);

    // sets up enumeration object
    IEnumWbemClassObject* pEnum = NULL;
    wstring query(L"SELECT * FROM Win32_NetworkAdapter WHERE ");
    query.append(where_cond);
    BSTR langArg = ::SysAllocString(L"WQL");
    BSTR queryArg = ::SysAllocString(query.c_str());
    DWORD hr = pSvc->ExecQuery(langArg, queryArg, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
    ::SysFreeString(langArg);
    ::SysFreeString(queryArg);
    if (FAILED(hr)) {
    	printf("Unable to retrieve network adapters. Error code = %x.\n", hr);
    	pLoc->Release();
    	pSvc->Release();
    	exit(1);
    }

    // loops through enums
    IWbemClassObject* obj = NULL;
   	ULONG numElm;
   	int cntElm = 0; // counter for number elements enumerated
    while ((hr = pEnum->Next(WBEM_INFINITE, 1, &obj, &numElm)) != WBEM_S_FALSE) {
    	if (FAILED(hr)) {
    		break;
    	}
    	cntElm++;

    	string subkey_path("System\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}\\");
    	HKEY hkResult;
    	DWORD errcode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCSTR) subkey_path.c_str(), 0, KEY_ALL_ACCESS, &hkResult);
    	if (ERROR_SUCCESS != errcode) {
    		printf("Unable to open registry key. Error code = %d\n", errcode);
    		printf("Attempted registry path: %s\n", subkey_path.c_str());
    		exit(1);
    	}
    	DWORD subkey_index = 0;
    	DWORD longest_name = 0;
		RegQueryInfoKey(hkResult, NULL, NULL, NULL, NULL, &longest_name, NULL, NULL, NULL, NULL, NULL, NULL); // gets length of longest subkey name without null terminator
    	DWORD subkey_name_size = longest_name+1;
    	char* subkey_name  = new char[subkey_name_size];
    	BSTR adapter_guid = getIWbemClassObjectField(obj, L"GUID")->value.bstrVal;

    	do {
    		DWORD enum_errcode = RegEnumKeyEx(hkResult, subkey_index, (LPTSTR) subkey_name, &subkey_name_size, NULL, NULL, NULL, NULL);
    		if (ERROR_SUCCESS == enum_errcode || ERROR_MORE_DATA == enum_errcode) {
    			string temp(subkey_path);
    			temp.append(subkey_name);
    			subkey_name_size = longest_name+1; // resets name size accordingly
	    		subkey_index++;

    			HKEY sub_hkResult;
    			DWORD sub_errcode = RegOpenKeyEx(hkResult, (LPCSTR) subkey_name, 0, KEY_ALL_ACCESS, &sub_hkResult);
    			if (ERROR_SUCCESS == sub_errcode) {
    				DWORD bufsize = 0; // holds size of longest value name
    				if (ERROR_SUCCESS != RegQueryInfoKey(sub_hkResult, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &bufsize, NULL, NULL)) {
    					printf("Unable to query for info.\n");
    					continue;
    				} // unable to query for info
  					LPBYTE buf = new BYTE[bufsize];
    				DWORD query_errcode = RegQueryValueEx(sub_hkResult, TEXT("NetCfgInstanceId"), NULL, NULL, buf, &bufsize);
    				if (ERROR_SUCCESS == query_errcode) {
    					if (strcmp((char*) buf, ConvertBSTRToString(adapter_guid)) == 0) {

	    					if (!MAC_SET) {
		    					mac = generateMACAddress();
		    				} else {
		    					MAC_SET = false;
		    				}
	    					unsigned int mac_len = strlen((char*) mac);
	    						DWORD set_errcode = RegSetValueEx(sub_hkResult, TEXT("NetworkAddress"), 0, REG_SZ, mac, mac_len); // creates value if NetworkAddress value is not already present

    						if (ERROR_SUCCESS == set_errcode) {
    							printf("MAC address was set to: %s.\n", mac);
    						} else {
    							printf("Setting MAC address failed. Error code = %d.\n", set_errcode);
    						}
					    	delete[] mac;
					    	RegCloseKey(sub_hkResult);
					    	break;
    					}
    				}
	    			RegCloseKey(sub_hkResult);
    			} else {
    				printf("Unable to open registry subkey. Error code = %d\n", sub_errcode);
    				printf("Attempted subkey: %s\n", subkey_name);
    				continue;
    			}
    		} else if (ERROR_NO_MORE_ITEMS == enum_errcode) {
    			break;
    		} else {
    			printf("Unable to enum keys. Error code = %d\n", enum_errcode);
    			break;
    		}
    	} while (true);
    	RegCloseKey(hkResult);

		VariantType* pDeviceID = getIWbemClassObjectField(obj, L"DeviceID");
		wchar_t* DeviceID = (wchar_t*) pDeviceID->value.bstrVal;
		wstring device = L"Win32_NetworkAdapter.DeviceID=\"";
		device += DeviceID;
		device += L"\"";
    	resetNIC(pSvc, device);
    	obj->Release();
    }

    if (cntElm == 0 && (strcmp(argv[1], "-n") == 0)) {
    	printf("No adapter was found with name \"%s\".\n", argv[2]);
    }

    pEnum->Release();
	pLoc->Release();
	printf("Script ended.\n");
	return 0;
}

// prints variant tupe
void printVariantType(VariantType* p) {
	if (p->VT_I4 == p->type) {
		printf("%ld\n", p->value.lVal);
	} else if (p->VT_I2 == p->type) {
		printf("%hd\n", p->value.iVal);
	} else if (p->VT_BSTR == p->type) {
		printf("%S\n", p->value.bstrVal);
	} else if (p->VT_NULL == p->type) {
		printf("(null)\n");
	} else if (p->VT_BOOL == p->type) {
		printf("%hd\n", p->value.boolVal);
	}
}

// generates mac address
const BYTE* generateMACAddress() {
	srand(time(NULL));
	BYTE* s = new BYTE[13];
	for (int i = 0; i < 12; i++) {
		s[i] = hexify(rand() % 17);
	}
	s[13] = '\0';
	char temp[] = {'2', '6', 'A', 'E'}; // valid 2nd nibbles for mac address
	s[1] = temp[(rand() % 4)];
	return s;
}

// converts ints to hexadecimal characters
char hexify(int x) {
	if (x >= 0 && x <= 9) {
		return '0' + x;
	} else if (x >= 10 && x <= 16) {
		return 'A' + (x-10);
	} else {
		printf("Invalid int.\n");
		exit(1);
	}
}

// resets NIC based on specified device
void resetNIC(IWbemServices* pSvc, wstring device) {
	IWbemClassObject* pOutParams = NULL;
	printf("Disabling adapter...\n");
	DWORD errcode = pSvc->ExecMethod((BSTR) device.c_str(), (BSTR) L"Disable", 0, NULL, NULL, &pOutParams, NULL);
	if (ERROR_SUCCESS != errcode) { printf("Disable failed. Error code = %x.\n", errcode); }
	printf("Enabling adapter...\n");
	errcode = pSvc->ExecMethod((BSTR) device.c_str(), (BSTR) L"Enable", 0, NULL, NULL, &pOutParams, NULL);
	if (ERROR_SUCCESS != errcode) { printf("Enable failed. Error code = %x.\n", errcode); }
}
