#pragma once
namespace xraft
{
namespace functors
{
namespace fs
{
	struct truncate_prefix
	{
		bool operator()(const std::string &filepath, int64_t offset)
		{
			std::ofstream tmp_file;
			int mode = std::ios::out | 
				std::ios::trunc | 
				std::ios::binary;
			tmp_file.open((filepath + ".tmp").c_str(), mode);
			if (!tmp_file.good())
			{
				//todo log error;
				return false;
			}
			std::ifstream old_file;
			mode = std::ios::in;
			old_file.open(filepath.c_str(), mode);
			if (!old_file.good())
			{
				//todo log error;
				xutil::vfs::unlink()((filepath + ".tmp"));
				return false;
			}
			old_file.seekg(offset, std::ios::beg);
			if (!tmp_file.good())
			{
				//todo log error;
				xutil::vfs::unlink()((filepath + ".tmp"));
				return false;
			}
			const int buffer_len = 16 * 1024;
			char buffer[buffer_len];
			do
			{
				old_file.read(buffer, buffer_len);
				tmp_file.write(buffer, old_file.gcount());
				if (old_file.eof())
					break;
			} while (true);
			old_file.close();
			tmp_file.close();
			if (!xutil::vfs::unlink()(filepath))
			{
				//todo log error;
				xutil::vfs::unlink()((filepath + ".tmp"));
				return false;
			}
			return xutil::vfs::rename()((filepath + ".tmp"),filepath);
		}
	};
}
		
}
}