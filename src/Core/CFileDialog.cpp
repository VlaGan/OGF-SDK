//----------------------------------------------------------------------------
//-- CFileDialog.h
//-- VlaGan: file/folder selection dialog
//----------------------------------------------------------------------------
#include "CFileDialog.h"
#include <shobjidl.h>


namespace FileDialog {

    std::filesystem::path OpenFolder(HWND owner)
    {
        IFileOpenDialog* dialog{};
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog));
        if (FAILED(hr))
            return {};

        dialog->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        if (FAILED(dialog->Show(owner)))
        {
            dialog->Release();
            return {};
        }

        IShellItem* item{};
        if (FAILED(dialog->GetResult(&item)))
        {
            dialog->Release();
            return {};
        }

        PWSTR path{};
        item->GetDisplayName(SIGDN_FILESYSPATH, &path);

        std::filesystem::path result(path);
        CoTaskMemFree(path);
        item->Release();
        dialog->Release();

        return result;
    }

    std::filesystem::path OpenFile(HWND owner, const wchar_t* filter, int defaultFilter)
    {
        IFileOpenDialog* dialog{};
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog));
        if (FAILED(hr))
            return {};

        COMDLG_FILTERSPEC filters[] = {
            { L"Supported files", filter}
        };

        dialog->SetFileTypes(1, filters);
        dialog->SetFileTypeIndex(defaultFilter);

        if (FAILED(dialog->Show(owner)))
        {
            dialog->Release();
            return {};
        }

        IShellItem* item{};
        dialog->GetResult(&item);

        PWSTR path{};
        item->GetDisplayName(SIGDN_FILESYSPATH, &path);

        std::filesystem::path result(path);

        CoTaskMemFree(path);
        item->Release();
        dialog->Release();

        return result;
    }

    std::filesystem::path OpenFile(HWND owner, UINT filter_cnt, COMDLG_FILTERSPEC* filters, int defaultFilter) {
        IFileOpenDialog* dialog{};
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog));
        if (FAILED(hr))
            return {};

        dialog->SetFileTypes(filter_cnt, filters);
        dialog->SetFileTypeIndex(defaultFilter);

        if (FAILED(dialog->Show(owner)))
        {
            dialog->Release();
            return {};
        }

        IShellItem* item{};
        dialog->GetResult(&item);

        PWSTR path{};
        item->GetDisplayName(SIGDN_FILESYSPATH, &path);

        std::filesystem::path result(path);

        CoTaskMemFree(path);
        item->Release();
        dialog->Release();

        return result;
    }

}
