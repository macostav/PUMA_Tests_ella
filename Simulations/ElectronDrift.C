#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <random>

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

int main() {
    constexpr bool plotting = true;
    constexpr bool debug = true;

    //Initialize application and canvas
    TApplication app("app", nullptr, nullptr);
    TCanvas canvas = new TCanvas("Canvas", "Drift", 800, 800);

    //Import COMSOL model
    ComponentComsol pumaModel;
    pumaModel.Initialise("/data/emajkic/mesh_export_feb21.mphtxt", "/home/emajkic/PUMA_Tests/Simulations/dielectric.dat", "/data/emajkic/data_export_feb21.txt", "mm");
    if (debug) {
        pumaModel.PrintRange();
        pumaModel.PrintMaterials();
        std::cout << "Model Initialized \n";
    }


    //Define gas medium
    MediumMagboltz gas;
    gas.SetTemperature(293.15);
    gas.SetPressure(760.0);
    gas.SetComposition("Ar", 100.);

    //Attatch medium to PUMA model
    pumaModel.SetMedium(4, &gas);

    //Visualize field
    if (plotting) {
        ViewField fieldView;
        fieldView.SetComponent(&pumaModel);
        fieldView.SetPlane(0, -1, 0, 0, 0, 0); //xz plane ?
        fieldView.SetArea(-40, 0, 40, 40);
        fieldView.SetVoltageRange(-1700., 0.);
        
        TCanvas* cf = new TCanvas("cf", "Electric Field", 800, 800);
        fieldView.SetCanvas(cf);
        //fieldView.PlotContour();
    }

    //Set up a sensor
    Sensor sensor;
    sensor.AddComponent(&pumaModel);

    //Electron aval
    AvalancheMicroscopic avalanche;
    avalanche.SetSensor(&sensor);

    
    //Histogram of drift v
    TH1F* hDriftVelocity = new TH1F("hDriftVelocity", "Electron Drift Velocity;Velocity [mm/ns];Count", 50, 0, 1);

    // Simulate electron drift
    //Generate radnom nums for starting positions
    std::mt19937 rng;
    std::uniform_real_distribution<double> distX(-5, 5);
    std::uniform_real_distribution<double> distY(-5, 5);
    std::uniform_real_distribution<double> distZ(43.5, 43.535);
 
    for (int i = 0; i < 1000; ++i) {
        double x0 = distX(rng);
        double y0 = distY(rng);
        double z0 = distZ(rng);
        double t0 = 0.0;
         
        avalanche.Clear();
        avalanche.AvalancheElectron(x0, y0, z0, t0);
         
        double xf, yf, zf, tf;
        avalanche.GetElectronEndpoint(0, x0, y0, z0, t0, xf, yf, zf, tf);
         
        double driftTime = tf - t0;
        double driftDistance = std::sqrt((xf - x0) * (xf - x0) + (yf - y0) * (yf - y0) + (zf - z0) * (zf - z0));
        double driftVelocity = driftDistance / driftTime;
 
        hDriftVelocity->Fill(driftVelocity);
    }
 
     // Plot drift velocity histogram
     if (plotting) {
         TCanvas* cDrift = new TCanvas("cDrift", "Drift Velocity", 800, 600);
         hDriftVelocity->Draw();
         cDrift->Update();
     }
    
 
    app.Run();
    
    return 0;
}