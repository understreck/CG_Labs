#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
    return glm::normalize(p1 - p0) * x;
}

glm::vec3
interpolation::evalCatmullRom(
        glm::vec3 const& p0,
        glm::vec3 const& p1,
        glm::vec3 const& p2,
        glm::vec3 const& p3,
        float const t,
        float const x)
{
    auto const scaleVec = glm::vec4{1.f, x, std::pow(x, 2.f), std::pow(x, 3.f)};
    auto const transformMatrix = glm::transpose(glm::mat4{
            {0.f, 1.f, 0.f, 0.f},
            {-t, 0.f, t, 0.f},
            {2 * t, t - 3, 3 - 2 * t, -t},
            {-t, 2 - t, t - 2, t}});
    auto const pointVec        = glm::transpose(
            glm::mat4{{p0, 0.f}, {p1, 0.f}, {p2, 0.f}, {p3, 0.f}});
    return scaleVec * transformMatrix * pointVec;
}
