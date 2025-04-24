#include "mnist_loader.h"
#include <curl/curl.h>
#include <zlib.h>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace {
const char* urls[] = {
    "http://yann.lecun.com/exdb/mnist/train-images-idx3-ubyte.gz",
    "http://yann.lecun.com/exdb/mnist/train-labels-idx1-ubyte.gz",
    "http://yann.lecun.com/exdb/mnist/t10k-images-idx3-ubyte.gz",
    "http://yann.lecun.com/exdb/mnist/t10k-labels-idx1-ubyte.gz"
};
const char* names[] = {
    "train-images-idx3-ubyte.gz",
    "train-labels-idx1-ubyte.gz",
    "t10k-images-idx3-ubyte.gz",
    "t10k-labels-idx1-ubyte.gz"
};

bool exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st)==0;
}

size_t write_file(void* ptr, size_t size, size_t nmemb, void* stream) {
    return fwrite(ptr, size, nmemb, (FILE*)stream);
}

void download(const std::string& url, const std::string& out_path) {
    CURL* curl = curl_easy_init();
    if(!curl) throw std::runtime_error("curl init fail");
    FILE* fp = fopen(out_path.c_str(), "wb");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);
    if(res!=CURLE_OK) throw std::runtime_error("curl download error");
}

void ungzip(const std::string& src, const std::string& dst) {
    gzFile in = gzopen(src.c_str(), "rb");
    std::ofstream out(dst, std::ios::binary);
    char buf[4096];
    int len;
    while ((len = gzread(in, buf, sizeof(buf)))>0) out.write(buf, len);
    gzclose(in);
}

int read_int(std::ifstream& f) {
    unsigned char buf[4];
    f.read((char*)buf,4);
    return (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
}

} // namespace

void init_mnist(const std::string& root) {
    mkdir(root.c_str(), 0755);
    for(int i=0;i<4;i++){
        std::string gz  = root+"/"+names[i];
        std::string dst = gz.substr(0, gz.size()-3);
        if(!exists(dst)) {
            if(!exists(gz)) download(urls[i], gz);
            ungzip(gz, dst);
        }
    }
}
