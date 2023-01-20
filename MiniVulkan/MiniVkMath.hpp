#pragma once
#ifndef MINIVK_MINIVKMATH
#define MINIVK_MINIVKMATH
#include "./MiniVk.hpp"

namespace MINIVULKAN_NS {
	class MiniVkMath {
	public:
        static glm::mat4 Project2D(double width, double height, double znear, double zfar) {
            //return glm::transpose(glm::mat4(
            //    2.0/(width - 1), 0.0, 0.0, -1.0,
            //    0.0, 2.0/(height-1), 0.0, -1.0,
            //    0.0, 0.0, -2.0/(zfar-znear), -((zfar+znear)/(znear-zfar)),
            //    0.0, 0.0, 0.0, 1.0));
            // Defining the TOP and BOTTOM upside down will provide the proper translation transform for scaling
            // with Vulkan due to Vulkan's inverted Y-Axis without having to transpose the matrix.
            return glm::ortho(0.0, width, 0.0, height, znear, zfar);
        }
	};
}
#endif