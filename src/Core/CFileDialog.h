//----------------------------------------------------------------------------
//-- CFileDialog.h
//-- VlaGan: file/folder selection dialog
//----------------------------------------------------------------------------
#pragma once
#include <shtypes.h>
#include <filesystem>


namespace FileDialog {
    std::filesystem::path OpenFolder(HWND owner);
    std::filesystem::path OpenFile(HWND owner, const wchar_t* filter, int defaultFilter = 1);
    std::filesystem::path OpenFile(HWND owner, UINT filter_cnt, COMDLG_FILTERSPEC* filters, int defaultFilter = 1);
    std::filesystem::path SaveFile(HWND owner, const wchar_t* defaultExt, UINT filter_cnt, COMDLG_FILTERSPEC* filters);
}
