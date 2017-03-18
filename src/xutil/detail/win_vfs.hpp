#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace xutil
{
	namespace vfs
	{
		struct username
		{
			std::string operator()()
			{
				char zUser[1024];
				DWORD nByte;
				nByte = sizeof(zUser);
				if (!GetUserName(zUser, &nByte))
					return{};
				return std::string(zUser, nByte);
			}
		};
		struct process_id
		{
			std::size_t operator()()
			{
				return GetProcessId(GetCurrentProcess());
			}
		};

		struct tempdir
		{
			std::string operator()()
			{
				char buffer[1024];
				DWORD n = GetTempPath(sizeof(buffer), buffer);
				if (n < 1)
					return "C:\\Windows\\Temp";
				return std::string(buffer, n);
			}
		};
		struct open_read_only
		{
			HANDLE operator ()(const std::string &path)
			{
				DWORD dwType = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
				DWORD dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
				DWORD dwAccess = GENERIC_READ;
				DWORD dwCreate = OPEN_EXISTING;
				HANDLE pHandle;
				pHandle = CreateFile(path.c_str(), dwAccess, dwShare, 0, dwCreate, dwType, 0);
				if (pHandle == INVALID_HANDLE_VALUE)
					return nullptr;
				return pHandle;
			}

		};
		struct chdir
		{
			bool operator()(const std::string &path)
			{
				return !!SetCurrentDirectory(path.c_str());
			}
		};
		struct getcwd
		{
			std::string operator()()
			{
				char buffer[2048] = { 0 };
				GetCurrentDirectory(sizeof(buffer), buffer);
				std::string result(buffer);
				if (result.size() && (result.back() != '\\'))
					result.push_back('\\');
				std::string tmp;
				for (auto &itr : result)
				{
					if (itr == '\\')
						tmp.push_back('/');
					else
						tmp.push_back(itr);
				}
				return tmp;
			}
		};
		struct mkdir
		{
			bool operator()(const std::string &path,bool p = true)
			{
				if(!p)
					return !!CreateDirectory(path.c_str(), 0);

				std::size_t offset = 0;
				do {

					auto pos = path.find_first_of('\\', offset);
					if (pos == std::string::npos)
						pos = path.find_first_of('/', offset);
					if (pos == std::string::npos)
					{
						return !!CreateDirectory(path.c_str(), NULL) || 
							ERROR_ALREADY_EXISTS == GetLastError();
					}
					else {
						auto parent_dir = path.substr(0, pos);
						offset = pos + 1;
						if (!!CreateDirectory(parent_dir.c_str(), NULL) ||
							ERROR_ALREADY_EXISTS == GetLastError())
							continue;
						return false;
					}
				} while (true);
			}
		};
		
		struct rmdir
		{
			bool operator()(const std::string &path)
			{
				return !!RemoveDirectory(path.c_str());
			}
		};
		struct isdir
		{
			bool operator()(const std::string &path)
			{
				DWORD dwAttr;
				dwAttr = GetFileAttributes(path.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
					return false;
				return !!(dwAttr & FILE_ATTRIBUTE_DIRECTORY);
			}
		};
		struct rename
		{
			bool operator()(const std::string &_old, const std::string &_new)
			{
				return !!MoveFile(_old.c_str(), _new.c_str());
			}
		};
		struct realpath
		{
			std::string operator()(const std::string &path)
			{
				DWORD len;
				std::string buffer;
				len = GetFullPathName(path.c_str(), 0, 0, 0);
				if (len > 0)
				{
					buffer.resize(len);
					GetFullPathName(path.c_str(), len, (char*)buffer.data(), 0);
				}
				return std::move(buffer);
			}
		};
		struct unlink
		{
			bool operator()(const std::string &path)
			{
				return !!DeleteFile(path.c_str());
			}
		};
		struct disk_free_space
		{
			int64_t operator()(const std::string &path)
			{
				DWORD dwSectPerClust,
					dwBytesPerSect,
					dwFreeClusters,
					dwTotalClusters;
				BOOL rc;
				std::string buffer;
				auto pos = path.find_first_of('\\');
				if (pos == std::string::npos)
					pos = path.find_first_of('/');
				if (pos == std::string::npos)
					buffer = path;
				else
					buffer = path.substr(0, pos);
				rc = GetDiskFreeSpace(buffer.c_str(),
					&dwSectPerClust,
					&dwBytesPerSect,
					&dwFreeClusters,
					&dwTotalClusters);
				if (!rc)
					return 0;
				return
					(int64_t)dwFreeClusters *
					dwSectPerClust *
					dwBytesPerSect;
			}
		};
		struct disk_total_space
		{
			int64_t operator()(const std::string &path)
			{
				DWORD dwSectPerClust,
					dwBytesPerSect,
					dwFreeClusters,
					dwTotalClusters;
				std::string buffer;
				auto pos = path.find_first_of('\\');
				if (pos == std::string::npos)
					pos = path.find_first_of('/');
				if (pos == std::string::npos)
					buffer = path;
				else
					buffer = path.substr(0, pos);
				if (!GetDiskFreeSpace(buffer.c_str(),
					&dwSectPerClust,
					&dwBytesPerSect,
					&dwFreeClusters,
					&dwTotalClusters))
					return 0;
				return
					(int64_t)dwTotalClusters *
					dwSectPerClust *
					dwBytesPerSect;
			}

		};
		struct file_type
		{
			enum type
			{
				e_file,
				e_dir,
				e_link,
				e_block,
				e_unknown,
			};

			type operator()(const std::string &path)
			{
				DWORD dwAttr;
				dwAttr = GetFileAttributes(path.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
				{
					return type::e_unknown;
				}
				if (dwAttr & (FILE_ATTRIBUTE_HIDDEN |
					FILE_ATTRIBUTE_NORMAL |
					FILE_ATTRIBUTE_ARCHIVE))
				{
					return type::e_file;
				}
				else if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
				{
					return type::e_dir;
				}
				else if (dwAttr & FILE_ATTRIBUTE_REPARSE_POINT)
				{
					return type::e_link;
				}
				else if (dwAttr & (FILE_ATTRIBUTE_DEVICE))
				{
					return type::e_block;
				}
				return type::e_unknown;
			}
		};
		struct is_file 
		{
			bool operator()(const std::string zPath)
			{
				DWORD dwAttr = GetFileAttributes(zPath.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
				{
					return false;
				}
				return !!(dwAttr & (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE));
			}
		};
		struct ls 
		{
			std::vector<std::string> operator()(const std::string &path)
			{
				std::vector<std::string> lists;
				WIN32_FIND_DATA find_data;
				HANDLE handle = ::FindFirstFile((path + "*.*").c_str(), &find_data);
				if (INVALID_HANDLE_VALUE == handle)
					return{};
				while (TRUE)
				{
					if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
					{
						if (strcmp(find_data.cFileName, ".") == 0 ||
							strcmp(find_data.cFileName, "..") == 0)
							;
						else
						{
							lists.emplace_back(find_data.cFileName);
							lists.back().push_back('/');
						}
					}
					else if (find_data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN |
						FILE_ATTRIBUTE_NORMAL |
						FILE_ATTRIBUTE_ARCHIVE))
					{
						lists.emplace_back(find_data.cFileName);
					}
					if (!FindNextFile(handle, &find_data))
						break;
				}
				FindClose(handle);
				return lists;
			}
		};
		struct ls_files
		{
			std::vector<std::string> operator()(const std::string &path, std::size_t depth = 1)
			{
				std::vector<std::string> files;
				WIN32_FIND_DATA find_data;
				HANDLE handle = ::FindFirstFile((path + "*.*").c_str(), &find_data);
				if (INVALID_HANDLE_VALUE == handle)
					return{};
				while (TRUE)
				{
					std::string filename(find_data.cFileName);
					std::string realpath;
					if (path.size() &&(path.back() == '\\' || path.back() == '/'))
						realpath = path + filename;
					else
						realpath = path + "/" + filename;

					auto type = file_type()(realpath);
					if (type == file_type::e_file)
					{
						files.emplace_back(realpath);
					}
					else if (type == file_type::e_dir )
					{
						if (filename != "." && filename != ".." && depth > 1)
						{
							auto tmp = ls_files()(realpath + "/", depth - 1);
							for (auto &itr : tmp)
								files.emplace_back(std::move(itr));
						}
					}
					if (!FindNextFile(handle, &find_data))
						break;
				}
				FindClose(handle);
				return files;
			}
		};
		struct is_link 
		{
			bool operator()(const std::string &path)
			{
				DWORD dwAttr= GetFileAttributes(path.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
				{
					return false;
				}
				return !!(dwAttr & FILE_ATTRIBUTE_REPARSE_POINT);
			}
		};
		struct is_writable
		{
			bool operator()(const std::string &path)
			{
				DWORD dwAttr= GetFileAttributes(path.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
				{
					return false;
				}
				if ((dwAttr & (FILE_ATTRIBUTE_ARCHIVE | 
					FILE_ATTRIBUTE_NORMAL)) == 0)
				{
					return false;
				}
				if (dwAttr & FILE_ATTRIBUTE_READONLY)
				{
					return false;
				}
				return true;
			}
		};
		struct is_executable
		{
			bool operator()(const std::string &zPath)
			{
				DWORD dwAttr = GetFileAttributes(zPath.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
				{
					return false;
				}
				if ((dwAttr & FILE_ATTRIBUTE_NORMAL) == 0)
				{
					return false;
				}
				return true;
			}
		};
		struct  to_time
		{
			static constexpr int64_t TICKS_PER_SECOND = 10000000;
			static constexpr int64_t EPOCH_DIFFERENCE = 11644473600LL;
			int64_t operator()(FILETIME pTime)
			{
				int64_t input, temp;
				input = pTime.dwHighDateTime;
				input <<= 32;
				input += pTime.dwLowDateTime;
				temp = input / TICKS_PER_SECOND;
				temp = temp - EPOCH_DIFFERENCE;
				return temp;
			}
		};
		struct last_modified
		{
			int64_t operator()(const std::string &filepath)
			{
				BY_HANDLE_FILE_INFORMATION sInfo;
				HANDLE pHandle= open_read_only()(filepath);
				if (pHandle == 0)
					return -1;
				if (!GetFileInformationByHandle(pHandle, &sInfo))
					return -1;
				CloseHandle(pHandle);
				return to_time()(sInfo.ftLastWriteTime);
			}
		};

		struct file_size 
		{
			int64_t operator()(const std::string &filepath)
			{
				DWORD dwLow, dwHigh;
				HANDLE handle = open_read_only()(filepath);
				if (!handle)
					return -1;
				dwLow = GetFileSize(handle, &dwHigh);
				CloseHandle(handle);
				int64_t nSize = dwHigh;
				nSize <<= 32;
				nSize += dwLow;
				return nSize;
			}
		};
		struct file_exists
		{
			bool operator()(const std::string &path)
			{
				DWORD dwAttr;
				dwAttr = GetFileAttributes(path.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
					return false;
				return true;
			}

		};
		struct mmap
		{
			void *operator ()(const std::string &path, int64_t &size)
			{
				DWORD size_low, size_high;
				HANDLE file_handle, map_handle;
				void *map;

				file_handle = open_read_only()(path);
				if (!file_handle)
					return nullptr;
				size_low = GetFileSize(file_handle, &size_high);
				map_handle = CreateFileMapping(file_handle,
					0, PAGE_READONLY, size_high, size_low, 0);
				if (map_handle == 0)
				{
					CloseHandle(file_handle);
					return nullptr;
				}
				size = ((int64_t)size_high << 32) | size_low;
				map = MapViewOfFile(map_handle, 
					FILE_MAP_READ, 0, 0, (SIZE_T)(size));
				CloseHandle(map_handle);
				CloseHandle(file_handle);
				return map;
			}

		};
		struct unmap
		{
			void operator()(void *handle, int64_t size = 0)
			{
				UnmapViewOfFile(handle);
			}
		};

		struct get_env
		{
			std::string operator ()(const std::string &name)
			{
				char buffer[1024];
				DWORD n = GetEnvironmentVariable(
					name.c_str(), 
					buffer, 
					sizeof(buffer));
				if (!n)
					return {};
				return std::string(buffer, n);
			}
		};

		struct set_env 
		{
			bool operator()(const std::string &name, const std::string &value)
			{
				return !!SetEnvironmentVariable(
					name.c_str(), 
					value.c_str());
			}
		};
	}
}