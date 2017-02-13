#pragma once
namespace xhttper
{
	class query
	{
	public:
		query()
		{

		}
		query(query &other)
			:query_str_(other.query_str_)
		{

		}
		query(query &&other)
			:query_str_(std::move(other.query_str_))
		{

		}
		query &operator = (query &&other)
		{
			if (this == &other)
				return *this;
			query_str_ = std::move(other.query_str_);
			return *this;
		}
		query(const std::string &query_str)
		{
			parse_query(query_str);
		}
		std::string get(const std::string &name)
		{
			auto itr = query_str_.find(name);
			if (itr == query_str_.end())
				return{};
			return itr->second;
		}
		template<typename T>
		typename std::enable_if<std::is_integral<T>::value,T>::type
			get(const std::string &name)
		{
			auto str = get(name);
			auto value = std::strtoll(str.c_str(), nullptr, 10);
			if (value < 0 && std::is_unsigned<T>::value)
				throw std::runtime_error("valid cast");
			return static_cast<T>(value);
		}

		template<typename T>
		typename std::enable_if<std::is_floating_point<T>::value,T>::type
			get(const std::string &name)
		{
			auto str = get(name);
			auto value = std::strtod(str.c_str(), nullptr);
			return static_cast<T>(value);
		}


	private:
		template<char CH, typename Itr>
		std::string get_token(Itr &itr, const Itr &end)
		{
			std::string str;
			while (itr != end && *itr != CH)
			{
				str.push_back(*itr);
				itr++;
			}
			return str;
		}
		void parse_query(const std::string &str)
		{
			auto itr = str.begin();
			auto end = str.end();
			do
			{
				auto name = get_token<'='>(itr, end);
				if (itr == end || name.empty())
					break;
				++itr;
				auto value = get_token<'&'>(itr, end);
				if (name.empty())
					break;
				query_str_.emplace(std::move(name), std::move(value));
				if (itr != end)
					++itr;
			} while (itr != end);
		}
		std::map<std::string, std::string> query_str_;
	};
}