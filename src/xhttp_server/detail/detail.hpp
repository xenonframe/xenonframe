#pragma once
#include <memory>
#include <functional>
#include <string>
#include <algorithm>
//deps
#include "../../../xnet/include/proactor_pool.hpp"
#include "../../../xhttper/include/http_builder.hpp"
#include "../../../xhttper/include/http_parser.hpp"
#include "../../../xhttper/include/mime_parser.hpp"
#include "../../../xhttper/include/query.hpp"
#include "../../../xutil/include/functional.hpp"
#include "../../../xutil/include/function_traits.hpp"
#include "../../../xutil/include/vfs.hpp"
#include "../../../xredis/include/hash.hpp"
#include "../../../xcoroutine/include/xcoroutine.hpp"
#include "../../../xasync/include/xasync.hpp"

#include "../middleware/redis_session/xsession.hpp"
#include "../middleware/async/async.hpp"
#include "../response.hpp"
#include "../request.hpp"
#include "../uploader.hpp"
#include "../downloader.hpp"
#include "../filelist.hpp"
