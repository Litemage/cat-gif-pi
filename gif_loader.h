#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct GifConfig {
    // Text file naming the directories below (see loadConfigFile). Absolute so the
    // init script and a bare "cat_gif" invocation agree.
    std::filesystem::path configFile = "/etc/cat-gif.conf";

    // Directories on the (removable) USB stick; overridden by configFile.
    std::filesystem::path newDir = "gifs/new";
    std::filesystem::path usedDir = "gifs/used";

    // Local files, relative to the directory the EXE runs in.
    std::filesystem::path cacheFile = "current_gif.txt";
    std::filesystem::path localCopy = "current.gif";

    // Installed by CMake (see install() in CMakeLists.txt); absolute, not cwd-relative.
    std::filesystem::path fallbackGif = "/usr/share/cat-gif/fallback.gif";
};

// Reads config.configFile: first line is the "new" directory, second the "used"
// directory. Blank lines and lines starting with '#' are skipped, surrounding
// whitespace is trimmed. Lines that are missing leave the existing value alone, so
// a missing or short file keeps the defaults. Returns true if the file was read.
bool loadConfigFile(GifConfig& config);

enum class StorageStatus {
    Ok,
    StorageError,  // USB missing, or new/used dir missing / unusable / no GIFs
};

// Picks one GIF per day (rotating at 7AM local time), copies it to a local file so
// that unplugging the USB stick never interrupts the display, and moves the source
// from the "new" to the "used" directory.
class GifLoader {
public:
    using Clock = std::function<std::chrono::system_clock::time_point()>;

    explicit GifLoader(GifConfig config, Clock clock = nullptr);

    // Local, displayable GIF: today's copy, else the fallback GIF, else nullopt.
    std::optional<std::filesystem::path> currentGif() const;

    // Retries any pending move to "used", and picks a new GIF if the rotation day
    // changed (or an earlier pick failed). Returns true if the local copy was replaced.
    bool update();

    // Forces a rotation to the next GIF. Returns true if the local copy was replaced.
    bool rotate();

    // Re-reads the config file. Returns true if the new/used directories changed.
    bool reloadConfig();

    StorageStatus status() const { return status_; }
    const GifConfig& config() const { return config_; }

    // Next 7AM (local time) strictly after now.
    std::chrono::system_clock::time_point nextRotationTime() const;

private:
    struct Cache {
        std::string day;                // rotation day the local copy was picked for
        bool pendingMove = false;       // source still needs to be moved to "used"
        std::filesystem::path source;   // source path inside "new"
    };

    std::chrono::system_clock::time_point now() const;
    std::string rotationDay() const;

    void loadCache();
    bool saveCache() const;

    bool pickNext();
    bool recycleUsed();
    bool moveFile(const std::filesystem::path& from, const std::filesystem::path& toDir) const;
    void retryPendingMove();
    std::vector<std::filesystem::path> listGifs(const std::filesystem::path& dir) const;

    GifConfig config_;
    Clock clock_;
    Cache cache_;
    StorageStatus status_ = StorageStatus::Ok;
};
