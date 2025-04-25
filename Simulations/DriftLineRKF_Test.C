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
  double radiusCathode = 0.5; //[cm]
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
    bool plotting = true;
    bool debug = true;

    TApplication app("app", nullptr, nullptr);

    //Import COMSOL model
    ComponentComsol pumaModel;
    pumaModel.Initialise("/data/emajkic/mesh_export_feb21.mphtxt", "/home/emajkic/PUMA_Tests/Simulations/dielectric_py.txt", "/data/emajkic/data_export_feb21.txt", "mm");
  
    if (debug) {
      pumaModel.PrintRange();
      pumaModel.PrintMaterials();
    }

    std::cout << "Model Initialized \n";

    TCanvas* cf = nullptr;
    ViewField* fieldView = nullptr;
    //Visualize field
    if (plotting) {
      fieldView = new ViewField(&pumaModel);
      fieldView->SetPlane(0, -1, 0, 0, 0, 0);
      fieldView->SetArea(-4, 0, 4, 6.5);
      fieldView->SetVoltageRange(-1700., 0.);

      cf = new TCanvas("cf", "Electric Field", 800, 800);
      fieldView->SetCanvas(cf);
      fieldView->Plot("emag", "colz");

        // fieldView = new ViewField(&pumaModel);
        // fieldView.SetPlane(0, -1, 0, 0, 0, 0); //xz plane 
        // fieldView.SetArea(-4, 0, 4, 6.5); //[cm]
        // fieldView.SetVoltageRange(-1700., 0.);
        
        // cf = new TCanvas("cf", "Electric Field", 800, 800);
        // fieldView.SetCanvas(cf);
        // fieldView.Plot("emag", "colz");
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
    gas.Initialise(false); // false -> non-verbose output 

    std::cout << "Gas Initialized \n";


    ViewMedium* mediumView = new ViewMedium();
    mediumView->SetMedium(&gas);
    mediumView->PlotElectronVelocity('e');

    std::cout << "ViewMedium Initialized \n";

    //Attatch medium to PUMA model
    pumaModel.SetGas(&gas);

    //Set up a sensor
    Sensor sensor;
    sensor.AddComponent(&pumaModel);
    sensor.SetArea(-3, -3, -15, 3, 3, 5); //[cm] EXACT area

    std::cout << "Sensor Initialized \n";

    // Set up particle tracking using TrackHeed
    TrackHeed trackHeed;
    trackHeed.SetParticle("e-");  // Electron
    trackHeed.SetSensor(&sensor);

    std::cout << "Trackheed Initialized \n";
  
    //Visualization of drift - setup
    TCanvas* cD = nullptr;
    ViewDrift* driftView1 = nullptr;
    driftView1 = new ViewDrift();
    driftView1->Clear();
    cD = new TCanvas("cD", "Drift View", 800, 800);
    driftView1->SetCanvas(cD);
    driftView1->SetArea(-4.5, -4.5, 0, 4.5, 4.5, 5); // Adjust as needed
  
    std::cout << "DriftView Initialized \n";

    int nRuns = 5;
    for (int i = 0; i < nRuns; i++) {
        // Simulate a positron track at random starting position from my function
        auto [x0, y0] = randInCircle();
        double z0 = 43.2; //slight offset
        std::cout << "x0, y0, z0: " << x0 << ", " << y0 << ", " << z0 << std::endl;
        // double energy = 1e6;  // 1 MeV positron
        // trackHeed.NewTrack(x0, y0, z0, 0, 0, 1, energy);

    //     DriftLineRKF drift;
    //     drift.SetSensor(&sensor);
    //     drift.EnablePlotting(&driftView1);  
    
    //     // Retrieve ionization clusters and drift electrons
    //     double xc, yc, zc, tc;
    //     int nc, nsec;
    //     double ec, esec;
    //     while (trackHeed.GetCluster(xc, yc, zc, tc, nc, nsec, ec, esec)) {
    //         std::cout << "Cluster at (" << xc << ", " << yc << ", " << zc << ")\n";
    //         for (int i = 0; i < nc; ++i) {
    //             drift.DriftPositron(xc, yc, zc, tc);
    //         }
    //     }
    }

    // driftView1.Plot();




    //CALCULATE SPEED VIA s = d/t
    //plot hsitrogram of speeds?..
    //for now just drew drift line, ok dont need this but just sanity check
   
      
    app.Run();
    
    return 0;
}