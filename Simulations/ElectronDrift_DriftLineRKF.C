#include <cstdlib>
#include <cmath>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1F.h>
#include "Garfield/ComponentComsol.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/ViewCell.hh"
#include "Garfield/ViewSignal.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/AvalancheMicroscopic.hh"
#include "Garfield/AvalancheMC.hh"
#include "Garfield/ViewDrift.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/Random.hh"
#include "Garfield/ComponentGrid.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/ViewFEMesh.hh"
#include "Garfield/ViewMedium.hh"

using namespace Garfield;

std::pair<double, double> randInCircle() {
  double radiusCathode = 0.5; 
  double radiusElectrons = radiusCathode/3.0;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist_angle(0, 2 * M_PI);
  std::uniform_real_distribution<double> dist_radius(0, 1); 

  // Generate random polar coordinates
  double theta = dist_angle(gen);
  double r = radiusElectrons * std::sqrt(dist_radius(gen));

  // Convert to Cartesian coordinates
  double x = r * std::cos(theta);
  double y = r * std::sin(theta); 

  return {x, y};
}

// use [cm] inputs

int main() {
    constexpr bool plotting = true;
    constexpr bool debug = true;

    TApplication app("app", nullptr, nullptr);

    //Import COMSOL model
    ComponentComsol pumaModel;
    pumaModel.Initialise("/data/emajkic/mesh_export_feb21.mphtxt", "/home/emajkic/PUMA_Tests/Simulations/dielectric.txt", "/data/emajkic/data_export_feb21.txt", "mm");
  
    //Visualize field
    if (plotting) {
        ViewField fieldView;
        fieldView.SetComponent(&pumaModel);
        fieldView.SetPlane(0, -1, 0, 0, 0, 0); //xz plane ?
        fieldView.SetArea(-5, 0, 5, 6.5); //[cm]
        fieldView.SetVoltageRange(-1700., 0.);
        
        TCanvas* canvas1 = new TCanvas("Canvas", "Electric Field", 800, 800);
        fieldView.SetCanvas(canvas1);
        fieldView.Plot("emag", "colz");
    }

    if (debug) {
      pumaModel.PrintRange();
      pumaModel.PrintMaterials();
    }

    //Define gas medium
    MediumMagboltz gas;
    gas.SetTemperature(293.15);
    gas.SetPressure(760.0);
    gas.SetComposition("Ar", 100.);
    gas.LoadIonMobility("IonMobility_Ar+_Ar.txt");
    bool loaded = gas.LoadGasFile("argon_table.gas");
    if (!loaded) {
      gas.GenerateGasTable(5, false);
      gas.WriteGasFile("argon_table.gas");
    } 
    gas.Initialise(true);

    //Attatch medium to PUMA model
    pumaModel.SetMedium(4, &gas);

    if (debug) {
      // To view e+ velocity
      ViewMedium mediumView;
      mediumView.SetMedium(&gas);
      mediumView.PlotElectronVelocity('e');
    }

    // //Set up a sensor
    // Sensor sensor;
    // sensor.AddComponent(&pumaModel);
    // sensor.SetArea(-5, -5, -16, 5, 5, 7); //[cm]

    // if (debug) {
    //   std::cout << "Sensor Initialized \n";
    // }
  





    //CALCULATE SPEED VIA s = d/t
    //plot hsitrogram of speeds?..
    //for now just drew drift line, ok dont need this but just sanity check
   
      
    app.Run();
    
    return 0;
}