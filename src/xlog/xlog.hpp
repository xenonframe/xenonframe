#pragma once
#include <tuple>
#include <atomic>
#include <memory>
#include <queue>
#include <thread>
#include <fstream>
#include <mutex>
#include <cassert>
#include <utility>
namespace xlog
{
	template<typename T, typename Tuple>
	struct  TupleIndex;

	template<typename T, typename ...Types>
	struct TupleIndex<T, std::tuple<T, Types...>>
	{
		static constexpr const short index = 0;
	};

	template<typename T, typename U, typename ...Types>
	struct TupleIndex<T, std::tuple<U, Types...>>
	{
		static constexpr const short index = 
			TupleIndex<T, std::tuple<Types...>>::index + 1;
	};
	
	enum class log_level : uint8_t { INFO, WARN, CRIT };

	struct item
	{
		struct static_str
		{
			explicit static_str(char const *ptr)
				:ptr_(ptr)
			{
			}
			std::ostream & operator >> (std::ostream &out)
			{
				out << ptr_;
				return out;
			}
			char const *ptr_;
		};
		typedef std::tuple<
			char, 
			char *, 
			const char *,
			bool, 
			double, 
			uint16_t, 
			int16_t, 
			uint32_t,
			int32_t, 
			uint64_t, 
			int64_t, 
			long,
			static_str
		> supported_type;
	public:
		item()
		{

		}
		item(log_level level,const char *file, int line)
			:buf_size_(sizeof(stack_buf_)),
			level_(level),
			 file_(file),
			 line_(line)
		{

		}
		template <typename Arg>
		typename std::enable_if<std::is_same<Arg, char*>::value, item&>::type
			operator<<(Arg const &arg)
		{
			append_obj(TupleIndex<Arg, supported_type>::index);
			copy_str(arg);
			return *this;
		}

		template <typename Arg>
		typename std::enable_if<std::is_same<Arg, const char*>::value, item&>::type
			operator<<(Arg const &arg)
		{
			append_obj(TupleIndex<Arg, supported_type>::index);
			copy_str(arg);
			return *this;
		}		item &operator << (const std::string  &str)
		{
			return *this << str.c_str();
		}
 		template<size_t N>
		item &operator<<(const char (&arg)[N])
		{
			static_str str(arg);
			append_obj(TupleIndex<static_str, supported_type >::index);
			append_obj(str);
			return *this;
		}
		template<typename T>
 		typename std::enable_if<std::is_integral<T>::value, item &>::type
		operator <<(T t)
		{
			append_obj(TupleIndex<T, supported_type >::index);
			append_obj(t);
			return *this;
		}
		void operator >> (std::ostream &out)
		{
			std::size_t pos = 0;
			char *buf = heap_buf_ ? heap_buf_.get() : stack_buf_;

			out << "[" << to_string(level_) << "] ";
			if (file_)
			{
				out << "[" << file_ << "]";
				out << "[" << line_ << "]";
			}

			out << "{";
			while (pos < buf_used_)
			{
				short type = *reinterpret_cast<short*>(buf + pos);
				pos += sizeof(type);
				switch (type)
				{
				case 0:
				{
					auto val = *reinterpret_cast<typename 
						std::tuple_element<0, supported_type>::type *>
						(buf + pos);
					out << val;
					pos += sizeof(val);
					break;
				}
				case 1:
				{
					auto val = reinterpret_cast<typename 
						std::tuple_element<1, supported_type>::type>
						(buf + pos);
					out << val;
					pos += strlen(val);
					break;
				}
				case 2:
				{
					auto val = reinterpret_cast<typename std::tuple_element<2,
						supported_type>::type>
						(buf + pos);
					out << val;
					pos += strlen(val);
					break;
				}
				case 3:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<3,
						supported_type>::type *>
						(buf + pos);
					out << (val ? "true":"false");
					pos += sizeof(val);
					break;
				}
				case 4:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<4,
						supported_type>::type *>
						(buf + pos);
					out << val;
					pos += sizeof(val);
					break;
				}
				case 5:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<5
						, supported_type>::type *>
						(buf + pos);
					out << val;
					pos += sizeof(val);
					break;
				}
				case 6:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<6,
						supported_type>::type *>
						(buf + pos);
					out << val;
					pos += sizeof(val);
					break;
				}
				case 7:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<7,
						supported_type>::type *>
						(buf + pos);
					out << val;
					pos += sizeof(val);
					break;
				}
				case 8:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<8, 
						supported_type>::type *>
						(buf + pos);
					out << val;
					pos += sizeof(val);
					break;
				}
				case 9:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<9,
						supported_type>::type *>(buf + pos);
					out << val;
					pos += sizeof(val);
					break;
				}
				case 10:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<10, 
						supported_type>::type *>(buf + pos);
					out << val;
					pos += sizeof(val);
					break;
				}
				case 11:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<11, 
						supported_type>::type *>(buf + pos);
					out << val;
					pos += sizeof(val);
					break;
				}
				case 12:
				{
					auto val = *reinterpret_cast<typename std::tuple_element<12,
						supported_type>::type *>(buf + pos);
					val >> out;
					pos += sizeof(val);
					break;
				}
				default:
					break;
				}
			}
			out << "}\r\n";
		}
	private:
		const char *to_string(log_level level)
		{
			switch (level)
			{
			case log_level::INFO:
				return "INFO";
			case log_level::WARN:
				return "WARN";
			case log_level::CRIT:
				return "CRIT";
			default:
				assert(false);
			}
			return "...";
		}
		template<typename T>
		void append_obj(T t)
		{
			check_buffer(sizeof(T));
			*reinterpret_cast<T*>(buffer()) = t;
			buf_used_ += sizeof(T);
		}
		void copy_str(const char *str)
		{
			std::size_t len = strlen(str)+1;// for \0
			check_buffer(len);
			memcpy(buffer(), str, len);
			buf_used_ += len;
		}
		void check_buffer(std::size_t size)
		{
			if (size + buf_used_ > buf_size_)
			{
				if (heap_buf_)
				{
					char *buf = new char[buf_size_ * 2];
					memcpy(buf, heap_buf_.get(), buf_size_);
					heap_buf_.reset(buf);
					buf_size_ *= 2;
				}
				else
				{
					heap_buf_.reset(new char[buf_size_ * 2]);
					memcpy(heap_buf_.get(), stack_buf_, buf_size_);
					buf_size_ *= 2;
				}
			}
		}
		char *buffer()
		{
			return heap_buf_ ? 
				(heap_buf_.get() + buf_used_) :
				(stack_buf_ + buf_used_);
		}
		const char *file_ = nullptr;
		log_level level_;
		std::size_t line_ = 0;
		std::unique_ptr<char[]> heap_buf_;
		std::size_t buf_used_ = 0;
		std::size_t buf_size_ ;
		char stack_buf_[256 - 
			sizeof(log_level) -
			sizeof(std::size_t) *3 -
			sizeof(decltype(file_)) - 
			sizeof(decltype(heap_buf_))];
	};
	
	class xlog
	{
	public:
		
		static xlog &get_inst()
		{
			static xlog inst;
			return inst;
		}
		~xlog()
		{
			stop_ = true;
			worker_.join();
		}
		void operator <= (item &i)
		{
			if (!is_open_)
				open_log();
			push(std::move(i));
		}
		xlog &set_file(const std::string &filepath)
		{
			filepath_ = filepath;
			open_log();
			return *this;
		}
		xlog &set_level(log_level level)
		{
			level_ = level;
			return *this;
		}
		log_level get_level()
		{
			return level_;
		}
	private:
		xlog()
			:worker_(&xlog::write_log, this)
		{

		}
		void open_log()
		{
			std::lock_guard<std::mutex> lock(mtx_);
			if (is_open_)
				return;
			log_file_.open(filepath_.c_str(), 
				std::ostream::binary | std::ostream::trunc);
			if (log_file_)
				is_open_ = true;
			cv_.notify_one();
		}
		void write_log()
		{
			std::unique_lock<std::mutex>lock(mtx_);
			cv_.wait(lock, [this] { return !!is_open_; });

			while(!stop_)
			{
				item i;
				if (!try_pop(i))
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
				i >> log_file_;
			}
		}
		struct spinlock
		{
			spinlock(std::atomic_flag & flag) : flag_(flag)
			{
				while (flag_.test_and_set(std::memory_order_acquire));
			}
			~spinlock()
			{
				flag_.clear(std::memory_order_release);
			}
		private:
			std::atomic_flag & flag_;
		};
		bool try_pop(item &i)
		{
			spinlock lock(flag_);
			if (items_.empty())
				return false;
			i = std::move(items_.front());
			items_.pop();
			return true;
		}
		void push(item &&i)
		{
			spinlock lock(flag_);
			items_.emplace(std::move(i));
		}
		std::atomic_flag flag_= ATOMIC_FLAG_INIT;
		std::condition_variable cv_;
		std::mutex mtx_;
		std::atomic_bool is_open_ = false;
		std::atomic<log_level> level_ = log_level::INFO;
		std::string filepath_ = "xlog.log";
		std::ofstream log_file_;
		std::queue<item> items_;
		std::atomic_bool stop_ = false;
		std::thread worker_;
	};
}

#define XLOG_INFO \
if(xlog::xlog::get_inst().get_level() <= xlog::log_level::INFO)\
	xlog::xlog::get_inst()<=xlog::item(xlog::log_level::INFO, __FILE__, __LINE__)

#define XLOG_WARN \
if(xlog::xlog::get_inst().get_level() <= xlog::log_level::WARN)\
	xlog::xlog::get_inst()<=xlog::item(xlog::log_level::INFO, __FILE__, __LINE__)

#define XLOG_CRIT \
if(xlog::xlog::get_inst().get_level() <= xlog::log_level::CRIT)\
	xlog::xlog::get_inst()<=xlog::item(xlog::log_level::INFO, __FILE__, __LINE__)

#define XLOG_OPEN(path,level) xlog::xlog::get_inst().set_file((path)).set_level((level))