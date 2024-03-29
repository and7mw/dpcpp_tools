#include <iostream>
#include <vector>

#include <CL/sycl.hpp>

int main(int argc, char* argv[]) {
    std::unique_ptr<sycl::device_selector> selector(new sycl::gpu_selector);

    try {
        sycl::queue queue{*selector};

        std::cout << "Target device: "
              << queue.get_info<sycl::info::queue::device>().get_info<sycl::info::device::name>() << std::endl;

        constexpr size_t size = 32;

        int A_val = 1, B_val = 2, C_val = 5;

        std::vector<int> A(size, A_val), B(size, B_val), C(size, C_val), result(size);

        const size_t iter_num = 1;

        sycl::buffer<int, 1> A_buffer(A.data(), A.size());
        sycl::buffer<int, 1> B_buffer(B.data(), B.size());
        sycl::buffer<int, 1> C_buffer(C.data(), C.size());
        sycl::buffer<int, 1> result_buffer(result.data(), result.size());

        sycl::buffer<int, 1> sub_res(size);

        for (size_t i = 0; i < iter_num; i++) {
            // Sub
            queue.submit([&](sycl::handler &cgh) {
                auto B_acc = B_buffer.get_access<sycl::access::mode::read>(cgh);
                auto C_acc = C_buffer.get_access<sycl::access::mode::read>(cgh);

                auto sub_res_acc = sub_res.get_access<sycl::access::mode::write>(cgh);

                cgh.parallel_for<class Sub>(sycl::range<1>(size), [=](sycl::id<1> idx) {
                    sub_res_acc[idx] = B_acc[idx] - C_acc[idx];
                });

            });
            // Add
            queue.submit([&](sycl::handler &cgh) {
                auto A_acc = A_buffer.get_access<sycl::access::mode::read>(cgh);
                auto sub_res_acc = sub_res.get_access<sycl::access::mode::read>(cgh);

                auto result_acc = result_buffer.get_access<sycl::access::mode::write>(cgh);

                cgh.parallel_for<class Add>(sycl::range<1>(size), [=](sycl::id<1> idx) {
                    result_acc[idx] = A_acc[idx] + sub_res_acc[idx];
                });
            });
            queue.wait_and_throw();
        }
    } catch(sycl::exception ex) {
        std::cout << "SYCL error: " << ex.what() << std::endl;
    } catch (std::exception ex) {
        std::cout << "Failed with message: " << ex.what() << std::endl;
    } catch (...) {
        std::cout << "Unexpected failure!" << std::endl;
    }

    return 0;
}
