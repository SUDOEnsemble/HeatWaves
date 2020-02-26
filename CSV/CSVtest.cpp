/*
NOTES

Going to need to use vsync or use some concept of time... is there a millis function?
*/


#include "al/app/al_App.hpp"
#include "al/system/al_Time.hpp"
#include "al/io/al_CSVReader.hpp"
using namespace al;

#include <fstream>
#include <vector>
using namespace std;



#define PIECE_TIME = 1200;                                  // This is total number of second in the piece


typedef struct {                                            // Struct for Temperature datapoints
  char site[32];
  char location[32];
  char date[32];
  char columnPos[32];
  float temp;
  char category[32];
} temperatures;

typedef struct {                                            // Struct for biodiversity datapoints
  int year;
  int month; 
  char date[32];
  char site[32];
  float transect; 
  float quad; 
  char side[32];
  char spCode[32];
  float size; 
  float count; 
  float area; 
  char sciName[32];
  char comName[32];
  char taxKingdom[32];
  char taxPhylum[32];
  char taxClass[32];
  char taxOrder[32];
  char taxFamily[32];
  char taxGenus[32];
  char group[32];
  char survey[32];
  char mobility[32];
  char growth[32];
} biodiversities;


struct MyApp : App {
  Clock clock;                                              // NEED TO FIGURE OUT SEQUENCE 

  vector<float> temps;
  
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
