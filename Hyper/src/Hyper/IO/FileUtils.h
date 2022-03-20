#pragma once

namespace Hyper::IO
{
	enum class WriteMode
	{
		Overwrite,
		Append
	};

	// Reads a file into a string.
	bool ReadFileSync(const std::filesystem::path& filePath, std::string& output);

	// Writes a string to a file.
	// The file will be created if it doesn't exist yet.
	// Default write mode is overwrite.
	bool WriteFileSync(const std::filesystem::path& filePath, const std::string& contents, WriteMode mode = WriteMode::Overwrite);
}
