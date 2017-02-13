#pragma once
namespace xhttper
{
	class mime_parser
	{
	public:
		using data_callback_t = std::function<void(std::string &&)>;
		using header_callback_t = std::function<void(const std::string &,const std::string &)>;
		using end_callback_t = std::function<void()>;
		
		struct disposition
		{
			std::string name_;
			std::unique_ptr<std::string> filename_;
		};
		enum state
		{
			e_boundary,
			e_headers,
			e_data,
			e_end_cr1,
			e_end_cf1,
		};
		mime_parser()
		{

		}
		bool do_parse(char *&data, std::size_t &len)
		{
			while (len)
			{
				switch (state_)
				{
				case state::e_boundary:
					try
					{
						if (!do_parse_boundary(data, len))
							return false;
					}
					catch (const std::exception&)
					{
						if (!check_boundary_end())
							throw;
					}
					state_ = state::e_headers;
					break;
				case state::e_headers:
					if (len && !do_parse_headers(data, len))
						return false;
					state_ = state::e_data;
					break;
				case state::e_data:
					if (len && do_recv_data(data, len))
					{
						if (check_boundary_end())
						{
							state_ = state::e_end_cr1;
							break;
						}
						state_ = state::e_boundary;
					}
					else
					{
						return false;
					}
					break;
				case state::e_end_cr1:
					if (!len || !get_CR1(data, len))
						return false;
					state_ = state::e_end_cf1;
					break;
				case state::e_end_cf1:
					if (!len || !get_CF1(data, len))
						return false;
					state_ = state::e_boundary;
					return true;
				default:
					break;
				}
			}
			return  false;
		}
		void set_boundary(const std::string &boundary)
		{
			boundary_.append("--");
			boundary_.append(boundary);
			boundary_.append("\r\n");
			boundary_end_.append("--");
			boundary_end_.append(boundary);
			boundary_end_.append("--");
		}
		void regist_end_callback(const end_callback_t &callback)
		{
			end_callback_ = callback;
		}
		void regist_data_callback(const data_callback_t &callback)
		{
			data_callback_ = callback;
		}
		void regist_header_callback(const header_callback_t &callback)
		{
			header_callback_ = callback;
		}
	private:
		struct boundary_not_found:std::exception 
		{
			virtual char const* what() const noexcept override
			{
				return nullptr;
			}

		};
		enum recv_data_state
		{
			e_recv_data,
			e_check_boundary,
		};
		bool do_recv_data(char *&data, std::size_t &len)
		{
			while (len)
			{
				switch (recv_data_state_)
				{
				case recv_data_state::e_recv_data:
					if (len && get_string<'-'>(buffer_, data, len))
					{
						if (buffer_.size() > 1 &&
							buffer_[buffer_.size() - 2] == '\r' &&
							buffer_[buffer_.size() - 1] == '\n')
						{
							boundary_buffer_.clear();
							recv_data_state_ = recv_data_state::e_check_boundary;
							break;
						}
						else if(len)
						{
							buffer_.push_back(data[0]);
							data += 1;
							len -= 1;
						}
					}
					if (buffer_.size() > 1024)
						data_callback();
					break;
				case recv_data_state::e_check_boundary:
					try
					{
						if (len && do_parse_boundary(data, len))
						{
							buffer_.pop_back();
							buffer_.pop_back();
							data_callback();
							recv_data_state_ = recv_data_state::e_recv_data;
							return true;
						}
					}
					catch (const boundary_not_found&)
					{
						if (check_boundary_end())
							return true;
						buffer_.append(boundary_buffer_);
						data_callback();
						recv_data_state_ = e_recv_data;
					}
					break;
				default:
					break;
				}
			}
			return false;
		}
		enum header_state
		{
			e_name,
			e_colon,
			e_name_data,
			e_cr1,
			e_cf1,
			e_check_end,
			e_cr2,
			e_cf2
		};
		bool do_parse_headers(char *&data, std::size_t &len)
		{
			while (len)
			{
				switch (header_state_)
				{
				case header_state::e_name:
					if (len && !do_parse_header_name(data, len))
						return false;
					header_state_ = header_state::e_colon;
					break;
				case header_state::e_colon:
					if (!get_char<':'>(data, len))
						return false;
					header_state_ = header_state::e_name_data;
					break;
				case header_state::e_name_data:
					if (len && !do_parse_header_data(data, len))
						return false;
					header_state_ = header_state::e_cr1;
					break;
				case header_state::e_cr1:
					if (len && !get_CR1(data, len))
						return false;
					header_state_ = header_state::e_cf1;
					break;
				case header_state::e_cf1:
					if (len && !get_CF1(data, len))
						return false;
					header_state_ = header_state::e_check_end;
					mime_header_callback();
					break;
				case header_state::e_check_end:
					if (len && check_headers_end(data))
						header_state_ = e_cr2;
					else if (len)
						header_state_ = e_name;
					break;
				case header_state::e_cr2:
					if (len && !get_CR2(data, len))
						return false;
					header_state_ = e_cf2;
					break;
				case header_state::e_cf2:
					if (len && !get_CF2(data, len))
						return false;
					header_state_ = e_name;
					end_callback_();
					return true;
					break;
				default:
					break;
				}
			} 
			return false;
		}
		bool get_CR1(char *&data, std::size_t &len)
		{
			return get_char<'\r'>(data, len);
		}
		bool get_CR2(char *&data, std::size_t &len)
		{
			return get_char<'\r'>(data, len);
		}
		bool get_CF1(char *&data, std::size_t &len)
		{
			return get_char<'\n'>(data, len);
		}
		bool get_CF2(char *&data, std::size_t &len)
		{
			return get_char<'\n'>(data, len);
		}
		bool check_headers_end(char *data)
		{
			return data[0] == '\r';
		}
		template<char Char>
		bool get_char(char *&data, std::size_t &len)
		{
			char ch = data[0];
			data += 1;
			len -= 1;
			return ch == Char;
		}
		template<char terminater>
		bool get_string(std::string &buffer, char *&data, std::size_t &len)
		{
			std::size_t i = 0;
			while (data[i] != terminater && i < len)
			{
				buffer.push_back(data[i]);
				++i;
			}
			auto ch = data[i];
			len -= i;
			data += i;
			return ch == terminater;
		}
		bool do_parse_header_data(char *&data, std::size_t &len)
		{
			return  get_string<'\r'>(header_data_, data, len);
		}
		bool do_parse_header_name(char *&data, std::size_t &len)
		{
			return get_string<':'>(header_name_, data, len);
		}
		bool get_string(std::string &buffer, std::size_t length, char *&data, std::size_t &len)
		{
			if (buffer.size() < length)
			{
				auto min_len = std::min<std::size_t>(length - buffer.size(), len);
				buffer.append(data, min_len);
				data += min_len;
				len -= min_len;
				return buffer.size() == length;
			}
			return true;
		}
		bool do_parse_boundary(char *&data, std::size_t &len)
		{
			if (get_string(boundary_buffer_, boundary_.size(), data, len))
			{
				if (boundary_buffer_ == boundary_)
					return true;
				throw boundary_not_found();
			}
			return false;
		}
		bool check_boundary_end()
		{
			if (boundary_buffer_ == boundary_end_)
				return true;
			return false;
		}
		void mime_header_callback()
		{
			header_callback_(header_name_, header_data_);
			reset();
		}
		void data_callback()
		{
			data_callback_(std::move(buffer_));
			buffer_.clear();
		}
		void reset()
		{
			header_data_.clear();
			header_name_.clear();
			cr1_done_ = false;
			cr2_done_ = false;
			cf1_done_ = false;
			cf2_done_ = false;
		}
		end_callback_t end_callback_;
		data_callback_t data_callback_;
		header_callback_t header_callback_;
		
		recv_data_state recv_data_state_ = e_recv_data;
		header_state header_state_ = e_name;
		state state_ = e_boundary;

		bool boundary_done_ = false;
		std::string buffer_;

		bool cr1_done_ = false;
		bool cf1_done_ = false;
		bool cr2_done_ = false;
		bool cf2_done_ = false;
		std::string header_name_;
		std::string header_data_;
		std::string boundary_;
		std::string boundary_end_;
		std::string boundary_buffer_;
	};

}
