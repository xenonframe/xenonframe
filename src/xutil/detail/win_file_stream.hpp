#pragma once
#include <string>
#include <windows.h>
namespace xutil
{
	namespace detail
	{
		class win_file_stream
		{
		public:
			enum open_mode
			{
				OPEN_RDONLY = 0x001,
				OPEN_WRONLY = 0x002,
				OPEN_RDWR = 0x004,
				OPEN_CREATE = 0x008,
				OPEN_TRUNC = 0x010,
				OPEN_APPEND = 0x020,
				OPEN_EXCL = 0x040,
				OPEN_BINARY = 0x080,
				OPEN_TEMP = 0x100,
				OPEN_TEXT = 0x200
			};

			enum whence
			{
				BEGIN = 0,
				CURRENT = 1,
				END = 2
			};
			win_file_stream()
			{

			}
			~win_file_stream()
			{
				close();
			}
			win_file_stream(const win_file_stream&) = delete;
			win_file_stream &operator = (const win_file_stream&) = delete;
			win_file_stream(win_file_stream &&other)
			{
				handle_ = other.handle_;
				other.handle_ = INVALID_HANDLE_VALUE;
			}
			win_file_stream &operator = (win_file_stream &&other)
			{
				if (this != &other)
				{
					handle_ = other.handle_;
					other.handle_ = INVALID_HANDLE_VALUE;
				}
				return *this;
			}
			operator bool()
			{
				return handle_ != INVALID_HANDLE_VALUE;
			}
			bool operator !()
			{
				return handle_ == INVALID_HANDLE_VALUE;
			}
			bool open(const std::string &filepath, int mode = open_mode::OPEN_CREATE)
			{
				DWORD type = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
				DWORD access = GENERIC_READ;
				DWORD share, create;

				if (mode & OPEN_CREATE)
				{
					create = OPEN_ALWAYS;
					if (mode & OPEN_TRUNC)
						create = CREATE_ALWAYS;
				}
				else if (mode & OPEN_EXCL)
				{
					create = CREATE_NEW;
				}
				else if (mode & OPEN_TRUNC)
				{
					create = TRUNCATE_EXISTING;
				}
				else
				{
					create = OPEN_EXISTING;
				}

				if (mode & OPEN_RDWR)
				{
					access |= GENERIC_WRITE;
				}
				else if (mode & OPEN_WRONLY)
				{
					access = GENERIC_WRITE;
				}
				if (mode & OPEN_APPEND)
				{
					access = FILE_APPEND_DATA;
				}
				if (mode & OPEN_TEMP)
				{
					type = FILE_ATTRIBUTE_TEMPORARY;
				}
				share = FILE_SHARE_READ | FILE_SHARE_WRITE;
				handle_ = CreateFile((LPCSTR)filepath.c_str(),
					access,
					share,
					0,
					create,
					type,
					0);
				return handle_ != INVALID_HANDLE_VALUE;
			}

			int64_t read(void *buffer, int64_t to_reads)
			{
				DWORD nRd;
				BOOL rc;
				rc = ReadFile(handle_, buffer, (DWORD)to_reads, &nRd, 0);
				if (!rc)
				{
					std::cout << GetLastError() << std::endl;
					return -1;
				}
				return (int64_t)nRd;
			}
			int64_t write(const void *buffer, int64_t len)
			{
				const char *data = (const char *)buffer;
				int64_t count = 0;
				DWORD bytes = 0;
				while (len > 0)
				{
					if (!WriteFile(handle_, data, (DWORD)len, &bytes, 0))
						break;
					len -= bytes;
					count += bytes;
					data += bytes;
				}
				if (len > 0)
				{
					return -1;
				}
				return count;
			}
			bool seek(int64_t offset, whence _whence)
			{
				DWORD mode, ret;
				LONG high_offset;
				switch (_whence)
				{
				case whence::CURRENT:
					mode = FILE_CURRENT;
					break;
				case whence::END:
					mode = FILE_END;
					break;
				case 0:
				default:
					mode = FILE_BEGIN;
					break;
				}

				high_offset = (LONG)(offset >> 32);
				ret = SetFilePointer(handle_, (LONG)offset, &high_offset, mode);
				if (ret == INVALID_SET_FILE_POINTER)
					return false;
				return true;
			}
			bool lock(bool exclusive = true)
			{
				DWORD low_bytes = 0, high_bytes = 0;
				OVERLAPPED dummy;
				memset(&dummy, 0, sizeof(dummy));

				DWORD flags = LOCKFILE_FAIL_IMMEDIATELY;
				if (exclusive == 1)
					flags |= LOCKFILE_EXCLUSIVE_LOCK;
				low_bytes = GetFileSize(handle_, &high_bytes);
				return !!LockFileEx(handle_, flags, 0, low_bytes, high_bytes, &dummy);
			}
			bool unlock()
			{
				DWORD dwLo = 0, dwHi = 0;
				OVERLAPPED sDummy;
				memset(&sDummy, 0, sizeof(sDummy));
				return !!UnlockFileEx(handle_, 0, dwLo, dwHi, &sDummy);
			}
			int64_t tell()
			{
				DWORD pos;
				pos = SetFilePointer(handle_, 0, 0, FILE_CURRENT);
				if (pos == INVALID_SET_FILE_POINTER)
					return -1;
				return (int64_t)pos;
			}
			bool trunc(int64_t offset)
			{
				LONG high_offset;
				DWORD pos;
				high_offset = (LONG)(offset >> 32);
				pos = SetFilePointer(handle_, (LONG)offset, &high_offset, FILE_BEGIN);
				if (pos == INVALID_SET_FILE_POINTER)
					return false;
				return !!SetEndOfFile(handle_);
			}
			bool sync()
			{
				return !!FlushFileBuffers(handle_);
			}
			void close()
			{
				if (handle_ != INVALID_HANDLE_VALUE)
					CloseHandle(handle_);
				handle_ = INVALID_HANDLE_VALUE;
			}
		private:
			HANDLE handle_ = INVALID_HANDLE_VALUE;
		};
	}
	using file_stream = detail::win_file_stream;
}