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

    //Initialize application and canvas
    TApplication app("app", nullptr, nullptr);

    //Import COMSOL model
    ComponentComsol pumaModel;
    pumaModel.Initialise("/data/emajkic/mesh_export_feb21.mphtxt", "/home/emajkic/PUMA_Tests/Simulations/dielectric.dat", "/data/emajkic/data_export_feb21.txt", "mm");

    if (debug) {
      std::ifstream file("/data/emajkic/mesh_export_feb21.mphtxt");
      if (!file) {
      std::cerr << "Error: mesh_export_feb21.mphtxt not found!" << std::endl;
      return 1;
    }

    }
    //Visualize field
    if (plotting) {
        ViewField fieldView;
        fieldView.SetComponent(&pumaModel);
        fieldView.SetPlane(0, -1, 0, 0, 0, 0); //xz plane ?
        fieldView.SetArea(-5, 0, 5, 6.5); //[cm]
        fieldView.SetVoltageRange(-1700., 0.);
        
        TCanvas* canvas1 = new TCanvas("Canvas", "Electric Field", 800, 800);
        fieldView.SetCanvas(canvas1);
        fieldView.PlotContour();
    }

    //Define gas medium
    MediumMagboltz gas;
    gas.SetTemperature(293.15);
    gas.SetPressure(760.0);
    gas.SetComposition("ar", 100.);
    gas.LoadIonMobility("IonMobility_Ar+_Ar.txt");
    gas.Initialise(true);

    //Attatch medium to PUMA model
    pumaModel.SetMedium(4, &gas);

    if (debug) {
        pumaModel.PrintRange();
        pumaModel.PrintMaterials();
        std::cout << "Model Initialized \n";
    }

    //Set up a sensor
    Sensor sensor;
    sensor.AddComponent(&pumaModel);
    sensor.SetArea(-5, -5, -16, 5, 5, 7); //[cm]

    if (debug) {
      std::cout << "Sensor Initialized \n";
    }

    //Send electrons from cathode into drift region
    DriftLineRKF driftline;
    driftline.SetSensor(&sensor);
    
    //TCanvas* cD = new TCanvas("Drift Canvas", "Canvas", 800, 800);

    // ViewDrift driftView;
    // driftView.SetCanvas(cD);
    // driftline.EnablePlotting(&driftView);
    
    //1 electron
    driftline.DriftElectron(0, 0, 4.5, 0); //NO E FIELD EHRE THATS THE PRIBLEM.... CHECK IMPORT?

    double xf, yf, zf, tf;
    int status;
  
    driftline.GetEndPoint(xf, yf, zf, tf, status);
    driftline.PrintDriftLine();

    if (debug) {
      std::cout << "status: " << status << " xf: " << xf << " yf: "<< yf << " zf: " << zf << " tf: " << tf << "\n" << std::endl;
    }
        

    /*
    int nRuns = 10;
    

    for (int i = 0; i < nRuns; i++) {
      auto [x0, y0] = randInCircle(); //extract x0 and y0 from my method
      const double z0 = 4.52; //[cm] few mm below cathode - from STEP CAD file measurement
      //std::cout << "x0: " << x0 << " y0: "<< y0 << " z0: " << z0 << "\n" << std::endl;

      //check if e-field exists
      if (debug) {
        double ex, ey, ez;
        Garfield::Medium* m = nullptr;  // Pointer to store the medium
        int status = 0;

        pumaModel.ElectricField(x0, y0, z0, ex, ey, ez, m, status);

        std::cout << "E-field at (" << x0 << ", " << y0 << ", " << z0 << "): "
          << ex << " " << ey << " " << ez << " Status: " << status << std::endl;
      }

      driftline.DriftElectron(x0, y0, z0, 0);

      double xf, yf, zf, tf;
      int status;

      driftline.GetEndPoint(xf, yf, zf, tf, status);

      if (debug) {
        std::cout << "status: " << status << " xf: " << xf << " yf: "<< yf << " zf: " << zf << " tf: " << tf << "\n" << std::endl;
      }
      

      //CALCULATE SPEED VIA s = d/t
      //plot hsitrogram of speeds?..
      //for now just drew drift line, ok dont need this but just sanity check
    }*/  

   // driftView.Plot();
      
    app.Run();
    
    return 0;
}