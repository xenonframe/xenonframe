#pragma once
namespace xwebsocket
{
	static inline std::string make_handshake(const std::string &key, const std::string &protocol, const std::map<std::string, std::string> &headers = {})
	{

		if (key.empty())
			throw std::runtime_error("key must not empty");

		std::string data;
		data.reserve(128);

		data.append("HTTP/1.1 101 Switching Protocols\r\n");
		data.append("Upgrade: websocket\r\n");
		data.append("Connection: Upgrade\r\n");
		for (auto &itr : headers)
			data.append(itr.first + ": "+itr.second+"\r\n");

		unsigned char digest[20];
		std::string accept_key;
		accept_key.reserve(128);
		accept_key.append(key);
		accept_key.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

		xutil::sha1 sha;
		sha.input(accept_key.data(), accept_key.size());
		sha.result((uint32_t*)digest);

		//little endian to big endian
		for (int i = 0; i < 20; i += 4)
		{
			unsigned char c;

			c = digest[i];
			digest[i] = digest[i + 3];
			digest[i + 3] = c;

			c = digest[i + 1];
			digest[i + 1] = digest[i + 2];
			digest[i + 2] = c;
		}
		data.append("Sec-WebSocket-Accept: ");
		data.append(xutil::base64::encode(std::string((char*)digest, 20)));
		data.append("\r\n");

		if (protocol.size())
		{
			data.append("Sec-WebSocket-Protocol: ");
			data.append(protocol);
			data.append("\r\n");
		}
		data.append("\r\n");
		return std::move(data);
	}
}