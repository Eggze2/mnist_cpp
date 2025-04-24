#include "mnist_loader.h"
#include <curl/curl.h>
#include <zlib.h>
#include <fstream>
#include <sys/stat.h> // For stat and mkdir
#ifdef _WIN32
#include <direct.h> // For _mkdir on Windows
#define mkdir(path, mode) _mkdir(path)
#include <cerrno> // Include for errno on Windows
#endif
#include <vector>
#include <string> // Make sure string is included
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cstdio> // For std::remove
#include <vector> // Include vector for base_urls

namespace {

// Define base URLs for downloading. Ensure they end with a slash.
const std::vector<std::string> base_urls = {
    "https://storage.googleapis.com/cvdf-datasets/mnist/", // Primary Google Cloud
    "https://ossci-datasets.s3.amazonaws.com/mnist/"     // Secondary AWS S3
};

// Filenames remain the same
const char* names[] = {
    "train-images-idx3-ubyte.gz",
    "train-labels-idx1-ubyte.gz",
    "t10k-images-idx3-ubyte.gz",
    "t10k-labels-idx1-ubyte.gz"
};

// --- Helper Functions (exists, write_file, download, ungzip, read_int, delete_if_exists, verify_magic_number) ---
// These functions remain largely the same as the previous refined version.
// Make sure they are present here. I'll include them for completeness.

// Check if a file or directory exists
bool exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

// Callback function for curl to write data to a file
size_t write_file(void* ptr, size_t size, size_t nmemb, void* stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
    // Check for write errors
    if (written < nmemb && ferror((FILE*)stream)) {
        return 0; // Indicate error to curl
    }
    return written;
}

// Downloads a file from a URL to a specified path
void download(const std::string& url, const std::string& out_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("curl_easy_init() failed");
    }

    // RAII handle for CURL cleanup
    struct CurlCleaner {
        CURL* curl_handle;
        CurlCleaner(CURL* h) : curl_handle(h) {}
        ~CurlCleaner() { if (curl_handle) curl_easy_cleanup(curl_handle); }
    } curl_cleaner(curl);

    FILE* fp = fopen(out_path.c_str(), "wb");
    if (!fp) {
        throw std::runtime_error("Failed to create output file: " + out_path);
    }

    // RAII handle for file closing
    struct FileCloser {
        FILE* file_handle;
        FileCloser(FILE* h) : file_handle(h) {}
        ~FileCloser() { if (file_handle) fclose(file_handle); }
    } file_closer(fp);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-mnist-downloader/1.1"); // Version bump
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 90L);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Debugging

    std::cout << "Attempting download from: " << url << " ..." << std::endl;
    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // Ensure file is closed before checking results or throwing
    file_closer.file_handle = nullptr;
    if (fclose(fp) != 0) {
         // Warning or error if closing failed
         std::cerr << "Warning: fclose failed for " << out_path << std::endl;
         // Depending on severity, could throw here
    }
    fp = nullptr; // Mark as closed


    if (res != CURLE_OK) {
        std::remove(out_path.c_str()); // Clean up partial file
        throw std::runtime_error("Download failed: " + std::string(curl_easy_strerror(res)) +
                                 " (URL: " + url +
                                 ", CURLcode: " + std::to_string(res) +
                                 ", HTTP code: " + std::to_string(http_code) + ")");
    }

    // Basic gzip header check
    std::ifstream file(out_path, std::ios::binary);
    if (!file.is_open()) {
         std::remove(out_path.c_str()); // Clean up file if can't verify
        throw std::runtime_error("Failed to open downloaded file for verification: " + out_path);
    }
    unsigned char header[2];
    file.read(reinterpret_cast<char*>(header), 2);
    bool is_gzip = (file.gcount() == 2 && header[0] == 0x1f && header[1] == 0x8b);
    file.close();

    if (!is_gzip) {
        std::remove(out_path.c_str()); // Clean up invalid file
        throw std::runtime_error("Downloaded file is not in valid gzip format: " + out_path + " from URL: " + url);
    }
     std::cout << "Download complete and verified (gzip header OK) for " << out_path << std::endl;
}

// Decompresses a gzipped file
void ungzip(const std::string& src_path, const std::string& dst_path) {
    gzFile in = gzopen(src_path.c_str(), "rb");
    if (!in) {
        throw std::runtime_error("Failed to open gzip file for reading: " + src_path);
    }

    struct GzCloser {
        gzFile file_handle;
        GzCloser(gzFile h) : file_handle(h) {}
        ~GzCloser() { if (file_handle) gzclose(file_handle); }
    } gz_closer(in);

    std::ofstream out(dst_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to open output file for writing: " + dst_path);
    }

    char buffer[4096];
    int len;
    std::cout << "Decompressing " << src_path << " to " << dst_path << " ..." << std::endl;
    while ((len = gzread(in, buffer, sizeof(buffer))) > 0) {
        out.write(buffer, len);
        if (!out) {
             throw std::runtime_error("Error writing to output file: " + dst_path);
        }
    }

     if (len < 0) {
        int err_no = 0;
        const char *err_msg = gzerror(in, &err_no);
        gz_closer.file_handle = nullptr; // Prevent double close in destructor
        gzclose(in);
        throw std::runtime_error("Error during decompression of " + src_path + ": " + std::string(err_msg) +
                                 " (zlib error code: " + std::to_string(err_no) + ")");
    }

    out.close();
     if (!out) {
         throw std::runtime_error("Error closing output file after writing: " + dst_path);
     }

    std::cout << "Decompression complete for " << dst_path << std::endl;
}

// Reads a 32-bit big-endian integer
int read_int(std::ifstream& f) {
    unsigned char buf[4];
    f.read(reinterpret_cast<char*>(buf), 4);
    if (f.gcount() != 4) {
        throw std::runtime_error("Failed to read 4 bytes for integer, file may be truncated or corrupt.");
    }
    return (static_cast<int>(buf[0]) << 24) | (static_cast<int>(buf[1]) << 16) |
           (static_cast<int>(buf[2]) << 8) | static_cast<int>(buf[3]);
}

// Deletes a file if it exists
void delete_if_exists(const std::string& path) {
    if (exists(path)) {
        if (std::remove(path.c_str()) != 0) {
            std::cerr << "Warning: Failed to delete existing file: " << path << " (errno: " << errno << ")" << std::endl;
        } else {
            std::cout << "Deleted existing file: " << path << std::endl;
        }
    }
}

// Verify the magic number
void verify_magic_number(const std::string& file_path, bool is_image_file) {
    std::ifstream file(file_path, std::ios::binary);
     if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for verification: " + file_path);
    }

    int magic = 0;
    try {
        magic = read_int(file);
    } catch (const std::runtime_error& e) {
        file.close();
        throw std::runtime_error("Failed to read magic number from " + file_path + ": " + e.what());
    }
    file.close();

    int expected_magic = is_image_file ? 2051 : 2049;
    if (magic != expected_magic) {
        throw std::runtime_error("Invalid magic number in " + file_path +
                                 ". Expected: " + std::to_string(expected_magic) +
                                 ", Got: " + std::to_string(magic));
    }
    std::cout << "Magic number verified for " << file_path << std::endl;
}


} // anonymous namespace

// Initializes MNIST data: creates directory, downloads (trying sources), and unzips if necessary.
void init_mnist(const std::string& root, bool force_download) {
    // Create root directory
    if (!exists(root)) {
        #ifdef _WIN32
            if (_mkdir(root.c_str()) != 0 && errno != EEXIST) {
                 throw std::runtime_error("Failed to create directory: " + root + " (errno: " + std::to_string(errno) + ")");
            }
        #else
            if (mkdir(root.c_str(), 0755) != 0 && errno != EEXIST) {
                throw std::runtime_error("Failed to create directory: " + root + " (errno: " + std::to_string(errno) + ")");
            }
        #endif
         std::cout << "Created directory: " << root << std::endl;
    }
    std::cout << "Initializing MNIST dataset in: " << root << std::endl;

    if (base_urls.empty()) {
         throw std::runtime_error("No download base URLs provided.");
    }

    for (int i = 0; i < 4; ++i) {
        std::string filename = names[i];
        std::string gz_path = root + "/" + filename;
        std::string dst_path = gz_path.substr(0, gz_path.size() - 3);

        if (force_download) {
            std::cout << "Force download requested for " << filename << "." << std::endl;
            delete_if_exists(gz_path);
            delete_if_exists(dst_path);
        }

        // Check if the final unzipped file exists and is valid
        bool dst_exists_and_valid = false;
        if (exists(dst_path)) {
             try {
                 bool is_image = filename.find("images") != std::string::npos;
                 verify_magic_number(dst_path, is_image);
                 std::cout << "Dataset file already exists and is valid: " << dst_path << std::endl;
                 dst_exists_and_valid = true;
             } catch (const std::exception& e) {
                  std::cerr << "Warning: Existing file " << dst_path << " failed verification: " << e.what() << std::endl;
                  std::cerr << "Will attempt to re-download and unzip." << std::endl;
                  delete_if_exists(dst_path); // Delete invalid existing file
                  delete_if_exists(gz_path);  // Delete corresponding gz too, likely corrupt source
             }
        }


        // If destination file doesn't exist or wasn't valid, proceed with download/unzip
        if (!dst_exists_and_valid) {
            // Download phase: Try sources until one works or all fail
            bool downloaded = false;
            if (!exists(gz_path)) { // Only download if gz doesn't exist
                std::cout << "Compressed file not found: " << gz_path << ". Starting download..." << std::endl;
                for (const auto& base_url : base_urls) {
                    std::string full_url = base_url + filename;
                    try {
                        download(full_url, gz_path);
                        downloaded = true;
                        break; // Success! Stop trying other sources
                    } catch (const std::exception& e) {
                        std::cerr << "Download attempt failed from " << base_url << ": " << e.what() << std::endl;
                        // Cleanup potentially failed partial download before trying next source
                        delete_if_exists(gz_path);
                    }
                }
                // If loop finishes without success
                if (!downloaded) {
                    throw std::runtime_error("Failed to download " + filename + " from all provided sources.");
                }
            } else {
                 std::cout << "Compressed file already exists: " << gz_path << ". Skipping download." << std::endl;
                 downloaded = true; // Treat existing gz as successfully "downloaded" for the next step
            }


            // Unzip phase (only if download was successful or gz existed)
            if (downloaded) {
                 try {
                    ungzip(gz_path, dst_path);
                    // Verify after unzipping
                    bool is_image = filename.find("images") != std::string::npos;
                    verify_magic_number(dst_path, is_image);

                    // Optional: Delete the .gz file after successful decompression and verification
                    // delete_if_exists(gz_path);

                } catch (const std::exception& e) {
                    // Clean up potentially corrupt unzipped file and rethrow
                    delete_if_exists(dst_path);
                    // Optionally delete gz file too if unzip failed badly
                    // delete_if_exists(gz_path);
                    throw std::runtime_error("Failed to process " + filename + " after download: " + e.what());
                }
            }
            // If !downloaded, we already threw an error earlier.
        }
    }
    std::cout << "MNIST dataset initialization complete." << std::endl;
}


// --- load_mnist function ---
// This function remains the same as the previous refined version.
// It calls init_mnist() first, then loads images/labels using the helper lambdas.
// Make sure it's present here. I'll include it for completeness.

// Loads the MNIST dataset from the specified root directory.
MnistData load_mnist(const std::string& root, bool normalize) {
    // Ensure data is present, downloading/unzipping if necessary using the new logic
    init_mnist(root);

    MnistData mnist;
    const int expected_rows = 28;
    const int expected_cols = 28;
    mnist.rows = expected_rows;
    mnist.cols = expected_cols;
    int img_size = mnist.rows * mnist.cols;

    auto load_images = [&](const std::string& path, Eigen::MatrixXf& matrix, int& count) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Cannot open image file: " + path);

        int magic = read_int(file);
        if (magic != 2051) throw std::runtime_error("Invalid magic number in image file: " + path);

        count = read_int(file);
        int rows = read_int(file);
        int cols = read_int(file);
        if (rows != expected_rows || cols != expected_cols) {
            throw std::runtime_error("Unexpected image dimensions in " + path +
                                     ": got " + std::to_string(rows) + "x" + std::to_string(cols) +
                                     ", expected " + std::to_string(expected_rows) + "x" + std::to_string(expected_cols));
        }

        matrix.resize(count, img_size);
        std::vector<unsigned char> buffer(img_size);
        for (int i = 0; i < count; ++i) {
            file.read(reinterpret_cast<char*>(buffer.data()), img_size);
            if (file.gcount() != img_size) throw std::runtime_error("Error reading image data for item " + std::to_string(i) + " from " + path);
            for (int j = 0; j < img_size; ++j) {
                matrix(i, j) = normalize ? buffer[j] / 255.0f : static_cast<float>(buffer[j]);
            }
        }
         if (!file.eof() && file.peek() != EOF) {
             std::cerr << "Warning: Extra data detected at the end of image file: " << path << std::endl;
         }
        std::cout << "Loaded " << count << " images from " << path << std::endl;
    };

    auto load_labels = [&](const std::string& path, Eigen::VectorXi& vector, int expected_count) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Cannot open label file: " + path);

        int magic = read_int(file);
        if (magic != 2049) throw std::runtime_error("Invalid magic number in label file: " + path);

        int count = read_int(file);
        if (count != expected_count) {
            throw std::runtime_error("Label count mismatch in " + path +
                                     ": expected " + std::to_string(expected_count) +
                                     ", got " + std::to_string(count));
        }

        vector.resize(count);
        // Read labels one by one for potentially better error reporting if needed
        // Can also read into a buffer like before for slight performance gain
        for (int i = 0; i < count; ++i) {
            unsigned char label;
             file.read(reinterpret_cast<char*>(&label), 1);
             if (file.gcount() != 1) throw std::runtime_error("Error reading label data for item " + std::to_string(i) + " from " + path);
             vector(i) = static_cast<int>(label);
        }
         if (!file.eof() && file.peek() != EOF) {
             std::cerr << "Warning: Extra data detected at the end of label file: " << path << std::endl;
         }
         std::cout << "Loaded " << count << " labels from " << path << std::endl;
    };

    // Load training data
    std::string train_img_path = root + "/train-images-idx3-ubyte";
    std::string train_lbl_path = root + "/train-labels-idx1-ubyte";
    load_images(train_img_path, mnist.train_images, mnist.train_count);
    load_labels(train_lbl_path, mnist.train_labels, mnist.train_count);

    // Load test data
    std::string test_img_path = root + "/t10k-images-idx3-ubyte";
    std::string test_lbl_path = root + "/t10k-labels-idx1-ubyte";
    load_images(test_img_path, mnist.test_images, mnist.test_count);
    load_labels(test_lbl_path, mnist.test_labels, mnist.test_count);

    std::cout << "MNIST dataset loading complete: "
              << mnist.train_count << " training samples, "
              << mnist.test_count << " test samples." << std::endl;

    return mnist;
}


// --- MnistBatchLoader Implementation ---
// This implementation remains the same as the previous version.
// Make sure it's present here.

MnistBatchLoader::MnistBatchLoader(const MnistData& data, int batch_size)
    : data(data), batch_size(batch_size > 0 ? batch_size : 1),
      train_index(0), test_index(0) {
    if (batch_size <= 0) {
        std::cerr << "Warning: Invalid batch_size " << batch_size << ". Using batch_size=1." << std::endl;
    }
}

bool MnistBatchLoader::next_train_batch(Eigen::MatrixXf& batch_x, Eigen::VectorXi& batch_y) {
    if (train_index >= data.train_count) {
        return false;
    }
    int count = std::min(batch_size, data.train_count - train_index);
    batch_x = data.train_images.block(train_index, 0, count, data.rows * data.cols);
    batch_y = data.train_labels.segment(train_index, count);
    train_index += count;
    return true;
}

bool MnistBatchLoader::next_test_batch(Eigen::MatrixXf& batch_x, Eigen::VectorXi& batch_y) {
    if (test_index >= data.test_count) {
        return false;
    }
    int count = std::min(batch_size, data.test_count - test_index);
    batch_x = data.test_images.block(test_index, 0, count, data.rows * data.cols);
    batch_y = data.test_labels.segment(test_index, count);
    test_index += count;
    return true;
}

void MnistBatchLoader::reset() {
    train_index = 0;
    test_index = 0;
    std::cout << "MnistBatchLoader reset." << std::endl;
}