#pragma once

class Ilayer {
 public:
  Ilayer()                         = default;
  virtual ~Ilayer()                = default;
  Ilayer(const Ilayer&)            = default;
  Ilayer(Ilayer&&)                 = default;
  Ilayer& operator=(const Ilayer&) = default;
  Ilayer& operator=(Ilayer&&)      = default;

  virtual void init() noexcept            = 0;
  virtual void update(double dt) noexcept = 0;
  virtual void cleanup() noexcept         = 0;

 private:
};