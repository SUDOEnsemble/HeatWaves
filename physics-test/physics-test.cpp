// Raphael Radna
// MAT-201B W20
// Physics Tests for Final Project
//
// Added a forcefield within the cube {-4, -4, -4} and {4, 4, 4} with 512 (8^3)
// individual cubic cells. If particles leave these bounds, they bounce back
// (their velocity is inverted).
//
// TO DO:
//    - Use perlin noise to initialize the force field
//    - Rotate the vectors of the field a small amount each frame

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

  int fieldLocation() {
    Vec3i q = position + 4;
    return q.x + q.y * 8 + q.z * 64;
  }

  bool withinBounds() {
    Vec3i q = position + 4;
    return (q.x >= 0 && q.x < 8) && (q.y >= 0 && q.y < 8) &&
           (q.z >= 0 && q.z < 8);
  }
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
  Parameter fieldStrength{"/fieldStrength", "", 0.1, "", 0, 10};
  Parameter fieldRotation{"/fieldRotation", "", 0.01, "", 0, 1};
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
  Vec3f field[512];

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
      float m = 10 + abs(rnd::uniformS()) * 10;
      initParticle(p, rv(4), Vec3f(0), Vec3f(0), m);

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

    for (int i = 0; i < 512; i++) {
      field[i] = rv(1);
      field[i].normalize();
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

    gui << pointSize << timeStep << fieldStrength << fieldRotation;
    gui.init();

    // DistributedApp provides a parameter server.
    // This links the parameters between "simulator" and "renderers"
    // automatically
    parameterServer() << pointSize << timeStep << fieldStrength
                      << fieldRotation;

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
      for (int i = 0; i < N; i++) {
        Particle &p(particle.at(i));
        unsigned l = p.fieldLocation();
        if (p.withinBounds()) {
          Vec3f flow = field[l] * (float)fieldStrength;
          p.acceleration += flow / p.mass;
        } else {
          p.velocity *= -1;
        }
      }

      // Rotate force field vectors
      //
      for (int i = 0; i < 512; i++)
        field[i] = field[i].rotate(PI * 2 * (float)fieldRotation, 0, 1);

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