#pragma once
namespace xsimple_rpc
{

	struct rpc_cancel : std::exception
	{
		virtual char const* what() const  throw()
		{
			return "rpc_cancel_error";
		}
	};
	struct rpc_error : std::exception
	{
		rpc_error(std::string error_code)
			:error_code_(error_code)
		{
		}
		virtual char const* what() const throw()
		{
			return error_code_.c_str();
		}
		std::string error_code_;
	};
	struct rpc_timeout: std::exception
	{
		virtual char const* what() const throw()
		{
			return "rpc_timeout";
		}
	};
	struct rpc_connect_timeout : std::exception
	{
		virtual char const* what() const throw()
		{
			return "rpc_connect_timeout";
		}
	};

}
