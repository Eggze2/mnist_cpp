#ifndef MNIST_LOADER_H
#define MNIST_LOADER_H

#include <Eigen/Dense>
#include <string>

// 若本地不存在数据集则下载并解压
void init_mnist(const std::string& root = "./data");

#endif // MNIST_LOADER_H