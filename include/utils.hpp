#include <vector>
#include <string>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;
using namespace std::literals;

typedef std::vector<unsigned char> hash_t;

hash_t sha256(std::vector<char> const& msg);
