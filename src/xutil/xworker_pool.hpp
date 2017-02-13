#pragma once
#include "xworker.hpp"
#include <memory>
#include <vector>
#include <atomic>
#include <time.h>
#include <stdlib.h>
namespace xutil
{
	class xworker_pool
	{
	public:
		xworker_pool(int worker_size = std::thread::hardware_concurrency())
			:worker_size_(worker_size)
		{
			init();
		}
		~xworker_pool()
		{
			stop();
		}
		template<typename T>
		void add_job(T &&job)
		{
			index_++;
			get_worker().add_job(std::forward<T>(job));
		}
		void stop()
		{
			for (auto &itr : workers_)
				itr->stop();
			workers_.clear();
		}
	private:
		void init()
		{
			srand((uint32_t)time(nullptr));

			for (uint32_t i = 0; i < worker_size_; ++i)
			{
				workers_.emplace_back(new xworker([this](xworker::job_t &job) {
					return steal_job(job);
				}));
			}
			is_init_done_ = true;
		}
		bool steal_job(xworker::job_t &job)
		{
			if (!is_init_done_)
				return false;

			std::vector<xworker*> tmp;
			for (auto &itr : workers_)
			{
				if (itr->jobs() > 0)
					tmp.emplace_back(itr.get());
			}
			if (tmp.empty())
				return false;
			return tmp[rand() % tmp.size()]->steal_job(job);
		}

		xworker &get_worker()
		{
			return *workers_[index_%workers_.size()].get();
		}

		std::atomic<uint64_t> index_ { 0 };
		bool is_init_done_ = false;
		uint32_t worker_size_;
		std::vector<std::unique_ptr<xworker>> workers_;
	};
}