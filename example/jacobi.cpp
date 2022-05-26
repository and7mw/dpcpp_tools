#include <iostream>
#include <vector>
#include <random>
#include <cassert>
#include <algorithm>
#include <memory>
#include <chrono>

#include <CL/sycl.hpp>

class Matrix {
    public:
        Matrix() = default;

        Matrix(const size_t newN) : N(newN) {
            matrix.resize(N * N);

            std::uniform_real_distribution<float> distr(-10.f, 10.f);
            std::mt19937 gen;
            for (size_t i = 0; i < matrix.size(); i++) {
                matrix[i] = distr(gen);
            }

            for (size_t i = 0; i < N; i++) {
                float absSum = 0.0f;
                for (size_t j = 0; j < N; j++) {
                    if (j == i) {
                        continue;
                    }
                    absSum += std::abs(matrix[i * N + j]);
                }
                matrix[i * N + i] = absSum + 15.0f;
            }
        }

        Matrix(const std::vector<float> &copyMatrix) : matrix(copyMatrix) {
            N = std::sqrt(copyMatrix.size());
        }

        const size_t getSize() const {
            return N;
        }

        const std::vector<float> &getBuffer() const {
            return matrix;
        }

        const std::vector<float> &getTransposeMartix() const {
            if (!transposeMatrix) {
                std::vector<float> result(matrix.size());
                for (size_t i = 0; i < N; i++) {
                    for (size_t j = 0; j < N; j++) {
                        result[i * N + j] = matrix[j * N + i];
                    }
                }
                transposeMatrix = std::make_unique<Matrix>(result);
            }
            return transposeMatrix->getBuffer();
        }

        void print() const {
            for (size_t i = 0; i < N; i++) {
                for (size_t j = 0; j < N; j++) {
                    std::cout << matrix[i * N + j] << " ";
                }
                std::cout << std::endl;
            }
        }

    private:
        size_t N;
        std::vector<float> matrix;
        mutable std::unique_ptr<Matrix> transposeMatrix;
};

class Equatation {
    public:
        Equatation(const size_t N) : A(N) {
            std::uniform_real_distribution<float> distr(-10.f, 10.f);
            std::mt19937 gen;

            B.resize(N);
            for (size_t i = 0; i < B.size(); i++) {
                B[i] = distr(gen);
            }
        }

        const Matrix &getA() const {
            return A;
        }

        const std::vector<float> &getB() const {
            return B;
        }

        float checkAccuracy(const std::vector<float>& x, const float accuracy) const {
            if (x.size() != B.size()) {
                throw std::runtime_error("X size incompatible with equatation");
            }

            const auto &aBuffer = A.getBuffer();
            size_t size = x.size();
            std::vector<float> eps(size);

            for (size_t i = 0; i < size; ++i) {
                float sum = 0.0f;
                for (size_t j = 0; j < size; ++j) {
                    sum += aBuffer[i * size + j] * x[j];
                }
                if (sum > accuracy) {
                    eps[i] = std::abs((sum - B[i]) / sum);
                } else if (B[i] > accuracy) {
                    eps[i] = std::abs((sum - B[i]) / B[i]);
                } else {
                    eps[i] = 0.0f;
                }
            }

            float avg = 0.0f;
            for (auto value : eps) {
                avg += value;
            }
            return avg / eps.size();
        }

    private:
        Matrix A;
        std::vector<float> B;
};

std::vector<float> JacobiSolveAccessor(const Equatation &eq, const size_t numIter, const float accuracy,
                                       const sycl::device_selector &selector) {
    constexpr size_t NUM_WORK_ITEM_PER_GROUP = 16;

    const size_t N = eq.getA().getSize();
    const auto &transposeA = eq.getA().getTransposeMartix();
    const auto &B = eq.getB();

    std::vector<float> X(N, 0.0f);
    std::vector<float> X_prev(N, 0.0f);
    for (size_t i = 0; i < N; i++) {
        X_prev[i] = B[i] / transposeA[i * N + i];
    }

    const size_t NUM_WORK_GROUP = (N + NUM_WORK_ITEM_PER_GROUP - 1) / NUM_WORK_ITEM_PER_GROUP;
    const size_t NUM_WORK_ITEM = NUM_WORK_GROUP * NUM_WORK_ITEM_PER_GROUP;
    std::vector<float> errorVector(NUM_WORK_GROUP, 0.0f);

    size_t iter = 0;

    try {
        sycl::queue queue{selector};

        std::cout << "Target device: "
                  << queue.get_info<sycl::info::queue::device>().get_info<sycl::info::device::name>() << std::endl;

        {
            float error = std::numeric_limits<float>::max();
            // while (error > accuracy && iter < numIter) {
            sycl::buffer<float, 1> A_matrix(transposeA.data(), transposeA.size());
            sycl::buffer<float, 1> B_vector(B.data(), B.size());
            sycl::buffer<float, 1> X_vector_prev(X_prev.data(), X_prev.size());
            sycl::buffer<float, 1> X_vector(X.data(), X.size());

            while (error > accuracy && iter < numIter) {
                {
                sycl::buffer<float, 1> err_vector(errorVector.data(), errorVector.size());

                    queue.submit([&](sycl::handler &cgh) {
                        auto A_acc = A_matrix.get_access<sycl::access::mode::read>(cgh);
                        auto B_acc = B_vector.get_access<sycl::access::mode::read>(cgh);
                        auto X_prev_acc = X_vector_prev.get_access<sycl::access::mode::read>(cgh);
                        auto X_acc = X_vector.get_access<sycl::access::mode::write>(cgh);
                        auto error_acc = err_vector.get_access<sycl::access::mode::write>(cgh);
// error_acc.get_offset
                        cgh.parallel_for<class JacobiIter>(sycl::nd_range<1>(sycl::range<1>(NUM_WORK_ITEM),
                                                                             sycl::range<1>(NUM_WORK_ITEM_PER_GROUP)),
                                [=](sycl::nd_item<1> item) {
                            const auto idx = item.get_global_id(0);

                            float error = 0.0f;
                            if (idx < N) {
                                // local variable?
                                float acc = B_acc[idx];
                                for (size_t i = 0; i < N; i++) {
                                    if (i != idx) {
                                        acc -= A_acc[i * N + idx] * X_prev_acc[i];
                                    }
                                }
                                acc /= A_acc[idx * N + idx];
                                error = fabs(acc - X_prev_acc[idx]) / fabs(X_prev_acc[idx]);
                                X_acc[idx] = acc;
                            }

                            float groupError = sycl::ONEAPI::reduce(item.get_group(), error, sycl::ONEAPI::maximum<float>());
                            if (item.get_local_linear_id() == 0) {
                                error_acc[item.get_group_linear_id()] = groupError;
                            }
                        });

                    });
                    queue.wait_and_throw();

                    queue.submit([&](sycl::handler &cgh) {
                        auto X_acc = X_vector.get_access<sycl::access::mode::read>(cgh);
                        auto X_prev_acc = X_vector_prev.get_access<sycl::access::mode::write>(cgh);

                        cgh.parallel_for<class Copy>(sycl::range<1>(N), [=](sycl::nd_item<1> item) {
                            const auto idx = item.get_global_id(0);
                            X_prev_acc[idx] = X_acc[idx];
                        });

                    });
                    queue.wait_and_throw();
                }

                iter++;
                error = *std::max_element(errorVector.begin(), errorVector.end());
            }
        }
    } catch(sycl::exception ex) {
        std::string message = std::string("SYCL error: ") + ex.what();
        std::cout << message << std::endl;
        throw message;
    }
    return X;
}

std::vector<float> JacobiSolveDevice(const Equatation &eq, const size_t numIter, const float accuracy,
                                     const sycl::device_selector &selector) {
    constexpr size_t NUM_WORK_ITEM_PER_GROUP = 16;

    const size_t N = eq.getA().getSize();
    const auto &transposeA = eq.getA().getTransposeMartix();
    const auto &B = eq.getB();

    std::vector<float> X(N, 0.0f);
    std::vector<float> X_prev(N, 0.0f);
    for (size_t i = 0; i < N; i++) {
        X_prev[i] = B[i] / transposeA[i * N + i];
    }

    const size_t NUM_WORK_GROUP = (N + NUM_WORK_ITEM_PER_GROUP - 1) / NUM_WORK_ITEM_PER_GROUP;
    const size_t NUM_WORK_ITEM = NUM_WORK_GROUP * NUM_WORK_ITEM_PER_GROUP;
    std::vector<float> errorVector(NUM_WORK_GROUP, 0.0f);

    size_t iter = 0;

    try {
        sycl::queue queue{selector};

        float *A_matrix = sycl::malloc_device<float>(transposeA.size(), queue);
        float *B_vector = sycl::malloc_device<float>(B.size(), queue);
        float *X_vector_prev = sycl::malloc_device<float>(X_prev.size(), queue);
        float *X_vector = sycl::malloc_device<float>(X.size(), queue);

        queue.memcpy(A_matrix, transposeA.data(), transposeA.size() * sizeof(float)).wait();
        queue.memcpy(B_vector, B.data(), B.size() * sizeof(float)).wait();
        queue.memcpy(X_vector_prev, X_prev.data(), X_prev.size() * sizeof(float)).wait();
        queue.memcpy(X_vector, X.data(), X.size() * sizeof(float)).wait();

        float *err_vector = sycl::malloc_device<float>(errorVector.size(), queue);
        queue.memcpy(err_vector, errorVector.data(), errorVector.size() * sizeof(float)).wait();

        std::cout << "Target device: "
                  << queue.get_info<sycl::info::queue::device>().get_info<sycl::info::device::name>() << std::endl;

        float error = std::numeric_limits<float>::max();
        while (error > accuracy && iter < numIter) {
            queue.submit([&](sycl::handler &cgh) {
                cgh.parallel_for<class JacobiIter>(sycl::nd_range<1>(sycl::range<1>(NUM_WORK_ITEM),
                                                                     sycl::range<1>(NUM_WORK_ITEM_PER_GROUP)),
                        [=](sycl::nd_item<1> item) {
                    const auto idx = item.get_global_id(0);
                    float error = 0.0f;
                    if (idx < N) {
                        // local variable?
                        float acc = B_vector[idx];
                        for (size_t i = 0; i < N; i++) {
                            if (i != idx) {
                                acc -= A_matrix[i * N + idx] * X_vector_prev[i];
                            }
                        }
                        acc /= A_matrix[idx * N + idx];
                        error = fabs(acc - X_vector_prev[idx]) / fabs(X_vector_prev[idx]);
                        X_vector[idx] = acc;
                    }
                    float groupError = sycl::ONEAPI::reduce(item.get_group(), error, sycl::ONEAPI::maximum<float>());
                    if (item.get_local_linear_id() == 0) {
                        err_vector[item.get_group_linear_id()] = groupError;
                    }
                });
            });
            queue.wait_and_throw();

            queue.submit([&](sycl::handler &cgh) {
                cgh.parallel_for<class Copy>(sycl::range<1>(N), [=](sycl::nd_item<1> item) {
                    const auto idx = item.get_global_id(0);
                    X_vector_prev[idx] = X_vector[idx];
                });
            });
            queue.wait_and_throw();
            iter++;
            queue.memcpy(errorVector.data(), err_vector, errorVector.size() * sizeof(float)).wait();
            error = *std::max_element(errorVector.begin(), errorVector.end());
        }
        queue.memcpy(X.data(), X_vector, X.size() * sizeof(float)).wait();

        sycl::free(A_matrix, queue);
        sycl::free(B_vector, queue);
        sycl::free(X_vector_prev, queue);
        sycl::free(X_vector, queue);
        sycl::free(err_vector, queue);
    } catch(sycl::exception ex) {
        std::string message = std::string("SYCL error: ") + ex.what();
        std::cout << message << std::endl;
        throw message;
    }
    return X;
}

std::vector<float> JacobiSolveShared(const Equatation &eq, const size_t numIter, const float accuracy,
                                     const sycl::device_selector &selector) {
    constexpr size_t NUM_WORK_ITEM_PER_GROUP = 16;

    const size_t N = eq.getA().getSize();
    const auto &transposeA = eq.getA().getTransposeMartix();
    const auto &B = eq.getB();

    std::vector<float> X(N, 0.0f);
    std::vector<float> X_prev(N, 0.0f);
    for (size_t i = 0; i < N; i++) {
        X_prev[i] = B[i] / transposeA[i * N + i];
    }

    const size_t NUM_WORK_GROUP = (N + NUM_WORK_ITEM_PER_GROUP - 1) / NUM_WORK_ITEM_PER_GROUP;
    const size_t NUM_WORK_ITEM = NUM_WORK_GROUP * NUM_WORK_ITEM_PER_GROUP;

    size_t iter = 0;

    try {
        sycl::queue queue{selector};

        float *A_matrix = sycl::malloc_shared<float>(transposeA.size(), queue);
        float *B_vector = sycl::malloc_shared<float>(B.size(), queue);
        float *X_vector_prev = sycl::malloc_shared<float>(X_prev.size(), queue);
        float *X_vector = sycl::malloc_shared<float>(X.size(), queue);

        memcpy(A_matrix, transposeA.data(), transposeA.size() * sizeof(float));
        memcpy(B_vector, B.data(), B.size() * sizeof(float));
        memcpy(X_vector_prev, X_prev.data(), X_prev.size() * sizeof(float));

        float *err_vector = sycl::malloc_shared<float>(NUM_WORK_GROUP, queue);

        std::cout << "Target device: "
                  << queue.get_info<sycl::info::queue::device>().get_info<sycl::info::device::name>() << std::endl;

        float error = std::numeric_limits<float>::max();
        while (error > accuracy && iter < numIter) {
            queue.submit([&](sycl::handler &cgh) {
                cgh.parallel_for<class JacobiIter>(sycl::nd_range<1>(sycl::range<1>(NUM_WORK_ITEM),
                                                                     sycl::range<1>(NUM_WORK_ITEM_PER_GROUP)),
                        [=](sycl::nd_item<1> item) {
                    const auto idx = item.get_global_id(0);
                    float error = 0.0f;
                    if (idx < N) {
                        // local variable?
                        float acc = B_vector[idx];
                        for (size_t i = 0; i < N; i++) {
                            if (i != idx) {
                                acc -= A_matrix[i * N + idx] * X_vector_prev[i];
                            }
                        }
                        acc /= A_matrix[idx * N + idx];
                        error = fabs(acc - X_vector_prev[idx]) / fabs(X_vector_prev[idx]);
                        X_vector[idx] = acc;
                    }
                    float groupError = sycl::ONEAPI::reduce(item.get_group(), error, sycl::ONEAPI::maximum<float>());
                    if (item.get_local_linear_id() == 0) {
                        err_vector[item.get_group_linear_id()] = groupError;
                    }
                });
            });
            queue.wait_and_throw();

            queue.submit([&](sycl::handler &cgh) {
                cgh.parallel_for<class Copy>(sycl::range<1>(N), [=](sycl::nd_item<1> item) {
                    const auto idx = item.get_global_id(0);
                    X_vector_prev[idx] = X_vector[idx];
                });
            });
            queue.wait_and_throw();
            iter++;
            error = err_vector[0];
            for (size_t i = 0; i < NUM_WORK_GROUP; i++) {
                if (err_vector[i] > error) {
                    error = err_vector[i];
                }
            }
        }
        memcpy(X.data(), X_vector, X.size() * sizeof(float));

        sycl::free(A_matrix, queue);
        sycl::free(B_vector, queue);
        sycl::free(X_vector_prev, queue);
        sycl::free(X_vector, queue);
        sycl::free(err_vector, queue);
    } catch(sycl::exception ex) {
        std::string message = std::string("SYCL error: ") + ex.what();
        std::cout << message << std::endl;
        throw message;
    }
    return X;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cout << "Please specify: number equatation, accuracy, max number iteration and device\n";
        return -1;
    }
    const size_t N = static_cast<size_t>(atoi(argv[1]));
    const float accuracy = std::stof(argv[2]);
    const size_t iterNum = static_cast<size_t>(atoi(argv[3]));
    const std::string preferDevice = std::string(argv[4]);

    std::unique_ptr<sycl::device_selector> selector(nullptr);
    if (preferDevice == "CPU") {
        selector.reset(new sycl::cpu_selector);
    } else if (preferDevice == "GPU") {
        selector.reset(new sycl::gpu_selector);
    } else {
        std::cout << "Prefered device value is incorrect!";
        return -1;
    }

    try {
        const auto equatation = Equatation(N);

        auto start = std::chrono::high_resolution_clock::now();
        auto X = JacobiSolveAccessor(equatation, iterNum, accuracy, *selector);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[Accessors] ";
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms "
                  << "Accuracy: " << equatation.checkAccuracy(X, accuracy) << std::endl;

        // start = std::chrono::high_resolution_clock::now();
        // X = JacobiSolveDevice(equatation, iterNum, accuracy, *selector);
        // end = std::chrono::high_resolution_clock::now();
        // std::cout << "[Device] ";
        // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms "
        //           << "Accuracy: " << equatation.checkAccuracy(X, accuracy) << std::endl;

        // start = std::chrono::high_resolution_clock::now();
        // X = JacobiSolveShared(equatation, iterNum, accuracy, *selector);
        // end = std::chrono::high_resolution_clock::now();
        // std::cout << "[Shared] ";
        // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms "
        //           << "Accuracy: " << equatation.checkAccuracy(X, accuracy) << std::endl;
    } catch(std::exception ex) {
        std::cout << ex.what() << std::endl;
    } catch(...) {
        std::cout << "Genral error" << std::endl;
    }

    return 0;
}
