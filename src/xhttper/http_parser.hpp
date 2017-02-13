#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
namespace xhttper
{
	
	struct parse_error : std::exception {};

	class http_parser
	{
	public:
		http_parser()
		{
		}
		void append(const char *data, std::size_t len)
		{
			if (len + pos_ < stack_size_ - 1)
			{
				std::memcpy(stack_buf_ + pos_, data, len);
				buf_ = stack_buf_;
				size_ += len;
				buf_[size_] = '\0';
				return;
			}
			if (heap_buf_ == nullptr)
			{
				heap_buf_.reset(new char[heap_size_ + 1]);
				std::memcpy(heap_buf_.get(), stack_buf_ ,size_);
			}
			if (heap_size_ < len + pos_)
			{
				while (heap_size_ < len + pos_)
					heap_size_ *= 2;
				auto buf = new char[heap_size_ + 1];
				std::memcpy(buf, buf_, size_);
				heap_buf_.reset(buf);
			}
			std::memcpy(heap_buf_.get() + pos_, data, len);
			buf_ = heap_buf_.get();
			size_ += len;
			buf_[size_] = '\0';
		}
		bool parse_req()
		{
			if (!parse_method())
				return false;
			if (!parse_path())
				return false;
			if (!parse_version())
				return false;
			if (!parse_headers())
				return false;
			return true;
		}
		
		bool parse_rsp()
		{
			if (!parse_version())
				return false;
			if (!parse_status())
				return false;
			if (!parse_status_str())
				return false;
			if (!parse_headers())
				return false;
			return true;
		}
		template<typename strncasecmper>
		std::string get_header(const char *header_name)
		{
			std::size_t len = strlen(header_name);
			for (auto &itr: headers_)
			{
				if (itr.first.len_ != (int)len)
					continue;
				if (strncasecmper()(itr.first.to_string(buf_).data(), header_name, len))
					return itr.second.to_string(buf_);
			}
			return {};
		}
		std::string  get_method()
		{
			return method_.to_string(buf_);
		}
		std::string get_version()
		{
			return version_.to_string(buf_);
		}
		std::string get_status()
		{
			return status_.to_string(buf_);
		}
		std::string get_status_str()
		{
			return status_str_.to_string(buf_);
		}
		std::string url()
		{
			return url_.to_string(buf_);
		}
		void reset()
		{
			size_ =  pos_ = 0;
			pos_ = 0;
			first_.reset();
			str_ref_.reset();
			headers_.clear();
			url_.reset();
			method_.reset();
			version_.reset();
			status_.reset();
			status_str_.reset();
		}
		std::string get_string()
		{
			return {buf_ + pos_, size_ - pos_};
		}
		std::string get_string(std::size_t len)
		{
			if (len > size_)
				throw std::out_of_range("string subscript out of range");
			std::string result(buf_+pos_, len);
			pos_ += len;
			return std::move(result);
		}
		std::size_t remain_len()
		{
			return size_ - pos_;
		}
	private:
		struct str_ref
		{
			str_ref() {}
			str_ref(const str_ref&self)
			{
				pos_ = self.pos_;
				len_ = self.len_;
			}
			void operator= (const str_ref&self)
			{
				pos_ = self.pos_;
				len_ = self.len_;
			}
			str_ref(str_ref &&self)
			{
				pos_ = self.pos_;
				len_ = self.len_;
				self.reset();
			}
			std::string to_string(const char* buf)
			{
				return std::string(buf + pos_, len_);
			}
			void set_end(std::size_t end)
			{
				len_ = (int)end - pos_;
			}
			void reset()
			{
				len_ = 0;
				pos_ = -1;
			}
			void set_pos(std::size_t pos)
			{
				if(pos_ == -1)
					pos_ = (int)pos;
			}
			int len()
			{
				return len_;
			}
			int pos_ = -1;
			int len_ = 0;
		};
		struct str_ref_cmp
		{
			bool operator ()(const str_ref &left,const str_ref &right)const
			{
				return left.len_ < right.len_;
			}
		};
		bool check_end()
		{
			if(curr() == '\r' && has_next())
				return  buf_[pos_-2] == '\r' &&
				        buf_[pos_-1] == '\n' &&
					    buf_[pos_]   == '\r' &&
					    buf_[pos_+1] == '\n' ;
			else if(curr() == '\n')
				return  buf_[pos_-3] == '\r' &&
						buf_[pos_-2] == '\n' &&
						buf_[pos_-1] == '\r' &&
						buf_[pos_]   == '\n' ;
			return false;
		}
		bool parse_headers()
		{
			while (true)
			{
				if (check_end())
				{
					if (curr() == '\r')
					{
						next();
						next();
						return true;
					}
					next();
					return true;
				}
				if (!first_.len_)
				{
					first_.set_pos(pos_);
					auto pos = find_pos(':');
					if (pos == -1)
						return false;
					first_.set_end(pos);
					next();
					continue;
				}
				if (!skip_space())
					return false;
				second_.set_pos(pos_);
				auto pos = find_pos('\n');
				if (pos == -1)
					return false;
				second_.set_end(pos - 1);
				headers_.emplace_back(std::move(first_), std::move(second_));
				next();
			}
		}
		bool parse_status()
		{
			if (status_.len_)
				return true;
			if (!skip_space())
				return false;
			status_.set_pos(pos_);
			auto pos = find_pos(' ');
			if (pos == -1)
				return false;
			status_.set_end(pos);
			return true;
		}
		bool parse_status_str()
		{
			if (status_str_.len_)
				return true;
			if (!skip_space())
				return false;
			auto pos = find_pos('\n');
			if (pos == -1)
				return false;
			status_str_.set_end(pos - 1);
			next();
			return true;
		}
		bool parse_version()
		{
			if (version_.len_)
				return true;
			version_.set_pos(pos_);
			auto pos = find_pos(' ');
			if (pos == -1)
				return false;
			version_.set_end(pos - 1);
			next();
			return true;
		}
		bool parse_path()
		{
			if (url_.len_)
				return true;
			url_.set_pos(pos_);
			auto pos = find_pos(' ');
			if (pos == -1)
				return false;
			url_.set_end(pos);
			next();
			return true;
		}
		bool skip_space()
		{
			while (buf_[pos_] == ' ')
				pos_++;
			return pos_ < size_;
		}
		bool parse_method()
		{
			if (method_.len())
				return true;
			method_.set_pos(pos_);
			auto pos = find_pos(' ');
			if (pos == -1)
				return false;
			method_.set_end(pos);
			next();
			return true;
		}
		int find_pos(char end)
		{
			while (has_next() && curr() != end)
				next();
			if (!has_next())
				return -1;
			return (int)pos_;
		}
		bool has_next()
		{
			return pos_ < size_;
		}
		char curr()
		{
			return buf_[pos_];
		}
		char look_ahead(int index)
		{
			if (pos_ + index >= size_)
				return '\0';
			return buf_[pos_+index];
		}
		void next()
		{
			++pos_;
		}

		str_ref  status_;
		str_ref status_str_;

		str_ref str_ref_;
		str_ref first_;
		str_ref second_;

		str_ref method_ ;
		str_ref url_;
		str_ref version_ ;

		std::size_t last_pos_ = 0;
		std::size_t pos_= 0;
		std::size_t size_ = 0;
		constexpr static int stack_size_ = 1024;
		std::size_t heap_size_ = stack_size_ *2;
		std::unique_ptr<char[]> heap_buf_;
		char stack_buf_[stack_size_];
		char *buf_;
		std::vector<std::pair<str_ref, str_ref>> headers_;
	};
}
