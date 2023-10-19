#include "EDAF80/parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/node.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

class Boat {
  std::vector<bonobo::mesh_data> m_meshes;
  std::array<Node, 3> m_boat{};
  std::array<Node, 4> m_sail{};

  float m_heading{};
  float const m_speed = 4.0f;

public:
  inline Boat(GLuint const *const shaderID,
              std::function<void(GLuint)> const &set_uniforms)
      : m_meshes{bonobo::loadObjects("./res/game/boat.obj")} {
    m_boat[0].set_geometry(m_meshes[0]);
    m_boat[0].set_material_constants(m_meshes[0].material);

    m_boat[1].set_geometry(m_meshes[1]);
    m_boat[1].set_material_constants(m_meshes[1].material);

    m_boat[2].set_geometry(m_meshes[2]);
    m_boat[2].set_material_constants(m_meshes[2].material);

    for (auto &&boatPart : m_boat) {
      boatPart.set_program(shaderID, set_uniforms);
    }

    m_sail[0].set_geometry(m_meshes[3]);
    m_sail[0].set_material_constants(m_meshes[3].material);

    m_sail[1].set_geometry(m_meshes[4]);
    m_sail[1].set_material_constants(m_meshes[4].material);

    m_sail[2].set_geometry(m_meshes[5]);
    m_sail[2].set_material_constants(m_meshes[5].material);

    m_sail[3].set_geometry(m_meshes[6]);
    m_sail[3].set_program(shaderID, set_uniforms);
    m_sail[3].set_material_constants(m_meshes[6].material);

    for (auto &&sailPart : m_sail) {
      sailPart.set_program(shaderID, set_uniforms);
    }
  }

  auto inline update(float deltaSeconds, InputHandler &ih) -> glm::vec3 {
    auto const rotationSpeed = -glm::pi<float>(); // Radians per second

    if (ih.GetKeycodeState(GLFW_KEY_A) == PRESSED) {
      m_heading -= rotationSpeed * deltaSeconds;
    };
    if (ih.GetKeycodeState(GLFW_KEY_D) == PRESSED) {
      m_heading += rotationSpeed * deltaSeconds;
    };

    auto movement = glm::vec3{};
    if (ih.GetKeycodeState(GLFW_KEY_W) == PRESSED) {
      movement -= glm::vec3{std::sin(m_heading) * m_speed * deltaSeconds, 0.0f,
                            std::cos(m_heading) * m_speed * deltaSeconds};
    }

    return movement;
  }

  auto inline render(glm::mat4 const &world, glm::mat4 const &mod = {})
      -> void {
    for (auto &&node : m_boat) {
      node.get_transform().SetRotate(m_heading, glm::vec3{0.0f, 1.0f, 0.0f});
      node.render(world);
    }

    for (auto &&node : m_sail) {
      node.get_transform().SetRotate(m_heading, glm::vec3{0.0f, 1.0f, 0.0f});
      node.render(world);
    }
  }
};
