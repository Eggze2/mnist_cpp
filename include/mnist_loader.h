#ifndef MNIST_LOADER_H
#define MNIST_LOADER_H

#include <Eigen/Dense>
#include <string>

// 若本地不存在数据集则下载并解压，force_download参数可强制重新下载
void init_mnist(const std::string& root = "./data", bool force_download = false);

// MNIST数据集结构
struct MnistData {
    Eigen::MatrixXf train_images; // 60000 x 784 (28*28)
    Eigen::VectorXi train_labels; // 60000
    Eigen::MatrixXf test_images;  // 10000 x 784 (28*28)
    Eigen::VectorXi test_labels;  // 10000
    
    int train_count = 0;
    int test_count = 0;
    int rows = 28;
    int cols = 28;
};

// 加载MNIST数据集
MnistData load_mnist(const std::string& root = "./data", bool normalize = true);

// 批处理加载器
class MnistBatchLoader {
public:
    MnistBatchLoader(const MnistData& data, int batch_size);
    
    // 获取下一批训练数据
    bool next_train_batch(Eigen::MatrixXf& batch_x, Eigen::VectorXi& batch_y);
    
    // 获取下一批测试数据
    bool next_test_batch(Eigen::MatrixXf& batch_x, Eigen::VectorXi& batch_y);
    
    // 重置批次索引
    void reset();
    
private:
    const MnistData& data;
    int batch_size;
    int train_index;
    int test_index;
};

#endif // MNIST_LOADER_H