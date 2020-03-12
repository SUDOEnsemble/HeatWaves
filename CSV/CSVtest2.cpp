/*

KRAMER ELWELL, RODNEY DUPLESSIS, RAPHAEL RADNA
201B FINAL PROJECT
HEATWAVES - ALLOSPHERE MARINE RESEARCH SIMULATIN 
Winter 2020

*/

#include "al/app/al_App.hpp"
#include "al/system/al_Time.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_CSVReader.hpp"
#include "al/math/al_Interpolation.hpp"
using namespace al;

#include <fstream>
#include <vector>
using namespace std;


// GLOBALS  
#define DUR 1200  
#define TIME_MOD 7 



typedef struct {                      // Struct for a single Temperature datapoint
  double date, temp;
} Temperatures;


typedef struct {                      // Struct for a single biodiversity datapoint
  double date, site, transect, quad, size, count, comName, taxPhylum, mobility, growth;        
} Biodiversities;

struct Sites {                      // Struct for a single biodiversity datapoint
  double date, site, transect, quad, size, count, comName, taxPhylum, mobility, growth;        
};




struct Species {
  vector<Biodiversities> data;
  Sites site[11];
  const int site        = data[0].site;
  const int taxPhylum   = data[0].taxPhylum;
  const bool mobility   = data[0].mobility;
  const bool growth     = data[0].growth;
  
  
  float currentCount, max, min, ave;
  Vec3f origin;

 void init () {
    min = data[0].count; 
    max = data[0].count;
    for (int i = 0; i < data.size(); i++){
      ave += data[i].count;
      if (data[i].count > max) { max = data[i].count; };
      if (data[i].count < min) { min = data[i].count; };
    }
    ave /= data.size();
  }


  void printAve() { std::cout << "Average: " << ave << std::endl; }
  void printMax() { std::cout << "Maximum: " << max << std::endl; }
  void printMin() { std::cout << "Minimum: " << min << std::endl; }
  

  void update (float time) {
    float k = int(time * TIME_MOD);
    float fract =  fmodf(time * TIME_MOD, 1.0); 
    
    float countDiff = data[k + 1].count - data[k].count;                  // Species Count
    currentCount = data[k].count + countDiff * fract; 

    float rDiff = data[k + 1].transect - data[k].transect;                // Origin Radius
    float r = data[k].transect + rDiff * fract; 

    float x = cos((site / 11) * M_2PI) * r;                               // Origin X-Position
    float z = sin((site / 11) * M_2PI) * r;                               // Origin Z-Position

    float yDiff = data[k + 1].quad - data[k].quad;                        // Origin Y-Position
    float y = data[k].quad + yDiff * fract; 

    Vec3f origin = (x, y, z);
  }
};


struct Heat {
  vector<Temperatures> data;
  float currentTemp, ave, min, max;
  

  void init () {
    min = data[0].temp; 
    max = data[0].temp;
    for (int i = 0; i < data.size(); i++){
      ave += data[i].temp;
      if (data[i].temp > max) { max = data[i].temp; };
      if (data[i].temp < min) { min = data[i].temp; };
    }
    ave /= data.size();
  }


  void printAve() { std::cout << "Average: " << ave << std::endl; }
  void printMax() { std::cout << "Maximum: " << max << std::endl; }
  void printMin() { std::cout << "Minimum: " << min << std::endl; }
  

  float update (float time) {
    float diff = (data[int(time * TIME_MOD) + 1].temp - data[int(time * TIME_MOD)].temp); 
    float fract =  fmodf(time * TIME_MOD, 1.0); 
    currentTemp = data[time * TIME_MOD].temp + diff * fract;
    return currentTemp; 
  }
};







struct MyApp : App {
  Heat heat;
  Species species[58];
  Clock c;


  void onCreate() override {
    CSVReader temperatureData;
    temperatureData.addType(CSVReader::REAL);               // Date
    temperatureData.addType(CSVReader::REAL);               // Temperature (c)
    temperatureData.readFile("../data/_TEMP.csv");

    std::vector<Temperatures> tRows = temperatureData.copyToStruct<Temperatures>();
    for (auto t : tRows) {
      heat.data.push_back(t);
    };



    CSVReader bioDiversityData;
    bioDiversityData.addType(CSVReader::REAL);              // Date                 | yymmdd Sequential
    bioDiversityData.addType(CSVReader::REAL);              // Site                 | 0 - 10
    bioDiversityData.addType(CSVReader::REAL);              // Transect             | 1 - 8
    bioDiversityData.addType(CSVReader::REAL);              // Quad                 | 0 - 6
    bioDiversityData.addType(CSVReader::REAL);              // Size                 | float > 0
    bioDiversityData.addType(CSVReader::REAL);              // Count                | float > 0 
    bioDiversityData.addType(CSVReader::REAL);              // Common Name          | 0 - 57
    bioDiversityData.addType(CSVReader::REAL);              // Taxonomy | Phylum    | 0 - 7
    bioDiversityData.addType(CSVReader::REAL);              // Mobility             | 0 or 1
    bioDiversityData.addType(CSVReader::REAL);              // Growth_Morph         | 0 or 1
    bioDiversityData.readFile("../data/biodiverse.csv");

    std::vector<Biodiversities> bRows = bioDiversityData.copyToStruct<Biodiversities>();
    for (auto b : bRows) {
      species[int(b.comName)].data.push_back(b);
    };

    // for (auto s : species) {
    //   cout  << s.data[0].comName << " : " << s.data[0].date << " | " << s.data[0].site 
    //         << " | " << s.data[0].transect  << " | " << s.data[0].quad      << " | " << s.data[0].size
    //         << " | " << s.data[0].count     << " | " << s.data[0].taxPhylum << " | " << s.data[0].taxPhylum
    //         << " | " << s.data[0].mobility  << " | " << s.data[0].growth    << " | " << s.data.size() << endl;
    // }



    // for (auto s : species){
    //   s.init();
    //   s.printCountMax();
    // }

    heat.init();
    // heat.printAve();
    // heat.printMax();
    // heat.printMin();
  }


  float currentTemp = -1;
  void onAnimate(double dt) override {
    float currentTime = c.now();

    float currentTemp = heat.update(currentTime);

    c.update();
  }
  

  void onDraw(Graphics& g) override {
    vsync(true);
    g.clear(0.25); 
  }
};


int main() {
  MyApp app;
  app.start();
}





/*
--------------------------------------------------------------------------------------------------------;
SCRATCH NOTES ------------------------------------------------------------------------------------------;
--------------------------------------------------------------------------------------------------------;


vector<Species> phylumG1, phylumG2, phylumG3, phylumG4; 
phylumG1.size()... 

>> vector <Species> or Species species[58] probably
    >> Species has a...
        >> Vector <CSVdata>
            >> One line of CSV data

Need to use vsync for firm concept of time... is there a millis function? Solid/unwavering time elapsed function?

Total unique species = 58

unique kingdoms = 2
unique phylums = 8                                                  Best split - each of us sonifies 2 phylums
unique class = 13
unique order = 27
unique family = 42
unique genus = 53

Echinodermata + Porifera  =   112,200 Total count                   Primary gestural character
Cnidaria + Arthropoda     =   52,360 total count                    less active / acommpanimental role
Ochrophyta + Annelida     =   79,644 total count                    2 secondary gestural characters
Mollusca + Chordata       =   80,784 total count 
             

>>  heat temp sites = 3                                             heat wave time interpolated over 3d space
      2 depths
      2 radiuses
    Common sites with bioDiversity  = 3
    bioDiversity unique sites       = 8


>> Polar spatialization? 2PI / site# (0-10) = vector from origin to draw transects? 


>> timeline logic                                                   event in time where effects of position and size change
    year
    month
    date


>>  potential spatial logic - defines birth origin and a "home" space  
    ("I [are boid] want to stay close to this point unless a behavor [flee] tells me otherwise")  

      transect 1 - 8 >>> mapped to x/z axis >>> 8 points >> distance from origin >> normal vector * transect              
      quadrant 0 - 40 >>> mapped to y axis >>> 6 points >> elevation on the Y
      site 1 - 11 >>> mapped to x/z >>> 11 points divided into 2PI to get vector from origin [one site ever 32.72 deg]


>> neighborhood size - defines flock size
  count
  size
  area

need to do something like... 
float increment = pieceTime * 1000 / temps.size(); 
interpolate from temp[0] to temp[1] over increment

but... need to do that for EVERY parameter... parameter parent struct? so struct temperatures : parameters {} or similar... 
then a species struct that includes all of these parameter structs that pulls all the data based on an if?



count = 1;
Vec2f interop (clock c, float dur, int arraySize) {
  inc = dur / arraySize;
  if (c.now % inc == 0) {count++}

  Vec2f count+1, inc 



  Vec2f out = destination, duration;
  return out; 
}

onAnimate

parameters();

  void parameters (clock c) {
    heat = heatFUNCTION(heatdata)
    parameter1 = bioFUNCTION(p1data);
  }

  bioFUNCTION (vector v) {

  }



  Vec2f stepCalc (Clock c, int dur,) {


  }



float hTime = lerp(interop(clock, PIECE_TIME));
float spec1 = lerp(interop(clock, PIECE_TIME));


Time scale from 0 - 1 in heatTime
map sequential data in parameter to 0 - 1



int count = 0;
vector<biodiversities> rows = bioDiversityData.copyToStruct<biodiversities>();
for (auto row : rows) {
  std::cout << row.date << " " << row.comName << " " << row.site << std::endl;     
}

std::vector<temperatures> cols = temperatureData.copyToStruct<temperatures>();

std::cout << cols[3].temp << std::endl;                                

std::cout << "the count =" << count << std::endl;
                                                     


for (auto name : temperatureData.getColumnNames()) {
  std::cout << name << " | ";
}
std::cout << std::endl << "---------------" << std::endl;


for (auto name : bioDiversityData.getColumnNames()) {
  std::cout << name << " | ";
}
std::cout << std::endl << "---------------" << std::endl;


for (auto temp : temperatureData.getColumn(4)) {
  temps.push_back(temp);
} 

void valuePair (int *f, unsigned index = 0) {
  Vec2f v; 
  v.x = *(f + index);
  v.y = *(f + 1 + index); 
  cout << v << endl;
}

  // if (float(heat.temps[int(currentTime * TIME_MOD) % heat.temps.size()]) != currentTemp) {
    //   std::cout << int(currentTime * TIME_MOD) % heat.temps.size() << ": Current Temperature: " << heat.temps[int(currentTime * TIME_MOD)] << std::endl;
    //   // float diff = float(heat.temps[int(currentTime * TIME_MOD)+1]) - float(heat.temps[int(currentTime * TIME_MOD)+1]);
    //   // float frac = float(heat.temps[int(currentTime * TIME_MOD)]) - int(heat.temps[int(currentTime * TIME_MOD)]);
    //   currentTemp = float(heat.temps[int(currentTime * TIME_MOD)]);
      
    //}



float currentTemp = temps[0];
if (temps[int(currentTime * TIME_MOD) % temps.size()] != temp) {
  if (temps[int(currentTime * TIME_MOD) % temps.size()] == 0) {
    std::cout << "FIRST ELEMENT OF THE ARRAY" << std::endl;
  }
  std::cout << "Current Temperature: " << temps[int(currentTime * TIME_MOD)] << std::endl;
  temp = temps[int(currentTime * TIME_MOD)];
}
}



>> What every species needs to know... 
        >> How many individual flocks?                                                            | some number of vector<Flocks> see 122
        >> Where is "home" for each flock                                                         | a Vec3f that has an attraction force
        >> How many agents in every flock over time...                                            | flock[1].pop_ or flock[1].push
        >                                                                                         | requires moving tally of count average over time BY FLOCK... oof
        >> When adding new flock members... WHERE are they added... (center of the flock)         | NOT home, flockCenter Vec3f
        >> flock behaviors...
        >> environmental influence... 


USE THE UNION!!!! REFER TO THE SAME MEMORY ADDRESS IN BOTH ARRAY AND SINGLE VALUES!!!
....How to do that with vectors though... need to define N in arrays... we'll needs to hodgepodge
something...



*/


