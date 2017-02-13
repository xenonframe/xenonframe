#pragma once
namespace xraft
{
	namespace detail
	{
		struct snapshot_head
		{
			uint32_t version_ = 1;
			uint32_t magic_num_ = 'X'+'R'+'A'+'F'+'T';
			int64_t last_included_index_;
			int64_t last_included_term_;
		};
		class snapshot_reader
		{
		public:
			snapshot_reader()
			{

			}
			bool open(const std::string &filepath)
			{
				filepath_ = filepath;
				if (file_)
					file_.close();
				int mode = 
					xutil::file_stream::OPEN_BINARY | 
					xutil::file_stream::OPEN_RDONLY;
				return file_.open(filepath_.c_str(), mode);
			}
			bool read_sanpshot_head(snapshot_head &head)
			{
				std::string buffer;
				buffer.resize(sizeof(head));
				file_.seek(0, xutil::file_stream::BEGIN);
				if (file_.read((char*)buffer.data(), buffer.size()) != buffer.size())
					return false;
				unsigned char *ptr = (unsigned char*)buffer.data();
				if (endec::get_uint32(ptr) != head.version_ || 
					endec::get_uint32(ptr) != head.magic_num_)
					return false;
				head.last_included_index_ = (int64_t)endec::get_uint64(ptr);
				head.last_included_term_ = (int64_t)endec::get_uint64(ptr);
				return true;
			}
			xutil::file_stream &get_snapshot_stream()
			{
				return file_;
			}
		private:
			std::string filepath_;
			xutil::file_stream file_;
		};
		class snapshot_writer
		{
		public:
			snapshot_writer() { }

			operator bool()
			{
				return file_;
			}
			bool open(const std::string &filepath)
			{
				filepath_ = filepath;
				if (file_)
					file_.close();
				int mode = 
					xutil::file_stream::OPEN_TRUNC | 
					xutil::file_stream::OPEN_CREATE | 
					xutil::file_stream::OPEN_RDWR;
				return file_.open(filepath_.c_str(), mode);;
			}
			void close()
			{
				if(file_)
				file_.close();
			}
			bool write_sanpshot_head(const snapshot_head &head)
			{
				std::string buffer;
				buffer.resize(sizeof(head));
				unsigned char *ptr = (unsigned char*)buffer.data();
				endec::put_uint32(ptr, head.version_);
				endec::put_uint32(ptr, head.magic_num_);
				endec::put_uint64(ptr, (uint64_t)head.last_included_index_);
				endec::put_uint64(ptr, (uint64_t)head.last_included_term_);
				assert(buffer.size() == ptr - (unsigned char*)buffer.data());
				return write(buffer);
			}
			bool write(const std::string &buffer)
			{
				return file_.write(buffer.data(), buffer.size())== buffer.size();
			}
			void discard()
			{
				close();
				if (!xutil::vfs::unlink()(filepath_))
					throw std::runtime_error("unlink file error: "+ filepath_);
			}
			std::size_t get_bytes_writted()
			{
				return file_.tell();
			}
			std::string get_snapshot_filepath()
			{
				return filepath_;
			}
		private:
			std::string filepath_;
			xutil::file_stream file_;
		};
		class snapshot_builder
		{
		public:
			using get_last_commit_index_handle = std::function<int64_t()>;
			using build_snapshot_callback = std::function<void(const std::function<bool(const std::string &)>&, int64_t)>;
			using build_snapshot_done_callback = std::function<void(int64_t)>;
			using get_log_entry_term_handle = std::function<int64_t(int64_t)>;

			snapshot_builder()
			{
				
			}
			void set_snapshot_base_path(const std::string &path)
			{
				snapshot_base_path_ = path;
				if (!xutil::vfs::mkdir()(snapshot_base_path_))
					throw std::runtime_error("mkdir "+ snapshot_base_path_+" failed");
			}
			void regist_get_last_commit_index(const get_last_commit_index_handle &handle)
			{
				get_last_commit_index_ = handle;
			}
			void regist_get_log_entry_term_handle(const get_log_entry_term_handle &handle)
			{
				get_log_entry_term_ = handle;
			}
			void regist_build_snapshot_callback(const build_snapshot_callback &callback)
			{
				build_snapshot_ = callback;
			}
			void regist_build_snapshot_done_callback(const build_snapshot_done_callback &callback)
			{
				build_snapshot_done_ = callback;
			}
			void make_snapshot()
			{
				do_make_snapshot();
			}
		private:
			void do_make_snapshot()
			{
				snapshot_head head;
				auto commit_index = get_last_commit_index_();

				head.last_included_index_ = commit_index;
				head.last_included_term_ = get_log_entry_term_(commit_index);
				snapshot_writer writer;
				std::string filepath = snapshot_base_path_ + std::to_string(commit_index) + ".ss";
				if (!writer.open(filepath))
					throw std::runtime_error("open "+filepath+ "error");
				writer.write_sanpshot_head(head);
				try
				{
					build_snapshot_([&writer](const std::string &buffer){
							return writer.write(buffer);
						},commit_index);
				}
				catch (const std::exception& e)
				{
					std::cout << e.what() << std::endl;;
					writer.discard();
					return;
				}
				writer.close();

				commit_index = get_last_commit_index_();
				if (head.last_included_index_ > commit_index || 
					head.last_included_term_ > get_log_entry_term_(commit_index))
				{
					std::cout << "commit_index " << commit_index << std::endl;;
				}
				build_snapshot_done_(commit_index);
			}
			std::string snapshot_base_path_;
			get_log_entry_term_handle get_log_entry_term_;
			get_last_commit_index_handle get_last_commit_index_;
			build_snapshot_callback build_snapshot_;
			build_snapshot_done_callback build_snapshot_done_;
			bool is_stop_ = false;
			int64_t distance_ = 10000;
			std::thread worker_;
		};
	}
} 