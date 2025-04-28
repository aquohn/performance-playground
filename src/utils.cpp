#include "utils.hpp"

#include <openssl/evp.h>
#include <openssl/sha.h>

hash_t sha256(std::vector<char> const &msg) {
  std::vector<unsigned char> hashbuf(SHA256_DIGEST_LENGTH);
  EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(md_ctx, EVP_sha256(), NULL);
  EVP_DigestUpdate(md_ctx, msg.data(), msg.size());
  EVP_DigestFinal_ex(md_ctx, hashbuf.data(), NULL);
  EVP_MD_CTX_free(md_ctx);
  return hashbuf;
}

bool find_pdf(fs::path dpath, fs::path &fpath) {
  for (auto const &f : fs::directory_iterator(dpath)) {
    if (f.path().extension() == "pdf"s) {
      fpath = f.path();
      return true;
    }
  }
  return false;
}
