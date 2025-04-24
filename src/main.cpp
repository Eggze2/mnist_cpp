#include <iostream>
#include "math.hpp"
#include "mnist_loader.h"

int main() {
    try {
        init_mnist("./data", false);
        
        // 加载MNIST数据集
        MnistData mnist = load_mnist();
        
        std::cout << "训练图像: " << mnist.train_count << " x " 
                  << mnist.rows << "x" << mnist.cols << std::endl;
        std::cout << "测试图像: " << mnist.test_count << " x " 
                  << mnist.rows << "x" << mnist.cols << std::endl;
        
        // 使用批处理加载器示例
        int batch_size = 100;
        MnistBatchLoader batch_loader(mnist, batch_size);
        
        Eigen::MatrixXf batch_x;
        Eigen::VectorXi batch_y;
        
        // 处理第一批数据示例
        if (batch_loader.next_train_batch(batch_x, batch_y)) {
            std::cout << "成功加载第一批训练数据: " << batch_x.rows() << " 样本" << std::endl;
            // 处理批次数据...
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

