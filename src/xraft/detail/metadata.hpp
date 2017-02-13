#pragma once
namespace xraft
{
namespace detail
{
	struct lock_free 
	{
		void lock() {};
		void unlock() {};
	};

	template<typename mutex = std::mutex>
	class metadata
	{
		enum type:char
		{
			e_string,
			e_integral
		};
		enum op:char
		{
			e_set,
			e_del,
		};
	public:
		metadata()
		{

		}
		~metadata()
		{
			max_log_file_ = 0;
			make_snapshot();
		}
		void clear()
		{
			std::lock_guard<mutex> lock(mtx_);
			string_map_.clear();
			integral_map_.clear();
			log_.close();
			std::vector<std::string> files = xutil::vfs::ls_files()(path_);
			if (files.empty())
				return;
			for (auto &itr: files)
			{
				xutil::vfs::unlink()(itr);
			}
			reopen_log(true);
		}
		void init(const std::string &path)
		{
			if (!xutil::vfs::mkdir()(path))
				throw std::runtime_error(FILE_LINE + "mkdir error"+ path);
			path_ = path;
			load();
		}
		void set(const std::string &key, const std::string &value)
		{
			std::lock_guard<mutex> lock(mtx_);
			write_log(build_log(key, value, op::e_set));
			string_map_[key] = value;
		}
		void set(const std::string &key, int64_t value)
		{
			std::lock_guard<mutex> lock(mtx_);
			write_log(build_log(key, value, op::e_set));
			integral_map_[key] = value;
		}
		bool del(const std::string &key)
		{
			std::lock_guard<mutex> lock(mtx_);

			bool found = false;
			auto itr = string_map_.find(key);
			if (itr != string_map_.end())
			{
				found = true;
				string_map_.erase(itr);
			}
			auto itr2 = integral_map_.find(key);
			if (itr2 != integral_map_.end())
			{
				found = true;
				integral_map_.erase(itr2);
			}
			if (found == true)
			{
				write_log(build_log(key, op::e_del, type::e_string));
				write_log(build_log(key, op::e_del, type::e_integral));
			}
			return found;
		}
		bool get(const std::string &key, std::string &value)
		{
			std::lock_guard<mutex> lock(mtx_);
			auto itr = string_map_.find(key);
			if (itr == string_map_.end())
				return false;
			value = itr->second;
			return true;
		}
		bool get(const std::string &key, int64_t &value)
		{
			std::lock_guard<mutex> lock(mtx_);
			auto itr = integral_map_.find(key);
			if (itr == integral_map_.end())
				return false;
			value = itr->second;
			return true;
		}
		void write_snapshot(const std::function<bool(const std::string &)>& writer)
		{
			std::lock_guard<mutex> lock(mtx_);
			for (auto &itr : string_map_)
			{
				std::string log = build_log(itr.first, itr.second, op::e_set);
				
				uint8_t buf[sizeof(uint32_t)];
				uint8_t *ptr = buf;
				endec::put_uint32(ptr, (uint32_t)log.size());

				if (!writer(std::string((char*)buf, sizeof(buf))))
					throw std::runtime_error(FILE_LINE + "write data error");
				if (!writer(log))
					throw std::runtime_error(FILE_LINE + "write data error");
			}
		}
		void load_snapshot(xutil::file_stream &file)
		{
			load_fstream(file);
		}
	private:
		std::string build_log(const std::string &key, const std::string &value,op _op)
		{
			std::string buffer;
			buffer.resize(endec::get_sizeof(key) +
				endec::get_sizeof(value) +
				endec::get_sizeof(typename std::underlying_type<type>::type()) +
				endec::get_sizeof(typename std::underlying_type<op>::type()));
			unsigned char* ptr = (unsigned char*)buffer.data();
			endec::put_uint8(ptr, static_cast<uint8_t>(_op));
			endec::put_uint8(ptr, static_cast<uint8_t>(type::e_string));
			endec::put_string(ptr, key);
			endec::put_string(ptr, value);
			return std::move(buffer);
		}
		std::string build_log(const std::string &key, int64_t &value, op _op)
		{
			std::string buffer;
			buffer.resize(endec::get_sizeof(key) +
				endec::get_sizeof(value) +
				endec::get_sizeof(typename std::underlying_type<type>::type()) +
				endec::get_sizeof(typename std::underlying_type<op>::type()));

			unsigned char* ptr = (unsigned char*)buffer.data();
			endec::put_uint8(ptr, static_cast<uint8_t>(_op));
			endec::put_uint8(ptr, static_cast<uint8_t>(type::e_integral));
			endec::put_string(ptr, key);
			endec::put_uint64(ptr, (uint64_t)value);
			return std::move(buffer);
		}
		std::string build_log(const std::string &key, op _op, type _type)
		{
			std::string buffer;
			buffer.resize(endec::get_sizeof(key) +
				endec::get_sizeof(typename std::underlying_type<type>::type()) +
				endec::get_sizeof(typename std::underlying_type<op>::type()));

			unsigned char* ptr = (unsigned char*)buffer.data();
			endec::put_uint8(ptr, static_cast<uint8_t>(_op));
			endec::put_uint8(ptr, static_cast<uint8_t>(_type));
			endec::put_string(ptr, key);
			return std::move(buffer);
		}
		void write_log(const std::string &data)
		{
			char buffer[sizeof(uint32_t)];
			uint8_t *ptr = (uint8_t *)buffer;
			endec::put_uint32(ptr, (uint32_t)data.size());
			
			if (log_.write((char*)(buffer), sizeof(buffer)) != sizeof(buffer))
				throw std::runtime_error(FILE_LINE + "log_ writer error");

			if (log_.write(data.data(), data.size()) != data.size())
				throw std::runtime_error(FILE_LINE + "log_ writer error");

			try_make_snapshot();
		}
		void load()
		{
			std::vector<std::string> files = xutil::vfs::ls_files()(path_);
			if (files.empty())
				reopen_log();

			std::sort(files.begin(), files.end(),std::greater<std::string>());
			for (auto &file: files)
			{
				auto end = file.find(".metadata");
				if (end == std::string::npos)
					continue;
				std::size_t beg = file.find_last_of('/') + 1;
				if (beg == std::string::npos)
					beg = file.find_last_of('\\') + 1;
				std::string index = file.substr(beg, end - beg);
				index_ = std::strtoull(index.c_str(), 0, 10);
				if (errno == ERANGE || index_ == 0)
					assert(false);
				try
				{
					load_file(get_snapshot_file());
				}
				catch (...)
				{
				}
				load_file(get_log_file());
				reopen_log(false);
			}
		}
		template<typename T>
		void write(xutil::file_stream &file, T &map)
		{
			for (auto &itr : map)
			{
				std::string log = build_log(itr.first, itr.second, op::e_set);
				uint8_t buf[sizeof(uint32_t)];
				uint8_t *ptr = buf;
				endec::put_uint32(ptr, (uint32_t)log.size());

				if (file.write((char*)buf, sizeof(buf)) != sizeof(buf))
					throw std::runtime_error(FILE_LINE + "file write failed");

				if (file.write(log.data(), log.size()) != log.size())
					throw std::runtime_error(FILE_LINE + "file write failed");
			}
		}
		void load_fstream(xutil::file_stream &file)
		{
			std::string buffer;
			uint8_t len_buf[sizeof(uint32_t)];
			do
			{
				auto bytes = file.read((char*)len_buf, sizeof(len_buf));
				if (bytes == 0)
					return;
				else if (bytes != sizeof(len_buf))
					throw std::runtime_error(FILE_LINE + "file read error");

				auto *ptr = len_buf;
				auto len = endec::get_uint32(ptr);
				
				buffer.resize(len);
				bytes =  file.read((char*)buffer.data(), len);
				if (bytes != len)
					throw std::runtime_error(FILE_LINE + "file read failed");

				ptr = (uint8_t*)buffer.data();
				char _op = (char)endec::get_uint8(ptr);
				char _t = (char)endec::get_uint8(ptr);

				if (type::e_string == static_cast<type>(_t))
				{
					if (static_cast<op>(_op) == op::e_set)
					{
						auto key = endec::get_string(ptr);
						auto value = endec::get_string(ptr);
						string_map_[key] = value;
					}
					else if (static_cast<op>(_op) == op::e_del)
					{
						auto key = endec::get_string(ptr);
						auto itr = string_map_.find(key);
						if (itr != string_map_.end())
							string_map_.erase(itr);
					}
				}
				else if (type::e_integral == static_cast<type>(_t))
				{
					if (static_cast<op>(_op) == op::e_set)
					{
						auto key = endec::get_string(ptr);
						auto value = endec::get_uint64(ptr);
						integral_map_[key] = value;
					}
					else if (static_cast<op>(_op) == op::e_del)
					{
						auto key = endec::get_string(ptr);
						auto itr = integral_map_.find(key);
						if (itr != integral_map_.end())
							integral_map_.erase(itr);
					}
				}
			} while (true);
		}

		void load_file(const std::string &filepath)
		{
			xutil::file_stream file;
			int mode = xutil::file_stream::open_mode::OPEN_BINARY | 
				xutil::file_stream::open_mode::OPEN_RDONLY;
			if (!file.open(filepath, mode))
				throw std::runtime_error(FILE_LINE + "open file error");
			load_fstream(file);
			file.close();
		}
		void try_make_snapshot()
		{
			if ((int)max_log_file_ > log_.tell())
				return ;
			make_snapshot();
		}
		void make_snapshot()
		{
			++index_;

			xutil::file_stream file;
			int mode =
				xutil::file_stream::open_mode::OPEN_RDWR |
				xutil::file_stream::open_mode::OPEN_CREATE |
				xutil::file_stream::open_mode::OPEN_TRUNC | 
				xutil::file_stream::open_mode::OPEN_BINARY;
			auto filepath = get_snapshot_file();
			if (!file.open(filepath, mode))
				throw std::runtime_error( FILE_LINE + "file open error,"+ filepath);

			write(file, string_map_);
			write(file, integral_map_);

			file.sync();
			file.close();
			reopen_log();
			touch_metadata_file();
			rm_old_files();
		}
		void reopen_log(bool trunc = true)
		{
			log_.close();
			int mode =
				xutil::file_stream::open_mode::OPEN_BINARY |
				xutil::file_stream::open_mode::OPEN_RDWR |
				xutil::file_stream::open_mode::OPEN_CREATE;

			if (trunc)
				mode = 
					xutil::file_stream::open_mode::OPEN_CREATE|
					xutil::file_stream::open_mode::OPEN_RDWR | 
					xutil::file_stream::open_mode::OPEN_TRUNC;

			auto filepath = get_log_file();
			if (!log_.open(filepath, mode))
				throw std::runtime_error(FILE_LINE + " open file erorr: " + filepath);
			touch_metadata_file();
		}
		void touch_metadata_file()
		{
			xutil::file_stream file;
			auto filepath = get_metadata_file();
			if (!file.open(filepath, xutil::file_stream::open_mode::OPEN_CREATE))
				throw std::runtime_error(FILE_LINE + "open file error, filepath: " + filepath);
		}
		void rm_old_files()
		{
			auto filepath = get_old_log_file();
			if (xutil::vfs::is_file()(filepath))
			{
				if (!xutil::vfs::unlink()(filepath))
					throw std::runtime_error(FILE_LINE + "unlink file error, filepath: " + filepath);
			}

			filepath = get_old_snapshot_file();
			if (xutil::vfs::is_file()(filepath))
			{
				if (!xutil::vfs::unlink()(filepath))
					throw std::runtime_error(FILE_LINE + "unlink file error, filepath: " + filepath);
			}
			
			filepath = get_old_metadata_file();
			if (xutil::vfs::is_file()(filepath))
			{
				if (!xutil::vfs::unlink()(filepath))
					throw std::runtime_error(FILE_LINE + "unlink file error, filepath: " + filepath);
			}
			
		}
		std::string get_snapshot_file()
		{
			return path_ + std::to_string(index_)+ ".data";
		}
		std::string get_log_file()
		{
			return path_ + std::to_string(index_) + ".Log";
		}
		std::string get_metadata_file()
		{
			return path_ + std::to_string(index_) + ".metadata";
		}
		std::string get_old_snapshot_file()
		{
			return path_ + std::to_string(index_ - 1) + ".data";
		}
		std::string get_old_log_file()
		{
			return path_ + std::to_string(index_ - 1) + ".Log";
		}
		std::string get_old_metadata_file()
		{
			return path_ + std::to_string(index_ - 1) + ".metadata";
		}
		uint64_t index_ = 1;
		std::size_t max_log_file_ = 10 * 1024 * 1024;
		xutil::file_stream log_;
		std::string path_;
		mutex mtx_;
		std::map<std::string, std::string> string_map_;
		std::map<std::string, int64_t> integral_map_;
	};
}
}