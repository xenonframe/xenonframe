#pragma once
#include <functional>
namespace xutil
{
	struct guard
	{
		guard(const std::function<void()> &func)
			:func_(func)
		{
			
		}
		void release()
		{
			func_ = nullptr;
		}
		~guard()
		{
			if(func_)
				func_();
		}
		std::function<void()> func_;
	};
}
