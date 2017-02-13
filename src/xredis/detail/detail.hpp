#pragma once
#ifdef _WIN32
#define  _CRT_SECURE_NO_WARNINGS
#endif
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <memory>
#include <cassert> 
#include <cstdio>

//deps
#include "../xnet/xnet.hpp"
//end of deps
#include "../reply_entry.hpp"
#include "../callback_func.hpp"
#include "cmd_builder.hpp"
#include "functional.hpp"
#include "reply_parser.hpp"
#include "client.hpp"

namespace xredis
{
	using detail::reply_parser;
	using detail::cmd_builder;
	using detail::master_info_parser;
	using detail::crc16er;
	using detail::client;

}
