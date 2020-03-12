#ifndef __HEATWAVES_DATA__
#define __HEATWAVES_DATA__

/*

KRAMER ELWELL, RODNEY DUPLESSIS, RAPHAEL RADNA
201B FINAL PROJECT
HEATWAVES - ALLOSPHERE MARINE RESEARCH SIMULATIN
Winter 2020

*/

#include "al/app/al_App.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_CSVReader.hpp"
#include "al/math/al_Interpolation.hpp"
#include "al/system/al_Time.hpp"
using namespace al;

#include <fstream>
#include <vector>
using namespace std;

// GLOBALS
#define DUR 1200
#define TIME_MOD 1

typedef struct { // Struct for a single Temperature datapoint
  double date, temp;
} Temperatures;

typedef struct { // Struct for a single Biodiversity datapoint
  double comName, site, date, count, transect, quad, taxPhylum, mobility,
      growth;
} Biodiversities;

//  ----------------------------------------------------------------------------------------------------------------------------------
//  BIODIVERSITY DATA STRUCT
//  ----------------------------------------------------------------------------------------------------------------------------------

struct Site {
  vector<Biodiversities> data;
  float currentCount, max, min, ave, site, taxPhylum, mobility, growth;
  Vec3f origin;

  void init() {
    if (data.size() > 0) {

      currentCount = data[0].count;
      float r = data[0].transect;
      float x = r * cos((data[0].site / 11.0) * M_2PI);
      float z = r * sin((data[0].site / 11.0) * M_2PI);
      float y = data[0].quad;
      origin.x = x;
      origin.y = y;
      origin.z = z;

      min = 10000;
      max = 0;
      for (int i = 0; i < data.size(); i++) {
        ave += data[i].count;
        if (data[i].count > max) {
          max = data[i].count;
        };
        if (data[i].count < min) {
          min = data[i].count;
        };
      }
      ave /= data.size();
    } else if (data.size() <= 0) {
      currentCount = 0;
      origin = Vec3f(0, 0, 0);
      min = 0;
      max = 0;
      ave = 0;
    }
  }

  void printAve() { std::cout << "Average: " << ave << std::endl; }
  void printMax() { std::cout << "Maximum: " << max << std::endl; }
  void printMin() { std::cout << "Minimum: " << min << std::endl; }

  void update(float time) {
    if (data.size() > 0) {
      float k = int(time * TIME_MOD) % data.size();
      float fract = fmodf(time * TIME_MOD, 1.0);

      float countDiff = data[k + 1].count - data[k].count; // Species Count
      currentCount = data[k].count + countDiff * fract;

      float rDiff = data[k + 1].transect - data[k].transect; // Origin Radius
      float r = data[k].transect + rDiff * fract;

      float x = r * cos((data[k].site / 11.0) * M_2PI); // Origin X-Position
      float z = r * sin((data[k].site / 11.0) * M_2PI); // Origin Z-Position

      float yDiff = data[k + 1].quad - data[k].quad; // Origin Y-Position
      float y = data[k].quad + yDiff * fract;

      origin.x = x;
      origin.y = y;
      origin.z = z;
    }
  }
};

struct Species {
  Site site[11];
};

//  ----------------------------------------------------------------------------------------------------------------------------------
//  HEAT DATA STRUCT
//  ----------------------------------------------------------------------------------------------------------------------------------

struct Heat {
  vector<Temperatures> data;
  float currentTemp, ave, min, max;

  void init() {
    min = data[0].temp;
    max = data[0].temp;
    for (int i = 0; i < data.size(); i++) {
      ave += data[i].temp;
      if (data[i].temp > max) {
        max = data[i].temp;
      };
      if (data[i].temp < min) {
        min = data[i].temp;
      };
    }
    ave /= data.size();
  }

  void printAve() { std::cout << "Average: " << ave << std::endl; }
  void printMax() { std::cout << "Maximum: " << max << std::endl; }
  void printMin() { std::cout << "Minimum: " << min << std::endl; }

  float update(float time) {
    float diff =
        (data[int(time * TIME_MOD) + 1].temp - data[int(time * TIME_MOD)].temp);
    float fract = fmodf(time * TIME_MOD, 1.0);
    currentTemp = data[time * TIME_MOD].temp + diff * fract;
    return currentTemp;
  }
};

// Heat heat; // May need to create this onCreate
// Species species[58];
// Clock c;

// void hLoad() {
//   CSVReader temperatureData;
//   temperatureData.addType(CSVReader::REAL); // Date
//   temperatureData.addType(CSVReader::REAL); // Temperature (c)
//   temperatureData.readFile("../data/_TEMP.csv");

//   std::vector<Temperatures> tRows =
//       temperatureData.copyToStruct<Temperatures>();
//   for (auto t : tRows) {
//     heat.data.push_back(t);
//   };

//   CSVReader bioDiversityData;
//   bioDiversityData.addType(CSVReader::REAL); // Name
//   bioDiversityData.addType(CSVReader::REAL); // Site
//   bioDiversityData.addType(CSVReader::REAL); // Date
//   bioDiversityData.addType(CSVReader::REAL); // Count
//   bioDiversityData.addType(CSVReader::REAL); // Transect
//   bioDiversityData.addType(CSVReader::REAL); // quad
//   bioDiversityData.addType(CSVReader::REAL); // Taxonomy | Phylum
//   bioDiversityData.addType(CSVReader::REAL); // Mobility
//   bioDiversityData.addType(CSVReader::REAL); // Growth_Morph
//   bioDiversityData.readFile("../data/_BIODIVERSE.csv");

//   std::vector<Biodiversities> bRows =
//       bioDiversityData.copyToStruct<Biodiversities>();
//   for (auto b : bRows) {
//     species[int(b.comName)].site[int(b.site)].data.push_back(b);
//   };

//   heat.init();
//   for (int i = 0; i < 58; i++) {
//     for (int j = 0; j < 11; j++) {
//       species[i].site[j].init();
//     }
//   }
// }

// float currentTemp = -1;
// float hStep() {
//   float currentTime = c.now();
//   float currentTemp = heat.update(currentTime);
//   // cout << currentTemp << endl;
//   for (int i = 0; i < 58; i++) {
//     for (int j = 0; j < 11; j++) {
//       species[i].site[j].update(currentTime);
//     }
//   }
//   c.update();
//   return currentTemp;
// }

#endif
