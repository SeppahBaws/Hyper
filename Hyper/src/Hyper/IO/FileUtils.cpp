#include "HyperPCH.h"
#include "FileUtils.h"
#include <fstream>

namespace Hyper::IO
{
	bool ReadFileSync(const std::filesystem::path& filePath, std::string& output)
	{
		std::ifstream file(filePath, std::ios::binary | std::ios::in);
		if (!file)
			return false;

		output.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		file.close();

		return true;
	}

	bool ReadFileSync(const std::filesystem::path& filePath, std::vector<u32>& output)
	{
		std::ifstream file(filePath, std::ios::binary | std::ios::in | std::ios::ate);
		if (!file)
			return false;

		const u32 numElements = file.tellg() / sizeof(u32);
		file.seekg(0);


		for (u32 i = 0; i < numElements; i++)
		{
			u32 element;
			file.read(reinterpret_cast<char*>(&element), sizeof(u32));
			output.push_back(element);
		}

		return true;
	}

	bool WriteFileSync(const std::filesystem::path& filePath, const std::string& contents, WriteMode mode)
	{
		std::ofstream file(filePath,
		                   mode == WriteMode::Append
			                   ? std::ios::binary | std::ios::out | std::ios::app
			                   : std::ios::binary | std::ios::out);

		if (!file)
			return false;

		file << contents;
		file.close();

		return true;
	}

	bool WriteFileSync(const std::filesystem::path& filePath, const std::vector<u32>& contents, WriteMode mode)
	{
		std::ofstream file(filePath,
		                   mode == WriteMode::Append
			                   ? std::ios::binary | std::ios::out | std::ios::app
			                   : std::ios::binary | std::ios::out);

		if (!file)
			return false;

		file.write(reinterpret_cast<const char*>(contents.data()), contents.size() * sizeof(u32));
		file.close();

		return true;
	}
}
