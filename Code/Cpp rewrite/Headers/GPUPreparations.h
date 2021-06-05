//
// Created by Timmy on 4/15/2021.
//

#ifndef DUMP1090_GPUPREPARATIONS_H
#define DUMP1090_GPUPREPARATIONS_H
#define CL_HPP_TARGET_OPENCL_VERSION 210
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl2.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::string readcode(std::string const& filename);
class GPU{
private:
    cl::Platform platform;
    cl::Device gpu;
    cl::Context context;
    cl::CommandQueue queue;
    std::vector<cl::Device> all_devices;
    cl::Kernel flipsingular;

public:
    GPU();
    ~GPU();
    cl::Program CompileProgram(std::string &code);
};


#endif // DUMP1090_GPUPREPARATIONS_H
