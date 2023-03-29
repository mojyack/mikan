#pragma once
#include <filesystem>

class TemporaryDirectory {
  private:
    const std::string path;

    static auto find_empty_dir() -> std::string {
        auto dir = std::string("/tmp/mikan-tmp");
        while(std::filesystem::exists(dir)) {
            dir += '.';
        }
        return dir;
    }


  public:
    auto str() const -> const std::string& {
        return path;
    }

    TemporaryDirectory() : path(find_empty_dir()) {
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory() {
        std::filesystem::remove_all(path);
    }
};
