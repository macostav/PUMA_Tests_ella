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
  double radiusCathode = 5.0/3.0; //check ~10mm right now

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist_angle(0, 2 * M_PI);
  std::uniform_real_distribution<double> dist_radius(0, 1); 

  // Generate random polar coordinates
  double theta = dist_angle(gen);
  double r = radiusCathode * std::sqrt(dist_radius(gen));

  // Convert to Cartesian coordinates
  double x = r * std::cos(theta);
  double y = r * std::sin(theta);

  return {x, y};

}

int main() {
    constexpr bool plotting = true;
    constexpr bool debug = true;

    //Initialize application and canvas
    TApplication app("app", nullptr, nullptr);

    //Import COMSOL model
    ComponentComsol pumaModel;
    pumaModel.Initialise("/data/emajkic/mesh_export_feb21.mphtxt", "/home/emajkic/PUMA_Tests/Simulations/dielectric.dat", "/data/emajkic/data_export_feb21.txt", "mm");

    //Visualize field
    if (plotting) {
        ViewField fieldView;
        fieldView.SetComponent(&pumaModel);
        fieldView.SetPlane(0, -1, 0, 0, 0, 0); //xz plane ?
        fieldView.SetArea(-5, 0, 5, 6.5);
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
    gas.Initialise(true);
    gas.LoadIonMobility("IonMobility_Ar+_Ar.txt");

    //Attatch medium to PUMA model
    pumaModel.SetGas(&gas);

    if (debug) {
        pumaModel.PrintRange();
        pumaModel.PrintMaterials();
        std::cout << "Model Initialized \n";
    }

    //Set up a sensor
    Sensor sensor;
    sensor.AddComponent(&pumaModel);
    sensor.SetArea(-4.5, -5, -16, 4.5, 5, 7);

    if (debug) {
      std::cout << "Sensor Initialized \n";
    }

    /*
    AvalancheMicroscopic aval(&sensor);

    AvalancheMC drift(&sensor);
    drift.SetDistanceSteps(2.e-4);
  
    ViewDrift driftView;

    if (plotting) {
      aval.EnablePlotting(&driftView);
      drift.EnablePlotting(&driftView);
    }
  
    //Start with 10 events while debugging; more later
    constexpr unsigned int nEvents = 10;
    for (unsigned int i = 0; i < nEvents; ++i) {
      std::cout << i << "/" << nEvents << "\n";
      // Randomize the initial position.
      auto [x0, y0] = randInCircle(); //extract x0 and y0 from my method
      const double z0 = 45.155;
      const double t0 = 0.;
      const double e0 = 0.0;

      aval.AvalancheElectron(x0, y0, z0, t0, e0, 0., 0., 0.);
      int ne = 0, ni = 0;
      aval.GetAvalancheSize(ne, ni);
      for (const auto& electron : aval.GetElectrons()) {
        const auto& p0 = electron.path.front();
        drift.DriftIon(p0.x, p0.y, p0.z, p0.t);
      }
    }

    if (plotting) {
      TCanvas* cd = new TCanvas();
      constexpr bool plotMesh = true;
      if (plotMesh) {
        ViewFEMesh* meshView = new ViewFEMesh(&pumaModel);
        meshView->SetArea(-4.5, 0, 0, 4.5, 20, 45);
        meshView->SetCanvas(cd);
        // x-z projection.
        meshView->SetPlane(0, -1, 0, 0, 0, 0);
        meshView->SetFillMesh(true);
        // Set the color of the kapton and the metal.
        meshView->SetColor(1, kYellow + 3);
        meshView->SetColor(2, kGray);
        meshView->EnableAxes();
        meshView->SetViewDrift(&driftView);
        meshView->Plot();
      } else {
        driftView.SetPlane(0, -1, 0, 0, 0, 0);
        driftView.SetArea(-4.5, 0, 0, 4.5, 20, 45);
        driftView.SetCanvas(cd);
        constexpr bool twod = true;
        driftView.Plot(twod);
      }
    }
    

    
    //Electron aval
    AvalancheMicroscopic avalanche;
    avalanche.SetSensor(&sensor);

    //Histogram of drift v
    TH1F* hDriftVelocity = new TH1F("hDriftVelocity", "Electron Drift Velocity;Velocity [mm/ns];Count", 50, 0, 1);
 
    for (int i = 0; i < 0; i++) {
        auto [x0, y0] = randInCircle(); //extract x0 and y0 from my method
        std::cout << "x0: " << x0 << "y0: " << y0 << std::endl;
        double z0 = 45.221;
        double t0 = 0.0;
        double e0 = 0.0;
         
        avalanche.AvalancheElectron(x0, y0, z0, t0, 0.0, 0.0, 0.0, 0.0);
         
        double xf, yf, zf, tf;
        double ef, dx, dy, dz;  // Capture additional output values
        int status;  // Status flag
        avalanche.GetElectronEndpoint(i, x0, y0, z0, t0, e0, xf, yf, zf, tf, ef, status);
         
        double driftTime = tf - t0;
        double driftDistance = std::sqrt((xf - x0) * (xf - x0) + (yf - y0) * (yf - y0) + (zf - z0) * (zf - z0));
        double driftVelocity = driftDistance / driftTime;

        std::cout << driftVelocity << std::endl;
 
        hDriftVelocity->Fill(driftVelocity);
    }

    if (debug) {
        std::cout << "Electron avalanche complete \n";
    }
 
     // Plot drift velocity histogram
     if (plotting) {
         TCanvas* cDrift = new TCanvas("cDrift", "Drift Velocity", 800, 600);
         hDriftVelocity->Draw();
         cDrift->Update();
     }*/
      
    app.Run();
    
    return 0;
}