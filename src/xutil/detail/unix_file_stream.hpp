#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
namespace xutil
{
	namespace detail
	{
		class unix_file_stream
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
			unix_file_stream()
			{

			}
			~unix_file_stream()
			{
				close();
			}
			unix_file_stream(const unix_file_stream &) = delete;
			unix_file_stream &operator= (const unix_file_stream &) = delete;

			unix_file_stream(unix_file_stream &&other)
			{
				fd_ = other.fd_;
				other.fd_ = -1;
			}
			unix_file_stream &operator= (unix_file_stream &&other)
			{
				if (this != &other)
				{
					fd_ = other.fd_;
					other.fd_ = -1;
				}
				return *this;
			}

			operator bool()
			{
				return fd_ > 0;
			}
			bool operator !()
			{
				return fd_ < 0;
			}
			static int open(const char *zPath, int _open_mode)
			{
				int mode = O_RDONLY;
				if (_open_mode & OPEN_CREATE)
				{
					mode = O_CREAT;
					if (_open_mode & OPEN_TRUNC)
					{
						mode |= O_TRUNC;
					}
				}
				else if (_open_mode & OPEN_EXCL)
				{
					mode = O_CREAT | O_EXCL;
				}
				else if (_open_mode & OPEN_TRUNC)
				{
					mode = O_RDWR | O_TRUNC;
				}
				if (_open_mode & OPEN_RDWR)
				{
					mode &= ~O_RDONLY;
					mode |= O_RDWR;
				}
				else if (_open_mode & OPEN_WRONLY)
				{
					mode &= ~O_RDONLY;
					mode |= O_WRONLY;
				}
				if (_open_mode & OPEN_APPEND)
				{
					/* Append mode */
					mode |= O_APPEND;
				}
#ifdef O_TEMP
				if (_open_mode & OPEN_TEMP)
				{
					mode |= O_TEMP;
				}
#endif
#define JX9_UNIX_OPEN_MODE	0640
				fd_ = ::open(zPath, mode, JX9_UNIX_OPEN_MODE);
				if (fd_ < 0)
					return false;
				return true;
			}
			int64_t read(void *pBuffer, jx9_int64 nDatatoRead)
			{
				ssize_t bytes;
				bytes = ::read(fd_, pBuffer, (size_t)nDatatoRead);
				if (bytes < 1)
				{
					return -1;
				}
				return (int64_t)bytes;
			}
			int64_t write(const void *pBuffer, int64_t len)
			{
				const char *data = (const char *)pBuffer;
				int64_t count = 0;
				ssize_t bytes = 0;
				while(len > 0)
				{
					bytes = ::write(fd_, data, (size_t)len);
					if (bytes < 1)
					{
						break;
					}
					len -= bytes;
					count += bytes;
					data += bytes;
				}
				if (len > 0)
					return -1;
				return count;
			}
			bool seek(int64_t offset, whence _whence)
			{
				int mode;
				switch (_whence)
				{
				case whence::CURRENT:
					mode = SEEK_CUR;
					break;
				case whence::END:
					mode = SEEK_END;
					break;
				case 0:
				default:
					mode = SEEK_SET;
					break;
				}
				return !!lseek(fd_, (off_t)offset, mode);
			}
			bool unlock()
			{
				return !flock(fd_, LOCK_UN);
			}
			static int lock(bool exclusive = true)
			{
				if (exclusive)
					return !flock(fd_, LOCK_EX);
				return !flock(fd_, LOCK_SH);
			}
			int64_t tell()
			{
				return (int64_t)lseek(fd_, 0, SEEK_CUR);
			}
			bool trunc(int64_t nOfft)
			{
				return !ftruncate(fd_, (off_t)nOfft);
			}
			bool sync()
			{
				return !fsync(fd_);
			}
			void close()
			{
				if(fd_ > 0)
					::close(fd_);
				fd_ = -1;
			}
		private:
			int fd_ = -1;
		};
	}
	using file_stream = detail::unix_file_stream;
}