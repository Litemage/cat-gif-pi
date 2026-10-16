#include "gif_loader.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <fstream>

namespace fs = std::filesystem;

namespace {

constexpr int kRotationHour = 7;

bool isGif(const fs::path& p)
{
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext == ".gif";
}

bool isDir(const fs::path& p)
{
    std::error_code ec;
    return fs::is_directory(p, ec);
}

bool isFile(const fs::path& p)
{
    std::error_code ec;
    return fs::is_regular_file(p, ec);
}

std::tm localTime(std::chrono::system_clock::time_point tp)
{
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm;
}

}  // namespace

GifLoader::GifLoader(GifConfig config, Clock clock)
    : config_(std::move(config)), clock_(std::move(clock))
{
    loadCache();
}

std::chrono::system_clock::time_point GifLoader::now() const
{
    return clock_ ? clock_() : std::chrono::system_clock::now();
}

// The "rotation day" is the local date, except before 7AM when it is still the
// previous day. Returned as YYYY-MM-DD.
std::string GifLoader::rotationDay() const
{
    std::tm tm = localTime(now());
    if (tm.tm_hour < kRotationHour) {
        tm.tm_mday -= 1;
        tm.tm_isdst = -1;
        std::mktime(&tm);  // normalize
    }
    char buf[64];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return buf;
}

std::chrono::system_clock::time_point GifLoader::nextRotationTime() const
{
    auto current = now();
    std::tm tm = localTime(current);
    tm.tm_hour = kRotationHour;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;
    auto next = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    if (next <= current) {
        tm.tm_mday += 1;
        tm.tm_isdst = -1;
        next = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
    return next;
}

// Cache file format (three lines): rotation day, pending-move flag (0/1), source path.
void GifLoader::loadCache()
{
    cache_ = {};
    std::ifstream in(config_.cacheFile);
    if (!in) {
        return;
    }
    std::string pending, source;
    std::getline(in, cache_.day);
    std::getline(in, pending);
    std::getline(in, source);
    cache_.pendingMove = (pending == "1");
    cache_.source = source;
}

bool GifLoader::saveCache() const
{
    fs::path tmp = config_.cacheFile;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) {
            return false;
        }
        out << cache_.day << '\n' << (cache_.pendingMove ? 1 : 0) << '\n'
            << cache_.source.string() << '\n';
        if (!out) {
            return false;
        }
    }
    std::error_code ec;
    fs::rename(tmp, config_.cacheFile, ec);
    return !ec;
}

std::optional<fs::path> GifLoader::currentGif() const
{
    if (isFile(config_.localCopy)) {
        return config_.localCopy;
    }
    if (isFile(config_.fallbackGif)) {
        return config_.fallbackGif;
    }
    return std::nullopt;
}

std::vector<fs::path> GifLoader::listGifs(const fs::path& dir) const
{
    std::vector<fs::path> gifs;
    std::error_code ec;
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        const fs::path& p = it->path();
        // Skip hidden files, e.g. macOS "._foo.gif" metadata files on USB sticks.
        if (p.filename().string().rfind('.', 0) == 0) {
            continue;
        }
        if (isGif(p) && isFile(p)) {
            gifs.push_back(p);
        }
    }
    std::sort(gifs.begin(), gifs.end());
    return gifs;
}

// Moves a file into toDir (rename, or copy + remove across filesystems), avoiding
// name clashes with a numeric suffix.
bool GifLoader::moveFile(const fs::path& from, const fs::path& toDir) const
{
    std::error_code ec;
    fs::path dest = toDir / from.filename();
    for (int n = 1; fs::exists(dest, ec) && n < 10000; ++n) {
        dest = toDir / (from.stem().string() + "_" + std::to_string(n) + from.extension().string());
    }

    fs::rename(from, dest, ec);
    if (!ec) {
        return true;
    }

    ec.clear();
    fs::copy_file(from, dest, fs::copy_options::none, ec);
    if (ec) {
        fs::remove(dest, ec);
        return false;
    }
    fs::remove(from, ec);
    if (ec) {
        fs::remove(dest, ec);  // couldn't finish the move; don't leave a duplicate
        return false;
    }
    return true;
}

// Moves every GIF in "used" back to "new" so the cycle can start over.
bool GifLoader::recycleUsed()
{
    bool all = true;
    for (const fs::path& p : listGifs(config_.usedDir)) {
        all = moveFile(p, config_.newDir) && all;
    }
    return all;
}

bool GifLoader::pickNext()
{
    if (!isDir(config_.newDir) || !isDir(config_.usedDir)) {
        status_ = StorageStatus::StorageError;
        return false;
    }

    std::vector<fs::path> gifs = listGifs(config_.newDir);
    if (gifs.empty()) {
        recycleUsed();
        gifs = listGifs(config_.newDir);
    }
    if (gifs.empty()) {
        status_ = StorageStatus::StorageError;
        return false;
    }
    const fs::path source = gifs.front();

    // Copy to the local disk first, so the display never depends on the USB stick.
    fs::path tmp = config_.localCopy;
    tmp += ".tmp";
    std::error_code ec;
    fs::copy_file(source, tmp, fs::copy_options::overwrite_existing, ec);
    if (!ec) {
        fs::rename(tmp, config_.localCopy, ec);
    }
    if (ec) {
        fs::remove(tmp, ec);
        status_ = StorageStatus::StorageError;
        return false;
    }

    cache_.day = rotationDay();
    cache_.source = source;
    cache_.pendingMove = true;
    saveCache();

    status_ = StorageStatus::Ok;
    retryPendingMove();
    return true;
}

void GifLoader::retryPendingMove()
{
    if (!cache_.pendingMove) {
        return;
    }
    if (!isDir(config_.newDir) || !isDir(config_.usedDir)) {
        status_ = StorageStatus::StorageError;  // keep pending; USB may come back
        return;
    }
    if (isFile(cache_.source)) {
        if (!moveFile(cache_.source, config_.usedDir)) {
            status_ = StorageStatus::StorageError;
            return;
        }
    }
    // Moved, or the source is gone while "new" is readable: nothing left to do.
    cache_.pendingMove = false;
    saveCache();
    status_ = StorageStatus::Ok;
}

bool GifLoader::update()
{
    retryPendingMove();
    if (cache_.day == rotationDay() && isFile(config_.localCopy)) {
        // Nothing to pick, but still surface an unplugged stick / bad directories.
        if (!isDir(config_.newDir) || !isDir(config_.usedDir)) {
            status_ = StorageStatus::StorageError;
        } else if (!cache_.pendingMove) {
            status_ = StorageStatus::Ok;
        }
        return false;
    }
    return pickNext();
}

bool GifLoader::rotate()
{
    retryPendingMove();
    return pickNext();
}
