#pragma once
namespace xutil
{
	class no_copy_able
	{
	public:
		no_copy_able()
		{

		}
		no_copy_able(const no_copy_able &) = delete;
		no_copy_able &operator = (const no_copy_able &) = delete;
		no_copy_able(no_copy_able &&)
		{

		}
	};
}