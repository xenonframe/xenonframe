#pragma once
#include <thread>
#include <iostream>
#include "sync_queue.hpp"
namespace xutil
{
	class xworker
	{
	public:
		using job_t = std::function<void()>;
		using steal_job_t = std::function<bool(job_t &)>;

		xworker(const steal_job_t &handle)
			:steal_job_(handle)
		{
			thread_ = std::thread([this] { do_job(); });
		}
		~xworker()
		{
			stop();
		}
		void add_job(job_t &&job)
		{
			job_queue_.push(std::move(job));
		}
		void stop()
		{
			is_stop_ = true;
			job_queue_.push([this] {});
			if(thread_.joinable())
				thread_.join();
		}
		bool steal_job(job_t &job)
		{
			return job_queue_.pop(job, 1);
		}
		std::size_t jobs()
		{
			return job_queue_.jobs();
		}
	private:
		void do_job()
		{
			try
			{
				while (!is_stop_)
				{
					job_t job;
					if (job_queue_.pop(job, 100))
					{
						job();
						continue;
					}
					if (steal_job_ && steal_job_(job))
					{
						job();
					}
				};
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
				throw e;
			}
			
		}
		bool is_stop_ = false;
		sync_queue<job_t> job_queue_;
		std::thread thread_;
		steal_job_t steal_job_;
	};
}