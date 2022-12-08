#include "File.h"

#include "Log.h"

#include <filesystem>

namespace Brigerad
{
bool File::CheckIfPathExists(const std::string& path)
{
    // We check the error instead of using the overload that doesn't require it to avoid using
    // exceptions.
    std::error_code error;
    bool            exists = std::filesystem::exists(path, error);

    if (error)
    {
        BR_CORE_ERROR("Error when checking path ({}): {}", error.value(), error.message());
    }

    return exists;
}

bool File::CreateDir(const std::string& path)
{
    // We check the error instead of using the overload that doesn't require it to avoid using
    // exceptions.
    std::error_code error;
    bool            success = std::filesystem::create_directory(path, error);

    if (error)
    {
        BR_CORE_ERROR("Error when creating directory with path '{}' ({}): {}",
                      path,
                      error.value(),
                      error.message());
    }

    return success;
}


}    // namespace Brigerad
