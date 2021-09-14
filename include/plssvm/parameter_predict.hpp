/**
* @file
* @author Alexander Van Craen
* @author Marcel Breyer
* @copyright
*
* @brief Implements a class encapsulating all necessary parameters for predicting using the C-SVM possibly provided through command line arguments.
 */

#pragma once

#include "plssvm/parameter.hpp"

#include <string>

namespace plssvm {

template <typename T>
class parameter_predict : public parameter<T> {
  public:
    using base_type = parameter<T>;
    using base_type::backend;
    using base_type::coef0;
    using base_type::cost;
    using base_type::degree;
    using base_type::epsilon;
    using base_type::gamma;
    using base_type::input_filename;
    using base_type::kernel;
    using base_type::model_filename;
    using base_type::predict_filename;
    using base_type::print_info;
    using base_type::target;

    using real_type = typename base_type::real_type;
    using size_type = typename base_type::size_type;

    /**
     * @brief Set all parameters to their default values.
     * @param[in] input_filename the name of the test data file
     * @param[in] model_filename the name of the model file
     */
    explicit parameter_predict(std::string input_filename, std::string model_filename);

    /**
     * @brief Parse the command line arguments @p argv using [`cxxopts`](https://github.com/jarro2783/cxxopts) and set the parameters accordingly.
     * @param[in] argc the number of passed command line arguments
     * @param[in] argv the command line arguments
     */
    parameter_predict(int argc, char **argv);

  private:
    /*
     * Generate predict filename based on the name of the test file.
     */
    void predict_name_from_input();
};

extern template class parameter_predict<float>;
extern template class parameter_predict<double>;

}  // namespace plssvm
