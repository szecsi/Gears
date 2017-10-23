
// =================================================================================================
// This file is part of the CLBlast project. The project is licensed under Apache Version 2.0. This
// project loosely follows the Google C++ styleguide and uses a tab-size of two spaces and a max-
// width of 100 characters per line.
//
// Author(s):
//   Cedric Nugteren <www.cedricnugteren.nl>
//
// This determines when to switch between the direct (for small sizes) and in-direct GEMM kernel
// with pre/post-processing kernels (for larger sizes). These can be set in a similar way as for the
// regular kernel tuning parameters: they can be specific for a certain vendor or device or can use
// some common default values.
//
// =================================================================================================

namespace clblast {
namespace database {
// =================================================================================================

const DatabaseEntry KernelSelectionHalf = {
  "KernelSelection", Precision::kHalf, {"XGEMM_MIN_INDIRECT_SIZE"}, {
    { // Intel GPUs
      kDeviceTypeGPU, "Intel", {
        { "default", { { kDeviceNameDefault, Params{ 1*1*1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { // NVIDIA GPUs
      kDeviceTypeGPU, "NVIDIA", {
        { "default", { { kDeviceNameDefault, Params{ 1280*1280*1280, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { // Default
      kDeviceTypeAll, "default", {
        { "default", { { kDeviceNameDefault, Params{ 512*512*512, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
  }
};

// =================================================================================================

const DatabaseEntry KernelSelectionSingle = {
  "KernelSelection", Precision::kSingle, {"XGEMM_MIN_INDIRECT_SIZE"}, {
    { // Intel GPUs
      kDeviceTypeGPU, "Intel", {
        { "default", { { kDeviceNameDefault, Params{ 1*1*1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { // NVIDIA GPUs
      kDeviceTypeGPU, "NVIDIA", {
        { "default", { { kDeviceNameDefault, Params{ 1280*1280*1280, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { 
      kDeviceTypeGPU, "ARM", {
        { "default", { { kDeviceNameDefault, Params{ 128*128*128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    }, 
    { // Default
      kDeviceTypeAll, "default", {
        { "default", { { kDeviceNameDefault, Params{ 512*512*512, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
  }
};

// =================================================================================================

const DatabaseEntry KernelSelectionComplexSingle = {
  "KernelSelection", Precision::kComplexSingle, {"XGEMM_MIN_INDIRECT_SIZE"}, {
    { // Intel GPUs
      kDeviceTypeGPU, "Intel", {
        { "default", { { kDeviceNameDefault, Params{ 1*1*1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { // NVIDIA GPUs
      kDeviceTypeGPU, "NVIDIA", {
        { "default", { { kDeviceNameDefault, Params{ 1280*1280*1280, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { // Default
      kDeviceTypeAll, "default", {
        { "default", { { kDeviceNameDefault, Params{ 512*512*512, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
  }
};

// =================================================================================================

const DatabaseEntry KernelSelectionDouble = {
  "KernelSelection", Precision::kDouble, {"XGEMM_MIN_INDIRECT_SIZE"}, {
    { // Intel GPUs
      kDeviceTypeGPU, "Intel", {
        { "default", { { kDeviceNameDefault, Params{ 1*1*1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { // NVIDIA GPUs
      kDeviceTypeGPU, "NVIDIA", {
        { "default", { { kDeviceNameDefault, Params{ 1280*1280*1280, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { // Default
      kDeviceTypeAll, "default", {
        { "default", { { kDeviceNameDefault, Params{ 512*512*512, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
  }
};

// =================================================================================================

const DatabaseEntry KernelSelectionComplexDouble = {
  "KernelSelection", Precision::kComplexDouble, {"XGEMM_MIN_INDIRECT_SIZE"}, {
    { // Intel GPUs
      kDeviceTypeGPU, "Intel", {
        { "default", { { kDeviceNameDefault, Params{ 1*1*1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { // NVIDIA GPUs
      kDeviceTypeGPU, "NVIDIA", {
        { "default", { { kDeviceNameDefault, Params{ 1280*1280*1280, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
    { // Default
      kDeviceTypeAll, "default", {
        { "default", { { kDeviceNameDefault, Params{ 512*512*512, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } },
      }
    },
  }
};

// =================================================================================================
} // namespace database
} // namespace clblast
