#include <iostream>
// #include <Eigen/Dense>
#include "math.hpp"

int main() {
    Eigen::VectorXd pred(3);
    pred << 1.1, 2.3, 2.8; // Predicted values

    Eigen::VectorXd act(3);
    act << 1.0, 2.0, 3.0; // Actual values

    try {
        double mse = Math::mean_square_error(pred, act);
        std::cout << "Predicted: [" << pred.transpose() << "]" << std::endl;
        std::cout << "Actual:    [" << act.transpose() << "]" << std::endl;
        std::cout << "MSE: " << mse << std::endl; // Expected: ((0.1)^2 + (0.3)^2 + (-0.2)^2) / 3 = (0.01 + 0.09 + 0.04) / 3 = 0.14 / 3 = 0.04666...

        // Example with empty vectors
        Eigen::VectorXd empty_pred(0);
        Eigen::VectorXd empty_act(0);
        double mse_empty = Math::mean_square_error(empty_pred, empty_act);
        std::cout << "MSE (empty): " << mse_empty << std::endl; // Expected: 0.0

        // Example that throws an error
        Eigen::VectorXd wrong_size(2);
        wrong_size << 1, 2;
        // This next line will throw std::invalid_argument
        // double mse_error = mean_square_error(pred, wrong_size);

    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }


    return 0;
}