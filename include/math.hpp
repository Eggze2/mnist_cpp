#include <Eigen/Dense>

class Math {
public:
    static double mean_square_error(const Eigen::VectorXd& predicted, const Eigen::VectorXd& actual) {
        const Eigen::Index n = predicted.size();
        if (n != actual.size()) {
            throw std::invalid_argument("mean_square_error: predicted and actual vectors must have the same size. "
                                        "Got predicted.size() = " + std::to_string(n) +
                                        " and actual.size() = " + std::to_string(actual.size()));
        }

        if (n == 0) {
            return 0.0;
        }

        double mse = (predicted - actual).array().square().mean();

        return mse;
    }
};