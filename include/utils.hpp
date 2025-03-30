#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace std::literals;

typedef std::vector<unsigned char> hash_t;

hash_t sha256(std::vector<char> const &msg);
