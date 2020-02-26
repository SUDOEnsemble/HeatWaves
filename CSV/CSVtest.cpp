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

      transect 1 - 8 >>> mapped to x/z axis >>> 8 points >> distance from origin                
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


struct temperatures {   // Struct for Temperature datapoints
  char site[32];        // temp[0]
  char location[32];    // temp[1]
  char date[32];        // temp[2]
  char columnPos[32];   // temp[3]
  float temp;           // temp[4]
  char category[32];    // temp[5]
};


struct biodiversities { // Struct for biodiversity datapoints
  int year;             // data[0]    
  int month;            // data[1]
  char date[32];        // data[2]
  char site[32];        // data[3]
  float transect;       // data[4]
  float quad;           // data[5]
  char side[32];        // data[6]
  char spCode[32];      // data[7]
  float size;           // data[8]
  float count;          // data[9]
  float area;           // data[10]
  char sciName[32];     // data[11]
  char comName[32];     // data[12]
  char taxKingdom[32];  // data[13]
  char taxPhylum[32];   // data[14]
  char taxClass[32];    // data[15]
  char taxOrder[32];    // data[16]
  char taxFamily[32];   // data[17]
  char taxGenus[32];    // data[18]
  char group[32];       // data[19]
  char survey[32];      // data[20]
  char mobility[32];    // data[21]
  char growth[32];      // data[22]
};



struct Flocks {                                 // << ???
  vector<Agents> agents;
  vector<float> count; 

  float minimum, maximum, average;              // Does each flock have it's concept of this? 
};


struct Species {
  vector<biodiversities> data;
  vector<Flocks> flocks;

  float minimum, maximum, average;              // these can be calculated one time on create... 
  Vec3f homeOrigin, flockCenter;

  /*
  >> What every species needs to know... 
          >> How many individual flocks?                                                            | some number of vector<Flocks> see 122
          >> Where is "home" for each flock                                                         | a Vec3f that has an attraction force
          >> How many agents in every flock over time...                                            | flock[1].pop_ or flock[1].push
          >                                                                                         | requires moving tally of count average over time BY FLOCK... oof
          >> When adding new flock members... WHERE are they added... (center of the flock)         | NOT home, flockCenter Vec3f
          >> flock behaviors...
          >> environmental influence... 
  */

  homeOrigin = (0,0,0);   // calculated from site, transect, quad, and side
};





vector<char*> names = "Bat Star", "Aggregating anemone";






struct MyApp : App {
  Clock clock;                                              // NEED TO FIGURE OUT SEQUENCE 

  vector<float> temps;

  Species batStar;
  
  void onCreate() override {
    CSVReader temperatureData;
    temperatureData.addType(CSVReader::STRING);             // Site
    temperatureData.addType(CSVReader::STRING);             // Location
    temperatureData.addType(CSVReader::STRING);             // Date
    temperatureData.addType(CSVReader::STRING);             // Column Position
    temperatureData.addType(CSVReader::REAL);               // Temperature (c)
    temperatureData.addType(CSVReader::STRING);             // category
    temperatureData.readFile("data/temperature.csv");


    CSVReader bioDiversityData;
    bioDiversityData.addType(CSVReader::REAL);              // Year
    bioDiversityData.addType(CSVReader::REAL);              // Month
    bioDiversityData.addType(CSVReader::STRING);            // Date
    bioDiversityData.addType(CSVReader::STRING);            // Site
    bioDiversityData.addType(CSVReader::REAL);              // Transect
    bioDiversityData.addType(CSVReader::REAL);              // Quad
    bioDiversityData.addType(CSVReader::STRING);            // Side
    bioDiversityData.addType(CSVReader::STRING);            // SP_code
    bioDiversityData.addType(CSVReader::REAL);              // Size
    bioDiversityData.addType(CSVReader::REAL);              // Count
    bioDiversityData.addType(CSVReader::REAL);              // Area
    bioDiversityData.addType(CSVReader::STRING);            // Scientific Name
    bioDiversityData.addType(CSVReader::STRING);            // Common Name
    bioDiversityData.addType(CSVReader::STRING);            // Taxonomy | Kingdom
    bioDiversityData.addType(CSVReader::STRING);            // Taxonomy | Phylum
    bioDiversityData.addType(CSVReader::STRING);            // Taxonomy | Class
    bioDiversityData.addType(CSVReader::STRING);            // Taxonomy | Order
    bioDiversityData.addType(CSVReader::STRING);            // Taxonomy | Family
    bioDiversityData.addType(CSVReader::STRING);            // Taxonomy | Genus
    bioDiversityData.addType(CSVReader::STRING);            // Group
    bioDiversityData.addType(CSVReader::STRING);            // Survey 
    bioDiversityData.addType(CSVReader::STRING);            // Mobility
    bioDiversityData.addType(CSVReader::STRING);            // Growth_Morph
    bioDiversityData.readFile("data/bioDiversity.csv");


    vector<biodiversities> rows = bioDiversityData.copyToStruct<biodiversities>();
    for (int i = 0; i < rows.size(); i++) {
      for (int j = 0; j < names.size(); j++) {
        if (rows[i].comName == names[j]){                             // if the data entry has a common name "Bat Star" add the entire line of data to 
          
          
          batStar.data.push_back(rows[i]);
          // but need to replace batStar with appropriate species...
        }
      }                                                     
    }                                                          


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


    // need to do something like... 
    float increment = pieceTime * 1000 / temps.size(); 
    // interpolate from temp[0] to temp[1] over increment

    // but... need to do that for EVERY parameter... parameter parent struct? so struct temperatures : parameters {} or similar... 
    // then a species struct that includes all of these parameter structs that pulls all the data based on an if?

    float average = 0;
  for (auto temp : temps) {
    average += temp;
  }

    average /= temps.size();
    float tMaximum = *max_element(temps.begin(), temps.end());
    float tMinimum = *min_element(temps.begin(), temps.end());
    
    std::cout << "Average:" << average << std::endl;
    std::cout << "Maximum:" << tMaximum << std::endl;
    std::cout << "Minimum:" << tMinimum << std::endl;


    //std::vector<temperatures> temper = temperatureData.copyToStruct<temperatures>();
  }

  void onAnimate(double dt) override {
  }
};


int main() {
  MyApp app;
  app.start();
}
