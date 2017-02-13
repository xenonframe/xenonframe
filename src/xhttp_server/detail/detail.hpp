#pragma once
#include <memory>
#include <functional>
#include <string>
#include <algorithm>
//deps
#include "../xnet/proactor_pool.hpp"
#include "../xhttper/http_builder.hpp"
#include "../xhttper/http_parser.hpp"
#include "../xhttper/mime_parser.hpp"
#include "../xhttper/query.hpp"
#include "../xutil/functional.hpp"
#include "../xutil/function_traits.hpp"
#include "../xutil/vfs.hpp"
#include "../xredis/hash.hpp"
#include "../xcoroutine/xcoroutine.hpp"
#include "../xasync/xasync.hpp"

#include "../middleware/redis_session/xsession.hpp"
#include "../middleware/async/async.hpp"
#include "../response.hpp"
#include "../request.hpp"
#include "../uploader.hpp"
#include "../downloader.hpp"
#include "../filelist.hpp"
