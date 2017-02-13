#pragma once
#include <map>
#include <string>
#include <cassert>
#include <iostream>
#include <vector>
#include <list>
#include <ctype.h>
#include <stdlib.h>
#include <initializer_list>
#include <memory>

namespace xjson
{

	#define MARCO_EXPAND(...)                 __VA_ARGS__
	#define MAKE_ARG_LIST_1(op, arg, ...)   op(arg)
	#define MAKE_ARG_LIST_2(op, arg, ...)   op(arg) MARCO_EXPAND(MAKE_ARG_LIST_1(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_3(op, arg, ...)   op(arg) MARCO_EXPAND(MAKE_ARG_LIST_2(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_4(op, arg, ...)   op(arg) MARCO_EXPAND(MAKE_ARG_LIST_3(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_5(op, arg, ...)   op(arg) MARCO_EXPAND(MAKE_ARG_LIST_4(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_6(op, arg, ...)   op(arg) MARCO_EXPAND(MAKE_ARG_LIST_5(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_7(op, arg, ...)   op(arg) MARCO_EXPAND(MAKE_ARG_LIST_6(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_8(op, arg, ...)   op(arg) MARCO_EXPAND(MAKE_ARG_LIST_7(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_9(op, arg, ...)   op(arg) MARCO_EXPAND(MAKE_ARG_LIST_8(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_10(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_9(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_11(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_10(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_12(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_11(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_13(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_12(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_14(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_13(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_15(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_14(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_16(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_15(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_17(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_16(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_18(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_17(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_19(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_18(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_20(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_19(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_21(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_20(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_22(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_21(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_23(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_22(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_24(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_23(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_25(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_24(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_26(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_25(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_27(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_26(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_28(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_27(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_29(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_28(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_30(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_29(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_31(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_30(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_32(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_31(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_33(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_32(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_34(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_33(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_35(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_34(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_36(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_35(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_37(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_36(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_38(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_37(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_39(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_38(op, __VA_ARGS__))
	#define MAKE_ARG_LIST_40(op, arg, ...)  op(arg) MARCO_EXPAND(MAKE_ARG_LIST_39(op, __VA_ARGS__))

	#define XPACK_OBJECT(Obj) o[#Obj]=Obj;
	#define XUNPACK_OBJECT(Obj) Obj = o[#Obj].get<decltype(Obj)>();

	#define GEN_XPACK_OBJECT(...) void xpack (xjson::obj_t &o) const { __VA_ARGS__; }
	#define GEN_XUNPACK_OBJECT(...) void xunpack(xjson::obj_t &o) { __VA_ARGS__; }

	#define MACRO_CONCAT(A, B)  A##_##B

	#define MAKE_ARG_LIST(N, op, arg, ...) \
			MACRO_CONCAT(MAKE_ARG_LIST, N)(op, arg, __VA_ARGS__)

	#define GEN_XPACK_FUNC(N, ...) \
	GEN_XPACK_OBJECT(MAKE_ARG_LIST(N, XPACK_OBJECT, __VA_ARGS__)) 

	#define GEN_XUNPACK_FUNC(N, ...) \
	GEN_XUNPACK_OBJECT(MAKE_ARG_LIST(N, XUNPACK_OBJECT, __VA_ARGS__)) 

	#define RSEQ_N() \
			 119,118,117,116,115,114,113,112,111,110,\
			 109,108,107,106,105,104,103,102,101,100,\
			 99,98,97,96,95,94,93,92,91,90, \
			 89,88,87,86,85,84,83,82,81,80, \
			 79,78,77,76,75,74,73,72,71,70, \
			 69,68,67,66,65,64,63,62,61,60, \
			 59,58,57,56,55,54,53,52,51,50, \
			 49,48,47,46,45,44,43,42,41,40, \
			 39,38,37,36,35,34,33,32,31,30, \
			 29,28,27,26,25,24,23,22,21,20, \
			 19,18,17,16,15,14,13,12,11,10, \
			 9,8,7,6,5,4,3,2,1,0

	#define ARG_N( \
			 _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
			 _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
			 _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
			 _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
			 _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
			 _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
			 _61,_62,_63,_64,_65,_66,_67,_68,_69,_70, \
			 _71,_72,_73,_74,_75,_76,_77,_78,_79,_80, \
			 _81,_82,_83,_84,_85,_86,_87,_88,_89,_90, \
			 _91,_92,_93,_94,_95,_96,_97,_98,_99,_100, \
			 _101,_102,_103,_104,_105,_106,_107,_108,_109,_110, \
			 _111,_112,_113,_114,_115,_116,_117,_118,_119,N, ...) N

	#define GET_ARG_COUNT_INNER(...)    MARCO_EXPAND(ARG_N(__VA_ARGS__))
	#define GET_ARG_COUNT(...)          GET_ARG_COUNT_INNER(__VA_ARGS__, RSEQ_N())

	#define XPACK(...) GEN_XPACK_FUNC(GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
	#define XUNPACK(...) GEN_XUNPACK_FUNC(GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

	#define  XGSON(...) XPACK(__VA_ARGS__) XUNPACK(__VA_ARGS__)

	#define xjson_assert(x)\
	if (!(x))\
	{\
		std::string buffer; \
		buffer.append("FILE:");\
		buffer.append(__FILE__);\
		buffer.append(" LINE:");\
		buffer.append(std::to_string(__LINE__ - 6));\
		buffer.append(" ");\
		buffer.append(#x);\
		throw std::runtime_error(buffer);\
	}
	
	struct null
	{
	};

	template<typename T>
	struct optional
	{
		using type = T;

		optional() = default;
		optional(const T &t)
		{
			val_ = t;
		}
		operator bool()
		{
			return valid_;
		}
		bool operator !()
		{
			return !valid_;
		}

		const T &get() const
		{
			return val_;
		}
		void operator=(const T &val)
		{
			valid_ = true;
			val_ = val;
		}
		optional &operator = (optional <T> &&val)
		{
			if (val.valid_)
			{
				val_ = std::move(val.val_);
				valid_ = true;
			}
			return  *this;
		}
		optional &operator = (const optional &val)
		{
			if (val.valid_)
			{
				val_ = val.val_;
				valid_ = true;
			}
			return *this;
		}
		optional(const optional & val)
		{
			if (val.valid_)
			{
				val_ = val.val_;
				valid_ = true;
			}
		}
	private:
		bool valid_ = false;
		T val_;
	};
	template<typename T>
	struct is_optional :std::false_type {};

	template<typename T>
	struct is_optional<optional<T>> :std::true_type {};

	struct obj_t
	{
		enum type_t
		{
			e_null,
			e_num,
			e_str,
			e_bool,
			e_float,
			e_obj,
			e_vec,
		};
		typedef std::map<std::string, obj_t> map_t;
		typedef std::vector<obj_t> vec_t;

		struct value_t
		{
			type_t type_ = e_null;
			union 
			{
				std::string *str_;
				int64_t num_ = 0;
				double double_;
				bool bool_;
				map_t *obj_;
				vec_t *vec_;
			};
			~value_t()
			{
				switch (type_)
				{
				case e_null:
				case e_num:
				case e_bool:
				case e_float:
					break;
				case e_str:
					delete str_;
					break;
				case e_obj:
					delete obj_;
					break;
				case e_vec:
					delete vec_;
					break;
				default:
					break;
				}
				type_ = e_null;
			}
		};
		std::shared_ptr<value_t> val_;

		obj_t()
			:val_(new value_t)
		{

		}
		template<typename T,typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		obj_t(T val)
			:obj_t()
		{
			*this = val;
		}
		explicit obj_t(bool val)
			:obj_t()
		{
			*this = val;
		}
		obj_t(const std::string &val)
			:obj_t()
		{
			*this = val;
		}
		obj_t(std::string &&val)
			:obj_t()
		{
			*this = std::move(val);
		}
		obj_t(const char *val)
			:obj_t()
		{
			*this = val;
		}
		

		obj_t(const std::map<std::string,obj_t> &objs)
			:obj_t()
		{
			for (auto &itr: objs)
			{
				operator[](itr.first) = itr.second;
			}
		}
		obj_t(const obj_t & self)
		{
			val_ = self.val_;
		}

		obj_t(obj_t && self)
		{
			val_ = self.val_;
		}
		obj_t &operator = (const obj_t & self)
		{
			val_ = self.val_;
			return *this;
		}
		obj_t &operator=(nullptr_t)
		{
			reset();
			return *this;
		}
		obj_t &operator=(const char *val)
		{
			reset();
			val_->type_ = e_str;
			val_->str_ = new std::string(val);
			return *this;
		}

		obj_t &operator =(const std::string &val)
		{
			reset();
			val_->type_ = e_str;
			val_->str_ = new std::string(val);
			return *this;
		}
		template<typename T>
		typename std::enable_if<std::is_integral<T>::value, obj_t &>::type
			operator=(T val)
		{
			reset();
			val_->type_ = e_num;
			val_->num_ = val;
			return *this;
		}

		obj_t &operator =(bool val)
		{
			reset();
			val_->type_ = e_bool;
			val_->bool_ = val;
			return *this;
		}
		template<typename T,class = decltype(&T::xpack)>
		obj_t & operator = (const T &o)
		{
			o.xpack(*this);
			return *this;
		}

		template<typename T>
		obj_t & operator = (const std::vector<T> &o)
		{
			if (val_->type_ != e_vec)
			{
				reset();
				val_->type_ = e_vec;
				val_->vec_ = new vec_t;
			}
			for (auto &itr: o)
			{
				add(itr);
			}
			return *this;
		}
		template<typename T>
		obj_t & operator = (const std::list<T> &o)
		{
			if (val_->type_ != e_vec)
			{
				reset();
				val_->type_ = e_vec;
				val_->vec_ = new vec_t;
			}
			for (auto &itr : o)
			{
				add(itr);
			}
			return *this;
		}
		template<typename T>
		obj_t &operator=(const std::map<std::string, T> & vals)
		{
			if (val_->type_ != e_obj)
			{
				reset();
				val_->type_ = e_obj;
				val_->obj_ = new map_t;
			}
			for (auto &itr: vals)
			{
				operator[](itr.first) = itr.second;
			}
			return *this;
		}
		template<typename T>
		obj_t &operator=(optional<T> && val)
		{
			*this = std::move(val.get());
			return *this;
		}
		template<typename T>
		obj_t &operator=(const optional<T> & val)
		{
			*this = val.get();
			return *this;
		}
		template<typename T>
		obj_t &operator= (std::initializer_list<T> vals)
		{
			for (auto &itr : vals)
			{
				add(itr);
			}
			return *this;
		}
		template<typename T>
		obj_t &operator=(std::initializer_list<std::pair<std::string, T>> &&vals)
		{
			for (auto &itr : vals)
			{
				operator[](itr.first) = itr.second;
			}
			return *this;
		}
		template<typename T>
		typename std::enable_if<std::is_floating_point<T>::value, obj_t &>::type
			operator=(T val)
		{
			reset();
			val_->type_ = e_float;
			val_->double_ = val;
			return *this;
		}
		
		void reset()
		{
			switch (val_->type_)
			{
			case e_null:
			case e_num:
			case e_bool:
			case e_float:
				break;
			case e_str:
				delete val_->str_;
				break;
			case e_obj:
				delete val_->obj_;
				break;
			case e_vec:
				delete val_->vec_;
				break;
			default:
				break;
			}
			val_->type_ = e_null;
		}
		
		
		obj_t &operator[](const std::string & key)
		{
			return operator[](key.c_str());
		}

		obj_t &operator[](const char *key)
		{
			if (val_->type_ != e_obj)
			{
				reset();
				val_->type_ = e_obj;
				val_->obj_ = new map_t;
			}
			auto itr = val_->obj_->find(key);
			if (itr != val_->obj_->end())
				return itr->second;
			return (*val_->obj_)[key];
		}
		obj_t &make_vec()
		{
			if(val_->type_ != e_vec)
			{
				reset();
				val_->type_ = e_vec;
				val_->vec_ = new vec_t;
			}
			return *this;
		}
		template<typename T>
		obj_t& add(const T &val)
		{
			if (val_->type_ != e_vec)
			{
				reset();
				val_->type_ = e_vec;
				val_->vec_ = new vec_t;
			}
			obj_t o;
			o = val;
			val_->vec_->push_back(o);
			return *this;
		}

		obj_t& add(obj_t &&obj)
		{
			if (val_->type_ != e_vec)
			{
				reset();
				val_->type_ = e_vec;
				val_->vec_ = new vec_t;
			}
			val_->vec_->push_back(std::move(obj)); 
			return *this;
		}
		obj_t &add(nullptr_t )
		{
			if (val_->type_ != e_vec)
			{
				reset();
				val_->type_ = e_vec;
				val_->vec_ = new vec_t;
			}
			
			val_->vec_->emplace_back();
			return *this;
		}
		template<class T>
		typename std::enable_if<std::is_integral<T>::value &&
			!std::is_same<T, bool>::value, T>::type
			get() const
		{
			xjson_assert(val_->type_ == e_num);
			return static_cast<T>(val_->num_);
		}
		template<class T>
		typename std::enable_if<std::is_same<T, bool>::value, T>::type
			get() const
		{
			xjson_assert(val_->type_ == e_bool);
			return val_->bool_;
		}
		template<class T>
		typename std::enable_if<std::is_floating_point<T>::value, T>::type
			get() const
		{
			xjson_assert(val_->type_ == e_float);
			return static_cast<T>(val_->double_);
		}
		template<class T>
		typename std::enable_if<std::is_same<T, std::string>::value, std::string &>::type
			get() const
		{
			xjson_assert(val_->type_ == e_str);
			return *val_->str_;
		}
		template<typename T,class = decltype(&T::xunpack)>
		T get()
		{
			xjson_assert(val_->type_ == e_obj);
			T t;
			t.xunpack(*this);
			return std::move(t);
		}
		template<class T>
		typename std::enable_if<is_optional<T>::value, T>::type
			get()
		{
			typename T::type val;
			try
			{
				val = get<T::type>();
			}
			catch (...)
			{
				return T();
			}
			return T(val);
		}
		
		template<typename Container, typename value_type = typename Container::value_type>
		typename std::enable_if<std::is_same<std::vector<value_type>, Container>::value || 
			std::is_same<std::list<value_type>, Container>::value, Container>::type
			get()
		{
			Container container;
			xjson_assert(val_->type_ == e_vec);
			for (auto &itr : *val_->vec_)
				container.emplace_back(itr.get<value_type>());
			return container;
		}

		template<typename Container,
			typename key_type = typename Container::key_type,
			typename mapped_type = typename Container::mapped_type>
			inline typename std::enable_if<std::is_same<std::map<key_type, mapped_type>, Container>::value,Container>::type
			get()
		{
			Container container;
			xjson_assert(val_->type_ == e_obj);
			for (auto &itr : *val_->obj_)
				container.emplace(itr.first, itr.second.get<mapped_type>());
			return container;
		}

		template<typename T>
		T get(std::size_t idx) const
		{
			xjson_assert(val_->type_ == e_vec);
			xjson_assert(val_->vec_);
			xjson_assert(idx < val_->vec_->size());
			return ((*val_->vec_)[idx]).get<T>();
		}
		obj_t &get(std::size_t idx) const 
		{
			xjson_assert(val_->type_ == e_vec);
			xjson_assert(val_->vec_);
			xjson_assert(idx < val_->vec_->size());
			return ((*val_->vec_)[idx]);
		}
		bool is_null() const
		{
			return val_->type_ == e_null;
		}
		std::size_t size() const 
		{
			xjson_assert(val_->type_ == e_vec);
			return val_->vec_->size();
		}
		type_t type() const
		{
			return val_->type_;
		}
		std::string encode_str(const std::string &str)const
		{
			std::string buffer;
			buffer.reserve(str.size());

			for (auto &itr : str)
			{
				switch (itr)
				{
				
				case '\\':
					buffer.append("\\\\");
					break;
				case '"':
					buffer.append("\\\"");
					break;
				case '/':
					buffer.append("\\/");
					break;
				case '\b':
					buffer.append("\\b");
					break;
				case '\f':
					buffer.append("\\f");
					break;
				case '\t':
					buffer.append("\\t");
					break;
				case '\n':
					buffer.append("\\n");
					break;
				case '\r':
					buffer.append("\\r");
					break;
				default:
					buffer.push_back(itr);
				}
			}
			return buffer;
		}
		std::string str()const
		{
			switch (val_->type_)
			{
			case e_bool:
				return val_->bool_ ? "true" : "false";
			case e_num:
				return std::to_string(val_->num_);
			case e_str:
				return "\"" + encode_str(*val_->str_) + "\"";
			case e_obj:
				do 
				{
					std::string str("{");
					for (auto &itr : *val_->obj_)
					{
						str += "\"" + itr.first + "\"";
						str += " : ";
						str += itr.second.str();
						str += ", ";
					}
					if (str.size() > 2)
					{
						str.pop_back();
						str.pop_back();
					}
					str += "}";
					return str;

				} while (0);
			case e_float:
				return std::to_string(val_->double_);
			case e_vec:
				do 
				{
					std::string str("[");
					xjson_assert(val_->type_ == e_vec);
					xjson_assert(val_->vec_ != NULL);

					for (auto &itr : *val_->vec_)
					{
						str += itr.str();
						str += ", ";
					}
					if (str.size() > 2)
					{
						str.pop_back();
						str.pop_back();
					}
					str += "]";
					return str;

				} while (0);

			default:
				return "null";
			}
			return "null";
		}
	};
	
	namespace json_parser
	{
		class error_json :std::logic_error
		{
		public:
			explicit error_json(const std::string &msg)
				:std::logic_error(msg)
			{

			}

			explicit error_json(const char *msg)
				:std::logic_error(msg)
			{

			}
		};

		static inline bool
			skip_space(int &pos, int len, const char * str)
		{
			assert(len > 0);
			assert(pos >= 0);
			assert(pos < len);

			while (pos < len &&
				(str[pos] == ' ' ||
					str[pos] == '\r' ||
					str[pos] == '\n' ||
					str[pos] == '\t'))
				++pos;

			return pos < len;
		}
		static inline std::string get_text(int &pos, int len, const char *str)
		{
			assert(len > 0);
			assert(pos >= 0);
			assert(pos < len);
			assert(str[pos] == '"');

			std::string text;
			++pos;
			bool rs = false;
			while (pos < len)
			{
				if (str[pos] == '\\')
				{
					if(!rs)
						rs = true;
					else
					{
						rs = false;
						text.push_back(str[pos]);
					}
				}
				else
				{
					if (rs)
					{
						rs = false;
						switch (str[pos])
						{
						case 'f':
							text.push_back('\f');
							break;
						case 'b':
							text.push_back('\b');
							break;
						case '/':
							text.push_back('/');
							break;
						case '"':
							text.push_back('"');
							break;
						case 't':
							text.push_back('\t');
							break;
						case 'n':
							text.push_back('\n');
							break;
						default:
							throw error_json(std::string("\\") + str[pos] + " error");
						}
					}
					else if (str[pos] == '"')
					{
						++pos;
						return text;
					}
					else
						text.push_back(str[pos]);
				}
				++pos;
			}
			throw error_json("get_text error");
		}
		static inline bool get_bool(int &pos, int len, const char *str)
		{
			assert(len > 0);
			assert(pos >= 0);
			assert(pos < len);
			assert(str[pos] == 't' || str[pos] == 'f');
			std::string key;

			while (pos < len)
			{
				if (isalpha(str[pos]))
					key.push_back(str[pos]);
				else
					break;
				++pos;
			}
			if (key == "true")
				return true;
			else if (key == "false")
				return false;

			throw error_json("get_bool");
			return false;
		}
		static inline nullptr_t get_null(int &pos, int len, const char *str)
		{
			assert(len > 0);
			assert(pos >= 0);
			assert(pos < len);
			assert(str[pos] == 'n');
			std::string null;

			while (pos < len)
			{
				if (isalpha(str[pos]))
					null.push_back(str[pos]);
				else
					break;
				++pos;
			}
			if (null == "null")
				return nullptr;
			throw error_json("get_null");
		}
		static inline std::string get_num(
			bool &sym, int &pos, int len, const char *str)
		{
			sym = false;
			std::string tmp;
			while (pos < len)
			{
				if (str[pos] >= '0' &&str[pos] <= '9')
				{
					tmp.push_back(str[pos]);
					++pos;
				}
				else if (str[pos] == '.')
				{
					tmp.push_back(str[pos]);
					++pos;
					if (sym == false)
						sym = true;
					else
						return tmp;
				}
				else
					break;
			}
			return tmp;
		}

		static inline obj_t get_obj(int &pos, int len, const char * str);
		static inline obj_t get_vec(int &pos, int len, const char *str)
		{
			obj_t vec;
			if (str[pos] == '[')
				pos++;
			while (pos < len)
			{
				switch (str[pos])
				{
				case ']':
					++pos;
					return vec;
				case '[':
				{
					vec.add(get_vec(pos, len, str));
					break;
				}
				case '"':
				{
					vec.add(get_text(pos, len, str));
					break;
				}
				case 'n':
						vec.add(get_null(pos, len, str));
						break;
				case '{':
				{
					vec.add(get_obj(pos, len, str));
					break;
				}
				case 'f':
				case 't':
				{
					vec.add(get_bool(pos, len, str));
					break;
				}
				case ',':
				case ' ':
				case '\r':
				case '\n':
				case '\t':
					++pos;
					break;
				default:
					if (str[pos] == '-' || (str[pos] >= '0' && str[pos] <= '9'))
					{
						bool is_float = false;
						std::string tmp = get_num(is_float, pos, len, str);
						errno = 0;
						if (is_float)
						{
							double d = std::strtod(tmp.c_str(), 0);
							if (errno == ERANGE)
								throw error_json("get_float error");
							vec.add(d);
						}
						else
						{
							int64_t val = std::strtoll(tmp.c_str(), 0, 10);
							if (errno == ERANGE)
								throw error_json("get num error");
							vec.add(val);
						}
					}
				}
			}
			
			return{};
		}

		#define check_ahead(ch)\
		do{\
				if(!skip_space(pos, len, str))\
					throw error_json("error_json"); \
					if(str[pos] != ch)\
						throw error_json("error_json");\
		} while(0)

		static inline obj_t get_obj(int &pos, int len, const char * str)
		{
			obj_t obj;
			std::string key;

			skip_space(pos, len, str);
			if (pos >= len)
				throw error_json("error json");
			if (str[pos] == '{')
				++pos;
			if (str[pos] == '[')
				return get_vec(pos, len, str);
			while (pos < len)
			{
				switch (str[pos])
				{
				case '"':
				{
					if (key.empty())
					{
						key = get_text(pos, len, str);
						check_ahead(':');
					}
					else
					{
						obj[key] = get_text(pos, len, str);
						key.clear();
					}
					break;
				}
				case 'f':
				case 't':
				{
					if (key.empty())
						throw error_json("error json");
					obj[key] = get_bool(pos, len, str);
					key.clear();
					break;
				}
				case 'n':
				{
					obj[key] = get_null(pos, len, str);
				}
				case '{':
				{
					if (key.empty())
						throw error_json("error json");
					obj[key] = std::move(get_obj(pos, len, str));
					key.clear();
					break;
				}
				case '}':
					if (key.size())
						throw error_json("error json");
					++pos;
					return std::move(obj);
				case ':':
				{
					if (key.empty())
						throw error_json("error json");
					++pos;
					break;
				}
				case ',':
				{
					++pos;
					check_ahead('"');
					break;
				}
				case ' ':
				case '\r':
				case '\n':
				case '\t':
					++pos;
					break;
				case '[':
				{
					obj[key] = std::move(get_vec(pos, len, str));
					key.clear();
					break;
				}
				default:
					if (str[pos] == '-' || (str[pos] >= '0' && str[pos] <= '9'))
					{
						bool is_float = false;
						std::string tmp = get_num(is_float, pos, len, str);
						errno = 0;
						if (is_float)
						{
							double d = std::strtod(tmp.c_str(), 0);
							if (errno == ERANGE)
								throw error_json("error json");
							obj[key] = d;
							key.clear();
						}
						else
						{
							int64_t val = std::strtoll(tmp.c_str(), 0, 10);
							if (errno == ERANGE)
								throw error_json("error json");
							obj[key] = val;
							key.clear();
						}
					}
				}
			}
			return {};
		}
	}
	inline obj_t build(const std::string &str)
	{
		int pos = 0;
		return json_parser::get_obj(pos, (int)str.size(), str.c_str());
	}
	inline obj_t build(const char *str)
	{
		int pos = 0;
		return json_parser::get_obj(pos, (int)strlen(str), str);
	}
}
