// Rodney DuPlessis
// Kramer Elwell
// Raphael Radna
// MAT-201B W20
//
// TO DO:
// Make outside flow field push in
// Tie temp to color in a cool way
// Fix global scale
// Only send OSC messages for active agents, notify when active state changes

#define BOUNDARY_RADIUS (20)
#define NUM_SPECIES (58)
#define NUM_SITES (11)
#define FLOCK_COUNT (NUM_SPECIES * NUM_SITES)
#define FLOCK_SIZE (10)
#define TAIL_LENGTH (25)
#define KELP_LENGTH (1.0f)
#define SPECK_COUNT (1000)

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

#include "HeatWave_Data.hpp"

Vec3f rvU() { return Vec3f(rnd::uniform(), rnd::uniform(), rnd::uniform()); }

Vec3f rvS() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

string slurp(string fileName);

float scale(float value, float inLow, float inHigh, float outLow, float outHigh, float curve);

Vec3f polToCar(float r, float t);

struct FlowField {
    int resolution;  // number of divisions per axis
    vector<Vec3f> grid;
    vector<bool> perimeter;

    FlowField(int r_) {
        resolution = r_;
        for (int i = 0; i < pow(resolution, 3); i++) {
            Vec3f f = rvS();
            f.normalize();
            grid.push_back(f);
        }
    }

    void rotate() {
        for (int i = 0; i < grid.size(); i++) {
            grid[i] += rvS() * 0.01;
            if (grid[i].mag() > 1) grid[i].normalize();
        }
    }
};

struct Agent : Pose {
    Vec3f velocity, acceleration;
};

// Flow field functions
//
Vec3i fieldAddress(FlowField &f, Vec3f p) {
    int &res(f.resolution);
    float x = scale(p.x, -BOUNDARY_RADIUS, BOUNDARY_RADIUS, 0, res, 1);
    float y = scale(p.y, -BOUNDARY_RADIUS, BOUNDARY_RADIUS, 0, res, 1);
    float z = scale(p.z, -BOUNDARY_RADIUS, BOUNDARY_RADIUS, 0, res, 1);
    return Vec3i(floor(x), floor(y), floor(z));
}
// get index of container voxel as an int between 0 and f.resolution^3
//
int fieldIndex(FlowField &f, Vec3f p) {
    int &res(f.resolution);
    Vec3i fA = fieldAddress(f, p);
    return fA.x + fA.y * res + fA.z * res * res;
}
// return true if agent within FlowField cube
//
bool withinBounds(FlowField &f, Vec3f p) {
    int &res(f.resolution);
    Vec3i fA = fieldAddress(f, p);
    return (fA.x >= 0 && fA.x < res) && (fA.y >= 0 && fA.y < res) && (fA.z >= 0 && fA.z < res);
}

struct Flock {
    Vec3f home;
    unsigned population;
    float hue;
    int species;
    vector<Agent> agent;

    Flock(Vec3f home_, unsigned population_, int species_) {
        home = home_;
        population = population_;
        species = species_;
        hue = (float)species / NUM_SPECIES;

        for (int i = 0; i < population; i++) {
            Agent a;
            a.pos(home);
            a.faceToward(rvU());
            agent.push_back(a);
        }
    }
};

struct DrawableAgent {
    Vec3f position;
    Quatf orientation;

    void from(const Agent &that) {
        position.set(that.pos());
        orientation.set(that.quat());
    }
};

struct SharedState {
    Pose cameraPose;
    float background, thickness, globalScale;
    DrawableAgent agent[FLOCK_COUNT * FLOCK_SIZE];
    Vec3f speckPos[SPECK_COUNT];
    Vec3f kelpVerts[FLOCK_SIZE * NUM_SITES * TAIL_LENGTH * 2];
};

struct AlloApp : public DistributedAppWithState<SharedState> {
    Parameter background{"/background", "", 8.0, "", 0.01, 16.0};
    //   Parameter red{"/red", "", 0.0, "", 0.0, 1.0};
    Parameter globalScale{"/globalScale", "", 1.0, "", 0.000001, 2.0};
    Parameter moveRate{"/moveRate", "", 0.1, "", 0.0, 2.0};
    Parameter turnRate{"/turnRate", "", 0.1, "", 0.0, 2.0};
    Parameter speckSize{"/speckSize", "", 0.51, "", 0.0, 5.0};
    Parameter homing{"/homing", "", 0.3, "", 0.0, 2.0};
    Parameter fieldStrength{"/fieldStrength", "", 0.1, "", 0.0, 2.0};
    Parameter tailLength{"/tailLength", "", 0.25, "", 0.0, 1.0};
    Parameter thickness{"/thickness", "", 0.25, "", 0.0, 1.0};
    Parameter saturation{"/saturation", "", 1.0, "", 0.0, 1.0};
    Parameter value{"/value", "", 1.0, "", 0.0, 1.0};

    ControlGUI gui;

    ShaderProgram shader;
    ShaderProgram speckShader;

    Mesh mesh;
    Mesh backgroundSphere;
    Mesh specks;

    Vec3f speckVelocities[SPECK_COUNT];
    Vec3f kelpVelocities[FLOCK_SIZE * NUM_SITES * TAIL_LENGTH * 2];

    vector<Vec3f> sphereDefaultVerts;

    Light light;

    vector<Flock> flock;
    FlowField field = FlowField(8);

    Heat heat;
    Species species[58];
    Clock c;
    float currentTemp = -1;

    std::shared_ptr<CuttleboneStateSimulationDomain<SharedState>> cuttleboneDomain;

    Mesh vectorVis;  // mesh for drawing a line visualizing a vector

    // Setup OSC
    //
    short port = 12345;
    const char *host = "127.0.0.1";
    osc::Send osc;

    void onCreate() override {
        cuttleboneDomain = CuttleboneStateSimulationDomain<SharedState>::enableCuttlebone(this);
        if (!cuttleboneDomain) {
            std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
            quit();
        }

        gui << background /* << red */ << globalScale << moveRate << turnRate << speckSize << homing
            << fieldStrength << thickness << tailLength << saturation << value;
        gui.init();

        // DistributedApp provides a parameter server.
        // This links the parameters between "simulator" and "renderers"
        // automatically
        parameterServer() << background /* << red */ << globalScale << moveRate << turnRate
                          << speckSize << homing << fieldStrength << thickness << tailLength
                          << saturation << value;

        navControl().useMouse(false);

        // Loading CSV data, initializing structs
        //
        CSVReader temperatureData;
        temperatureData.addType(CSVReader::REAL);  // Date
        temperatureData.addType(CSVReader::REAL);  // Temperature (c)
        temperatureData.readFile("../data/_TEMP.csv");

        std::vector<Temperatures> tRows = temperatureData.copyToStruct<Temperatures>();
        for (auto t : tRows) {
            heat.data.push_back(t);
        };

        tRows.clear();

        CSVReader bioDiversityData;
        bioDiversityData.addType(CSVReader::REAL);  // Name
        bioDiversityData.addType(CSVReader::REAL);  // Site
        bioDiversityData.addType(CSVReader::REAL);  // Date
        bioDiversityData.addType(CSVReader::REAL);  // Count
        bioDiversityData.addType(CSVReader::REAL);  // Transect
        bioDiversityData.addType(CSVReader::REAL);  // quad
        bioDiversityData.addType(CSVReader::REAL);  // Taxonomy | Phylum
        bioDiversityData.addType(CSVReader::REAL);  // Mobility
        bioDiversityData.addType(CSVReader::REAL);  // Growth_Morph
        bioDiversityData.readFile("../data/_BIODIVERSE.csv");

        std::vector<Biodiversities> bRows = bioDiversityData.copyToStruct<Biodiversities>();
        for (auto b : bRows) {
            species[int(b.comName)].site[int(b.site)].data.push_back(b);
        };

        bRows.clear();

        heat.init();
        for (int i = 0; i < NUM_SPECIES; i++) {
            for (int j = 0; j < NUM_SITES; j++) {
                species[i].site[j].init();
                species[i].site[j].update(c.now());
            }
        }

        currentTemp = heat.update(c.now());
        c.update();

        // compile shaders
        shader.compile(slurp("../paint-vertex.glsl"), slurp("../paint-fragment.glsl"),
                       slurp("../paint-geometry.glsl"));

        speckShader.compile(slurp("../speck-vertex.glsl"), slurp("../speck-fragment.glsl"),
                            slurp("../speck-geometry.glsl"));

        // Background sphere
        addSphereWithTexcoords(backgroundSphere, BOUNDARY_RADIUS);
        const size_t verts = backgroundSphere.vertices().size();
        auto &col = backgroundSphere.colors();
        col.resize(verts);
        for (size_t i = 0; i < verts; i++) {
            col[i].set(0.0f, 0.0f,
                       (backgroundSphere.vertices()[i].y + BOUNDARY_RADIUS) /
                           (BOUNDARY_RADIUS * background));
            sphereDefaultVerts.push_back(backgroundSphere.vertices()[i]);
        }

        // Specks
        specks.primitive(Mesh::POINTS);
        for (int i = 0; i < SPECK_COUNT; i++) {
            state().speckPos[i].set(Vec3f(rnd::uniformS() * BOUNDARY_RADIUS * 0.7,
                                          rnd::uniformS() * BOUNDARY_RADIUS * 0.7,
                                          rnd::uniformS() * BOUNDARY_RADIUS * 0.7));
            specks.vertex(state().speckPos[i]);
            specks.color(0.7, 1.0, 0.7, 0.4);
        }

        // lighting
        light.pos(0.0, (globalScale * BOUNDARY_RADIUS) + 10, 0.0);
        light.diffuse(RGB(1, 1, 1));

        // Make Agents/Flocks
        mesh.primitive(Mesh::LINE_STRIP_ADJACENCY);
        int kelp = 0;
        for (int i = 0; i < FLOCK_COUNT; i++) {
            int sp = i / NUM_SITES;
            int si = i % NUM_SITES;

            Vec3f &h(species[sp].site[si].origin);
            h.y -= 3;

            Flock f = Flock(h, FLOCK_SIZE, i / NUM_SITES);

            for (Agent a : f.agent) {
                for (int j = 0; j < TAIL_LENGTH; j++) {
                    if (sp != 16 && sp != 56) {
                        mesh.vertex(a.pos());
                        mesh.normal(a.uf());
                        if (j == 0) {
                            // texcoord x is boolean "alive" state, y=1 used to indicate head
                            // of a strip
                            mesh.texCoord(0.0f, 1.0f);
                            mesh.color(1.0f, 1.0f, 1.0f);
                        } else if (j == TAIL_LENGTH - 1) {
                            // texcoord y=2 used to indicate tail of a strip
                            mesh.texCoord(0.0f, 2.0f);
                            mesh.color(HSV(f.hue, 1.0f, 1.0f));
                        } else {
                            // texcoord y=0 used to indicate body of a strip
                            mesh.texCoord(0.0f, 0.0f);
                            mesh.color(HSV(f.hue, 1.0f, 1.0f));
                        }
                    } else {
                        // int which = sp == 16 ? 1 : 2;
                        // int vert = (si * FLOCK_SIZE * which) + (ag * TAIL_LENGTH) + j;
                        state().kelpVerts[kelp].set(a.pos().x + rnd::uniformS() / 10,
                                                    -BOUNDARY_RADIUS + (j * KELP_LENGTH),
                                                    a.pos().z + rnd::uniformS() / 10);
                        mesh.vertex(state().kelpVerts[kelp]);
                        mesh.normal(a.uf());
                        if (j == 0) {
                            // texcoord x is boolean "alive" state, y=1 used to indicate head
                            // of a strip
                            mesh.texCoord(0.0f, 1.0f);
                            mesh.color(0.0f, 0.0f, 0.0f);
                        } else if (j == TAIL_LENGTH - 1) {
                            // texcoord y=2 used to indicate tail of a strip
                            mesh.texCoord(0.0f, 2.0f);
                            mesh.color(0.0, 0.4, 0.0, 0.8);
                        } else {
                            // texcoord y=0 used to indicate body of a strip
                            mesh.texCoord(0.0f, 0.0f);
                            mesh.color(0.0, 0.3, 0.0, 0.4);
                        }
                        kelp++;
                    }
                }
            }
            flock.push_back(f);
        }

        nav().pos(0.0, 0.0, 0.0);
        osc.open(port, host);

        vectorVis.primitive(Mesh::LINES);
        vectorVis.color(1, 1, 1);
        vectorVis.color(1, 1, 1);
        vectorVis.vertex(0, 0, 0);
        vectorVis.vertex(0, 0, 0);
    }

    float t = 0;
    int frameCount = 0;
    void onAnimate(double dt) override {
        if (cuttleboneDomain->isSender()) {
            // print frame rate
            t += dt;
            frameCount++;
            if (t > 1) {
                t -= 1;
                cout << frameCount << "fps " << endl;
                frameCount = 0;
            }

            // Rotate force field vectors
            //
            field.rotate();

            float currentTime = c.now();
            currentTemp = heat.update(currentTime);

            int sharedIndex = 0;
            int totalAgents = 0;
            for (int sp = 0; sp < NUM_SPECIES; sp++) {
                for (int si = 0; si < NUM_SITES; si++) {
                    species[sp].site[si].update(currentTime);
                    int n = sp * NUM_SITES + si;

                    // update flock population and origin position
                    Flock &f(flock[n]);
                    f.population = species[sp].site[si].currentCount;
                    totalAgents += f.population;
                    f.home = species[sp].site[si].origin;
                    f.home.y -= 3;
                    f.home *= globalScale;

                    if (sp != 16 && sp != 56) {  // if species is not kelp

                        // Reset agent quantities before calculating frame
                        //
                        for (int i = 0; i < f.population; i++) {
                            f.agent[i].acceleration.zero();
                        }

                        // Apply forces to each agent
                        for (int i = 0; i < f.population; i++) {
                            // turn a bit toward home
                            f.agent[i].faceToward(f.home, homing * turnRate);

                            // move ahead
                            f.agent[i].acceleration += f.agent[i].uf() * moveRate * 0.001;

                            // flow field effect on agent
                            //
                            if (withinBounds(field, f.agent[i].pos())) {
                                f.agent[i].acceleration +=
                                    field.grid.at(fieldIndex(field, f.agent[i].pos())) *
                                    ((float)fieldStrength / 1000);
                            } else {
                                f.agent[i].acceleration += -f.agent[i].pos().normalized() * 0.1;
                            }

                            // drag
                            //
                            f.agent[i].acceleration += -f.agent[i].velocity * 0.01;
                        }

                        // Integration
                        //
                        for (int i = 0; i < f.population; i++) {
                            // "backward" Euler integration
                            //
                            f.agent[i].velocity += f.agent[i].acceleration;
                            f.agent[i].pos() += f.agent[i].velocity;
                        }

                        // Send positions over OSC
                        //
                        for (int i = 0; i < f.population; i++) {
                            Vec3d &pos(f.agent[i].pos());
                            osc.send("/pos", n, i, (float)pos.x, (float)pos.y, (float)pos.z);
                        }

                        // Copy all the agents into shared state;
                        //
                        for (unsigned i = 0; i < FLOCK_SIZE; i++) {
                            state().agent[sharedIndex].from(f.agent[i]);
                            sharedIndex++;
                        }
                    }
                }
            }

            // move specks
            for (int i = 0; i < SPECK_COUNT; i++) {
                // acceleration due to flowfield
                if (withinBounds(field, specks.vertices()[i])) {
                    speckVelocities[i] += field.grid.at(fieldIndex(field, specks.vertices()[i])) *
                                          ((float)fieldStrength / 1000);
                } else {
                    speckVelocities[i] += -state().speckPos[i].normalized() * 0.1;
                }
                // drag
                speckVelocities[i] += -speckVelocities[i] * 0.01;
                state().speckPos[i] += speckVelocities[i];
            }

            // move kelp
            for (int i = 0; i < FLOCK_SIZE * TAIL_LENGTH * NUM_SITES * 2; i++) {
                if (i % TAIL_LENGTH != 0) {
                    // acceleration due to flowfield
                    if (withinBounds(field, specks.vertices()[i])) {
                        kelpVelocities[i] +=
                            field.grid.at(fieldIndex(field, state().kelpVerts[i])) *
                            ((float)fieldStrength / 10000);
                    } else {
                        kelpVelocities[i] += -state().kelpVerts[i].normalized() * 0.1;
                    }

                    // structural integrity
                    Vec3f diff = state().kelpVerts[i - 1] - state().kelpVerts[i];
                    if (diff.mag() > KELP_LENGTH) {
                        kelpVelocities[i] +=
                            diff.normalized() * (diff.mag() - KELP_LENGTH) * 0.0001;
                    }

                    // drag
                    kelpVelocities[i] += -kelpVelocities[i] * 0.01;

                    state().kelpVerts[i] += kelpVelocities[i];
                }
            }
            // cout << 25 % 25 << endl;
            c.update();
            state().cameraPose.set(nav());
            state().background = background.get();
            state().thickness = thickness.get();
            state().globalScale = globalScale.get();
        } else {
            // use the camera position from the simulator
            //
            nav().set(state().cameraPose);
        }

        // set background color according to temperature
        float white = scale(currentTemp, heat.min, heat.max, 0.0f, 1.0f, 1.0f);
        for (size_t i = 0; i < backgroundSphere.vertices().size(); i++) {
            backgroundSphere.colors()[i].set(
                white * (backgroundSphere.vertices()[i].y + BOUNDARY_RADIUS) /
                    (BOUNDARY_RADIUS * state().background),
                white * (backgroundSphere.vertices()[i].y + BOUNDARY_RADIUS) /
                    (BOUNDARY_RADIUS * state().background),
                (backgroundSphere.vertices()[i].y + BOUNDARY_RADIUS) /
                    (BOUNDARY_RADIUS * state().background));
        }

        // visualize the agents
        //
        vector<Vec3f> &v(mesh.vertices());
        vector<Vec2f> &t(mesh.texCoord2s());
        vector<Vec3f> &n(mesh.normals());
        vector<Color> &c(mesh.colors());

        // resize the sphere
        for (int i = 0; i < backgroundSphere.vertices().size(); i++) {
            backgroundSphere.vertices()[i] = sphereDefaultVerts[i] * state().globalScale;
        }

        // set speck position
        for (int i = 0; i < SPECK_COUNT; i++) {
            specks.vertices()[i].set(state().speckPos[i] * state().globalScale);
        }

        // set kelp position
        int segmentVertex = 16 * FLOCK_SIZE * TAIL_LENGTH * NUM_SITES;
        for (int i = 0; i < FLOCK_SIZE * TAIL_LENGTH * NUM_SITES * 2; i++) {
            v[segmentVertex].set(state().kelpVerts[i] * state().globalScale);
            segmentVertex++;
            if (segmentVertex == (17 * FLOCK_SIZE * TAIL_LENGTH * NUM_SITES))
                segmentVertex = 56 * FLOCK_SIZE * TAIL_LENGTH * NUM_SITES;
        }

        for (int iFlock = 0; iFlock < FLOCK_COUNT; iFlock++) {  // for each flock
            int sp = iFlock / NUM_SITES;
            Flock &f(flock[iFlock]);
            int flockStartIndex = iFlock * FLOCK_SIZE;  // index of first agent in flock
            for (int iAgent = flockStartIndex; iAgent < flockStartIndex + FLOCK_SIZE;
                 iAgent++) {                            // for each agent
                int headVertex = iAgent * TAIL_LENGTH;  // head vertex of agent

                if (iAgent - flockStartIndex < flock[iFlock].population) {  // if agent is alive
                    // for body of each agent, counting down from its tail
                    //
                    for (int segmentVertex = headVertex + TAIL_LENGTH - 1;
                         segmentVertex >= headVertex; segmentVertex--) {  // for each vertex
                        t[segmentVertex].x = 1.0f;
                        if (sp != 16 && sp != 56) {
                            if (t[segmentVertex].y != 1) {  // if not a head segment, follow head
                                v[segmentVertex].lerp(v[segmentVertex - 1], tailLength) *
                                    state().globalScale;
                                c[segmentVertex] = HSV(f.hue, saturation, value);
                                c[segmentVertex].a = 0.6;
                            }

                            if (t[segmentVertex].y == 1) {  // if head, lead the way
                                v[segmentVertex].set(state().agent[iAgent].position *
                                                     state().globalScale);
                            }
                        }
                    }
                    n[iAgent * TAIL_LENGTH] = -state().agent[iAgent].orientation.toVectorZ();
                } else {  // if agent is dead
                    for (int i = headVertex; i < headVertex + TAIL_LENGTH; i++) {
                        t[i].x = 0.0f;
                    }
                }
            }
        }
        vectorVis.vertices()[1].set(field.grid[1]);  // add vector to be visualized here
    };

    void onDraw(Graphics &g) override {
        g.clear(0, 0, 0.2);
        gl::depthTesting(true);
        gl::blending(true);
        gl::blendTrans();
        g.lighting(true);
        g.light(light);

        g.shader(shader);
        g.shader().uniform("size", thickness);
        g.draw(mesh);

        g.meshColor();
        g.draw(backgroundSphere);
        // g.draw(vectorVis);

        g.shader(speckShader);
        g.shader().uniform("pointSize", speckSize / 100);
        g.draw(specks);

        if (cuttleboneDomain->isSender()) {
            gui.draw(g);
        }
    }
};

int main() { AlloApp().start(); }

// useful functions
//
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

float scale(float value, float inLow, float inHigh, float outLow, float outHigh, float curve) {
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