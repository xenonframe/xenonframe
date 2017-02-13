#pragma once
namespace xraft
{
namespace detail
{
	class file
	{
	public:
		file()
		{
		}
		~file()
		{
		}
		void operator = (file &&f)
		{
			if (&f == this)
				return;
			move_reset(std::move(f));
		}
		file(file &&self)
		{
			move_reset(std::move(self));
		}
		bool open(const std::string &filepath)
		{
			filepath_ = filepath;
			last_log_index_ = -1;
			log_index_start_ = -1;
			return do_open();
		}

		void write(int64_t index, std::string &&data)
		{
			int64_t file_pos = data_file_.tell();
			uint32_t len = (uint32_t)data.size();

			if(data_file_.write((char*)&len, sizeof(len)) != sizeof(len))
				throw std::runtime_error(FILE_LINE + " data_file_ write error");

			if(data_file_.write(data.data(), data.size()) != data.size())
				throw std::runtime_error(FILE_LINE + " data_file_ write error");

			if(index_file_.write(reinterpret_cast<char*>(&index), sizeof(index)) != sizeof(index))
				throw std::runtime_error(FILE_LINE + " index_file_ write error");

			if (index_file_.write(reinterpret_cast<char*>(&file_pos), sizeof(int64_t)) != sizeof(int64_t))
				throw std::runtime_error(FILE_LINE + " index_file_ write error");

			last_log_index_ = index;
		}

		void get_entries(int64_t &index,
			std::size_t &count,
			std::list<log_entry> &log_entries)
		{
			xutil::guard g([this] {data_file_.seek(0, xutil::file_stream::END); });
			int64_t data_file_offset = 0;
			if(!get_data_file_offset(index, data_file_offset))
				throw std::out_of_range(FILE_LINE + "get_entries index invalid:" + std::to_string(index));
			if(!data_file_.seek(data_file_offset, xutil::file_stream::BEGIN))
				throw std::out_of_range(FILE_LINE + "seek file error,offset:" + std::to_string(data_file_offset));
			do
			{
				uint32_t len;
				if (data_file_.read((char*)&len, sizeof(uint32_t)) != sizeof(uint32_t))
					return;
				std::string buffer;
				buffer.resize(len);
				if(data_file_.read((char*)buffer.data(), len) != len)
					throw std::runtime_error(FILE_LINE + "data_file read error");
				log_entry entry;
				if(!entry.from_string(buffer))
					throw std::runtime_error(FILE_LINE + "entry decode error");
				log_entries.emplace_back(std::move(entry));
				++index;
				--count;
			} while (count > 0);
		}

		log_entry get_entry(int64_t index)
		{
			xutil::guard g([this] {data_file_.seek(0, xutil::file_stream::END); });
			int64_t data_file_offset = 0;
			if (!get_data_file_offset(index, data_file_offset))
				throw std::out_of_range(FILE_LINE + "get_entry index invalid:"+std::to_string(index));
			if (!data_file_.seek(data_file_offset, xutil::file_stream::BEGIN))
				throw std::out_of_range(FILE_LINE + "seek file error,offset:"+ std::to_string(data_file_offset));
			uint32_t len;
			if (data_file_.read((char*)&len, sizeof(uint32_t)) < 0)
				throw std::runtime_error(FILE_LINE + "data_file read error");
			std::string buffer;
			buffer.resize(len);
			if (data_file_.read((char*)buffer.data(), len) < 0)
				throw std::runtime_error(FILE_LINE + "data_file read error");
			log_entry entry;
			if (!entry.from_string(buffer))
				throw std::runtime_error(FILE_LINE + "entry decode error");
			return std::move(entry);
		}
		int64_t size()
		{
			return data_file_.tell();
		}
		bool rm()
		{
			return rm_on_lock();
		}
		void truncate_suffix(int64_t index)
		{
			int64_t offset = 0;

			if (!get_data_file_offset(index, offset))
				throw std::out_of_range(FILE_LINE + "index out_of_range:" + std::to_string(index));
			if (!data_file_.trunc(offset))
				throw std::runtime_error(FILE_LINE +"data_file trunc error");
			if(!index_file_.trunc(get_index_file_offset(index)))
				throw std::runtime_error(FILE_LINE +"index_file_ trunc error");
		}
		void truncate_prefix(int64_t index)
		{
			int64_t offset = 0;
			if (!get_data_file_offset(index + 1, offset))
				throw std::runtime_error(FILE_LINE+ "get_data_file_offset failed");
			data_file_.close();
			index_file_.close();
			if (!functors::fs::truncate_prefix()(get_data_file_path(), offset))
				throw std::runtime_error(FILE_LINE  + "functors::fs::truncate_prefix error");

			offset = get_index_file_offset(index + 1);
			auto tmp = log_index_start_;
			log_index_start_ = -1;
			if (!functors::fs::truncate_prefix()(get_index_file_path(), offset))
				throw std::runtime_error(FILE_LINE+ "functors::fs::truncate_prefix() error");

			auto old_data_file = get_data_file_path();
			auto old_index_file = get_index_file_path();
			auto beg = filepath_.find_last_of('/') + 1;
			filepath_ = filepath_.substr(0, beg);
			filepath_ += std::to_string(index + 1);
			filepath_ += ".log";
			xutil::vfs::rename()(old_data_file, get_data_file_path());
			xutil::vfs::rename()(old_index_file, get_index_file_path());
			do_open();
		}
		int64_t get_last_log_index()
		{
			if (last_log_index_ == -1)
			{
				xutil::guard g([this] {index_file_.seek(0, xutil::file_stream::BEGIN); });
				if (!index_file_.seek(-(int32_t)(sizeof(int64_t) * 2), xutil::file_stream::END))
					return 0;
				if (index_file_.read((char*)&last_log_index_, sizeof(last_log_index_)) < 0)
					return 0;
			}
			return last_log_index_;
		}
		int64_t get_log_start()
		{
			return get_log_start_no_lock();
		}
		bool is_open()
		{
			return data_file_ && index_file_;
		}
		void sync()
		{
			data_file_.sync();
			index_file_.sync();
		}
	private:
		void move_reset(file &&self)
		{
			data_file_ = std::move(self.data_file_);
			index_file_ = std::move(self.index_file_);
			filepath_ = std::move(self.filepath_);
			last_log_index_ = self.last_log_index_;
			log_index_start_ = self.log_index_start_;
			self.last_log_index_ = -1;
			self.log_index_start_ = -1;
		}
		std::string get_data_file_path()
		{
			return filepath_;
		}
		std::string get_index_file_path()
		{
			auto filepath = filepath_;
			filepath.pop_back();
			filepath.pop_back();
			filepath.pop_back();
			filepath.pop_back();
			return filepath + ".index";
		}
		int64_t get_index_file_offset(int64_t index)
		{
			auto diff = index - get_log_start_no_lock();
			return diff * sizeof(int64_t) * 2;;
		}
		bool get_data_file_offset(int64_t index, int64_t &data_file_offset)
		{
			data_file_offset = 0;
			auto offset = get_index_file_offset(index);
			if (offset == 0)
				return true;
			xutil::guard g([this] {index_file_.seek(0, xutil::file_stream::END);});
			if (!index_file_)
				throw std::runtime_error(FILE_LINE+ "file not open");
			if (!index_file_.seek(offset, xutil::file_stream::BEGIN))
				return false;
			int64_t index_buffer_;
			if (index_file_.read((char*)&index_buffer_, sizeof(int64_t)) != sizeof(int64_t))
			{
				return false;
			}
			if (index_buffer_ != index)
			{
				throw std::runtime_error(FILE_LINE+ " index file's data error");
			}
			if (index_file_.read((char*)&data_file_offset, sizeof(int64_t)) < 0)
				return false;
			return true;
		}
		bool rm_on_lock()
		{
			log_index_start_ = -1;
			last_log_index_ = -1;
			data_file_.close();
			index_file_.close();
			if (!xutil::vfs::unlink()(get_data_file_path()) ||
				!xutil::vfs::unlink()(get_index_file_path()))
				return false;
			return true;
		}
		int64_t get_log_start_no_lock()
		{
			if (log_index_start_ == -1)
			{
				index_file_.seek(0, xutil::file_stream::BEGIN);
				char buffer[sizeof(int64_t)] = { 0 };
				auto bytes = index_file_.read(buffer, sizeof(buffer));
				index_file_.seek(0, xutil::file_stream::END);
				if (bytes  != sizeof(buffer))
					return 0;

				log_index_start_ = *(int64_t*)(buffer);
			}
			return log_index_start_;
		}
		bool do_open()
		{
			if (data_file_)
				data_file_.close();
			if (index_file_)
				index_file_.close();
			int mode = 
				xutil::file_stream::OPEN_BINARY | 
				xutil::file_stream::OPEN_RDWR | 
				xutil::file_stream::OPEN_CREATE;
			data_file_.open(get_data_file_path().c_str(), mode);
			index_file_.open(get_index_file_path().c_str(), mode);
			if (!data_file_|| !index_file_)
				return false;
			return true;
		}

		int64_t last_log_index_ = -1;
		int64_t log_index_start_ = -1;
		xutil::file_stream data_file_;
		xutil::file_stream index_file_;
		std::string filepath_;
	};

	class filelog
	{
	public:
		filelog()
		{
		}
		bool init(const std::string &path)
		{
			path_ = path;
			if(!xutil::vfs::mkdir()(path))
				throw std::runtime_error(FILE_LINE + " mkdir error, "+ path);
			auto files = xutil::vfs::ls_files()(path_);
			if (files.empty())
				return true;
			for (auto &itr : files)
			{
				if (itr.find(".log") != std::string::npos)
				{
					file f;
					if (!f.open(itr))
						throw std::runtime_error(FILE_LINE + " file open error, " + itr);
					logfiles_.emplace(f.get_log_start(), std::move(f));
				}
			}
			current_file_ = std::move(logfiles_.rbegin()->second);
			logfiles_.erase(logfiles_.find(current_file_.get_log_start()));
			last_index_ = current_file_.get_last_log_index();
			return true;
		}

		bool write(detail::log_entry &&entry, int64_t &index)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			if (entry.index_)
			{
				last_index_ = entry.index_;
			}
			else
			{
				++last_index_;
				index = last_index_;
				entry.index_ = last_index_;
			}
			std::string buffer = entry.to_string();
			log_entries_cache_size_ += buffer.size();
			log_entries_cache_.emplace_back(std::move(entry));
			check_log_entries_size();
			if (!current_file_.is_open())
			{
				current_file_.open(path_ + std::to_string(entry.index_) + ".log");
			}
			current_file_.write(last_index_, std::move(buffer));
			check_current_file_size();
			return true;
		}
		log_entry get_entry(int64_t index)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			log_entry entry;
			if (get_entry_from_cache(entry, index))
				return std::move(entry);
			if (current_file_.is_open() && 
				current_file_.get_log_start() <= index &&
				index <= current_file_.get_last_log_index())
			{
				return current_file_.get_entry(index);
			}
			for (auto itr = logfiles_.begin(); itr != logfiles_.end(); ++itr)
			{
				auto &f = itr->second;
				if (f.get_log_start() <= index && index <= f.get_last_log_index())
					return f.get_entry(index);
			}
			return {};
		}
		std::list<log_entry> get_entries(int64_t index, std::size_t count = 10)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			std::list<log_entry> log_entries;
			get_entries_from_cache(log_entries, index, count);
			if (count == 0)
				return std::move(log_entries);
			for (auto itr = logfiles_.begin(); itr != logfiles_.end(); itr++)
			{
				file &f = itr->second;
				get_entries_from_cache(log_entries, index, count);
				if (count == 0)
					return std::move(log_entries);
				if (f.get_log_start() <= index && index <= f.get_last_log_index())
				{
					f.get_entries(index, count, log_entries);
					if (count == 0)
						return std::move(log_entries);
				}
			}
			get_entries_from_cache(log_entries, index, count);
			if (count == 0)
				return std::move(log_entries);
			if (current_file_.is_open() && 
				index <= current_file_.get_last_log_index() &&
				current_file_.get_log_start() <= index)
			{
				current_file_.get_entries(index, count, log_entries);
			}
			return std::move(log_entries);
		}
		void truncate_prefix(int64_t index)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			for (auto itr = log_entries_cache_.begin(); itr != log_entries_cache_.end();)
			{
				if (itr->index_ <= index)
					itr = log_entries_cache_.erase(itr);
				else
					++itr;
			}
			for (auto itr = logfiles_.begin(); itr != logfiles_.end(); )
			{
				if (itr->second.get_last_log_index() <= index)
				{
					itr->second.rm();
					itr = logfiles_.erase(itr);
					continue;
				}
				else if (index < itr->second.get_last_log_index() && itr->second.get_log_start() <= index)
				{
					itr->second.truncate_prefix(index);
					auto f = std::move(itr->second);
					logfiles_.erase(itr);
					logfiles_.emplace(f.get_log_start(), std::move(f));
					break;
				}
				++itr;
			}
			if (current_file_.is_open() && index == current_file_.get_last_log_index())
			{
				current_file_.rm();
			}
			else if ( current_file_.is_open () && 
						current_file_.get_log_start() <= index &&
						index < current_file_.get_last_log_index())
			{
				current_file_.truncate_prefix(index);
			}
		}
		void truncate_suffix(int64_t index)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			for (auto itr = log_entries_cache_.begin(); itr != log_entries_cache_.end();)
			{
				if (index <= itr->index_)
					itr = log_entries_cache_.erase(itr);
				else
					++itr;
			}
			for (auto itr = logfiles_.begin(); itr != logfiles_.end(); ++itr)
			{
				if (index <= itr->second.get_last_log_index() && itr->second.get_log_start() <= index)
				{
					itr->second.truncate_suffix(index);
					current_file_.rm();
					last_index_ = index - 1;
					if (last_index_ < 0)
						last_index_ = 0;
					if (itr->second.get_last_log_index() > 0)
					{
						current_file_ = std::move(itr->second);
						last_index_ = current_file_.get_last_log_index();
						logfiles_.erase(itr);
						break;
					}
					
				}
			}
			for (auto itr = logfiles_.upper_bound(index); itr != logfiles_.end();)
			{
				itr->second.rm();
				itr = logfiles_.erase(itr);
			}
		}
		void clear()
		{
			std::lock_guard<std::mutex> lock(mtx_);
			current_file_.rm();
			for (auto &itr : logfiles_)
				itr.second.rm();
			logfiles_.clear();
		}
		int64_t get_last_log_entry_term()
		{
			std::lock_guard<std::mutex> lock(mtx_);
			if (log_entries_cache_.size())
				return log_entries_cache_.back().term_;
			return 0;
		}
		int64_t get_last_index()
		{
			std::lock_guard<std::mutex> lock(mtx_);
			return last_index_;
		}
		int64_t get_log_start_index()
		{
			std::lock_guard<std::mutex> lock(mtx_);
			if (logfiles_.size())
				return logfiles_.begin()->second.get_log_start();
			else if(current_file_.is_open())
				return current_file_.get_log_start();
			return 0;
		}
		
		void set_make_snapshot_trigger(std::function<void()> callback)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			make_snapshot_trigger_ = callback;
		}

		void max_log_size(std::size_t value)
		{
			max_log_size_ = value;
			if (max_log_size_ == 0)
				max_log_size_ = 1024 * 1024 * 10;//10M
		}

		void max_log_count(std::size_t value)
		{
			max_log_count_ = value;
			if (max_log_count_ == 0)
				max_log_count_ = 5;
		}
	private:
		bool get_entries_from_cache(std::list<log_entry> &log_entries,
			int64_t &index, std::size_t &count)
		{
			if (log_entries_cache_.size() && log_entries_cache_.front().index_ <= index)
			{
				for (auto &itr : log_entries_cache_)
				{
					if (index == itr.index_)
					{
						log_entries.push_back(itr);
						++index;
						count--;
						if (count == 0)
							return true;
					}
				}
				return true;
			}
			return false;
		}
		bool get_entry_from_cache(log_entry &entry, int64_t index)
		{
			if (log_entries_cache_.size() &&
				log_entries_cache_.front().index_ <= index)
			{
				for (auto &itr : log_entries_cache_)
				{
					if (index == itr.index_)
					{
						entry = itr;
						return true;
					}
				}
			}
			return false;
		}
		void check_log_entries_size()
		{
			while (log_entries_cache_.size() &&
				log_entries_cache_size_ > max_cache_size_)
			{
				log_entries_cache_size_ -= log_entries_cache_.front().bytes();
				log_entries_cache_.pop_front();
			}
		}
		bool check_current_file_size()
		{
			if (current_file_.size() > max_log_size_)
			{
				current_file_.sync();
				logfiles_.emplace(current_file_.get_log_start(),
					std::move(current_file_));
				std::string filepath = path_ + std::to_string(last_index_ + 1) + ".log";
				if(!current_file_.open(filepath))
					throw std::runtime_error(FILE_LINE + " file open error, " + filepath);
				check_make_snapshot_trigger();
			}
			return true;
		}

		void check_make_snapshot_trigger()
		{
			if (logfiles_.size() > max_log_count_ 
				&& make_snapshot_trigger_)
			{
				make_snapshot_trigger_();
				make_snapshot_trigger_ = nullptr;
			}
		}
		std::mutex mtx_;
		std::list<log_entry> log_entries_cache_;
		std::size_t log_entries_cache_size_ = 0;
		std::size_t max_cache_size_ = 1;
		std::size_t max_log_count_ = 4;
		int64_t max_log_size_ = 10*1024;
		int64_t last_index_ = 0;
		std::string path_;
		file current_file_;
		int64_t current_file_last_index_ = 0;
		std::map<int64_t, detail::file> logfiles_;
		std::function<void()> make_snapshot_trigger_;
	};
}

}