#pragma once
namespace xsocket_io
{
	inline std::string build_resp(const std::string & resp,
		int status,const std::string &origin ,bool binary = false)
	{
		xhttper::http_builder http_builder;

		http_builder.set_status(status);
		std::string content_type = "text/plain; charset=utf-8";
		if (binary)
			content_type = "application/octet-stream";
		if (origin.size())
		{
			http_builder.append_entry("Access-Control-Allow-Credentials", "true");
			http_builder.append_entry("Access-Control-Allow-Origin", origin);
		}
		http_builder.append_entry("Content-Length", std::to_string(resp.size()));
		http_builder.append_entry("Content-Type", content_type);
		http_builder.append_entry("Connection", "keep-alive");
		http_builder.append_entry("Date", xutil::functional::get_rfc1123()());

		auto buffer = http_builder.build_resp();
		buffer.append(resp);
		return  buffer;
	}
}