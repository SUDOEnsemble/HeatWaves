// Raphael Radna
// MAT-201B W20
// Flock Tests for Final Project
//
// Convert "agent-test.cpp" to support multiple "flocks" of agents
//
// TO DO:

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

Vec3f rvU() { return Vec3f(rnd::uniform(), rnd::uniform(), rnd::uniform()); }

Vec3f rvS() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

string slurp(string fileName);

float scale(float value, float inLow, float inHigh, float outLow, float outHigh,
            float curve);

Vec3f polToCar(float r, float t);

struct ForceField {
  int resolution; // number of divisions per axis
  vector<Vec3f> grid;
  // Vec3f center;

  ForceField(int r_) {
    resolution = r_;
    for (int i = 0; i < pow(resolution, 3); i++) {
      Vec3f f = rvS();
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
  bool active;

  void setActive(bool b) { active = b; }

  bool isActive() { return active; }

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

  // return true if agent within {0, 0, 0} ... {1, 1, 1} cube
  //
  bool withinBounds(ForceField &f) {
    int &res(f.resolution);
    Vec3i fA = fieldAddress(f);
    return (fA.x >= 0 && fA.x < res) && (fA.y >= 0 && fA.y < res) &&
           (fA.z >= 0 && fA.z < res);
  }
};

struct Flock {
  Vec3f home;
  unsigned population;
  float hue;
  vector<Agent> agent;

  Flock(Vec3f home_, float population_, unsigned hue_) {
    home = home_;
    population = population_;
    hue = hue_;

    for (int i = 0; i < population; i++) {
      Agent a;
      a.pos(home);
      a.faceToward(rvU());
      a.setActive(true);
      agent.push_back(a);
    }
  }
};

struct DrawableAgent {
  Vec3f position;
  Quatf orientation;
  bool active;
  // float hue;

  void from(const Agent &that) {
    position.set(that.pos());
    orientation.set(that.quat());
    active = that.active;
  }
};

#define N (500)
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
  Parameter localRadius{"/localRadius", "", 0.1, "", 0.01, 0.9};
  ParameterInt k{"/k", "", 5, "", 1, 15};
  Parameter size{"/size", "", 1.0, "", 0.0, 2.0};
  Parameter ratio{"/ratio", "", 1.0, "", 0.0, 2.0};
  Parameter fieldStrength{"/fieldStrength", "", 0.1, "", 0.0, 1.0};
  ControlGUI gui;

  ShaderProgram shader;
  Mesh mesh;

  vector<Flock> flock;
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

    gui << moveRate << turnRate << localRadius << k << size << ratio
        << fieldStrength;
    gui.init();

    // DistributedApp provides a parameter server.
    // This links the parameters between "simulator" and "renderers"
    // automatically
    parameterServer() << moveRate << turnRate << localRadius << k << size
                      << ratio << fieldStrength;

    navControl().useMouse(false);

    // compile shaders
    shader.compile(slurp("../tetrahedron-vertex.glsl"),
                   slurp("../tetrahedron-fragment.glsl"),
                   slurp("../tetrahedron-geometry.glsl"));

    mesh.primitive(Mesh::POINTS);

    int n = 0;
    for (int i = 0; i < 50; i++) {
      float t = i / 50.0f * M_PI * 2;
      Flock f = Flock(Vec3f(0.5, 0, 0.5) + polToCar(0.5, t), 10, 0.0f);
      for (Agent a : f.agent) {
        space.move(n, a.pos() * space.dim()); // crashes when using "n"?
        mesh.vertex(a.pos());
        mesh.normal(a.uf());
        const Vec3f &up(a.uu());
        mesh.color(up.x, up.y, up.z);
        n++;
      }
      flock.push_back(f);
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
        cout << frameCount << "fps " << endl;
        frameCount = 0;
      }

      // Setup OSC
      //
      short port = 12345;
      const char *host = "127.0.0.1";
      osc::Send osc;
      osc.open(port, host);

      // Rotate force field vectors
      //
      // ???

      int sharedIndex = 0;
      for (int n = 0; n < flock.size(); n++) {
        Flock &f(flock[n]);
        //   for (Flock f : flock) {

        // Reset agent quantities before calculating frame
        //
        for (int i = 0; i < f.population; i++) {
          if (f.agent[i].isActive() == true) {
            f.agent[i].center = f.agent[i].pos();
            f.agent[i].heading = f.agent[i].uf();
            f.agent[i].flockCount = 1;
            f.agent[i].acceleration.zero();
          }
        }

        // Search for neighbors
        //
        float sum = 0;
        for (int i = 0; i < f.population; i++) {
          if (f.agent[i].isActive() == true) {
            HashSpace::Query query(k);
            int results = query(space, f.agent[i].pos() * space.dim(),
                                space.maxRadius() * localRadius);
            for (int j = 0; j < results; j++) {
              int id = query[j]->id;
              f.agent[i].heading += f.agent[id].uf();
              f.agent[i].center += f.agent[id].pos();
              f.agent[i].flockCount++;
            }
            sum += f.agent[i].flockCount;
          }
        }

        // if (frameCount == 0) //
        //   cout << sum / f.population << "nn" << endl;

        // Calculate agent behaviors
        //
        for (int i = 0; i < f.population; i++) {
          if (f.agent[i].isActive() == true) {
            if (f.agent[i].flockCount < 1) {
              printf("shit is f-cked\n");
              fflush(stdout);
              exit(1);
            }

            // if (agent[i].flockCount == 1) {
            //   agent[i].faceToward(Vec3f(0, 0, 0), 0.003 * turnRate);
            //   continue;
            // }

            // make averages
            // agent[i].center /= agent[i].flockCount;
            // agent[i].heading /= agent[i].flockCount;

            // float distance = (agent[i].pos() - agent[i].center).mag();

            // alignment: steer towards the average heading of local flockmates
            //
            // agent[i].faceToward(agent[i].pos() + agent[i].heading,
            //                     0.003 * turnRate);
            // agent[i].faceToward(agent[i].center, 0.003 * turnRate);
            // agent[i].faceToward(agent[i].pos() - agent[i].center, 0.003 *
            // turnRate);

            // steer towards the home location
            //
            f.agent[i].faceToward(f.home, 0.3 * turnRate);

            // change steering based on force field?
          }
        }

        // Apply forces
        //
        for (int i = 0; i < f.population; i++) {
          if (f.agent[i].isActive() == true) {
            // agency
            //
            f.agent[i].acceleration += f.agent[i].uf() * moveRate * 0.001;

            // force field
            //
            if (f.agent[i].withinBounds(field)) {
              f.agent[i].acceleration +=
                  field.grid.at(f.agent[i].fieldIndex(field)) *
                  ((float)fieldStrength / 1000);
            }

            // drag
            //
            f.agent[i].acceleration += -f.agent[i].velocity * 0.01;
          }
        }

        // Integration
        //
        for (int i = 0; i < f.population; i++) {
          if (f.agent[i].isActive() == true) {
            // "backward" Euler integration
            //
            f.agent[i].velocity += f.agent[i].acceleration;
            f.agent[i].pos() += f.agent[i].velocity;
          }
        }

        // Send positions over OSC
        //
        for (int i = 0; i < f.population; i++) {
          if (f.agent[i].isActive() == true) {
            Vec3d &pos(f.agent[i].pos());
            osc.send("/pos", n, i, (float)pos.x, (float)pos.y, (float)pos.z);
          }
        }

        // Copy all the agents into shared state;
        //
        for (unsigned i = 0; i < f.population; i++) {
          state().agent[sharedIndex].from(f.agent[i]);
          sharedIndex++;
        }
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
    for (int i = 0; i < N; i++) {
      if (state().agent[i].active == true) {
        v[i * T] = state().agent[i].position;
        n[i * T] = -state().agent[i].orientation.toVectorZ();
        const Vec3d &up(state().agent[i].orientation.toVectorY());
        c[i * T].set(up.x, up.y, up.z);
      } else {
        n[i * T] = Vec3f(4, 4, 4);
      }
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

Vec3f polToCar(float r, float t) {
  float x = r * cos(t);
  float y = r * sin(t);
  return Vec3f(x, 0, y);
}