#include <plssvm/CSVM.hpp>
#include <plssvm/operators.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace plssvm {

// TODO: move elsewhere?
template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
T string_to_floating_point(const std::string &str) {
    if constexpr (std::is_same_v<T, float>) {
        return std::stof(str);
    } else if constexpr (std::is_same_v<T, double>) {
        return std::stod(str);
    } else if constexpr (std::is_same_v<T, long double>) {
        return std::stold(str);
    }
}

//Einlesen libsvm Dateien
void CSVM::libsvmParser(const std::string_view filename) {
    std::vector<std::string> data_lines;

    {
        std::ifstream file{filename.data()};
        std::string line;
        while (std::getline(file, line)) {
            data_lines.push_back(std::move(line));
        }
    }
    std::cout << "Read " << data_lines.size() << " lines." << std::endl;

    data.resize(data_lines.size());
    value.resize(data_lines.size());

    std::istringstream line_iss;
    std::istringstream token_iss;
    std::string token;
    std::size_t max_size = 0;

#pragma omp parallel for shared(data, max_size), private(line_iss, token_iss, token)
    for (std::size_t i = 0; i < data_lines.size(); ++i) {
        line_iss.str(data_lines[i]);

        // get class
        std::getline(line_iss, token, ' ');
        value[i] = string_to_floating_point<real_t>(token) > real_t{0.0} ? 1 : -1; //TODO: exception if not?

        // get data
        std::vector<real_t> vline(max_size);
        while (std::getline(line_iss, token, ' ')) {
            if (!token.empty()) {
                token_iss.str(token);
                std::getline(token_iss, token, ':');

                // get index
                const unsigned long index = std::stoul(token);
                if (index >= vline.size()) {
                    vline.resize(index + 1);
                }

                // get actual value
                std::getline(token_iss, token, ':');
                vline[index] = string_to_floating_point<real_t>(token);

                // restore stream state
                token_iss.clear();
            }
        }
        // restore stream state
        line_iss.clear();
        data[i] = std::move(vline);

        // update max_size
#pragma omp critical
        {
            max_size = std::max(max_size, data[i].size());
        }
    }

// resize all vectors to the same size
#pragma omp parallel for
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i].resize(max_size);
    }

    // update values
    num_data_points = data.size();
    num_features = max_size; // TODO: umbenennen num_features

    if (gamma == 0) {
        gamma = 1. / num_data_points;
    }
}

//Einlesen ARF Dateien
void CSVM::arffParser(const std::string_view filename) {
    std::ifstream file(filename.data());
    std::string line, escape = "@";
    std::istringstream line_iss;
    std::vector<real_t> vline;
    std::string token;
    while (std::getline(file, line, '\n')) {
        if (line.compare(0, 1, "@") != 0 && line.size() > 1) {
            line_iss.str(line);
            while (std::getline(line_iss, token, ',')) {
                vline.push_back(string_to_floating_point<real_t>(token));
            }
            line_iss.clear();
            if (vline.size() > 0) {
                value.push_back(vline.back());
                vline.pop_back();
                data.push_back(vline);
            }
            vline.clear();
        } else {
            std::cout << line;
        }
    }
    num_data_points = data.size();
    num_features = data[0].size();
}

void CSVM::writeModel(const std::string_view model_name) {
    int nBSV = 0;
    int count_pos = 0;
    int count_neg = 0;
    for (int i = 0; i < alpha.size(); ++i) {
        if (value[i] > 0)
            ++count_pos;
        if (value[i] < 0)
            ++count_neg;
        if (alpha[i] == cost)
            ++nBSV;
    }
    //Terminal Ausgabe
    if (info) {
        std::cout << "Optimization finished \n";
        std::cout << "nu = " << cost << "\n";
        std::cout << "obj = "
                  << "\t"
                  << ", rho " << -bias << "\n";
        std::cout << "nSV = " << count_pos + count_neg - nBSV << ", nBSV = " << nBSV << "\n";
        std::cout << "Total nSV = " << count_pos + count_neg << std::endl;
    }

    //Model Datei
    std::ofstream model(model_name.data(), std::ios::out | std::ios::trunc);
    model << "svm_type c_svc\n";
    switch (kernel) {
    case 0:
        model << "kernel_type "
              << "linear"
              << "\n";
        break;
    case 1:
        model << "kernel_type "
              << "polynomial"
              << "\n";
        break;
    case 2:
        model << "kernel_type "
              << "rbf"
              << "\n";
        break;
    default:
        throw std::runtime_error("Can not decide wich kernel!");
    }
    model << "nr_class 2\n";
    model << "total_sv " << count_pos + count_neg << "\n";
    model << "rho " << -bias << "\n";
    model << "label "
          << "1"
          << " "
          << "-1"
          << "\n";
    model << "nr_sv " << count_pos << " " << count_neg << "\n";
    model << "SV\n";
    model << std::scientific;

    int count = 0;

#pragma omp parallel
    {
        std::stringstream out_pos;
        std::stringstream out_neg;

// Alle SV Klasse 1
#pragma omp for nowait
        for (int i = 0; i < alpha.size(); ++i) {
            if (value[i] > 0)
                out_pos << alpha[i] << " " << data[i] << "\n";
        }

#pragma omp critical
        {
            model << out_pos.rdbuf();
            count++;
#pragma omp flush(count, model)
        }

// Alle SV Klasse -1
#pragma omp for nowait
        for (int i = 0; i < alpha.size(); ++i) {
            if (value[i] < 0)
                out_neg << alpha[i] << " " << data[i] << "\n";
        }

        //Wait for all have writen Klass 1
        while (count < omp_get_thread_num()) {
        };

#pragma omp critical
        model << out_neg.rdbuf();
    }
    model.close();
}

}

// writeModel second version, TODO: implement correctly
//void CSVM::writeModel(std::string &model_name){
//  //  __itt_resume();
//  int nBSV = 0;
//  int count_pos = 0;
//  int count_neg = 0;
//  for(int i = 0; i < alpha.size(); ++i){
//    if(value[i] > 0) ++count_pos;
//    if(value[i] < 0) ++count_neg;
//    if(alpha[i] == cost) ++nBSV;
//
//  }
//  //Terminal Ausgabe
//  if(info){
//    std::cout << "Optimization finished \n";
//    std::cout << "nu = " << cost << "\n";
//    std::cout << "obj = " << "\t" << ", rho " << - bias << "\n";
//    std::cout << "nSV = " << count_pos + count_neg  - nBSV << ", nBSV = " << nBSV << "\n";
//    std::cout << "Total nSV = " << count_pos + count_neg << std::endl;
//  }
//  return;
//  //Model Datei
//  const unsigned int length = 1048576;
//  char buffer[length];
//  std::ofstream model;
//  model.rdbuf()->pubsetbuf(buffer, length);
//  model.open(model_name, std::ios::out | std::ios::trunc);
//  model << "svm_type c_svc\n";
//  switch(kernel){
//    case 0: model << "kernel_type " << "linear" << "\n";
//      break;
//    case 1: model << "kernel_type " << "polynomial" << "\n";
//      break;
//    case 2: model << "kernel_type " << "rbf" << "\n";
//      break;
//    default: throw std::runtime_error("Can not decide wich kernel!");
//  }
//  model << "nr_class 2\n";
//  model << "total_sv " << count_pos + count_neg << "\n";
//  model << "rho " << -bias << "\n";
//  model << "label " << "1" << " "<< "-1"<<"\n";
//  model << "nr_sv " << count_pos << " "<< count_neg<<"\n";
//  model << "SV\n";
//  // model << std::scientific;
//  model.unsetf(std::ios_base::floatfield);
//
//  int count = 0;
//  const size_t num_threads = 80; //omp_get_max_threads();
//  omp_set_num_threads(num_threads);
//  // auto start = std::chrono::high_resolution_clock::now();
//  // std::vector<char*> out_pos(80, new char[(data[0].size()*20 + 20)* data.size()/ omp_get_num_threads()]);
//  // auto stop = std::chrono::high_resolution_clock::now();
//  // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count()<< std::endl;
//  // std::vector<char*> out_neg(80, new char[(data[0].size()*20 + 20)* data.size()/ omp_get_num_threads()]);
//
//#pragma omp parallel shared(count)
//  {
//    std::string out_pos;
//    out_pos.reserve((data[0].size()*20 + 20)* data.size()/ omp_get_num_threads());
//    std::string out_neg;
//    out_neg.reserve((data[0].size()*20 + 20)* data.size()/ omp_get_num_threads());
//    // char* ptr = out_pos[omp_get_thread_num()];
//    // unsigned long ptr_int = 0;
//    // std::stringstream out_neg;
//    // auto start = std::chrono::high_resolution_clock::now();
//    // Alle SV Klasse 1
//#pragma omp for nowait
//    for(int i = 0; i < alpha.size(); ++i){
//      if(value[i] > 0){
//        // sprintf(ptr,"%e %n", alpha[i], &ptr_int);
//        // ptr += ptr_int;
//        out_pos += std::to_string(alpha[i]) + ' ';
//        char buffer[20];
//        for(unsigned j = 0; j < data[i].size() ; ++j){
//          if(data[i][j] != 0.0 ){
//            // sprintf(ptr, "%i:%e %n",j,data[i][j], &ptr_int);
//            // ptr += ptr_int;
//            sprintf(buffer, "%i:%e ",j,data[i][j]);
//            // sprintf(buffer, "%i:%i.%ie^0 ",j,(int)data[i][j], static_cast<int>((data[i][j] - (int)data[i][j])*1000000)  );
//            out_pos += buffer;
//          } //out << i << ":" << vec[i] << " ";
//        }
//        out_pos += '\n';
//        //  *ptr = '\n';
//        //  ++ptr;
//        // out_pos << alpha[i]  << " " << buffer << '\n';
//        //  out_pos << alpha[i]  << " " << data[i] << "\n";
//
//      }
//    }
//    // auto stop = std::chrono::high_resolution_clock::now();
//    // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count()<< std::endl;
//#pragma omp critical
//    {
//      // auto start = std::chrono::high_resolution_clock::now();
//      model << out_pos;
//#pragma omp flush (model)
//      // auto stop = std::chrono::high_resolution_clock::now();
//      // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count()<< std::endl;
//    }
//
//#pragma omp atomic
//    count++;
//    // #pragma omp single
//    // {
//    // 	auto start = std::chrono::high_resolution_clock::now();
//    // 	for(auto i : out_pos)
//    // 		model << i;
//    // 	// count++;
//    // 	#pragma omp flush ( model)
//    // 	auto stop = std::chrono::high_resolution_clock::now();
//    // 	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count()<< std::endl;
//    // }
//    // Alle SV Klasse -1
//    // #pragma omp for schedule(guided)
//    // for(int i = 0; i < alpha.size(); ++i){
//    // 	if(value[i] < 0) out_neg << alpha[i]  << " " << data[i] << "\n";
//    // }
//
//    {
//      // auto start = std::chrono::high_resolution_clock::now();
//      // Alle SV Klasse 1
//#pragma omp for nowait schedule(guided,4)
//      for(int i = 0; i < alpha.size(); ++i){
//        if(value[i] > 0){
//          out_pos += std::to_string(alpha[i]) + ' ';
//          char buffer[20];
//          for(unsigned j = 0; j < data[i].size() ; ++j){
//            if(data[i][j] != 0.0 ){
//              // sprintf(ptr, "%i:%e %n",j,data[i][j], &ptr_int);
//              // ptr += ptr_int;
//              sprintf(buffer, "%i:%e ",j,data[i][j]);
//              // sprintf(buffer, "%i:%i.%ie^0 ",j,(int)data[i][j], static_cast<int>((data[i][j] - (int)data[i][j])*1000000)  );
//              out_pos += buffer;
//            } //out << i << ":" << vec[i] << " ";
//          }
//          out_pos += '\n';
//
//        }
//      }
//      // auto stop = std::chrono::high_resolution_clock::now();
//      // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count()<< std::endl;
//    }
//
//    //Wait for all have writen Klass 1
//    while(count < num_threads) {};
//
//
//#pragma omp critical
//    model << out_neg;
//  }
//  model.close();
//
//}