
#include <fstream>
#include <vector>
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

using namespace al;
using namespace std;

string slurp(string fileName);  // forward declaration

struct MyApp : public App {
    Parameter lerpAmount{"tail length", "", 0.2, "", 0.0001, 0.9};
    Parameter pointSize{"/pointSize", "", 0.51, "", 0.0, 3.0};
    ShaderProgram brushShader;
    ShaderProgram speckShader;
    Mesh mesh;

    Mesh backgroundSphere;
    Mesh specks;

    ControlGUI gui;

    float length = 50;
    int agentCount = 3;
    float time = 0;

    void onInit() override {
        gui << lerpAmount << pointSize;
        gui.init();
    }

    void onCreate() override {
        brushShader.compile(slurp("../paint-vertex.glsl"), slurp("../paint-fragment.glsl"),
                            slurp("../paint-geometry.glsl"));

        speckShader.compile(slurp("../speck-vertex.glsl"), slurp("../speck-fragment.glsl"),
                            slurp("../speck-geometry.glsl"));

        float r = 10;
        addSphereWithTexcoords(backgroundSphere, r);
        const size_t n = backgroundSphere.vertices().size();
        auto &c = backgroundSphere.colors();
        c.resize(n);
        for (size_t i = 0; i < n; i += 1) {
            c[i].set(0.0f, 0.0f, (backgroundSphere.vertices()[i].y + r) / (r * 8));
            // std::cout << (backgroundSphere.vertices()[i].y + r) / (r * 2) << std::endl;
        }
        specks.primitive(Mesh::POINTS);
        for (int i = 0; i < 200; i++) {
            specks.vertex(Vec3f(rnd::uniformS() * r * 0.7, rnd::uniformS() * r * 0.7,
                                rnd::uniformS() * r * 0.7));
            specks.color(0.7, 1.0, 0.7, 0.6);
        }

        mesh.primitive(Mesh::LINE_STRIP_ADJACENCY);
        for (int i = 0; i < length * agentCount; i++) {
            // mesh.vertex(sin(i), cos(i), 0);
            mesh.vertex(sin(-i / length) - int(i / length) + 1, cos(-i / length), 0);

            mesh.color((length - (i % int(length))) / length, 0, (i % int(length)) / length);

            if (i % int(length) == 0) {
                mesh.texCoord(0.3, 1.0);  // normal 1 used to indicate head of a strip
            } else if (i % int(length) == int(length) - 1) {
                mesh.texCoord(0.3,
                              2.0);  // normal 2 used to indicate tail of a strip
            } else {
                mesh.texCoord(0.3,
                              0.0);  // normal 0 used to indicate body of a strip
            }
        };

        nav().pos(0, 0, 7);
    }

    void onAnimate(double dt) override {
        time += dt;

        for (int i = mesh.vertices().size() - 1; i >= 0; i--) {
            if (mesh.texCoord2s()[i].y != 1)
                mesh.vertices()[i].lerp(mesh.vertices()[i - 1], lerpAmount);

            if (mesh.texCoord2s()[i].y == 1) {
                mesh.vertices()[i].set(Vec3f(sin(time - i) - int(i / length) + 1,
                                             sin(((i + 0.1) / (length * agentCount)) * time + 0.2),
                                             cos(4 * time)));
            }
        }

        // std::cout << mesh.vertices().size() << std::endl;
        nav().faceToward(0, 1);
    }

    void onDraw(Graphics &g) override {
        g.clear(0);
        gl::depthTesting(true);  // g.depthTesting(true);
        gl::blending(true);
        gl::blendAdd();
        g.shader(brushShader);
        // g.shader().uniform("size", 1.0);
        // g.shader().uniform("ratio", 0.2);
        g.draw(mesh);

        g.meshColor();
        g.draw(backgroundSphere);

        g.shader(speckShader);
        g.shader().uniform("pointSize", pointSize / 100);
        g.draw(specks);

        gui.draw(g);
    }

    void onSound(AudioIOData &io) override {}

    void onMessage(osc::Message &m) override {}
};

int main() {
    MyApp app;
    app.start();
    return 0;
}

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