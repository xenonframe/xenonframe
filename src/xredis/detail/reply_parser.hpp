#pragma once

namespace xredis
{
	namespace detail
	{
		class reply_parser
		{
		private:
			struct callback
			{
				enum type_t
				{
					e_null,
					e_cluster_slots_cb,
					e_str_map_cb,
					e_integral_cb,
					e_bulk_cb,
					e_array_string_cb
				} type_;
				union
				{
					cluster_slots_callback *cluster_slots_cb_;
					string_map_callback *str_map_cb_;
					array_string_callback *array_string_cb_;
					string_callback *bulk_cb_;
					integral_callback *integral_cb_;
				};
				callback() = default;
				~callback()
				{
					if (type_ == e_str_map_cb)
						delete str_map_cb_;
					else if (type_ == e_bulk_cb)
						delete bulk_cb_;
					else if (type_ == e_integral_cb)
						delete integral_cb_;
					else if (type_ == e_array_string_cb)
						delete array_string_cb_;
					else if (type_ == e_cluster_slots_cb)
						delete cluster_slots_cb_;
				}
				void close(std::string &&error_code)
				{
					switch (type_)
					{
					case e_cluster_slots_cb:
						(*cluster_slots_cb_)(std::move(error_code), {});
						break;
					case e_str_map_cb:
						(*str_map_cb_)(std::move(error_code), {});
						break;
					case e_integral_cb:
						(*integral_cb_)(std::move(error_code), {});
						break;
					case e_bulk_cb:
						(*bulk_cb_)(std::move(error_code), {});
						break;
					default:
						break;
					}
				}
				callback(callback && self)
				{
					if (this != &self)
					{
						memcpy(this, &self, sizeof(callback));
						self.type_ = e_null;
					}
				}
				callback &operator =(callback &&self)
				{
					if (this != &self)
					{
						memcpy(this, &self, sizeof(callback));
						self.type_ = e_null;
					}
					return *this;
				}
				callback(const callback &) = delete;
				callback &operator =(const callback &) = delete;
			};
			struct obj
			{
				obj() {}
				~obj() 
				{
					if(type_ == obj::e_array)
					{
						for(auto &itr : *array_)
							delete itr;
						delete array_;
					}
					else if(type_ == obj::e_bulk)
						delete str_;
				}
				enum type_t
				{
					e_new,
					e_bulk,//
					e_null,
					e_integal,
					e_array,
					e_error,//status error
					e_ok,//status ok
				} type_ = e_new;
				union 
				{
					std::string *str_;
					std::vector<obj*> *array_;
					int32_t integral_;
				};
				/*
				* for Bulk Strings length
				*     Arrays elements 
				*/
				int len_ = 0;
			};
			struct task
			{
				task() {}
				~task() {}
				typedef std::function<bool()> parse_func;
				std::vector<parse_func> parser_funcs_;
				obj obj_;
				void reset()
				{
					if (obj_.type_ == obj::e_array)
					{
						for (auto &itr : *obj_.array_)
							delete itr;
						delete obj_.array_;
					}
					else if (obj_.type_ == obj::e_bulk)
						delete obj_.str_;
					memset(&obj_, 0, sizeof obj_);
				}
			};
			
		public:
			reply_parser()
			{

			}
			void parse(const char *data, std::size_t len)
			{
				data_.append(data, len);
				run();
			}
			void close(std::string &&error_code)
			{
				for (auto &itr: callbacks_)
				{
					itr.close(std::move(std::string(error_code)));
				}
				callbacks_.clear();
			}
			template<typename CB>
			void regist_callback(CB &&cb)
			{
				append_cb(std::move(cb));
			}
			template<typename CB>
			void regist_moved_error_callback(CB &&cb)
			{
				moved_errro_callback_ = std::move(cb);
			}
		private:
			void run()
			{
				do 
				{
					if (task_.parser_funcs_.empty())
					{
						if(prepare(task_.obj_) == false)
							return;
					}
					while (task_.parser_funcs_.size())
					{
						auto func = task_.parser_funcs_.back();
						task_.parser_funcs_.pop_back();
						if (func() == false)
						{
							task_.parser_funcs_.emplace_back(func);
							return;
						}
					}
					if (is_moved_error_)
					{
						moved_errro_callback_(moved_error_);
						return;
					}
				} while (true);
				
			}
			bool prepare(obj &o)
			{
				if(pos_ >= data_.size())
					return false;// no data

				if(task_.parser_funcs_.empty())
					task_.parser_funcs_.emplace_back(
					std::bind(&reply_parser::parser_done, this));

				char ch = data_[pos_];
				switch (ch)
				{
				case ':':
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_integal,this, std::ref(o)));
					break;
				case '$':
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_bulk, this, std::ref(o)));
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_len, this, std::ref(o)));
					break;
				case '*':
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_array, this, std::ref(o)));
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_len, this, std::ref(o)));
					break;
				case '-':
					task_.obj_.type_ = obj::e_error;
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_str, this, std::ref(o)));
					break;
				case '+':
					task_.obj_.type_ = obj::e_ok;
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_str, this, std::ref(o)));
					break;
				}
				++pos_;
				return true;
			}
			bool get_str(obj &o)
			{
				if (!o.str_)
					o.str_ = new std::string;

				while (pos_ < data_.size())
				{
					char ch = data_[pos_];
					if (ch != '\r')
						o.str_->push_back(ch);
					else if (data_[pos_ + 1] == '\n')
					{
						pos_ += 2;
						return true;
					}
					++pos_;
				}
				return false;
			}
			bool get_bulk(obj &o)
			{
				if(pos_ + o.len_ > data_.size())
					return false;
				o.str_ = new std::string(data_.data() + pos_, o.len_);
				pos_ += o.len_ + 2;//\r\n
				o.type_ = obj::e_bulk;
				return true;
			}
			bool get_len(obj &o)
			{
				set_roll_back();
				std::string buf;
				while (pos_ < data_.size())
				{
					char ch = data_[pos_];
					if (ch != '\r')
						buf.push_back(ch);
					else if (data_[pos_ + 1] == '\n')
					{
						pos_ += 2;
						o.len_ = std::strtol(buf.c_str(), 0, 10);
						if (errno == ERANGE)
							assert(false);
						return true;
					}
					++pos_;
				}
				roll_back();
				return false;
			}
			bool get_integal(obj &o)
			{
				set_roll_back();
				std::string buf;
				while (pos_ < (int)data_.size())
				{
					char ch = data_[pos_];
					if (ch != '\r')
						buf.push_back(ch);
					else if (data_[pos_ + 1] == '\n')
					{
						pos_ += 2;
						o.integral_ = std::strtol(buf.c_str(), 0, 10);
						if (errno == ERANGE)
							assert(__FILE__ == 0);
						o.type_ = obj::e_integal;
						return true;
					}
					++pos_;
				}
				roll_back();
				return false;
			}
			bool get_array(obj &o)
			{
				int elememts = o.len_;
				if (elememts == -1)
				{
					o.type_ = obj::e_null;
					return true;
				}
				o.array_ = new std::vector<obj*>;
				o.type_ = obj::e_array;
				for (int i = 0; i < elememts; ++i)
				{
					obj *tmp = new obj;
					o.array_->push_back(tmp);
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::prepare, this, std::ref(*tmp)));
				}
				o.type_ = obj::e_array;
				return true;
			}
			bool reset_task()
			{
				if (pos_ == data_.size())
				{
					data_.clear();
				}
				else
				{
					data_ = data_.substr(pos_, data_.size() - pos_);
				}
				pos_ = 0;
				task_.reset();
				return true;
			}
			void set_roll_back()
			{
				last_pos_ = pos_;
			}
			void roll_back()
			{
				pos_ = last_pos_;
			}

			bool parser_done()
			{
				assert(callbacks_.size());
				callback cb = std::move(callbacks_.front());
				callbacks_.pop_front();
				switch (cb.type_)
				{
				case callback::e_bulk_cb:
					string_cb(cb);
					break;
				case callback::e_integral_cb:
					integral_cb(cb);
					break;
				case callback::e_str_map_cb:
					str_map_cb(cb);
					break;
				case callback::e_cluster_slots_cb:
					cluster_slots_cb(cb);
					break;
				default:
					break;
				}
				reset_task();
				return true;
			}
			void cluster_slots_cb(const callback &cb)
			{
				obj &o = task_.obj_;
				if (o.type_ == obj::e_null)
				{
					(*cb.cluster_slots_cb_)("null", {});
				}
				else if (o.type_ == obj::e_error)
				{
					check_moved_error();
					(*cb.cluster_slots_cb_)(std::move(*o.str_), {});
				}
				else if (task_.obj_.type_ != obj::e_array)
				{
					(*cb.cluster_slots_cb_)("error", {});
				}
				else
				{
					cluster_slots slots;
					for (std::size_t i = 0; i < o.array_->size(); i++)
					{
						obj &sub_ = *(*o.array_)[i];
						if (sub_.type_ != obj::e_array)
							(*cb.cluster_slots_cb_)("error", {});

						cluster_slots::cluster_info info;
						std::size_t size = sub_.array_->size();
						if (size < 3)
							goto error;
						info.min_slots_ = (*(*sub_.array_)[size - 1]).integral_;
						info.max_slots_ = (*(*sub_.array_)[size - 2]).integral_;
						for (int k = (int)size - 3; k >= 0;)
						{
							cluster_slots::cluster_info::node node;
							obj &n = *(*sub_.array_)[k--];
							if (n.type_ != obj::e_array)
								goto error;
							node.id_ = std::move(*(*(*n.array_)[0]).str_);
							node.port_ = (*(*n.array_)[1]).integral_;
							node.ip_ = std::move(*(*(*n.array_)[2]).str_);
							info.node_.emplace_back(std::move(node));
						}
						slots.cluster_.emplace_back(std::move(info));
					}
					(*cb.cluster_slots_cb_)({}, std::move(slots));
					return;
				error:
					(*cb.cluster_slots_cb_)("parser_error", {});
					return;
				}
			}
			void str_map_cb(const callback &cb)
			{
				obj &o = task_.obj_;
				if (o.type_ == obj::e_null)
				{
					(*cb.str_map_cb_)("null", {});
				}
				else if (o.type_ == obj::e_error)
				{
					check_moved_error();
					(*cb.str_map_cb_)(std::move(*(o.str_)), {});
					return;
				}
				else if (task_.obj_.type_ == obj::e_array)
				{
					std::map<std::string, std::string> result;
					assert(o.array_->size()%2 == 0);
					for (std::size_t i =0; i < o.array_->size();)
					{
						result.emplace(std::move(*(*o.array_)[i]->str_), 
									   std::move(*(*o.array_)[i+1]->str_));
						i += 2;
					}
				}
				
			}
			void string_cb(const callback &cb)
			{
				obj &o = task_.obj_;
				if (o.type_ == obj::e_null)
				{
					(*cb.bulk_cb_)("null", "");
				}
				else if(o.type_ == obj::e_error)
				{
					check_moved_error();
					(*cb.bulk_cb_)(std::move(*o.str_), "");
				}
				else
				{
					(*cb.bulk_cb_)("", std::move(*o.str_));
				}
			}
			void integral_cb(const callback &cb)
			{
				obj &o = task_.obj_;
				if (o.type_ == obj::e_null)
				{
					(*cb.bulk_cb_)("null", "");
				}
				else if (o.type_ == obj::e_error)
				{
					check_moved_error();
					(*cb.bulk_cb_)(std::move(*o.str_), 0);
				}
				else
				{
					(*cb.integral_cb_)("", std::move(o.integral_));
				}
			}
			void append_cb(integral_callback &&cb)
			{
				callback tmp;
				tmp.integral_cb_ = new integral_callback(std::move(cb));
				tmp.type_ = callback::e_integral_cb;
				callbacks_.emplace_back(std::move(tmp));
			}
			void append_cb(string_map_callback &&cb)
			{
				callback tmp;
				tmp.str_map_cb_= new string_map_callback(std::move(cb));
				tmp.type_ = callback::e_str_map_cb;
				callbacks_.emplace_back(std::move(tmp));
			}
			void append_cb(array_string_callback &&cb)
			{
				callback tmp;
				tmp.array_string_cb_= new array_string_callback(std::move(cb));
				tmp.type_ = callback::e_array_string_cb;
				callbacks_.emplace_back(std::move(tmp));
			}
			void append_cb(string_callback  &&cb)
			{
				callback tmp;
				tmp.bulk_cb_ = new string_callback(std::move(cb));
				tmp.type_ = callback::e_bulk_cb;
				callbacks_.emplace_back(std::move(tmp));
			}
			void append_cb(cluster_slots_callback &&cb)
			{
				callback tmp;
				tmp.cluster_slots_cb_= new cluster_slots_callback(std::move(cb));
				tmp.type_ = callback::e_cluster_slots_cb;
				callbacks_.emplace_back(std::move(tmp));
			}

			void check_moved_error()
			{
				if (task_.obj_.str_->find("MOVED") != std::string::npos)
				{
					moved_error_ = task_.obj_.str_->c_str();
					is_moved_error_ = true;
				}
			}
			std::string::size_type last_pos_ = 0;
			std::string::size_type pos_ = 0;
			std::string data_;
			task task_;
			std::list<callback> callbacks_;
			bool is_moved_error_ = false;
			std::string moved_error_;
			std::function<void(const std::string &)> moved_errro_callback_;
		};

	}
}
