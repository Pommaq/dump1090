//
// Created by Timmy on 4/15/2021.
//

#include "../Headers/GPUPreparations.hpp"

std::string readcode(std::string const& filename) {
  // TODO: Avoid unnecessary copies with std::move
  std::string buffer;
  std::ifstream code(filename);
  if (code) {
    while (code >> buffer)
      ;
  } else {
    std::cout << "file " << filename << " Not found!" << std::endl;
  }
  return buffer;
}


  GPU::GPU() {
    /* Created with help from
     * http://developer.amd.com/wordpress/media/2013/01/Introduction_to_OpenCL_Programming-Training_Guide-201005.pdf
     * */
    /*
     Our goal is to create a working execution queue and return it.
     */

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.empty()) {
      std::cerr << "No valid OpenCL platforms detected" << std::endl;
      exit(1);
    }

    platforms.front().getDevices(CL_DEVICE_TYPE_GPU,
                       &all_devices); // Get all available GPUs from it.
    if (all_devices.empty()) {
      std::cerr << "No devices found. Is the OpenCL runtime properly installed?"
                << std::endl;
      exit(1);
    }

    this->context = cl::Context(all_devices);

    this->queue = cl::CommandQueue(this->context, this->gpu);

    // TODO: Finish the GPU program itself and create kernels
  }
  GPU::~GPU() {}

  cl::Program GPU::CompileProgram(std::string &code) {
    cl::Program::Sources sourcecode;
    sourcecode.push_back(code);
    cl::Program program(this->context, sourcecode);
    if (program.build(this->all_devices) != CL_SUCCESS) {
      std::cout << "Error building program" << std::endl;
      exit(1); // TODO: Consider not exiting here and just reporting an error
               // instead
    }
    return program;
  }

