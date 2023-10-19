#include "assignment5.hpp"

#include "game/boat.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/InputHandler.h"
#include "core/ShaderProgramManager.hpp"
#include "core/helpers.hpp"
#include "core/node.hpp"

#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_exponential.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <random>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>

edaf80::Assignment5::Assignment5(WindowManager &windowManager)
    : mCamera(0.5f * glm::half_pi<float>(),
              static_cast<float>(config::resolution_x) /
                  static_cast<float>(config::resolution_y),
              0.01f, 1000.0f),
      inputHandler(), mWindowManager(windowManager), window(nullptr) {
  WindowManager::WindowDatum window_datum{inputHandler,
                                          mCamera,
                                          config::resolution_x,
                                          config::resolution_y,
                                          0,
                                          0,
                                          0,
                                          0};

  window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum,
                                           config::msaa_rate);
  if (window == nullptr) {
    throw std::runtime_error("Failed to get a window: aborting!");
  }

  bonobo::init();
}

edaf80::Assignment5::~Assignment5() { bonobo::deinit(); }

void edaf80::Assignment5::run() {
  // Set up the camera
  mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 60.0f, 0.0f));
  mCamera.mMouseSensitivity = glm::vec2(0.003f);
  mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h

  // Create the shader programs
  ShaderProgramManager program_manager;
  GLuint fallback_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Fallback",
      {{ShaderType::vertex, "common/fallback.vert"},
       {ShaderType::fragment, "common/fallback.frag"}},
      fallback_shader);
  if (fallback_shader == 0u) {
    LogError("Failed to load fallback shader");
    return;
  }

  GLuint wavesShader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Waves",
      {{ShaderType::vertex, "game/water.vert"},
       {ShaderType::fragment, "game/water.frag"}},
      wavesShader);
  if (wavesShader == 0u) {
    LogError("Failed to load fallback shader");
    return;
  }

  GLuint boatShader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Boat",
      {{ShaderType::vertex, "game/boat.vert"},
       {ShaderType::fragment, "game/boat.frag"}},
      boatShader);
  if (boatShader == 0u) {
    LogError("Failed to load fallback shader");
    return;
  }

#define NUM_POOLS 5

  auto wirlPoolBuffer = 0u;
  glGenBuffers(1, &wirlPoolBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, wirlPoolBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec2) * NUM_POOLS, 0,
               GL_DYNAMIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, wirlPoolBuffer);
  auto whirlPools = std::array<glm::vec2, NUM_POOLS>{};
  auto mt = std::mt19937{std::random_device{}()};
  auto distribution = std::uniform_int_distribution{0, 100};

  for (auto &&whirlPool : whirlPools) {
    whirlPool = {distribution(mt), distribution(mt)};
  }

  glBindBuffer(GL_UNIFORM_BUFFER, wirlPoolBuffer);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec2) * NUM_POOLS,
                  whirlPools.data());
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  auto elapsedTimeSeconds = 0.0f;

  struct Wave {
    glm::vec2 direction;
    float amplitude;
    float frequency;
    float phase;
    float sharpness;
  } mainWave{{-1.0, 0.0}, 3.0, 0.2, 0.5, 2.0};

  auto boatPos = glm::vec3{0.f, 0.0f, 0.f};
  auto boat =
      Boat{&boatShader, [&elapsedTimeSeconds, &camera = mCamera, &mainWave,
                         &boatPos](GLuint program) {
             glUniform1f(glGetUniformLocation(program, "elapsedTimeSeconds"),
                         elapsedTimeSeconds);
             glUniform3fv(glGetUniformLocation(program, "cameraPosition"), 1,
                          glm::value_ptr(camera.mWorld.GetTranslation()));

             glUniform2fv(glGetUniformLocation(program, "mainWave.direction"),
                          1, glm::value_ptr(mainWave.direction));
             glUniform1f(glGetUniformLocation(program, "mainWave.amplitude"),
                         mainWave.amplitude);
             glUniform1f(glGetUniformLocation(program, "mainWave.frequency"),
                         mainWave.frequency);
             glUniform1f(glGetUniformLocation(program, "mainWave.phase"),
                         mainWave.phase);
             glUniform1f(glGetUniformLocation(program, "mainWave.sharpness"),
                         mainWave.sharpness);

             glUniform3fv(glGetUniformLocation(program, "boatPosition"), 1,
                          glm::value_ptr(boatPos));
           }};

  auto terrainProgram = [&elapsedTimeSeconds, &camera = mCamera, &mainWave,
                         &boatPos](GLuint program) {
    glUniform1f(glGetUniformLocation(program, "elapsedTimeSeconds"),
                elapsedTimeSeconds);
    glUniform3fv(glGetUniformLocation(program, "cameraPosition"), 1,
                 glm::value_ptr(camera.mWorld.GetTranslation()));

    glUniform2fv(glGetUniformLocation(program, "mainWave.direction"), 1,
                 glm::value_ptr(mainWave.direction));
    glUniform1f(glGetUniformLocation(program, "mainWave.amplitude"),
                mainWave.amplitude);
    glUniform1f(glGetUniformLocation(program, "mainWave.frequency"),
                mainWave.frequency);
    glUniform1f(glGetUniformLocation(program, "mainWave.phase"),
                mainWave.phase);
    glUniform1f(glGetUniformLocation(program, "mainWave.sharpness"),
                mainWave.sharpness);

    glUniform3fv(glGetUniformLocation(program, "boatPosition"), 1,
                 glm::value_ptr(boatPos));
  };

  auto terrainNoiseTexture = bonobo::loadTexture2D("res/textures/waves.png");

  auto terrainGeometry =
      parametric_shapes::createQuad(100.0, 100.0, 1000, 1000);
  auto terrainNode = Node{};
  terrainNode.set_geometry(terrainGeometry);
  terrainNode.get_transform().Translate(glm::vec3{-50.0f, 0.0, -50.0f});
  terrainNode.add_texture("normalTexture", terrainNoiseTexture, GL_TEXTURE_2D);
  terrainNode.set_program(&wavesShader, terrainProgram);

  glClearDepthf(1.0f);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  auto lastTime = std::chrono::high_resolution_clock::now();

  bool show_logs = true;
  bool show_gui = true;
  bool shader_reload_failed = false;
  bool show_basis = false;
  float basis_thickness_scale = 1.0f;
  float basis_length_scale = 1.0f;

  bool controlBoat = true;

  mCamera.mWorld.RotateX(-glm::half_pi<float>());

  auto gameOver = false;
  while (!glfwWindowShouldClose(window)) {
    if (gameOver) {
      controlBoat = false;
    }
    auto const nowTime = std::chrono::high_resolution_clock::now();
    auto const deltaTimeUs =
        std::chrono::duration_cast<std::chrono::microseconds>(nowTime -
                                                              lastTime);
    auto const deltaTimeS = std::chrono::duration<float>(deltaTimeUs).count();
    elapsedTimeSeconds += deltaTimeS;
    lastTime = nowTime;

    auto &io = ImGui::GetIO();
    inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

    glfwPollEvents();
    inputHandler.Advance();

    if (controlBoat) {
      boatPos += boat.update(deltaTimeS, inputHandler);
    } else {
      mCamera.Update(deltaTimeUs, inputHandler);
    }

    for (auto &&whirlPool : whirlPools) {
      auto boatToWhirlpool =
          whirlPool - glm::vec2{boatPos.x + 50, boatPos.z + 50};
      if (glm::dot(boatToWhirlpool, boatToWhirlpool) > 75 * 75) {
        whirlPool = {distribution(mt) + boatPos.x,
                     distribution(mt) + boatPos.z};
        boatToWhirlpool = whirlPool - glm::vec2{boatPos.x + 50, boatPos.z + 50};
      }

      auto distance = glm::sqrt(glm::dot(boatToWhirlpool, boatToWhirlpool));
      if (distance < 5.0f) {
        boatPos += glm::vec3(boatToWhirlpool.x, 0.0, boatToWhirlpool.y) *
                   (1.0f / std::pow(distance / 5.f, 2.f)) * deltaTimeS;

        boatToWhirlpool = whirlPool - glm::vec2{boatPos.x + 50, boatPos.z + 50};
        distance = glm::sqrt(glm::dot(boatToWhirlpool, boatToWhirlpool));
        if (distance < 0.1) {
          gameOver = true;
        }
      }
    }

    glBindBuffer(GL_UNIFORM_BUFFER, wirlPoolBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec2) * NUM_POOLS,
                    whirlPools.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
      shader_reload_failed = !program_manager.ReloadAllPrograms();
      if (shader_reload_failed)
        tinyfd_notifyPopup("Shader Program Reload Error",
                           "An error occurred while reloading shader "
                           "programs; "
                           "see the logs for details.\n"
                           "Rendering is suspended until the issue is "
                           "solved. "
                           "Once fixed, just reload the shaders again.",
                           "error");
    }
    if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
      show_logs = !show_logs;
    if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
      show_gui = !show_gui;
    if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
      mWindowManager.ToggleFullscreenStatusForWindow(window);

    // Retrieve the actual framebuffer size: for HiDPI monitors,
    // you might end up with a framebuffer larger than what you
    // actually asked for. For example, if you ask for a 1920x1080
    // framebuffer, you might get a 3840x2160 one instead.
    // Also it might change as the user drags the window between
    // monitors with different DPIs, or if the fullscreen status is
    // being toggled.
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glViewport(0, 0, framebuffer_width, framebuffer_height);

    //
    // Todo: If you need to handle inputs, you can do it here
    //

    mWindowManager.NewImGuiFrame();

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    if (!shader_reload_failed) {
      //
      // Todo: Render all your geometry here.
      //
      terrainNode.render(mCamera.GetWorldToClipMatrix());
      if (!gameOver) {
        boat.render(mCamera.GetWorldToClipMatrix());
      }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //
    // Todo: If you want a custom ImGUI window, you can set it up
    //       here
    //
    bool const opened =
        ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
    if (opened && !gameOver) {
      ImGui::Checkbox("Control boat", &controlBoat);
    }
    ImGui::End();

    if (show_basis)
      bonobo::renderBasis(basis_thickness_scale, basis_length_scale,
                          mCamera.GetWorldToClipMatrix());
    if (show_logs)
      Log::View::Render();
    mWindowManager.RenderImGuiFrame(show_gui);

    glfwSwapBuffers(window);
  }
}

int main() {
  std::setlocale(LC_ALL, "");

  Bonobo framework;

  try {
    edaf80::Assignment5 assignment5(framework.GetWindowManager());
    assignment5.run();
  } catch (std::runtime_error const &e) {
    LogError(e.what());
  }
}
