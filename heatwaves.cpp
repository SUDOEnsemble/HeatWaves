// Starting from Karl's implementation of boids

#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/ui/al_ControlGUI.hpp"  // gui.draw(g)
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

Vec3f rv(float scale = 1.0f) {
    return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

string slurp(string fileName);  // forward declaration

//
struct Agent : Pose {
    Vec3f heading, center;
    unsigned flockCount{1};
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
struct SharedState {
    Pose cameraPose;
    float background;
    float size, ratio;
    DrawableAgent agent[N];
};

struct AlloApp : public DistributedAppWithState<SharedState> {
    Parameter moveRate{"/moveRate", "", 1.0, "", 0.0, 2.0};
    Parameter turnRate{"/turnRate", "", 1.0, "", 0.0, 2.0};
    Parameter tooFar{"/tooFar", "", 0.2, "", 0.01, 2.0};
    Parameter tooNear{"/tooNear", "", 0.1, "", 0.01, 2.0};
    Parameter localRadius{"/localRadius", "", 0.4, "", 0.01, 0.9};
    Parameter size{"/size", "", 1.0, "", 0.0, 2.0};
    Parameter ratio{"/ratio", "", 1.0, "", 0.0, 2.0};
    ControlGUI gui;

    ShaderProgram shader;
    Mesh mesh;

    vector<Agent> agent;

    std::shared_ptr<CuttleboneStateSimulationDomain<SharedState>>
        cuttleboneDomain;

    void onCreate() override {
        cuttleboneDomain =
            CuttleboneStateSimulationDomain<SharedState>::enableCuttlebone(
                this);
        if (!cuttleboneDomain) {
            std::cerr << "ERROR: Could not start Cuttlebone. Quitting."
                      << std::endl;
            quit();
        }

        // add more GUI here
        gui << moveRate << turnRate << tooFar << tooNear << localRadius << size
            << ratio;
        gui.init();
        navControl().useMouse(false);

        // compile shaders
        shader.compile(slurp("../tetrahedron-vertex.glsl"),
                       slurp("../tetrahedron-fragment.glsl"),
                       slurp("../tetrahedron-geometry.glsl"));

        mesh.primitive(Mesh::LINES);

        for (int _ = 0; _ < N; _++) {
            Agent a;
            a.pos(rv());
            a.faceToward(rv());
            agent.push_back(a);
            //
            mesh.vertex(a.pos());
            mesh.normal(a.uf());
            const Vec3f &up(a.uu());
            mesh.color(up.x, up.y, up.z);
        }

        nav().pos(0, 0, 10);
    }

    void onAnimate(double dt) override {
        // return;

        if (cuttleboneDomain->isSender()) {
            for (unsigned i = 0; i < N; i++) {
                agent[i].center = agent[i].pos();
                agent[i].heading = agent[i].uf();
                agent[i].flockCount = 1;
            }

            // for each pair of agents
            //
            for (unsigned i = 0; i < N; i++)
                for (unsigned j = 1 + i; j < N; j++) {
                    float distance = (agent[j].pos() - agent[i].pos()).mag();
                    if (distance < localRadius) {
                        // put code here
                        //

                        agent[i].heading += agent[j].uf();
                        agent[i].center += agent[j].pos();
                        agent[i].flockCount++;

                        agent[j].heading += agent[i].uf();
                        agent[j].center += agent[i].pos();
                        agent[j].flockCount++;
                    }
                }

            // only once the above loop is done do we have good data on average
            // headings and centers

            //
            // put code here
            //
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

                // alignment: steer towards the average heading of local
                // flockmates
                //
                agent[i].faceToward(agent[i].pos() + agent[i].heading,
                                    0.003 * turnRate);

                // cohesion: steer to move towards the average position (center
                // of mass)
                //
                if (distance > tooFar)
                    agent[i].faceToward(agent[i].center, 0.003 * turnRate);

                // separation: steer to avoid crowding local flockmates
                //
                if (distance < tooNear)
                    agent[i].faceToward(agent[i].pos() - agent[i].center,
                                        0.003 * turnRate);

                if (agent[i].flockCount > 5) {
                    agent[i].faceToward(rv(), 0.003 * turnRate);
                }
            }

            //
            //
            for (unsigned i = 0; i < N; i++) {
                agent[i].pos() += agent[i].uf() * moveRate * 0.02;
            }

            // respawn agents if they go too far (MAYBE KEEP)
            //
            for (unsigned i = 0; i < N; i++) {
                if (agent[i].pos().mag() > 1.1) {
                    // to toward the center
                    agent[i].faceToward(Vec3f(0, 0, 0), 0.003 * turnRate);
                }
            }

            //
            // Copy all the agents into shared state;
            for (unsigned i = 0; i < N; i++) {
                state().agent[i].from(agent[i]);
            }
            state().cameraPose.set(nav());
            state().background = 0.1;
            state().size = size.get();
            state().ratio = ratio.get();
        } else {
            //
            // use the camera position from the simulator
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
        gl::depthTesting(true);  // or g.depthTesting(true);
        gl::blending(true);      // or g.blending(true);
        gl::blendTrans();        // or g.blendModeTrans();
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
