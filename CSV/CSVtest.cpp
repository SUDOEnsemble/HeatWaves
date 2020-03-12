/*

KRAMER ELWELL, RODNEY DUPLESSIS, RAPHAEL RADNA
201B FINAL PROJECT
HEATWAVES - ALLOSPHERE MARINE RESEARCH SIMULATIN 
Winter 2020


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
*/


#include "al/app/al_App.hpp"
#include "al/system/al_Time.hpp"
#include "al/io/al_CSVReader.hpp"
using namespace al;

#include <fstream>
#include <vector>
using namespace std;


// GLOBALS  
int pieceTime = 1200;   // This is the total number of seconds in the piece
//int fps = FRAMERATE;


typedef struct {   // Struct for a single Temperature datapoint
  char site[32];        // temp[0]
  char location[32];    // temp[1]
  char date[32];        // temp[2]
  char columnPos[32];   // temp[3]
  float temp;           // temp[4]
  char category[32];    // temp[5]
} temperatures;


typedef struct { // Struct for a single biodiversity datapoint
  float date;        // data[0]
  float site;        // data[1]
  float transect;    // data[2]
  float quad;        // data[3]
  float size;        // data[4]
  float count;        // data[5]
  float comName;     // data[6]
  float taxPhylum;   // data[7]
  float mobility;    // data[8]
  float growth;      // data[9]
} biodiversities;



// struct Flocks {                                 // << ???
//   vector<Agents> agents;
//   vector<float> count; 

//   float minimum, maximum, average;              // Does each flock have it's concept of this? 
// };




// struct Species {
//   vector<biodiversities> data;
//   //vector<Flocks> flocks;

//   float minimum, maximum, average;              // these can be calculated one time on create... 
//   Vec3f homeOrigin, flockCenter;

  /*

  void step() {
    // all parameters that need to be interpolated ish
    float count = lerp(data[i].count, data[i+1].count, pieceTime / data.size());

  }



  void dump() {
      // dump data to std::out ...
  };
  void dump(int index) {
  };

  /*
  >> What every species needs to know... 
          >> How many individual flocks?                                                            | some number of vector<Flocks> see 122
          >> Where is "home" for each flock                                                         | a Vec3f that has an attraction force
          >> How many agents in every flock over time...                                            | flock[1].pop_ or flock[1].push
          >                                                                                         | requires moving tally of count average over time BY FLOCK... oof
          >> When adding new flock members... WHERE are they added... (center of the flock)         | NOT home, flockCenter Vec3f
          >> flock behaviors...
          >> environmental influence... 
  

  Vec3f homeOrigin = (0,0,0);   // calculated from site, transect, quad, and side
  */
// };




struct MyApp : App {
  Clock clock;                                              // NEED TO FIGURE OUT SEQUENCE 

  vector<float> temps;
  // vector<Species> species;




  void onCreate() override {
    CSVReader temperatureData;
    temperatureData.addType(CSVReader::STRING);             // Site
    temperatureData.addType(CSVReader::STRING);             // Location
    temperatureData.addType(CSVReader::STRING);             // Date
    temperatureData.addType(CSVReader::STRING);             // Column Position
    temperatureData.addType(CSVReader::REAL);               // Temperature (c)
    temperatureData.addType(CSVReader::STRING);             // category
    temperatureData.readFile("../data/temperature.csv");


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


    //int count = 0;
    // vector<biodiversities> rows = bioDiversityData.copyToStruct<biodiversities>();
    // for (auto row : rows) {
    //   std::cout << row.date << " " << row.comName << " " << row.site << std::endl;     
    // }

    // vector<temperatures> cols = temperatureData.copyToStruct<temperatures>();
    //  for (auto col : cols) {
    //   std::cout << col.temp << std::endl;     
    // }                                      
    
    // std::cout << "the count =" << count << std::endl;
                                                     


    // for (auto name : temperatureData.getColumnNames()) {
    //   std::cout << name << " | ";
    // }
    // std::cout << std::endl << "---------------" << std::endl;


    // for (auto name : bioDiversityData.getColumnNames()) {
    //   std::cout << name << " | ";
    // }
    // std::cout << std::endl << "---------------" << std::endl;


    for (auto temp : temperatureData.getColumn(4)) {
      temps.push_back(temp);
    } 

    std::cout << temps[5] << std::endl;



    // // need to do something like... 
    // float increment = pieceTime * 1000 / temps.size(); 
    // // interpolate from temp[0] to temp[1] over increment

    // // but... need to do that for EVERY parameter... parameter parent struct? so struct temperatures : parameters {} or similar... 
    // // then a species struct that includes all of these parameter structs that pulls all the data based on an if?

    // float average = 0;
    // for (auto temp : temps) {
    //   average += temp;
    // }
    // average /= temps.size();

    // float tMaximum = *max_element(temps.begin(), temps.end());
    // float tMinimum = *min_element(temps.begin(), temps.end());
    
    // std::cout << "Average:" << average << std::endl;
    // std::cout << "Maximum:" << tMaximum << std::endl;
    // std::cout << "Minimum:" << tMinimum << std::endl;
  }

  void onAnimate(double dt) override {
  }
};


int main() {
  MyApp app;
  app.start();
}
