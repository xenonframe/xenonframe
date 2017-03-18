#pragma once
#define DEFINE_RPC_PROTO(name, proto)\
struct name\
{\
	using func_type = proto ;\
	static const char *rpc_name(){	return #name; }\
};
namespace xsimple_rpc
{

	namespace detail
	{
		static const std::string magic_code = "xsimple_rpc";

		struct rpc_req
		{
			enum class status
			{
				e_null,
				e_ok,
				e_cancel,
				e_rpc_error,
			};
			status status_ = status::e_null;
			int64_t req_id_;
			std::string result_;
			std::string req_buffer_;
		};
		inline std::size_t min_rpc_msg_len()
		{
			static std::size_t min_rpc_len_ = 
			xutil::endec::get_sizeof(uint64_t()) +
				xutil::endec::get_sizeof(std::string()) +
				xutil::endec::get_sizeof(magic_code) +
				xutil::endec::get_sizeof(uint32_t());
			return min_rpc_len_;
		}
		template<typename ...Args>
		inline std::string make_req(const std::string &rpc_name, int64_t req_id, const std::tuple<Args...> &tp)
		{
			using namespace detail;
			std::string buffer;
			uint32_t buffer_size = uint32_t(
				xutil::endec::get_sizeof(req_id) +
				xutil::endec::get_sizeof(tp) +
				xutil::endec::get_sizeof(rpc_name) +
				xutil::endec::get_sizeof(magic_code) +
				xutil::endec::get_sizeof(uint32_t()));

			buffer.resize(buffer_size);
			uint8_t*ptr = (uint8_t*)buffer.data();
			xutil::endec::put(ptr, buffer_size);
			xutil::endec::put(ptr, magic_code);
			xutil::endec::put(ptr, req_id);
			xutil::endec::put(ptr, rpc_name);
			xutil::endec::put(ptr, tp);
			return std::move(buffer);
		}
		inline std::string make_req(const std::string &rpc_name, int64_t req_id)
		{
			using namespace detail;
			std::string buffer;
			uint32_t buffer_size = uint32_t(
				xutil::endec::get_sizeof(req_id) +
				xutil::endec::get_sizeof(rpc_name) +
				xutil::endec::get_sizeof(magic_code) +
				xutil::endec::get_sizeof(uint32_t()));

			buffer.resize(buffer_size);
			uint8_t*ptr = (uint8_t*)buffer.data();
			xutil::endec::put(ptr, buffer_size);
			xutil::endec::put(ptr, magic_code);
			xutil::endec::put(ptr, req_id);
			xutil::endec::put(ptr, rpc_name);
			return std::move(buffer);
		}

		template<typename ...Args>
		inline std::string make_resp(int64_t req_id, std::string &&data)
		{
			using namespace detail;
			std::string buffer;
			uint32_t buffer_size = uint32_t(
				xutil::endec::get_sizeof(req_id) +
				xutil::endec::get_sizeof(data) +
				xutil::endec::get_sizeof(magic_code) +
				xutil::endec::get_sizeof(uint32_t()));

			buffer.resize(buffer_size);
			uint8_t*ptr = (uint8_t*)buffer.data();
			xutil::endec::put(ptr, buffer_size);
			xutil::endec::put(ptr, magic_code);
			xutil::endec::put(ptr, req_id);
			xutil::endec::put(ptr, std::move(data));
			return std::move(buffer);
		}
	}
}
