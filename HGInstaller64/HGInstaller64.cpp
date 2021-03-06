#include "stdafx.h"

#define HG_HARDWARE_ID L"Root\\HidGuardian"
#define HG_FILTER_NAME L"HidGuardian"

typedef BOOL(WINAPI *UpdateDriverProto)(HWND, LPCTSTR, LPCTSTR, DWORD, PBOOL);

using namespace std;

GUID classGuid;

int doInstall(wchar_t *infPath) {

    wchar_t hwIds[64];
    wchar_t className[MAX_CLASS_NAME_LEN];
    HDEVINFO devInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA devInfoData;
    int ret = 0;

    ZeroMemory(hwIds, sizeof(hwIds));
    StringCchCopy(hwIds, 64, HG_HARDWARE_ID);

    if (!SetupDiGetINFClass(infPath, &classGuid, className, MAX_CLASS_NAME_LEN, 0))
        return 2;

    devInfo = SetupDiCreateDeviceInfoList(&classGuid, NULL);
    if (devInfo == INVALID_HANDLE_VALUE)
        return 3;

    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (
        !SetupDiCreateDeviceInfoW(
            devInfo, className, &classGuid, NULL, NULL, DICD_GENERATE_ID, &devInfoData
        )
    ) {
        ret = 4;
        goto end;
    }

    if (
        !SetupDiSetDeviceRegistryPropertyW(
            devInfo, &devInfoData, SPDRP_HARDWAREID, (LPBYTE)hwIds,
            (DWORD)(wcslen(hwIds) + 2) * sizeof(wchar_t)
        )
    ) {
        ret = 5;
        goto end;
    }

    if (
        !SetupDiCallClassInstaller(
            DIF_REGISTERDEVICE, devInfo, &devInfoData
        )
    ) {
        ret = 6;
        goto end;
    }

end:
    if (devInfo != INVALID_HANDLE_VALUE)
        SetupDiDestroyDeviceInfoList(devInfo);
    return ret;

}

int updateDriver(wchar_t *infPath) {

    HMODULE newDevMod = LoadLibrary(L"newdev.dll");
    UpdateDriverProto updateFn;

    if (!newDevMod)
        return 9;

    updateFn = (UpdateDriverProto)GetProcAddress(newDevMod, "UpdateDriverForPlugAndPlayDevicesW");

    if (!updateFn)
        return 10;

    if (!updateFn(NULL, HG_HARDWARE_ID, infPath, INSTALLFLAG_FORCE, NULL))
        return 11;

    return 0;

}

int main(int argc, char **argv)
{

    wchar_t infPath[MAX_PATH];
    size_t num;

    if (argc < 2)
        return 1;

    mbstowcs_s(&num, infPath, argv[1], MAX_PATH);

    int ret = doInstall(infPath);
    if (ret != 0)
        return ret;

    return updateDriver(infPath);

}
