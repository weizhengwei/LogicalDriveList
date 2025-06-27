// Enable Visual Style
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <dbt.h>
#include <winioctl.h>
#include <initguid.h>
#include <usbiodef.h>
#include <ntddstor.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <map>
#include <codecvt>
#include <sstream>

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "User32.lib")
#endif

#define CLASSNAME _T("CN_DRIVELIST")
#define WIN_TITLE _T("DriveList")
#define VER _T("V1.0.0")

BOOL bTopMost = FALSE;
HWND hListView;
#define ID_LISTVIEW 1001
HDEVNOTIFY hDevNotifyDevice;

//file menu
#define IDM_FILE_REFRESH 101
#define IDM_FILE_EXIT 102

//setting menu
#define IDM_SETTING_TOP 201
#define IDM_SETTING_DARK 201

//help menu
#define IDM_HELP_ABOUT 301

#define IDM_CTX_OPEN 1001
#define IDM_CTX_COPY 1002
#define IDM_CTX_PROP 1003
#define IDM_CTX_LOCKVOLUME 1004

using tstring = std::basic_string<TCHAR>;

struct DriveData
{
	TCHAR chDrive;// 'C'
	std::wstring strDrive;//"C:\\"
	std::wstring strLabel;//
	UINT uDriveType;//0-5
	std::wstring strType;//
	std::wstring strFSName;//
	std::wstring strDosDevice;
	std::wstring strVolumeGUIDPath;//GUID卷路径
	std::wstring strSerialNum;//序列号
	std::wstring strTotalSpace;
	std::wstring strAvailableSpace;
	std::wstring strUsedPercent;
	std::wstring strStatus;
};

std::map<UINT, std::wstring> mapDriveTypes
{
	{DRIVE_UNKNOWN, L"UNKNOWN"},
	{DRIVE_NO_ROOT_DIR, L"NO_ROOT_DIR"},
	{DRIVE_REMOVABLE, L"REMOVABLE"},
	{DRIVE_FIXED, L"FIXED"},
	{DRIVE_REMOTE, L"REMOTE"},
	{DRIVE_CDROM, L"CDROM"},
	{DRIVE_RAMDISK, L"RAMDISK"},
};

std::vector<std::wstring> vecCols = {
	L"Drive（盘符）",
	L"Label（标签）",
	L"Type（类型）",
	L"FileSystem（文件系统）",
	L"SerialNumber（序列号）",
	L"MS Dos Device",
	L"GUID Volume Path（卷路径）",
	L"TotalSpace（总空间）",
	L"AvailableSpace（可用空间）",
	L"UsedPercent（使用百分比）"
};

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateMainMenu(HWND hwnd);
void RegisterDeviceNotify(HWND hwnd);
HFONT MakeFont(LPTSTR szFont, INT nPointSize, INT nWeight, BOOL bItalic, BOOL bUnderline, BOOL bStrikeOut, DWORD dwCharSet);
void CreateListView(HWND hwnd);
std::vector<DriveData> GetDriveList();
std::wstring GetDriveTypeString(UINT uDriveType);
void GetDriveVolumeInfo(const std::wstring& strDrive, DriveData& itemDrive);
void GetDriveSpace(const std::wstring& strDrive, DriveData& itemDrive);
void FillListView();
void ProcessCommandMessage(HWND hwnd, WPARAM wParam);
void CopySelectItemInfo(HWND hwnd);
void CopyTextToClipboard(HWND hwnd, const tstring& text);
void RefreshDeviceList();
BOOL IsAdmin();
BOOL IsProcessElevated();

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPreInstance, LPTSTR lpCmdLine, int ShowMode)
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	INITCOMMONCONTROLSEX iccex;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&iccex);

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WinProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = CLASSNAME;
	ATOM res = RegisterClass(&wc);
	if (!res)
	{
		MessageBox(NULL, _T("Can't Register Class"), _T("error"), MB_OK);
		return 0;
	}
	HWND hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		CLASSNAME,
		WIN_TITLE,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1400,
		600,
		NULL,
		NULL,
		hInstance,
		NULL);
	CreateMainMenu(hwnd);
	RegisterDeviceNotify(hwnd);

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

void CreateMainMenu(HWND hwnd)
{
	HMENU hMainMenu = CreateMenu();
	HMENU hFileSubMenu = CreatePopupMenu();
	AppendMenu(hFileSubMenu, MF_STRING, IDM_FILE_REFRESH, _T("Refresh"));
	AppendMenu(hFileSubMenu, MF_SEPARATOR, 0, _T(""));
	AppendMenu(hFileSubMenu, MF_STRING, IDM_FILE_EXIT, _T("Exit"));
	AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hFileSubMenu, _T("File"));

	HMENU hSettingSubMenu = CreatePopupMenu();
	AppendMenu(hSettingSubMenu, MF_STRING | bTopMost ? MF_CHECKED : MF_UNCHECKED , IDM_SETTING_TOP, _T("顶置"));
	AppendMenu(hSettingSubMenu, MF_STRING, IDM_SETTING_DARK, _T("DarkMode"));
	AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hSettingSubMenu, _T("Setting"));

	HMENU hHelpSubMenu = CreatePopupMenu();
	AppendMenu(hHelpSubMenu, MF_STRING, IDM_HELP_ABOUT, _T("About"));
	AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hHelpSubMenu, _T("Help"));
	SetMenu(hwnd, hMainMenu);

	//DestroyMenu(hFileSubMenu);
	//DestroyMenu(hSettingSubMenu);
	//DestroyMenu(hHelpSubMenu);
	//DestroyMenu(hMainMenu);
}

void RegisterDeviceNotify(HWND hwnd)
{
	DEV_BROADCAST_DEVICEINTERFACE dbd = {};
	ZeroMemory(&dbd, sizeof(dbd));
	dbd.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	//dbd.dbcc_devicetype = DBT_DEVTYP_VOLUME;
	dbd.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;// DBT_DEVTYP_DEVICEINTERFACE;
	dbd.dbcc_classguid = GUID_DEVINTERFACE_VOLUME;
		
	//GUID_DEVINTERFACE_USB_HUB {F18A0E88-C30C-11D0-8815-00A0C906BED8}
	//GUID_DEVINTERFACE_USB_DEVICE {A5DCBF10-6530-11D2-901F-00C04FB951ED}
	//GUID_DEVINTERFACE_VOLUME {53F5630D-B6BF-11D0-94F2-00A0C91EFB8B}
	hDevNotifyDevice = RegisterDeviceNotification(hwnd, &dbd, DEVICE_NOTIFY_WINDOW_HANDLE);
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		CreateListView(hwnd);
		break;
	}
	case WM_SIZE:
	{
		RECT rcClient;
		GetClientRect(hwnd, &rcClient);
		InflateRect(&rcClient, -15, -15);
		SetWindowPos(hListView,
			NULL,
			rcClient.left,
			rcClient.top,
			rcClient.right - rcClient.left,
			rcClient.bottom - rcClient.top,
			//SWP_SHOWWINDOW
			SWP_NOACTIVATE | SWP_NOZORDER
		);
		break;
	}
	case WM_COMMAND:
	{
		ProcessCommandMessage(hwnd, wParam);
		break;
	}
	case WM_CONTEXTMENU:
	{
		POINT pt;
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);
		ScreenToClient(hListView, &pt);
		LVHITTESTINFO hitTestInfo;
		hitTestInfo.pt = pt;
		ListView_HitTest(hListView, &hitTestInfo);
		if (hitTestInfo.iItem != -1) {
			UINT uCount = ListView_GetSelectedCount(hListView);
			HMENU hMenu = CreatePopupMenu();
			if (uCount == 1)
			{
				AppendMenu(hMenu, MF_STRING, IDM_CTX_OPEN, L"打开");

				AppendMenu(hMenu, MF_STRING, IDM_CTX_COPY, L"复制信息");
				AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
				AppendMenu(hMenu, MF_STRING, IDM_CTX_PROP, L"属性");
				AppendMenu(hMenu, MF_STRING, IDM_CTX_LOCKVOLUME, L"锁定");
				
			}
			else
			{
				AppendMenu(hMenu, MF_STRING, IDM_CTX_COPY, L"复制信息");
			}
			

			// Determine the menu position
			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			UINT uFlags = TPM_RIGHTBUTTON; // Use this flag to handle right clicks
			TrackPopupMenuEx(hMenu, uFlags, pt.x, pt.y, hwnd, NULL);
			DestroyMenu(hMenu);
		}
		break;
	}
	case WM_DEVICECHANGE:
	{
		if (wParam == DBT_DEVICEARRIVAL)
		{
			DEV_BROADCAST_HDR* pHdr = (DEV_BROADCAST_HDR*)lParam;
			TCHAR msg[128] = {};
			_stprintf_s(msg, 128, _T("DBT_DEVICEARRIVAL type=%d\n"), pHdr->dbch_devicetype);
			OutputDebugString(msg);
			if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
			{
				PDEV_BROADCAST_DEVICEINTERFACE lpdbv = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
				std::wstring device_path = std::wstring(lpdbv->dbcc_name);
				OutputDebugString(L"USB device added.    ");
				OutputDebugString(device_path.c_str());
				OutputDebugString(L"\n");
			}
			else if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				DEV_BROADCAST_VOLUME* pVolume = (DEV_BROADCAST_VOLUME*)pHdr;
				std::wstring strLetter = L"Volume Arrival ";
				// 获取驱动器号
				for (int i = 0; i < 26; i++)
				{
					if (pVolume->dbcv_unitmask & (1 << i))
					{
						TCHAR chLetter = 'A' + i;
						strLetter += chLetter;
						strLetter += L":";
					}
				}
				strLetter += L"\n";
				OutputDebugString(strLetter.c_str());
				RefreshDeviceList();
			}
			
		}
		else if (wParam == DBT_DEVICEREMOVECOMPLETE)
		{
			DEV_BROADCAST_HDR* pHdr = (DEV_BROADCAST_HDR*)lParam;
			TCHAR msg[128] = {};
			_stprintf_s(msg, 128, _T("DBT_DEVICEREMOVECOMPLETE type=%d\n"), pHdr->dbch_devicetype);
			OutputDebugString(msg);
			if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
				PDEV_BROADCAST_DEVICEINTERFACE lpdbv = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
				std::wstring device_path = std::wstring(lpdbv->dbcc_name);
				OutputDebugString(L"USB device removed.    ");
				OutputDebugString(device_path.c_str());
				OutputDebugString(L"\n");

			}
			else if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				DEV_BROADCAST_VOLUME* pVolume = (DEV_BROADCAST_VOLUME*)pHdr;
				std::wstring strLetter = L"Volume Removed ";
				// 获取驱动器号
				for (int i = 0; i < 26; i++)
				{
					if (pVolume->dbcv_unitmask & (1 << i))
					{
						TCHAR chLetter = 'A' + i;
						strLetter += chLetter;
						strLetter += L":";
					}
				}
				strLetter += L"\n";
				OutputDebugString(strLetter.c_str());
				RefreshDeviceList();
			}
			
		}
		break;
	}
	case WM_DESTROY:
	{
		UnregisterDeviceNotification(hDevNotifyDevice);
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

HFONT MakeFont(LPCTSTR szFont, INT nPointSize, INT nWeight, BOOL bItalic, BOOL bUnderline, BOOL bStrikeOut, DWORD dwCharSet) // by Napalm
{
	HDC hDC = GetDC(HWND_DESKTOP);
	nPointSize = -MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(HWND_DESKTOP, hDC);
	return CreateFont(nPointSize, 0, 0, 0, nWeight, bItalic, bUnderline, bStrikeOut,
		dwCharSet, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, szFont);
}

void CreateListView(HWND hwnd)
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	InflateRect(&rc, -15, -15);
	hListView = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_LISTVIEW,
		NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | LVS_REPORT | LVS_NOSORTHEADER,
		rc.left,
		rc.top,
		rc.right - rc.left,
		rc.bottom - rc.top,
		hwnd,
		(HMENU)ID_LISTVIEW,
		GetModuleHandle(NULL),
		NULL);
	HFONT hfArial = MakeFont(_T("Arial"), 11, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET);
	SendMessage(hListView, WM_SETFONT, (WPARAM)hfArial, FALSE);
	ListView_SetTextColor(hListView, RGB(10, 10, 160));
	//ListView_SetTextColor(hListView, RGB(80, 80, 80));

	ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);

	TCHAR szText[32] = {};
	LVCOLUMN lvCol;
	lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.cx = 150;
	lvCol.pszText = szText;

	std::map<int, int> vecColWidth = {
		{0, 90},
		{1, 100},
		{2, 100},
		{3, 100},
		{4, 100},
		{5, 200},
		{6, 380},
		{7, 100},
		{8, 100},
		{9, 100},
		{10, 100}
	};

	for (int iCol = 0; iCol < vecCols.size(); iCol++)
	{
		lvCol.cx = vecColWidth.at(iCol);
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), vecCols[iCol].c_str());
		ListView_InsertColumn(hListView, iCol, &lvCol);
	}
	
	FillListView();
}

void FillListView()
{
	auto DriveList = GetDriveList();
	LVITEM lvItem = {};
	TCHAR szText[128] = {};
	for (lvItem.iItem = 0; lvItem.iItem < DriveList.size(); lvItem.iItem++)
	{
		auto itemDrive = DriveList[lvItem.iItem];

		lvItem.iSubItem = 0;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%C:"), itemDrive.chDrive);
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_InsertItem(hListView, &lvItem);
		
		lvItem.iSubItem = 1;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strLabel.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);
		
		lvItem.iSubItem = 2;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strType.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);
		
		lvItem.iSubItem = 3;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strFSName.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);
		
		lvItem.iSubItem = 4;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strSerialNum.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);

		lvItem.iSubItem = 5;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strDosDevice.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);

		lvItem.iSubItem = 6;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strVolumeGUIDPath.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);
		
		lvItem.iSubItem = 7;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strTotalSpace.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);
		
		lvItem.iSubItem = 8;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strAvailableSpace.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);

		lvItem.iSubItem = 9;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strUsedPercent.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);

		lvItem.iSubItem = 10;
		_stprintf_s(szText, ARRAYSIZE(szText), _T("%s"), itemDrive.strStatus.c_str());
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = szText;
		ListView_SetItem(hListView, &lvItem);
	}
}

std::vector<DriveData> GetDriveList()
{
	TCHAR szBuffer[1024] = {};
	DWORD dwRes = GetLogicalDriveStrings(ARRAYSIZE(szBuffer), szBuffer);
	TCHAR* szDrive = szBuffer;
	std::vector<DriveData> vecDrives;
	while (*szDrive)
	{
		std::wstring strDrive(szDrive);
		DriveData itemDrive;
		GetDriveVolumeInfo(szDrive, itemDrive);
		GetDriveSpace(szDrive, itemDrive);
		szDrive += _tcslen(szDrive) + 1;
		vecDrives.push_back(itemDrive);
	}
	return vecDrives;
}

std::wstring GetDriveTypeString(UINT uDriveType)
{
	auto item = mapDriveTypes.find(uDriveType);
	if (item != mapDriveTypes.end())
	{
		return item->second;
	}
	return L"";
}

void GetDriveVolumeInfo(const std::wstring& strDrive, DriveData& itemDrive)
{
	itemDrive.chDrive = strDrive[0];
	itemDrive.strDrive = strDrive;
	UINT uDriveType = GetDriveType(strDrive.c_str());
	itemDrive.uDriveType = uDriveType;
	itemDrive.strType = GetDriveTypeString(uDriveType);

	TCHAR szDevName[16] = {};
	_stprintf_s(szDevName, ARRAYSIZE(szDevName), _T("%C:"), strDrive[0]);
	TCHAR szTargetPath[128] = {};
	DWORD dwRes = QueryDosDevice(szDevName, szTargetPath, ARRAYSIZE(szTargetPath));
	if (dwRes)
	{
		itemDrive.strDosDevice = szTargetPath;
	}

	TCHAR szVolumeGUIDPath[MAX_PATH + 1] = {};
	BOOL bRes = GetVolumeNameForVolumeMountPoint(strDrive.c_str(), szVolumeGUIDPath, ARRAYSIZE(szVolumeGUIDPath));
	if (bRes)
	{
		itemDrive.strVolumeGUIDPath = szVolumeGUIDPath;
	}

	TCHAR szVolumeName[MAX_PATH + 1] = {};
	DWORD dwVolumeSeralNum = 0;
	DWORD dwMaxComponentLen = 0;
	DWORD dwFSFlags = 0;
	TCHAR szFSName[MAX_PATH + 1] = {};
	bRes = GetVolumeInformation(strDrive.c_str(),
		szVolumeName,
		ARRAYSIZE(szVolumeName),
		&dwVolumeSeralNum,
		&dwMaxComponentLen,
		&dwFSFlags,
		szFSName,
		ARRAYSIZE(szFSName));
	if (bRes)
	{
		TCHAR szSerialNum[128] = {};
		_stprintf_s(szSerialNum, ARRAYSIZE(szSerialNum), _T("%4X-%4X"), dwVolumeSeralNum>>16, dwVolumeSeralNum & 0x0000FFFF);
		itemDrive.strSerialNum = szSerialNum;
		itemDrive.strLabel = szVolumeName;
		itemDrive.strFSName = szFSName;
	}
}

std::wstring FormatSize(ULONGLONG bytes) {
	const ULONGLONG KB = 1024;
	const ULONGLONG MB = KB * 1024;
	const ULONGLONG GB = MB * 1024;
	const ULONGLONG TB = GB * 1024;

	std::wstringstream ss;

	if (bytes >= TB) {
		ss << static_cast<double>(bytes) / TB << L" TB";
	}
	else if (bytes >= GB) {
		ss << static_cast<double>(bytes) / GB << L" GB";
	}
	else if (bytes >= MB) {
		ss << static_cast<double>(bytes) / MB << L" MB";
	}
	else if (bytes >= KB) {
		ss << static_cast<double>(bytes) / KB << L" KB";
	}
	else {
		ss << bytes << L" Bytes";
	}

	return ss.str();
}

void GetDriveSpace(const std::wstring& strDrive, DriveData& itemDrive)
{
	DISK_SPACE_INFORMATION dsi;
	std::wstring strInfo;
	if (GetDiskSpaceInformation(strDrive.c_str(), &dsi) == S_OK)
	{
		TCHAR szBuf[128] = {};
		_stprintf_s(szBuf, 128, _T("ActualAvailableAllocationUnits=%llu\n"), dsi.ActualAvailableAllocationUnits);
		strInfo += szBuf;
		//todo
		_stprintf_s(szBuf, 128, _T("ActualAvailableAllocationUnits=%llu\n"), dsi.ActualAvailableAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("ActualPoolUnavailableAllocationUnits=%llu\n"), dsi.ActualPoolUnavailableAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("CallerTotalAllocationUnits=%llu\n"), dsi.CallerTotalAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("CallerAvailableAllocationUnits=%llu\n"), dsi.CallerAvailableAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("CallerPoolUnavailableAllocationUnits=%llu\n"), dsi.CallerPoolUnavailableAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("UsedAllocationUnits=%llu\n"), dsi.UsedAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("TotalReservedAllocationUnits=%llu\n"), dsi.TotalReservedAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("VolumeStorageReserveAllocationUnits=%llu\n"), dsi.VolumeStorageReserveAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("AvailableCommittedAllocationUnits=%llu\n"), dsi.AvailableCommittedAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("PoolAvailableAllocationUnits=%llu\n"), dsi.PoolAvailableAllocationUnits);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("SectorsPerAllocationUnit=%u\n"), dsi.SectorsPerAllocationUnit);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("BytesPerSector=%u\n"), dsi.BytesPerSector);
		strInfo += szBuf;



		// 计算总空间、已用空间和可用空间
		ULONGLONG totalSpace = dsi.ActualTotalAllocationUnits * dsi.BytesPerSector * dsi.SectorsPerAllocationUnit;
		ULONGLONG usedSpace = dsi.UsedAllocationUnits * dsi.BytesPerSector * dsi.SectorsPerAllocationUnit;
		ULONGLONG availableSpace = dsi.ActualAvailableAllocationUnits * dsi.BytesPerSector * dsi.SectorsPerAllocationUnit;

		// 计算使用百分比
		double usedPercentage = (totalSpace > 0) ? (static_cast<double>(usedSpace) / totalSpace) * 100.0 : 0.0;

		// 格式化输出信息
		_stprintf_s(szBuf, 128, _T("Total Space: %llu bytes\n"), totalSpace);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("Used Space: %llu bytes\n"), usedSpace);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("Available Space: %llu bytes\n"), availableSpace);
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("Used Percentage: %.2f%%\n"), usedPercentage);
		strInfo += szBuf;

		// 格式化输出信息
		_stprintf_s(szBuf, 128, _T("Total Space: %s\n"), FormatSize(totalSpace).c_str());
		strInfo += szBuf;
		_stprintf_s(szBuf, 128, _T("%s[%llu bytes]"), FormatSize(totalSpace).c_str(), totalSpace);
		itemDrive.strTotalSpace = szBuf;

		_stprintf_s(szBuf, 128, _T("Used Space: %s\n"), FormatSize(usedSpace).c_str());
		strInfo += szBuf;

		_stprintf_s(szBuf, 128, _T("Available Space: %s\n"), FormatSize(availableSpace).c_str());
		strInfo += szBuf;
		_stprintf_s(szBuf, 128, _T("%s[%llu bytes]"), FormatSize(availableSpace).c_str(), availableSpace);
		itemDrive.strAvailableSpace = szBuf;

		_stprintf_s(szBuf, 128, _T("Used Percentage: %.2f%%\n"), usedPercentage);
		strInfo += szBuf;
		_stprintf_s(szBuf, 128, _T("%.2f%%"), usedPercentage);
		itemDrive.strUsedPercent = szBuf;

		strInfo += L"\n";
	}
	else
	{
		//fuck API GetDiskSpaceInformation not support exFAT FS
		ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
		if (GetDiskFreeSpaceEx(strDrive.c_str(), &freeBytes, &totalBytes, &totalFreeBytes))
		{
			// 计算已用空间
			ULONGLONG usedSpace = totalBytes.QuadPart - freeBytes.QuadPart;

			// 计算已用空间百分比
			double usedPercentage = (totalBytes.QuadPart > 0) ? (static_cast<double>(usedSpace) / totalBytes.QuadPart) * 100.0 : 0.0;
			TCHAR szBuf[128] = {};

			// 格式化输出信息
			_stprintf_s(szBuf, 128, _T("Total Space: %llu bytes\n"), totalBytes.QuadPart);
			strInfo += szBuf;
			_stprintf_s(szBuf, 128, _T("%s[%llu bytes]"), FormatSize(totalBytes.QuadPart).c_str(), totalBytes.QuadPart);
			itemDrive.strTotalSpace = szBuf;

			_stprintf_s(szBuf, 128, _T("Used Space: %llu bytes\n"), usedSpace);
			strInfo += szBuf;

			_stprintf_s(szBuf, 128, _T("Available Space: %llu bytes\n"), freeBytes.QuadPart);
			strInfo += szBuf;
			_stprintf_s(szBuf, 128, _T("%s[%llu bytes]"), FormatSize(freeBytes.QuadPart).c_str(), freeBytes.QuadPart);
			itemDrive.strAvailableSpace = szBuf;

			_stprintf_s(szBuf, 128, _T("Used Percentage: %.2f%%\n"), usedPercentage);
			strInfo += szBuf;
			_stprintf_s(szBuf, 128, _T("%.2f%%"), usedPercentage);
			itemDrive.strUsedPercent = szBuf;

			strInfo += L"\n";
		}
	}
	OutputDebugString(strInfo.c_str());
}

void ProcessCommandMessage(HWND hwnd, WPARAM wParam)
{
	if (LOWORD(wParam) == IDM_CTX_OPEN)
	{
		UINT itemid = -1;
		itemid = ListView_GetNextItem(hListView, itemid, LVNI_SELECTED);
		if (itemid != -1)
		{
			TCHAR szText[32] = {};
			ListView_GetItemText(hListView, itemid, 0, szText, ARRAYSIZE(szText));
			SHELLEXECUTEINFO ShExecInfo;
			ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			ShExecInfo.fMask = NULL;
			ShExecInfo.hwnd = NULL;
			ShExecInfo.lpVerb = NULL;
			ShExecInfo.lpFile = _T("explorer");
			ShExecInfo.lpParameters = szText;
			ShExecInfo.lpDirectory = NULL;
			ShExecInfo.nShow = SW_SHOWDEFAULT;
			ShExecInfo.hInstApp = NULL;

			ShellExecuteEx(&ShExecInfo);
		}
	}
	else if (LOWORD(wParam) == IDM_CTX_COPY)
	{
		CopySelectItemInfo(hwnd);
	}
	else if (LOWORD(wParam) == IDM_CTX_LOCKVOLUME)
	{
		//Lock Vol
		UINT itemid = -1;
		itemid = ListView_GetNextItem(hListView, itemid, LVNI_SELECTED);
		if (itemid != -1)
		{
			TCHAR szText[32] = {};
			TCHAR szVol[MAX_PATH] = {};
			ListView_GetItemText(hListView, itemid, 0, szText, ARRAYSIZE(szText));
			_stprintf_s(szVol, ARRAYSIZE(szVol), _T("\\\\.\\%s"), szText);
			HANDLE hVol = CreateFile(szVol, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
			if (hVol != INVALID_HANDLE_VALUE)
			{
				DWORD dwBytesReturn = 0;
				BOOL bRes = DeviceIoControl(hVol, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturn, NULL);
				if (bRes)
				{
					DWORD dwError = GetLastError();
				}
				int a = 10;
				CloseHandle(hVol);
			}
		}
	}
	else if (LOWORD(wParam) == IDM_CTX_PROP)
	{
		

	}
	else if (LOWORD(wParam) == IDM_FILE_REFRESH)
	{
		RefreshDeviceList();
	}
	else if (LOWORD(wParam) == IDM_FILE_EXIT)
	{
		
	}
	else if (LOWORD(wParam) == IDM_SETTING_TOP)
	{
		if (bTopMost)
		{
			::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			bTopMost = FALSE;
		}
			
		else
		{
			::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			bTopMost = TRUE;
		}
		HMENU hMenu = GetMenu(hwnd);
		HMENU hSubMenu = GetSubMenu(hMenu, 1);
		CheckMenuItem(hSubMenu, IDM_SETTING_TOP, bTopMost? MF_CHECKED : MF_UNCHECKED);
	}
	else if (LOWORD(wParam) == IDM_SETTING_DARK)
	{

	}
	else if (LOWORD(wParam) == IDM_HELP_ABOUT)
	{
		TCHAR szTitle[256] = { 0 };
		_stprintf_s(szTitle, ARRAYSIZE(szTitle), _T("%s %s\n\nCopyright(C) 2025 hostzhengwei@gmail.com"), WIN_TITLE, VER);
		MessageBox(hwnd, szTitle, WIN_TITLE, MB_OK);
	}
}

void CopySelectItemInfo(HWND hwnd)
{
	UINT itemid = -1;
	itemid = ListView_GetNextItem(hListView, itemid, LVNI_SELECTED);
	tstring strInfo;
	while (itemid != -1)
	{
		for (int i = 0; i < 10; i++)
		{
			TCHAR szText[128] = {}; 
			ListView_GetItemText(hListView, itemid, i, szText, ARRAYSIZE(szText));
			strInfo += vecCols[i];
			strInfo += _T(" : ");
			strInfo += szText;
			strInfo += _T("\n");
		}
		strInfo += _T("\n");
		itemid = ListView_GetNextItem(hListView, itemid, LVNI_SELECTED);
	}
	CopyTextToClipboard(hwnd, strInfo);
}

void CopyTextToClipboard(HWND hwnd, const tstring& text)
{
	BOOL bCopied = FALSE;
	if (OpenClipboard(NULL))
	{
		if (EmptyClipboard())
		{
			size_t len = text.length();
			HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (len+1)*sizeof(TCHAR));
			if (hData != NULL)
			{
				TCHAR* pszData = static_cast<TCHAR*>(GlobalLock(hData));
				if (pszData != NULL)
				{
					//CopyMemory(pszData, text.c_str(), len*sizeof(TCHAR));
					_tcscpy_s(pszData, len+1, text.c_str());
					GlobalUnlock(hData);
					if (SetClipboardData(CF_UNICODETEXT, hData) != NULL)
					{
						bCopied = TRUE;;
					}
				}
			}
		}
		CloseClipboard();
	}
}

void RefreshDeviceList()
{
	ListView_DeleteAllItems(hListView);
	UpdateWindow(hListView);

	FillListView();
}

BOOL IsAdmin()
{
	//IsUserAnAdmin
	BOOL bRes = FALSE;
	SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
	PSID AdminGroup;
	bRes = AllocateAndInitializeSid(&sidAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdminGroup);
	if (bRes)
	{
		if (!CheckTokenMembership(NULL, AdminGroup, &bRes))
		{
			bRes = FALSE;
		}
		FreeSid(AdminGroup);
	}
	return bRes;
}

BOOL IsProcessElevated()
{
	HANDLE hToken = NULL;
	TOKEN_ELEVATION elevation;
	DWORD dwSize = 0;

	auto m = GetModuleHandle(NULL);
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		return FALSE;
	}
	if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
	{
		CloseHandle(hToken);
		return FALSE;
	}
	return elevation.TokenIsElevated;
}