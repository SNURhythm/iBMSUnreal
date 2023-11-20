#include "FilePicker.h"
#if PLATFORM_WINDOWS
	#include "windows.h"
	#include <shlobj.h>
#endif
FString FFilePicker::PickDirectory(const FString& Title)
{
	// for windows, use the native dialog (IFileOpenDialog)
#if PLATFORM_WINDOWS

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	IFileOpenDialog* dialog;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&dialog));
	if(SUCCEEDED(hr))
	{
		dialog->SetTitle(*Title);
		dialog->SetOptions(FOS_PICKFOLDERS);
		hr = dialog->Show(nullptr);
		if(SUCCEEDED(hr))
		{
			IShellItem* item;
			hr = dialog->GetResult(&item);
			if(SUCCEEDED(hr))
			{
				PWSTR path;
				hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
				if(SUCCEEDED(hr))
				{
					FString result = path;
					CoTaskMemFree(path);
					return result;
				}
				item->Release();
			}
		}
		dialog->Release();
	}
	CoUninitialize();
	return "";
	
#else
#endif
}

FString FFilePicker::PickFile(const FString& Title, const TArray<TPair<FString, FString>>& FileTypes)
{
	// restrict to FileTypes
#if PLATFORM_WINDOWS
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	IFileOpenDialog* dialog;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&dialog));
	if(SUCCEEDED(hr))
	{
		dialog->SetTitle(*Title);
		dialog->SetOptions(FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
		// convert FileTypes to COMDLG_FILTERSPEC
		TArray<COMDLG_FILTERSPEC> specs;
		for(auto& pair : FileTypes)
		{
			COMDLG_FILTERSPEC spec;
			spec.pszName = *pair.Key;
			spec.pszSpec = *pair.Value;
			specs.Add(spec);
		}
		
		dialog->SetFileTypes(specs.Num(), specs.GetData());
		hr = dialog->Show(nullptr);
		if(SUCCEEDED(hr))
		{
			IShellItem* item;
			hr = dialog->GetResult(&item);
			if(SUCCEEDED(hr))
			{
				PWSTR path;
				hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
				if(SUCCEEDED(hr))
				{
					FString result = path;
					CoTaskMemFree(path);
					return result;
				}
				item->Release();
			}
		}
		dialog->Release();
	}
	CoUninitialize();
	return "";
#else
#endif
	
}
