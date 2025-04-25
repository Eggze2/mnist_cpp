#ifndef MATH_HPP
#define MATH_HPP

#include <Eigen/Dense>

class Math {
public:
    /**
    * @brief 计算交叉熵误差
    * 
    * @param y 模型预测输出，形状为(batch_size, num_classes)的矩阵
    * @param t 目标标签，可以是one-hot形式(batch_size, num_classes)
    *          或标签索引形式(batch_size, 1)或(batch_size)
    * @return 交叉熵误差值
    */
    static double cross_entropy_error(const Eigen::MatrixXd& y_input, const Eigen::MatrixXd& t_input) {
        // 创建副本以便修改
        Eigen::MatrixXd y = y_input;
        Eigen::MatrixXd t = t_input;
        
        // 处理单个样本情况(一维向量)
        if (y.cols() == 1 && y.rows() > 1) {
            // 如果y是列向量，转为行向量形式
            y = y.transpose();
            t = t.transpose();
        }
        
        // 确保批处理格式(batch_size, features)
        if (y.rows() == 1 && y.cols() > 1) {
            // 单个样本的情况，重塑为(1, size)的批处理形式
            int y_size = y.cols();
            int t_size = t.cols();
            
            y.conservativeResize(1, y_size);
            t.conservativeResize(1, t_size);
        }
        
        int batch_size = y.rows();
        int num_classes = y.cols();
        
        // 存储标签索引
        Eigen::VectorXi t_idx(batch_size);

        // 判断t是否为one-hot向量(通过比较大小判断)
        if (t.cols() == y.cols()) {
            // t是one-hot形式，转换为索引形式
            for (int i = 0; i < batch_size; i++) {
                Eigen::MatrixXd::Index maxIndex;
                t.row(i).maxCoeff(&maxIndex);
                t_idx(i) = static_cast<int>(maxIndex);
            }
        } else {
            // t已经是索引形式
            for (int i = 0; i < batch_size; i++) {
                t_idx(i) = static_cast<int>(t(i, 0));
            }
        }
        
        // 计算交叉熵误差: -sum(log(y[i, t[i]])) / batch_size
        double loss = 0.0;
        for (int i = 0; i < batch_size; i++) {
            // 添加1e-7避免log(0)
            loss += std::log(y(i, t_idx(i)) + 1e-7);
        }
        
        return -loss / batch_size;
    }
};

#endif // MATH_HPP