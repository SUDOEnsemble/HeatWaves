// Raphael Radna
// MAT-201B W20
// Physics Tests for Final Project

#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Font.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

using namespace al;

#include <fstream>
#include <vector>

using namespace std;

#define PI 3.141592654

// Distributed App provides a simple way to share states and parameters between
// a simulator and renderer apps using a single source file.
//
// You must define a state to be broadcast to all listeners. This state is
// synchronous but unreliable information i.e. missed data should not affect the
// overall state of the application.
//
struct Particle {
  Vec3f position, velocity, acceleration;
  float mass;
};

struct DrawableParticle {
  Vec3f position;

  void from(const Particle &that) { position.set(that.position); }
};

#define N (1000)

struct SharedState {
  Pose cameraPose;
  float background, pointSize;
  DrawableParticle particle[N]; // copy from some vector<Particle>
};

Vec3f rv(float scale) {
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
} // assigns randomness in three dimensions, multiplies it by scale

string slurp(string fileName); // forward declaration

// must do this instead of including a constructor in Particle
//
void initParticle(Particle &p, Vec3f p_, Vec3f v_, Vec3f a_, float m_);

// Inherit from DistributedApp and template it on the shared state data struct
//
class AlloApp : public DistributedAppWithState<SharedState> {
  Parameter pointSize{"/pointSize", "", 0.5, "", 0.0, 5.0};
  Parameter timeStep{"/timeStep", "", 0.05, "", 0.01, 0.6};
  Parameter gravConst{"/gravConst", "", 0.75, "", 0, 1};
  Parameter dragFactor{"/dragFactor", "", 0.075, "", 0.01, 0.99};
  Parameter maxAccel{"/maxAccel", "", 10, "", 0, 20};
  Parameter scaleVal{"/scaleVal", "", 0.8, "", 0, 2};
  ControlGUI gui;

  /* DistributedApp provides a parameter server. In fact it will
   * crash if you have a parameter server with the same port,
   * as it will raise an exception when it is unable to acquire
   * the port
   */

  ShaderProgram pointShader;
  Mesh mesh; // simulation state position is located in the mesh (positions are
             // the direct simulation states that we use to draw)

  vector<Particle> particle;

  void reset() { // empty all containers
    mesh.reset();
    particle.clear();

    // c++11 "lambda" function
    // seed random number generators to maintain determinism
    rnd::Random<> rng;
    rng.seed(42);
    auto rc = [&]() { return HSV(rng.uniform() / 2 + 0.5f, 1.0f, 1.0f); };
    auto rv = [&](float scale) -> Vec3f {
      return Vec3f(rng.uniformS(), rng.uniformS(), rng.uniformS()) * scale;
    };

    mesh.primitive(Mesh::POINTS);

    for (int _ = 0; _ < N; _++) { // create 1000 points, put it into mesh
      Particle p;
      float m = 10; // 25 + rnd::uniformS() * 25;
      initParticle(p, rv(5), Vec3f(0), Vec3f(0), m);

      mesh.vertex(p.position);
      mesh.color(rc());
      // set texture coordinate to be the size of the point (related to the
      // mass) using a simplified volume size relationship -> V = 4/3 * pi * r^3
      // pow is power -> m^(1/3)
      mesh.texCoord((4 / 3) * 3.14 * pow(p.mass, 1.0f / 3), 0);
      // pass in an s, t (like x, y)-> where on an image do we want to grab the
      // color from for this pixel (2D texture) normalized between 0 and 1

      particle.push_back(p);
    }
  }

  // You can keep a pointer to the cuttlebone domain
  // This can be useful to ask the domain if it is a sender or receiver
  //
  std::shared_ptr<CuttleboneStateSimulationDomain<SharedState>>
      cuttleboneDomain;

  void onCreate() override {
    cuttleboneDomain =
        CuttleboneStateSimulationDomain<SharedState>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }

    gui << pointSize << timeStep << gravConst << dragFactor << maxAccel
        << scaleVal;
    gui.init();

    // DistributedApp provides a parameter server.
    // This links the parameters between "simulator" and "renderers"
    // automatically
    parameterServer() << pointSize << timeStep << gravConst << dragFactor
                      << maxAccel << scaleVal;

    navControl().useMouse(false);

    // compile shaders
    //
    pointShader.compile(slurp("../point-vertex.glsl"),
                        slurp("../point-fragment.glsl"),
                        slurp("../point-geometry.glsl"));

    reset();
    nav().pos(0, 0, 25); // push camera back
  }

  int t = 0;
  float frequency = 1;
  void onAnimate(double dt) override {
    if (cuttleboneDomain->isSender()) {

      dt = timeStep;

      if (t >= 1 / (float)frequency * 60)
        t = 0;
      t++;

      // Calculate forces
      //
      // Vec3f flow = rv(10);
      float phase = sinf((t / 60.0f) * 2 * PI);
      Vec3f flow(phase * 10, 0, 0);

      for (int i = 0; i < N; i++) {
        Particle &p(particle.at(i));
        p.acceleration += flow / p.mass;
      }

      /* for (int i = 0; i < N; i++) {
        for (int j = 1 + i; j < N; j++) {

          rnd::Random<> rng;
          auto rv = [&](float scale) -> Vec3f {
            return Vec3f(rng.uniformS(), rng.uniformS(), rng.uniformS()) *
                   scale;
          };

          Particle &p1(particle.at(i));
          Particle &p2(particle.at(j));

          Vec3f distance(p2.position - p1.position);
          Vec3f gravityVal = gravConst * p1.mass * p2.mass *
                             distance.normalize() / pow(distance.mag(), 2);

          p1.acceleration += gravityVal * rv(scaleVal) / p1.mass;
          p2.acceleration -= gravityVal * rv(scaleVal) / p2.mass;

          // drag
          //
          p1.acceleration -= p1.velocity * dragFactor;

          // limit acceleration
          //
          if (p1.acceleration.mag() > maxAccel)
            p1.acceleration.normalize(maxAccel);
        }
      } */

      // Integration
      //
      vector<Vec3f> &position(mesh.vertices());
      for (int i = 0; i < N; i++) {
        Particle &p(particle.at(i));

        p.velocity += p.acceleration / p.mass * dt;
        p.position += p.velocity * dt;

        // clear all accelerations (IMPORTANT!!) -> accelerations have been used
        // and counted already
        //
        p.acceleration.zero();
      }

      // Copy all the agents into shared state;
      //
      for (unsigned i = 0; i < N; i++) {
        state().particle[i].from(particle[i]);
      }
      state().cameraPose.set(nav());
      state().background = 0;
      state().pointSize = pointSize.get();

    } else {
      // use the camera position from the simulator
      //
      nav().set(state().cameraPose);
    }

    for (int i = 0; i < N; i++)
      mesh.vertices()[i] = state().particle[i].position;
  }

  bool onKeyDown(const Keyboard &k) override {
    if (k.key() == '1') { // introduce some "random" forces
      for (int i = 0; i < N; i++)
        particle.at(i).acceleration = rv(1) / particle.at(i).mass;
    }

    if (k.key() == 'r') {
      reset();
    }

    return true;
  }

  void onDraw(Graphics &g) override {
    float scaledSize = pointSize / 50;
    float f = state().background;
    g.clear(f, f, f);
    g.shader(pointShader);
    g.shader().uniform("pointSize", scaledSize);
    gl::blending(true);
    gl::blendTrans();
    gl::depthTesting(true);
    g.draw(mesh);

    // Draw th GUI on the simulator only
    if (cuttleboneDomain->isSender()) {
      gui.draw(g);
    }
  }
};

int main() { AlloApp().start(); }

string slurp(string fileName) {
  fstream file(fileName);
  string returnValue = "";
  while (file.good()) {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}

void initParticle(Particle &p, Vec3f p_, Vec3f v_, Vec3f a_, float m_) {
  p.position = p_;
  p.velocity = v_;
  p.acceleration = a_;
  p.mass = m_;
}