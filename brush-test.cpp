
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
    ShaderProgram brushShader;
    Mesh mesh;

    float length = 50;
    float time = 0;

    void onInit() override {}

    void onCreate() override {
        brushShader.compile(slurp("../paint-vertex.glsl"),
                            slurp("../paint-fragment.glsl"),
                            slurp("../paint-geometry.glsl"));

        mesh.primitive(Mesh::LINE_STRIP_ADJACENCY);
        for (int i = 0; i < length; i++) {
            mesh.vertex(sin(-i / length), cos(-i / length), 0);
            mesh.texCoord(0.3, 0.0);
            mesh.color((length - i) / length, 0, i / length);
        };

        nav().pos(0, 0, 5);
    }

    void onAnimate(double dt) override {
        time += dt;

        for (int i = mesh.vertices().size() - 1; i > 0; i--) {
            mesh.vertices()[i].set(mesh.vertices()[i - 1]);
        }

        mesh.vertices()[0].set(
            Vec3f(sin(time), sin(2 * time + 0.2), cos(4 * time)));

        // std::cout << mesh.vertices().size() << std::endl;
        nav().faceToward(0, 1);
    }

    void onDraw(Graphics &g) override {
        g.clear(0);
        gl::depthTesting(true);  // g.depthTesting(true);
        g.shader(brushShader);
        // g.shader().uniform("size", 1.0);
        // g.shader().uniform("ratio", 0.2);
        g.draw(mesh);
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