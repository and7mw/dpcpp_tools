#include <iostream>
#include <vector>

#include <CL/sycl.hpp>

int main(int argc, char* argv[]) {
    std::unique_ptr<sycl::device_selector> selector(new sycl::gpu_selector);

    try {
        sycl::queue queue{*selector};

        std::cout << "Target device: "
              << queue.get_info<sycl::info::queue::device>().get_info<sycl::info::device::name>() << std::endl;

        constexpr size_t size = 1000000;

        int A_val = 1, B_val = 2, C_val = 5;

        std::vector<int> A(size, A_val), B(size, B_val), C(size, C_val), result(size);

        // std::vector<int> add(size), sub(size);

        // sub_res = (B - C)
        // add_res = A + sub_res
        // res = sub_res * add_res
        const int tmp_sub = (B_val - C_val);
        const int expected = (A_val + tmp_sub) * tmp_sub;

        // sycl::program program_sub(queue.get_context());
        // program_sub.build_with_kernel_type<class Sub>();
        // sycl::kernel kernelSub = program_sub.get_kernel<class Sub>();

        sycl::program program_add(queue.get_context());
        program_add.build_with_kernel_type<class Add>();
        sycl::kernel kernelAdd = program_add.get_kernel<class Add>();

        const size_t iter_num = 2;
        for (size_t i = 0; i < iter_num; i++)
        {
            sycl::buffer<int, 1> A_buffer(A.data(), A.size());
            sycl::buffer<int, 1> B_buffer(B.data(), B.size());
            sycl::buffer<int, 1> C_buffer(C.data(), C.size());
            sycl::buffer<int, 1> result_buffer(result.data(), result.size());

            sycl::buffer<int, 1> sub_res(size);
            sycl::buffer<int, 1> add_res(size);
            // sycl::buffer<int, 1> sub_res(sub.data(), sub.size());
            // sycl::buffer<int, 1> add_res(add.data(), add.size());

        // for (size_t i = 0; i < iter_num; i++) {
            // Sub
            auto start = std::chrono::high_resolution_clock::now();
            // queue.submit([&](sycl::handler &cgh) {
            //     auto B_acc = B_buffer.get_access<sycl::access::mode::read>(cgh);
            //     auto C_acc = C_buffer.get_access<sycl::access::mode::read>(cgh);

            //     auto sub_res_acc = sub_res.get_access<sycl::access::mode::write>(cgh);

            //     cgh.parallel_for<class Sub>(kernelSub, sycl::range<1>(size), [=](sycl::id<1> idx) {
            //         sub_res_acc[idx] = B_acc[idx] - C_acc[idx];
            //     });

            // });
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "SUB time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

            // Add
            start = std::chrono::high_resolution_clock::now();
            queue.submit([&](sycl::handler &cgh) {
                auto A_acc = A_buffer.get_access<sycl::access::mode::read>(cgh);
                auto sub_res_acc = sub_res.get_access<sycl::access::mode::read>(cgh);

                auto add_res_acc = add_res.get_access<sycl::access::mode::write>(cgh);

                cgh.parallel_for<class Add>(kernelAdd, sycl::range<1>(size), [=](sycl::id<1> idx) {
                    add_res_acc[idx] = A_acc[idx] + sub_res_acc[idx];
                });
            });
            end = std::chrono::high_resolution_clock::now();
            std::cout << "ADD time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
            
            // Mult
            start = std::chrono::high_resolution_clock::now();
            queue.submit([&](sycl::handler &cgh) {
                auto sub_res_acc = sub_res.get_access<sycl::access::mode::read>(cgh);
                auto add_res_acc = add_res.get_access<sycl::access::mode::read>(cgh);

                auto result_acc = result_buffer.get_access<sycl::access::mode::write>(cgh);

                cgh.parallel_for<class Mul>(sycl::range<1>(size), [=](sycl::id<1> idx) {
                    result_acc[idx] = add_res_acc[idx] * sub_res_acc[idx];
                });
            });
            end = std::chrono::high_resolution_clock::now();
            std::cout << "MULT time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
            queue.wait_and_throw();
        }

        // for (size_t i = 0; i < result.size(); i++) {
        //     if (result[i] != expected) {
        //         std::cout << "Unexpected result on " << std::to_string(i) << " position!"
        //                   << " Expected: " << expected << " Actual: " << result[i] << std::endl;
        //         return 1;
        //     }
        // }
    } catch(sycl::exception ex) {
        std::cout << "SYCL error: " << ex.what() << std::endl;
    } catch (std::exception ex) {
        std::cout << "Failed with message: " << ex.what() << std::endl;
    } catch (...) {
        std::cout << "Unexpected failure!" << std::endl;
    }

    return 0;
}
