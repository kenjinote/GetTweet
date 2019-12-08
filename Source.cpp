#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "wininet")
#pragma comment(lib, "crypt32")

#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include "json11.hpp"
#include "resource.h"

TCHAR szClassName[] = TEXT("Window");

BOOL GetStringFromJSON(LPCSTR lpszJson, LPCSTR lpszKey, LPSTR lpszValue, int nSizeValue)
{
	std::string src(lpszJson);
	std::string err;
	json11::Json v = json11::Json::parse(src, err);
	if (err.size()) return FALSE;
	lpszValue[0] = 0;
	strcpy_s(lpszValue, nSizeValue, v[lpszKey].string_value().c_str());
	return strlen(lpszValue) > 0;
}

BOOL String2Base64(LPWSTR lpszBase64String, DWORD dwSize)
{
	BOOL bReturn = FALSE;
	DWORD dwLength = WideCharToMultiByte(CP_ACP, 0, lpszBase64String, -1, 0, 0, 0, 0);
	LPSTR lpszStringA = (LPSTR)GlobalAlloc(GPTR, dwLength * sizeof(char));
	WideCharToMultiByte(CP_ACP, 0, lpszBase64String, -1, lpszStringA, dwLength, 0, 0);
	DWORD dwResult = 0;
	if (CryptBinaryToStringW((LPBYTE)lpszStringA, dwLength - 1, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, 0, &dwResult))
	{
		if (dwResult <= dwSize && CryptBinaryToStringW((LPBYTE)lpszStringA, dwLength - 1, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, lpszBase64String, &dwResult))
		{
			bReturn = TRUE;
		}
	}
	GlobalFree(lpszStringA);
	return bReturn;
}

BOOL GetTwitterBearerToken(IN LPCWSTR lpszConsumerKey, IN LPCWSTR lpszConsumerSecret, OUT LPWSTR lpszBearerToken, IN DWORD dwSize)
{
	HINTERNET hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL)
	{
		return FALSE;
	}
	HINTERNET hSession = InternetConnectW(hInternet, L"api.twitter.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (hSession == NULL)
	{
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	HINTERNET hRequest = HttpOpenRequestW(hSession, L"POST", L"/oauth2/token", NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
	if (hRequest == NULL)
	{
		InternetCloseHandle(hSession);
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	WCHAR credential[1024];
	wsprintfW(credential, L"%s:%s", lpszConsumerKey, lpszConsumerSecret);
	String2Base64(credential, sizeof(credential));
	WCHAR header[1024];
	wsprintfW(header, L"Authorization: Basic %s\r\nContent-Type: application/x-www-form-urlencoded;charset=UTF-8", credential);
	CHAR param[] = "grant_type=client_credentials";
	if (!HttpSendRequestW(hRequest, header, (DWORD)wcslen(header), param, (DWORD)strlen(param)))
	{
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hSession);
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	BOOL bResult = FALSE;
	WCHAR szBuffer[256] = { 0 };
	DWORD dwBufferSize = _countof(szBuffer);
	HttpQueryInfoW(hRequest, HTTP_QUERY_CONTENT_LENGTH, szBuffer, &dwBufferSize, NULL);
	DWORD dwContentLength = _wtol(szBuffer);
	LPBYTE lpByte = (LPBYTE)GlobalAlloc(0, dwContentLength + 1);
	DWORD dwReadSize;
	InternetReadFile(hRequest, lpByte, dwContentLength, &dwReadSize);
	lpByte[dwReadSize] = 0;
	CHAR szAccessToken[256];
	if (GetStringFromJSON((LPCSTR)lpByte, "access_token", szAccessToken, _countof(szAccessToken)))
	{
		DWORD nLength = MultiByteToWideChar(CP_THREAD_ACP, 0, szAccessToken, -1, 0, 0);
		if (nLength <= dwSize)
		{
			MultiByteToWideChar(CP_THREAD_ACP, 0, szAccessToken, -1, lpszBearerToken, nLength);
			bResult = TRUE;
		}
	}
	GlobalFree(lpByte);
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hSession);
	InternetCloseHandle(hInternet);
	return bResult;
}

BOOL GetTweet(HWND hEdit, LPCWSTR lpszBearerToken, LPCWSTR lpszTwitterID)
{
	HINTERNET hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL)
	{
		return FALSE;
	}
	HINTERNET hSession = InternetConnectW(hInternet, L"api.twitter.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (hSession == NULL)
	{
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	WCHAR szURL[1024];
	wcscpy_s(szURL, _countof(szURL), L"1.1/statuses/user_timeline.json?screen_name=");
	wcscat_s(szURL, _countof(szURL), lpszTwitterID);
	wcscat_s(szURL, _countof(szURL), L"&count=5");
	HINTERNET hRequest = HttpOpenRequestW(hSession, L"GET", szURL, NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
	if (hRequest == NULL)
	{
		InternetCloseHandle(hSession);
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	WCHAR header[1024];
	wsprintfW(header, L"Authorization: Bearer %s", lpszBearerToken);
	if (!HttpSendRequestW(hRequest, header, (DWORD)wcslen(header), 0, 0))
	{
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hSession);
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	WCHAR szBuffer[256] = { 0 };
	DWORD dwBufferSize = _countof(szBuffer);
	HttpQueryInfoW(hRequest, HTTP_QUERY_CONTENT_LENGTH, szBuffer, &dwBufferSize, NULL);
	DWORD dwContentLength = _wtol(szBuffer);
	LPBYTE lpByte = (LPBYTE)GlobalAlloc(0, dwContentLength + 1);
	DWORD dwReadSize;
	InternetReadFile(hRequest, lpByte, dwContentLength, &dwReadSize);
	lpByte[dwReadSize] = 0;
	{
		SetWindowText(hEdit, 0);
		std::string src((LPSTR)lpByte);
		std::string err;
		json11::Json v = json11::Json::parse(src, err);
		if (err.size() == 0)
		{
			SendMessageW(hEdit, WM_SETREDRAW, FALSE, 0);
			SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)L"----------------------------------------------------------------------------------------------------\r\n");
			for (auto &k : v.array_items())
			{
				int nSize = (int)k["text"].string_value().size() + 1;
				DWORD dwSize = MultiByteToWideChar(CP_UTF8, 0, k["text"].string_value().c_str(), nSize, 0, 0);
				LPWSTR lpszContentW = (LPWSTR)GlobalAlloc(0, dwSize * sizeof(WCHAR));
				MultiByteToWideChar(CP_UTF8, 0, k["text"].string_value().c_str(), nSize, lpszContentW, dwSize);
				{
					LPWSTR next = NULL;
					LPWSTR lpszLine = wcstok_s(lpszContentW, L"\n", &next);
					while (lpszLine)
					{
						SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)lpszLine);
						SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
						lpszLine = wcstok_s(NULL, L"\n", &next);
					}
				}
				SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)L"----------------------------------------------------------------------------------------------------\r\n");
				GlobalFree(lpszContentW);
			}
			SendMessageW(hEdit, EM_SETSEL, 0, 0);
			SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
			SendMessageW(hEdit, WM_SETREDRAW, TRUE, 0);
		}
	}
	GlobalFree(lpByte);
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hSession);
	InternetCloseHandle(hInternet);
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hButton;
	static HWND hEdit1;
	static HWND hEdit2;
	static HWND hEdit3;
	static HWND hEdit4;
	static HFONT hFont;
	switch (msg)
	{
	case WM_CREATE:
		hFont = CreateFontW(22, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Yu Gothic UI");
		hEdit1 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessageW(hEdit1, EM_SETCUEBANNER, TRUE, (LPARAM)L"Consumer key");
		hEdit2 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessageW(hEdit2, EM_SETCUEBANNER, TRUE, (LPARAM)L"Consumer secret");
		hEdit3 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessageW(hEdit3, EM_SETCUEBANNER, TRUE, (LPARAM)L"Twitter ID");
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("取得"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hEdit4 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_TABSTOP | ES_MULTILINE | ES_READONLY, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit4, EM_LIMITTEXT, 0, 0);
		SendMessage(hEdit4, WM_SETFONT, (WPARAM)hFont, 0);
		break;
	case WM_SIZE:
		MoveWindow(hEdit1, 10, 10, LOWORD(lParam) - 20, 32, TRUE);
		MoveWindow(hEdit2, 10, 50, LOWORD(lParam) - 20, 32, TRUE);
		MoveWindow(hEdit3, 10, 90, LOWORD(lParam) - 20, 32, TRUE);
		MoveWindow(hButton, 10, 130, 256, 32, TRUE);
		MoveWindow(hEdit4, 10, 170, LOWORD(lParam) - 20, HIWORD(lParam) - 180, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			WCHAR szConsumerKey[128];
			WCHAR szConsumerSecret[128];
			WCHAR szBearerToken[128];
			WCHAR szTwitterID[128];
			GetWindowTextW(hEdit1, szConsumerKey, _countof(szConsumerKey));
			GetWindowTextW(hEdit2, szConsumerSecret, _countof(szConsumerSecret));
			GetWindowTextW(hEdit3, szTwitterID, _countof(szTwitterID));
			if (GetTwitterBearerToken(szConsumerKey, szConsumerSecret, szBearerToken, _countof(szBearerToken)))
			{
				GetTweet(hEdit4, szBearerToken, szTwitterID);
				SendMessage(hEdit3, EM_SETSEL, 0, -1);
				SetFocus(hEdit3);
			}
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefDlgProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		DLGWINDOWEXTRA,
		hInstance,
		LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)),
		LoadCursor(0,IDC_ARROW),
		0,
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("Get Tweet"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}
