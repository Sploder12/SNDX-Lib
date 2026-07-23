#pragma once

#include <optional>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "./windows.h"
#ifdef _WIN32
#include <ShObjIdl.h>
#endif

namespace sndx::platform {
	using OptFsPath = std::optional<std::filesystem::path>;
	using DialogFilter = std::pair<std::string, std::vector<std::string>>;

	namespace detail {
	#ifdef _WIN32
		[[nodiscard]]
		inline auto assembleFilters(std::span<const DialogFilter> filters) {
			std::vector<std::pair<std::wstring, std::wstring>> out{};
			out.reserve(filters.size());
			for (const auto& [n, f] : filters) {
				std::wstring filter{};

				for (const auto& ext : f) {
					filter += toWinStr(ext) + L";";
				}

				if (filter.empty()) {
					filter = L"*";
				}
				else {
					// remove trailing ;
					filter.pop_back();
				}

				out.emplace_back(toWinStr(n), filter);
			}
			return out;
		}
	#endif
	}

	[[nodiscard]]
	inline OptFsPath fileOpenDialog(const OptFsPath& startFolder = std::nullopt, std::span<const DialogFilter> filters = {}, size_t defaultFilter = -1) {
	#ifdef _WIN32
		std::optional<std::filesystem::path> out{};
		HRESULT ec = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(ec)) {
			IFileOpenDialog* dialog;
			ec = CoCreateInstance(
				CLSID_FileOpenDialog,
				NULL,
				CLSCTX_ALL,
				IID_IFileOpenDialog,
				reinterpret_cast<void**>(&dialog)
			);

			if (SUCCEEDED(ec)) {
				IShellItem* startItem = nullptr;
				if (startFolder) {
					ec = SHCreateItemFromParsingName(startFolder->native().c_str(), nullptr, IID_PPV_ARGS(&startItem));
					if (!SUCCEEDED(ec)) {
						startItem = nullptr;
					}
				}

				if (startItem) {
					dialog->SetFolder(startItem);
				}

				auto filterList = detail::assembleFilters(filters);
				std::vector<COMDLG_FILTERSPEC> winFilters{};
				if (filters.size() > 0) {
					winFilters.reserve(filters.size());
					for (size_t i = 0; i < filters.size(); ++i) {
						const auto& cur = filterList[i];
						winFilters.emplace_back(cur.first.c_str(), cur.second.c_str());
					}

					dialog->SetFileTypes(UINT(filters.size()), winFilters.data());

					if (defaultFilter == -1) {
						dialog->SetFileTypeIndex(UINT(filters.size()));
					}
					else {
						dialog->SetFileTypeIndex(UINT(defaultFilter + 1));
					}
				}

				ec = dialog->Show(NULL);
				if (SUCCEEDED(ec)) {
					IShellItem* item;
					ec = dialog->GetResult(&item);
					if (SUCCEEDED(ec)) {
						PWSTR path;
						ec = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
						if (SUCCEEDED(ec)) {
							out.emplace(path);
						}
						item->Release();
					}
				}

				if (startItem) {
					startItem->Release();
				}

				dialog->Release();
			}

			CoUninitialize();
		}
		return out;
	#else
		assert(("unimplemented", false));
		return std::nullopt;
	#endif
	}

	[[nodiscard]]
	inline OptFsPath fileSaveDialog(const OptFsPath& startFolder = std::nullopt, const OptFsPath& startFile = std::nullopt, const std::string& extension = "", std::span<const DialogFilter> filters = {}, size_t defaultFilter = -1) {
	#ifdef _WIN32
		std::optional<std::filesystem::path> out{};
		HRESULT ec = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(ec)) {
			IFileSaveDialog* dialog;
			ec = CoCreateInstance(
				CLSID_FileSaveDialog,
				NULL,
				CLSCTX_ALL,
				IID_IFileSaveDialog,
				reinterpret_cast<void**>(&dialog)
			);

			if (SUCCEEDED(ec)) {
				IShellItem* startFld = nullptr;
				if (startFolder) {
					ec = SHCreateItemFromParsingName(startFolder->native().c_str(), nullptr, IID_PPV_ARGS(&startFld));
					if (!SUCCEEDED(ec)) {
						startFld = nullptr;
					}
				}

				if (startFld) {
					dialog->SetFolder(startFld);
				}

				IShellItem* startItem = nullptr;
				if (startFile) {
					ec = SHCreateItemFromParsingName(startFile->native().c_str(), nullptr, IID_PPV_ARGS(&startItem));
					if (!SUCCEEDED(ec)) {
						startItem = nullptr;
					}
				}

				if (startItem) {
					dialog->SetSaveAsItem(startItem);
				}
				else if (startFile) {
					dialog->SetFileName(startFile->native().c_str());
				}

				auto filterList = detail::assembleFilters(filters);
				std::vector<COMDLG_FILTERSPEC> winFilters{};
				if (filters.size() > 0) {
					winFilters.reserve(filters.size());
					for (size_t i = 0; i < filters.size(); ++i) {
						const auto& cur = filterList[i];
						winFilters.emplace_back(cur.first.c_str(), cur.second.c_str());
					}

					dialog->SetFileTypes(UINT(filters.size()), winFilters.data());

					if (defaultFilter == -1) {
						dialog->SetFileTypeIndex(UINT(filters.size()));
					}
					else {
						dialog->SetFileTypeIndex(UINT(defaultFilter + 1));
					}
				}

				ec = dialog->Show(NULL);
				if (SUCCEEDED(ec)) {
					IShellItem* item;
					ec = dialog->GetResult(&item);
					if (SUCCEEDED(ec)) {
						PWSTR path;
						ec = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
						if (SUCCEEDED(ec)) {
							out.emplace(path);
						}
						item->Release();
					}
				}

				if (startItem) {
					startItem->Release();
				}

				dialog->Release();
			}

			CoUninitialize();
		}
		return out;
	#else
		assert(("unimplemented", false));
		return std::nullopt;
	#endif
	}
}