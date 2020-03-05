// Raphael Radna
// MAT-201B W20
// Agent Tests for Final Project
//
// Convert hashspace boid example with velocity and acceleration into a
// distributed app, add force field from 'physics-test.cpp'.
//
// TO DO:
// – builds, but crashes

#include "al/app/al_DistributedApp.hpp"
#include "al/math/al_Random.hpp"
#include "al/spatial/al_HashSpace.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

using namespace al;

#include <cmath>
#include <fstream>
#include <vector>

using namespace std;

Vec3f rv() { return Vec3f(rnd::uniform(), rnd::uniform(), rnd::uniform()); }
Vec3f rvs() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

string slurp(string fileName);

float scale(float value, float inLow, float inHigh, float outLow, float outHigh,
            float curve);

struct ForceField {
  int resolution; // number of divisions per axis
  vector<Vec3f> grid;
  // Vec3f center;

  ForceField(int r_) {
    resolution = r_;
    for (int i = 0; i < pow(resolution, 3); i++) {
      Vec3f f = rvs();
      f.normalize();
      grid.push_back(f);
    }
    // center = Vec3f(0.5, 0.5, 0.5);
  }
};

struct Agent : Pose {
  Vec3f heading, center;
  Vec3f velocity, acceleration;
  unsigned flockCount{1};

  // get address of container voxel
  //
  Vec3i fieldAddress(ForceField &f) {
    int &res(f.resolution);
    float x = scale(pos().x, 0, 1, 0, res, 1);
    float y = scale(pos().y, 0, 1, 0, res, 1);
    float z = scale(pos().z, 0, 1, 0, res, 1);
    return Vec3i(floor(x), floor(y), floor(z));
  }

  // get index of container voexl as an int between 0 and f.resolution^3
  //
  int fieldIndex(ForceField &f) {
    int &res(f.resolution);
    Vec3i fA = fieldAddress(f);
    return fA.x + fA.y * res + fA.z * res * res;
  }

  // return true if agent within {1, 1, 1} cube cornered on the origin
  //
  bool withinBounds(ForceField &f) {
    int &res(f.resolution);
    Vec3i fA = fieldAddress(f);
    return (fA.x >= 0 && fA.x < res) && (fA.y >= 0 && fA.y < res) &&
           (fA.z >= 0 && fA.z < res);
  }
};

struct DrawableAgent {
  Vec3f position;
  Quatf orientation;
  // float hue;

  void from(const Agent &that) {
    position.set(that.pos());
    orientation.set(that.quat());
  }
};

#define N (1000)
HashSpace space(6, N);
struct SharedState {
  Pose cameraPose;
  float background;
  float size, ratio;
  DrawableAgent agent[N];
};

struct AlloApp : public DistributedAppWithState<SharedState> {
  Parameter moveRate{"/moveRate", "", 0.1, "", 0.0, 2.0};
  Parameter turnRate{"/turnRate", "", 0.1, "", 0.0, 2.0};
  Parameter localRadius{"/localRadius", "", 0.4, "", 0.01, 0.9};
  ParameterInt k{"/k", "", 5, "", 1, 15};
  Parameter size{"/size", "", 1.0, "", 0.0, 2.0};
  Parameter ratio{"/ratio", "", 1.0, "", 0.0, 2.0};
  ControlGUI gui;

  ShaderProgram shader;
  Mesh mesh;

  vector<Agent> agent;
  ForceField field = ForceField(8);

  std::shared_ptr<CuttleboneStateSimulationDomain<SharedState>>
      cuttleboneDomain;

  void onCreate() override {
    cuttleboneDomain =
        CuttleboneStateSimulationDomain<SharedState>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }

    gui << moveRate << turnRate << localRadius << k << size << ratio;
    gui.init();
    navControl().useMouse(false);

    // compile shaders
    shader.compile(slurp("../tetrahedron-vertex.glsl"),
                   slurp("../tetrahedron-fragment.glsl"),
                   slurp("../tetrahedron-geometry.glsl"));

    mesh.primitive(Mesh::POINTS);

    for (int i = 0; i < N; i++) {
      Agent a;
      a.pos(rv());
      space.move(i, a.pos() * space.dim());
      a.faceToward(rv());
      agent.push_back(a);
      //
      mesh.vertex(a.pos());
      mesh.normal(a.uf());
      const Vec3f &up(a.uu());
      mesh.color(up.x, up.y, up.z);
    }

    nav().pos(0.5, 0.5, 5);
  }

  float t = 0;
  int frameCount = 0;
  void onAnimate(double dt) override {
    if (cuttleboneDomain->isSender()) {

      t += dt;
      frameCount++;
      if (t > 1) {
        t -= 1;
        cout << frameCount << "fps ";
        frameCount = 0;
      }

      // Setup OSC
      //
      short port = 12345;
      const char *host = "127.0.0.1";
      osc::Send osc;
      osc.open(port, host);

      for (unsigned i = 0; i < N; i++) {
        agent[i].center = agent[i].pos();
        agent[i].heading = agent[i].uf();
        agent[i].flockCount = 1;
        agent[i].acceleration.zero();
      }

      float sum = 0;
      for (int i = 0; i < N; i++) {
        HashSpace::Query query(k);
        int results = query(space, agent[i].pos() * space.dim(),
                            space.maxRadius() * localRadius);
        for (int j = 0; j < results; j++) {
          int id = query[j]->id;
          agent[i].heading += agent[id].uf();
          agent[i].center += agent[id].pos();
          agent[i].flockCount++;
        }
        sum += agent[i].flockCount;
      }

      if (frameCount == 0) //
        cout << sum / N << "nn" << endl;

      for (unsigned i = 0; i < N; i++) {
        if (agent[i].flockCount < 1) {
          printf("shit is f-cked\n");
          fflush(stdout);
          exit(1);
        }

        if (agent[i].flockCount == 1) {
          agent[i].faceToward(Vec3f(0, 0, 0), 0.003 * turnRate);
          continue;
        }

        // make averages
        agent[i].center /= agent[i].flockCount;
        agent[i].heading /= agent[i].flockCount;

        float distance = (agent[i].pos() - agent[i].center).mag();

        // alignment: steer towards the average heading of local flockmates
        //
        agent[i].faceToward(agent[i].pos() + agent[i].heading,
                            0.003 * turnRate);
        agent[i].faceToward(agent[i].center, 0.003 * turnRate);
        agent[i].faceToward(agent[i].pos() - agent[i].center, 0.003 * turnRate);
      }

      // Calculate forces
      //
      for (int i = 0; i < N; i++) {
        // boids
        agent[i].acceleration += agent[i].uf() * moveRate * 0.002;
        // force field
        agent[i].acceleration += field.grid.at(agent[i].fieldIndex(field));
        // drag
        agent[i].acceleration += -agent[i].velocity * 0.1;
      }

      // Rotate force field vectors
      //

      // Integration
      //
      for (int i = 0; i < N; i++) {
        // "backward" Euler integration
        agent[i].velocity += agent[i].acceleration;
        agent[i].pos() += agent[i].velocity;
      }

      for (unsigned i = 0; i < N; i++) {
        Vec3d p = agent[i].pos();

        // WRAP! (make a toroidal space)
        if (p.x > 1)
          p.x -= 1;
        if (p.y > 1)
          p.y -= 1;
        if (p.z > 1)
          p.z -= 1;
        if (p.x < 0)
          p.x += 1;
        if (p.y < 0)
          p.y += 1;
        if (p.z < 0)
          p.z += 1;

        agent[i].pos(p);
        space.move(i, agent[i].pos() * space.dim());
      }

      // Send positions over OSC
      //
      for (unsigned short i = 0; i < N; i++) {
        Vec3d &pos(agent[i].pos());
        osc.send("/pos", i, pos.x, pos.y, pos.z);
      }

      // Copy all the agents into shared state;
      //
      for (unsigned i = 0; i < N; i++) {
        state().agent[i].from(agent[i]);
      }
      state().cameraPose.set(nav());
      state().background = 0.1;
      state().size = size.get();
      state().ratio = ratio.get();
    } else {
      // use the camera position from the simulator
      //
      nav().set(state().cameraPose);
    }

    // visualize the agents
    //
    vector<Vec3f> &v(mesh.vertices());
    vector<Vec3f> &n(mesh.normals());
    vector<Color> &c(mesh.colors());
    for (unsigned i = 0; i < N; i++) {
      v[i] = state().agent[i].position;
      n[i] = -state().agent[i].orientation.toVectorZ();
      const Vec3d &up(state().agent[i].orientation.toVectorY());
      c[i].set(up.x, up.y, up.z);
    }
  }

  void onDraw(Graphics &g) override {
    float f = state().background;
    g.clear(f, f, f);

    gl::depthTesting(true);
    gl::blending(true);
    gl::blendTrans();
    g.shader(shader);
    g.shader().uniform("size", state().size * 0.03);
    g.shader().uniform("ratio", state().ratio * 0.2);
    g.draw(mesh);

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

float scale(float value, float inLow, float inHigh, float outLow, float outHigh,
            float curve) {
  float normValue = (value - inLow) / (inHigh - inLow);
  if (curve == 1) {
    return normValue * (outHigh - outLow) + outLow;
  } else {
    return (pow(normValue, curve) * (outHigh - outLow)) + outLow;
  }
}