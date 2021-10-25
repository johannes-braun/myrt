//#pragma once
//
//#include <vector>
//
//namespace myrt {
//inline void run_testing() {
//  cl::Platform default_platform = cl::Platform::getDefault();
//  std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";
//
//  std::vector<cl::Device> devices;
//  default_platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
//  cl::Device compute_device = devices[0];
//  std::cout << "Using device: " << compute_device.getInfo<CL_DEVICE_NAME>() << "\n";
//
//  cl::Context context({compute_device});
//  __debugbreak();
//}
//} // namespace myrt
