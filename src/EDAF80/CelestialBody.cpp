#include "CelestialBody.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include "core/Log.h"
#include "core/helpers.hpp"

CelestialBody::CelestialBody(bonobo::mesh_data const &shape,
                             GLuint const *program, GLuint diffuse_texture_id) {
  _body.node.set_geometry(shape);
  _body.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
  _body.node.set_program(program);
}

glm::mat4 CelestialBody::render(std::chrono::microseconds elapsed_time,
                                glm::mat4 const &view_projection,
                                glm::mat4 const &parent_transform,
                                bool show_basis) {
  // Convert the duration from microseconds to seconds.
  auto const elapsed_time_s =
      std::chrono::duration<float>(elapsed_time).count();
  // If a different ratio was needed, for example a duration in
  // milliseconds, the following would have been used:
  // auto const elapsed_time_ms = std::chrono::duration<float,
  // std::milli>(elapsed_time).count();

  _body.spin.rotation_angle += _body.spin.speed * elapsed_time_s;

  auto scale = glm::scale(glm::mat4{1.0f}, _body.scale);
  auto axisRotation = glm::rotate(glm::mat4{1.0f}, _body.spin.rotation_angle,
                                  glm::vec3{0.0f, 1.0f, 0.0f});
  auto axisTilt = glm::rotate(glm::mat4{1.0f}, _body.spin.axial_tilt,
                              glm::vec3{0.0f, 0.0f, 1.0f});

  _body.orbit.rotation_angle += _body.orbit.speed * elapsed_time_s;

  auto orbitTranslation = glm::translate(
      glm::mat4{1.0f}, glm::vec3{1.0f, 0.0f, 0.0f} * _body.orbit.radius);

  auto orbitRotation = glm::rotate(glm::mat4{1.0f}, _body.orbit.rotation_angle,
                                   glm::vec3{0.0, 1.0f, 0.0f});
  auto orbitTilt = glm::rotate(glm::mat4{1.0f}, _body.orbit.inclination,
                               glm::vec3{0.0f, 0.0f, 1.0f});

  glm::mat4 world =
      parent_transform * orbitTilt * orbitRotation * orbitTranslation;
  glm::mat4 model =
      scale * glm::inverse(orbitRotation) * axisTilt * axisRotation;

  if (show_basis) {
    bonobo::renderBasis(1.0f, 2.0f, view_projection, world * model);
  }

  // Note: The second argument of `node::render()` is supposed to be the
  // parent transform of the node, not the whole world matrix, as the
  // node internally manages its local transforms. However in our case we
  // manage all the local transforms ourselves, so the internal transform
  // of the node is just the identity matrix and we can forward the whole
  // world matrix.
  _body.node.render(view_projection, world * model);

  if (_ring.is_set) {
    _ring.node.render(
        view_projection,
        world *
            glm::rotate(glm::mat4{1.0f}, glm::half_pi<float>(),
                        glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(glm::mat4{1.0f}, glm::vec3{_ring.scale, 0.0f}));
  }

  return world * axisTilt;
}

void CelestialBody::add_child(CelestialBody *child) {
  _children.push_back(child);
}

std::vector<CelestialBody *> const &CelestialBody::get_children() const {
  return _children;
}

void CelestialBody::set_orbit(OrbitConfiguration const &configuration) {
  _body.orbit.radius = configuration.radius;
  _body.orbit.inclination = configuration.inclination;
  _body.orbit.speed = configuration.speed;
  _body.orbit.rotation_angle = 0.0f;
}

void CelestialBody::set_scale(glm::vec3 const &scale) { _body.scale = scale; }

void CelestialBody::set_spin(SpinConfiguration const &configuration) {
  _body.spin.axial_tilt = configuration.axial_tilt;
  _body.spin.speed = configuration.speed;
  _body.spin.rotation_angle = 0.0f;
}

void CelestialBody::set_ring(bonobo::mesh_data const &shape,
                             GLuint const *program, GLuint diffuse_texture_id,
                             glm::vec2 const &scale) {
  _ring.node.set_geometry(shape);
  _ring.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
  _ring.node.set_program(program);

  _ring.scale = scale;

  _ring.is_set = true;
}
