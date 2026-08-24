#pragma once
#include "glm.hpp"
#include <console/console.hpp>

enum class TrackingDirectionMode : uint8_t { ConstantDirection, LookAtPoint, LookAtObject };

class Camera {
 public:
  explicit Camera(float fov) : _fov(glm::radians(fov)) {}
  Camera(float fov, const glm::vec3& position) : _fov(glm::radians(fov)), _position(position) {}
  Camera(float fov, const glm::vec3& position, const glm::vec3& direction)
      : _fov(glm::radians(fov)), _position(position) {
    update_angles_from_direction(direction);
  }

  void set_vfov(float fov) noexcept { _fov = glm::radians(fov); }
  [[nodiscard]] float get_vfov() const noexcept { return _fov; }
  [[nodiscard]] float get_hfov() const noexcept {
    return _fov * static_cast<float>(_width) / static_cast<float>(_height);
  }

  /** Clamps to [-latitude range, +latitude range] */
  // void set_latitude(double latitude) {
  //   _direction_latitude = latitude;
  //   return;
  //   _direction_latitude = std::clamp(latitude, -_latitude_range, _latitude_range);
  // }
  // void set_longitude(double longitude) noexcept { _direction_latitude = longitude; }
  void increment_latitude_longitude(double dtheta, double dphi) {
    _direction_latitude  += dtheta;
    _direction_latitude   = std::clamp(_direction_latitude, -_latitude_range, _latitude_range);
    _direction_longitude += dphi;
  }
  [[nodiscard]] std::pair<double, double> get_latitude_longitude() const noexcept {
    return {_direction_latitude, _direction_longitude};
  }
  [[nodiscard]] glm::vec3 get_direction() const noexcept { return direction_from_angles(); }

  //   void look_at(const glm::vec3& point) {
  //     _lookAtPosition        = point;
  //     _trackingDirectionMode = TrackingDirectionMode::LookAtPoint;
  //   }
  /** Camera will stop looking at the transform it has been assigned, nothing happens if already
   * unassigned */

  void set_position(const glm::vec3& position) { _position = position; }
  void increment_position(const glm::vec3& dxdydz) { _position += dxdydz; }
  [[nodiscard]] glm::vec3 get_position() const noexcept { return _position; }
  /** Camera will point to this transform */
  //   void lookAtTarget(GameObject* targetObject) {
  //     _lookAtTarget          = targetObject;
  //     _trackingDirectionMode = TrackingDirectionMode::LookAtObject;
  //   }
  /** Camera will stop looking at the transform it has been assigned, nothing happens if already
   * unassigned */
  //   void stopLookingAt() {
  //     _lookAtTarget          = nullptr;
  //     _trackingDirectionMode = TrackingDirectionMode::ConstantDirection;
  //   }
  /** Camera will follow this transform with a constant offset */
  //   void followTarget(GameObject* targetObject) {
  //     _followTarget     = targetObject;
  //     _offsetFromTarget = targetObject->getRigidBody()->getPosition() - _position;
  //   }
  /** Camera will stop follow the transform it has been assigned, nothing happens if already
   * unfollowed */
  //   void unfollowTarget() {
  //     _followTarget     = nullptr;
  //     _offsetFromTarget = {};
  //   }

  void update(uint32_t width, uint32_t height) {
    _width  = width;
    _height = height;
    switch (_trackingDirectionMode) {
        //   case TrackingDirectionMode::LookAtPoint: {
        //     _direction = _lookAtPosition - _position;
        //     break;
        //   }
        //   case TrackingDirectionMode::LookAtObject: {
        //     _direction = _lookAtTarget->getRigidBody()->getPosition() - _position;
        //     break;
        //   }
      default:
        break;
    }
  }

  // Returns an normal (non orthogonal) basis : FWD, LEFT, UP (UP is +z)
  [[nodiscard]] std::tuple<glm::vec3, glm::vec3, glm::vec3> get_view_basis() const {
    auto fwd  = direction_from_angles();
    auto up   = glm::vec3{0.F, 0.F, 1.F};
    auto left = cross(up, fwd);
    return {fwd, glm::normalize(left), up};
  }

  [[nodiscard]] glm::mat4x4 get_view_matrix() const {
    return glm::lookAt(
        _position + _offsetFromTarget, _position + direction_from_angles(), {0.F, 0.F, 1.F}
    );
  }

  [[nodiscard]] glm::mat4x4 get_projection_matrix() const {
    if (_width == 0 || _height == 0) {
      throw std::runtime_error(
          "Width or height of a camera is 0, the camera likely hasn't been updated"
      );
    }
    return glm::perspective(
        _fov, static_cast<float>(_width) / static_cast<float>(_height), _nearClip, _farClip
    );
  }

  [[nodiscard]] glm::mat4x4 get_view_projection_matrix() const {
    return get_projection_matrix() * get_view_matrix();
  }

 private:
  [[nodiscard]] glm::vec3 direction_from_angles() const noexcept {
    glm::vec3 v;
    v.x = static_cast<float>(cos(_direction_latitude) * cos(_direction_longitude));
    v.y = static_cast<float>(cos(_direction_latitude) * sin(_direction_longitude));
    v.z = static_cast<float>(sin(_direction_latitude));
    return v;
  }

  void update_angles_from_direction(glm::vec3 direction) {
    auto u = glm::normalize(glm::vec<3, double>(direction));
    if (u.x == 0 && u.y == 0) {
      console::get(consoles::engine)
          ->error(
              "Camera direction is pointing up, this should never happen. Changing to default "
              "forward direction"
          );
      _direction_latitude  = 0.;
      _direction_longitude = 0.;
      return;
    }
    _direction_latitude  = std::clamp(asin(u.z), -_latitude_range, _latitude_range);
    _direction_longitude = atan2(u.y, u.x);
  }

  glm::vec3 _position{};
  //   glm::vec3 _direction{1.F, 0.F, 0.F};
  double _direction_latitude  = 0.;
  double _direction_longitude = 0.;
  double _latitude_range      = glm::radians(80.);
  glm::vec3 _offsetFromTarget{};
  glm::vec3 _lookAtPosition{};
  TrackingDirectionMode _trackingDirectionMode = TrackingDirectionMode::ConstantDirection;

  uint32_t _width{};
  uint32_t _height{};

  float _fov{};
  float _nearClip = 0.01F;
  float _farClip  = 100.F;
};
