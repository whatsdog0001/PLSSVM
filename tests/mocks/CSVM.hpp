#pragma once
#include "plssvm/CSVM.hpp"
#include "plssvm/OpenMP/OpenMP_CSVM.hpp"
#include "plssvm/typedef.hpp"
#include "gmock/gmock.h"
#include <plssvm/kernel_types.hpp>

class MockCSVM : public plssvm::CSVM {
  public:
    MockCSVM(real_t cost_, real_t epsilon_, plssvm::kernel_type kernel_, real_t degree_, real_t gamma_, real_t coef0_, bool info_) : plssvm::CSVM(cost_, epsilon_, kernel_, degree_, gamma_, coef0_, info_) {}
    MOCK_METHOD(void, load_w, (), (override));
    MOCK_METHOD(std::vector<real_t>, predict, (real_t *, int, int), (override));
    MOCK_METHOD(void, learn, (), (override));
    MOCK_METHOD(void, loadDataDevice, (), (override));
    MOCK_METHOD(std::vector<real_t>, CG, (const std::vector<real_t> &b, const int, const real_t), (override));

    const real_t get_num_data_points() const { return num_data_points; }
    const real_t get_num_features() const { return num_features; }
    std::vector<std::vector<real_t>> get_data() const { return data; }
    const real_t get_gamma() const { return gamma; }
};
class MockOpenMP_CSVM : public plssvm::OpenMP_CSVM {
  public:
    MockOpenMP_CSVM(real_t cost_, real_t epsilon_, plssvm::kernel_type kernel_, real_t degree_, real_t gamma_, real_t coef0_, bool info_) : plssvm::OpenMP_CSVM(cost_, epsilon_, kernel_, degree_, gamma_, coef0_, info_) {}
    // MOCK_METHOD(void, load_w, (), (override));
    MOCK_METHOD(std::vector<real_t>, predict, (real_t *, int, int), (override));
    // MOCK_METHOD(void, learn, (), (override));
    MOCK_METHOD(void, loadDataDevice, (), (override));
    MOCK_METHOD(std::vector<real_t>, CG, (const std::vector<real_t> &b, const int, const real_t), (override));

    const real_t get_num_data_points() const { return num_data_points; }
    const real_t get_num_features() const { return num_features; }
    std::vector<std::vector<real_t>> get_data() const { return data; }
    const real_t get_gamma() const { return gamma; }
};