
#include <fstream>
#include <vector>
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/ui/al_ControlGUI.hpp"  // gui.draw(g)
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

using namespace al;
using namespace std;

string slurp(string fileName);  // forward declaration

struct MyApp : public App {
    ShaderProgram shader;
    Mesh mesh;

    void onInit() override {}

    void onCreate() override {
        shader.compile(slurp("../tetrahedron-vertex.glsl"),
                       slurp("../tetrahedron-fragment.glsl"),
                       slurp("../tetrahedron-geometry.glsl"));
    }

    void onAnimate(double dt) override {}

    void onDraw(Graphics &g) override { g.clear(0); }

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