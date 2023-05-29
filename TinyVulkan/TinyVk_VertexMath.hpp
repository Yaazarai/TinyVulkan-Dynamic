#pragma once
#ifndef TINYVK_TINYVKVKMATH
#define TINYVK_TINYVKVKMATH
    #include "./TinyVK.hpp"

    namespace TINYVULKAN_NAMESPACE {
        struct TinyVkVertex {
            glm::vec2 texcoord;
            glm::vec3 position;
            glm::vec4 color;

            TinyVkVertex(glm::vec2 tex, glm::vec3 pos, glm::vec4 col) : texcoord(tex), position(pos), color(col) {}

            static TinyVkVertexDescription GetVertexDescription() {
                return TinyVkVertexDescription(GetBindingDescription(), GetAttributeDescriptions());
            }

            static VkVertexInputBindingDescription GetBindingDescription() {
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(TinyVkVertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                return bindingDescription;
            }

            static const std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
                std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(TinyVkVertex, texcoord);

                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(TinyVkVertex, position);

                attributeDescriptions[2].binding = 0;
                attributeDescriptions[2].location = 2;
                attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                attributeDescriptions[2].offset = offsetof(TinyVkVertex, color);
                return attributeDescriptions;
            }
        };

        class TinyVkMath {
	    public:
            const static glm::mat4 Project2D(double width, double height, double camerax, double cameray, double znear = 1.0, double zfar = 0.0) {
                //return glm::transpose(glm::mat4(
                //    2.0/(width - 1), 0.0, 0.0, -1.0,
                //    0.0, 2.0/(height-1), 0.0, -1.0,
                //    0.0, 0.0, -2.0/(zfar-znear), -((zfar+znear)/(znear-zfar)),
                //    0.0, 0.0, 0.0, 1.0));
                // Defining the TOP and BOTTOM upside down will provide the proper translation transform for scaling
                // with Vulkan due to Vulkan's inverted Y-Axis without having to transpose the matrix.
                glm::mat4 projection = glm::ortho(0.0, width, 0.0, height, znear, zfar);
                return glm::translate(projection, glm::vec3(-camerax, -cameray, 0.0));

            }
            
            const static glm::vec2 GetUVCoords(glm::vec2 xy, glm::vec2 wh, bool forceClamp = true) {
                if (forceClamp)
                    xy = glm::clamp(xy, glm::vec2(0.0, 0.0), wh);

                return glm::vec2(xy.x * (1.0 / wh.x), xy.y * (1.0 / wh.y));
            }

            const static glm::vec2 GetXYCoords(glm::vec2 uv, glm::vec2 wh, bool forceClamp = true) {
                if (forceClamp)
                    uv = glm::clamp(uv, glm::vec2(0.0, 0.0), glm::vec2(1.0, 1.0));

                return glm::vec2(uv.x * wh.x, uv.y * wh.y);
            }
	    };

        class TinyVkQuad {
        public:
            static const std::vector<glm::vec4> defvcolors;

            static std::vector<TinyVkVertex> CreateExt(glm::vec3 whd, const std::vector<glm::vec4> vcolors = defvcolors) {
                return {
                    TinyVkVertex({0.0,0.0}, {0.0, 0.0, whd.z}, vcolors[0]),
                    TinyVkVertex({1.0,0.0}, {whd.x, 0.0, whd.z}, vcolors[1]),
                    TinyVkVertex({1.0,1.0}, {whd.x, whd.y, whd.z}, vcolors[2]),
                    TinyVkVertex({0.0,1.0}, {0.0, whd.y, whd.z}, vcolors[3]),
                };
            }

            static std::vector<TinyVkVertex> CreateWithOffsetExt(glm::vec2 xy, glm::vec3 whd, const std::vector<glm::vec4> vcolors = defvcolors) {
                return {
                    TinyVkVertex({0.0,0.0}, {xy.x, xy.y, whd.z}, vcolors[0]),
                    TinyVkVertex({1.0,0.0}, {xy.x + whd.x, xy.y, whd.z}, vcolors[1]),
                    TinyVkVertex({1.0,1.0}, {xy.x + whd.x, xy.y + whd.y, whd.z}, vcolors[2]),
                    TinyVkVertex({0.0,1.0}, {xy.x, xy.y + whd.y, whd.z}, vcolors[3]),
                };
            }

            static std::vector<TinyVkVertex> CreateFromAtlasExt(glm::vec2 xy, glm::vec3 whd, glm::vec2 atlaswh, const std::vector<glm::vec4> vcolors = defvcolors) {
                glm::vec2 uv1 = { xy.x / whd.x, xy.y / whd.y };
                glm::vec2 uv2 = uv1 + glm::vec2(whd.x / atlaswh.x, whd.y / atlaswh.y);

                return {
                    TinyVkVertex({uv1.x, uv1.y}, {xy.x, xy.y, whd.z}, vcolors[0]),
                    TinyVkVertex({uv2.x, uv1.y}, {xy.x + whd.x, xy.y, whd.z}, vcolors[1]),
                    TinyVkVertex({uv2.x, uv2.y}, {xy.x + whd.x, xy.y + whd.y, whd.z}, vcolors[2]),
                    TinyVkVertex({uv1.x, uv2.y}, {xy.x, xy.y + whd.y, whd.z}, vcolors[3]),
                };
            }

            static std::vector<TinyVkVertex> Create(glm::vec3 whd, const glm::vec4 vcolor = defvcolors[0]) {
                return CreateExt(whd, { vcolor,vcolor,vcolor,vcolor });
            }

            static std::vector<TinyVkVertex> CreateWithOffset(glm::vec2 xy, glm::vec3 whd, const glm::vec4 vcolor = defvcolors[0]) {
                return CreateWithOffsetExt(xy, whd, { vcolor,vcolor,vcolor,vcolor });
            }

            static std::vector<TinyVkVertex> CreateFromAtlas(glm::vec2 xy, glm::vec3 whd, glm::vec2 atlaswh, const glm::vec4 vcolor = defvcolors[0]) {
                return CreateFromAtlasExt(xy, whd, atlaswh, {vcolor,vcolor,vcolor,vcolor});
            }

            static void RotateScaleFromOrigin(std::vector<TinyVkVertex>& quad, glm::vec3 origin, glm::float32 radians, glm::float32 scale) {
                glm::mat2 rotation = glm::mat2(glm::cos(radians), -glm::sin(radians), glm::sin(radians), glm::cos(radians));
                glm::vec2 pivot = origin;
                glm::vec2 position;

                for (size_t i = 0; i < quad.size(); i++) {
                    position = quad[i].position;

                    position -= pivot * scale;
                    position = rotation * scale * position;
                    position += pivot * scale;

                    quad[i].position = glm::vec3(position, quad[i].position.z);
                }
            }
        };

        const std::vector<glm::vec4> TinyVkQuad::defvcolors = { {1.0,1.0,1.0,1.0},{1.0,1.0,1.0,1.0},{1.0,1.0,1.0,1.0},{1.0,1.0,1.0,1.0} };

        class TinyVkPolygon {
        public:
            static glm::float32 AngleClamp(glm::float32 a) {
                #ifndef GLM_FORCE_RADIANS
                    return std::fmod((360.0f + std::fmod(a, 360.0f)), 360.0f);
                #else
                    constexpr glm::float32 pi2 = glm::pi<glm::float32>() * 2.0f;
                    a = std::fmod((pi2 + std::fmod(a, pi2)), pi2);
                #endif
            }

            static glm::float32 AngleDelta(glm::float32 a, glm::float32 b) {
                //// https://gamedev.stackexchange.com/a/4472
                glm::float32 absa, absb;
                #ifndef GLM_FORCE_RADIANS
                    absa = std::fmod((360.0f + std::fmod(a, 360.0f)), 360.0f);
                    absb = std::fmod((360.0f + std::fmod(b, 360.0f)), 360.0f);
                    glm::float32 delta = glm::abs(absa - absb);
                    glm::float32 sign = absa > absb || delta >= 180.0f? -1.0f : 1.0f;
                    return (180.0f - glm::abs(delta - 180.0f) * sign;
                #else
                    constexpr glm::float32 pi = glm::pi<glm::float32>();
                    constexpr glm::float32 pi2 = pi * 2.0f;
                    absa = std::fmod((pi2 + std::fmod(a, pi2)), pi2);
                    absb = std::fmod((pi2 + std::fmod(b, pi2)), pi2);
                    glm::float32 delta = glm::abs(absa - absb);
                    glm::float32 sign = (absa > absb || delta >= pi) ? -1.0f : 1.0f;
                    return (pi - glm::abs(delta - pi)) * sign;
                #endif
            }

            static glm::vec3 TriangleCircumcircle(glm::vec2 a, glm::vec2 b, glm::vec2 c) {
                glm::vec2 A(a), B(b), C(c);
                glm::vec2 sqrA = glm::pow(glm::vec2(a), { 2.0f, 2.0f });
                glm::vec2 sqrB = glm::pow(glm::vec2(b), { 2.0f, 2.0f });
                glm::vec2 sqrC = glm::pow(glm::vec2(c), { 2.0f, 2.0f });
                
                glm::vec3 XYR = {}; // X-coord, Y-coord, Radius.
                XYR.z = 1 / ((A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y)) * 2.0f);
                XYR.x = ((sqrA.x + sqrA.y) * (B.y - C.y) + (sqrB.x + sqrB.y) * (C.y - A.y) + (sqrC.x + sqrC.y) * (A.y - B.y)) * XYR.z;
                XYR.y = ((sqrA.x + sqrA.y) * (C.x - B.x) + (sqrB.x + sqrB.y) * (A.x - C.x) + (sqrC.x + sqrC.y) * (B.x - A.x)) * XYR.z;
                XYR.z = glm::distance(glm::vec2(XYR.x, XYR.y), A);
                return XYR;
            }

            static bool TriangleIsClockwise(TinyVkVertex a, TinyVkVertex b, TinyVkVertex c) {
                glm::vec2 A(a.position), B(b.position), C(c.position);
                return ((B.x - A.x) * (C.y - A.y) - (C.x - A.x) * (B.y - A.y)) <= 0.0f;
            }

            static glm::float32 SquaredDistance(glm::vec2 xy1, glm::vec2 xy2) {
                return static_cast<glm::float32>(glm::pow(xy2.x - xy1.x, 2) + glm::pow(xy2.y - xy1.y, 2));
            }

            static std::vector<uint32_t> TriangulatePointList(std::vector<TinyVkVertex> plist) {
                using Point = std::array<glm::float32, 2>;
                std::vector<std::vector<Point>> polygon;

                std::vector<Point> poly;
                for (TinyVkVertex& v : plist)
                    poly.push_back({v.position.x, v.position.y});
                
                polygon.push_back(poly);
                return mapbox::earcut<uint32_t>(polygon);
            }

            static std::vector<uint32_t> TriangulatePointListHoles(std::vector<std::vector<TinyVkVertex>> plist_holes) {
                using Point = std::array<glm::float32, 2>;
                std::vector<std::vector<Point>> polygon;

                for(size_t i = 0; i < plist_holes.size(); i++) {
                    std::vector<TinyVkVertex>& vh = plist_holes[i];
                    polygon.push_back(std::vector<Point>());

                    for (TinyVkVertex& v : vh)
                        polygon[i].push_back({v.position.x, v.position.y});
                }

                return mapbox::earcut<uint32_t>(polygon);
            }
        };
    }
#endif`