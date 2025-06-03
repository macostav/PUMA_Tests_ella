#include <cstdlib>
#include <cmath>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <cstdlib>
#include <TApplication.h>
#include <TCanvas.h>

#include "Garfield/ComponentComsol.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/ViewMedium.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/ViewCell.hh"


using namespace Garfield;

std::pair<double, double> randInCircle() {
    double radiusElectrons = 2.0; //[cm]
  
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist_angle(0, 2 * M_PI);
    std::uniform_real_distribution<double> dist_radius(0, 1); 
  
    // Generate random polar coordinates
    double theta = dist_angle(gen);
    double r = radiusElectrons * std::sqrt(dist_radius(gen));
  
    // Convert to Cartesian coordinates
    double x = r * std::cos(theta) + 2.5;
    double y = r * std::sin(theta) + 2.5; 
  
    return {x, y};
  }

int main() {
    TApplication app("app", nullptr, nullptr);
    
    ComponentComsol platesModel;
    platesModel.Initialise("/home/macosta/ella_work/PUMA_Tests/Simulations/Comsol_Testing_Miguel/good_mesh.mphtxt","/home/macosta/PUMA/emma_work/PUMA/Simulations/COMSOL/SmallVersions/minimal_dielectric_dat.txt" ,"/home/macosta/ella_work/PUMA_Tests/Simulations/Comsol_Testing_Miguel/good_potential1.txt", "m");

    // File that works fine: /home/macosta/PUMA/emma_work/PUMA/Simulations/COMSOL/SmallVersions/minimal_mesh1.mphtxt
    // File that causes issues: /home/macosta/ella_work/PUMA_Tests/Simulations/Comsol_Testing_Miguel/minimal_mesh1_finer.mphtxt
    // /home/macosta/ella_work/PUMA_Tests/Simulations/Comsol_Testing_Miguel/working_mesh.mphtxt
    
    // std::cout << "Initialized model" << std::endl;

    platesModel.PrintRange();
    platesModel.PrintMaterials();

    ViewField fieldView(&platesModel);
    fieldView.SetPlane(0, -1, 0, 0, 0, 0);
    fieldView.SetArea(-1, -1, 6, 11);
    fieldView.SetVoltageRange(0., 1000.);

    TCanvas* cf = new TCanvas("cf", "", 600, 600);
    fieldView.SetCanvas(cf);
    fieldView.Plot("emag", "colz");

    //Define gas medium
    MediumMagboltz argon;
    argon.SetTemperature(293.15);
    argon.SetPressure(760.0);
    argon.SetComposition("Ar", 100.);

    bool loaded = argon.LoadGasFile("../argon_table.gas");
    if (!loaded) {
        std::cout << "Gas file not found. Generating gas table..." << std::endl;
        argon.GenerateGasTable(5, false);
        argon.WriteGasFile("argon_table.gas");
    } else {
        std::cout << "Gas table loaded successfully." << std::endl;
    }

    argon.Initialise(false); // false -> non-verbose output

    // To view e+ velocity
    ViewMedium mediumView;
    mediumView.SetMedium(&argon);
    mediumView.PlotElectronVelocity('e');

    platesModel.SetGas(&argon);

    // Initialize sensor
    Sensor sensor;
    sensor.AddComponent(&platesModel);
    sensor.SetArea(0, 0, 0, 5, 5, 10.2); //[cm]

    // Set up particle tracking using TrackHeed
    TrackHeed trackHeed;
    trackHeed.SetParticle("e+");  // Positron
    trackHeed.SetSensor(&sensor);

    // Visualization setup
    ViewDrift driftView;
    TCanvas* cD = new TCanvas("cD", "Drift View", 600, 600);
    driftView.SetCanvas(cD);
    driftView.SetArea(0, 0, 0, 5, 5, 10);  // Manually setting plot limits

    int nRuns = 150;
    for (int i = 0; i < nRuns; i++) {
        // Simulate a positron track at random starting position from my function
        auto [x0, y0] = randInCircle();
        double z0 = 9.5;
        double energy = 1e6;  // 1 MeV positron
        trackHeed.NewTrack(x0, y0, z0, 0, 0, 1, energy);

        DriftLineRKF drift;
        drift.SetSensor(&sensor);
        drift.EnablePlotting(&driftView);  
    
        // Retrieve ionization clusters and drift electrons
        double xc, yc, zc, tc;
        int nc, nsec;
        double ec, esec;
        while (trackHeed.GetCluster(xc, yc, zc, tc, nc, nsec, ec, esec)) {
            std::cout << "Cluster at (" << xc << ", " << yc << ", " << zc << ")\n";
            for (int i = 0; i < nc; ++i) {
                drift.DriftPositron(xc, yc, zc, tc);
            }
        }
    }

    driftView.Plot();

    app.Run();
    return 0;
}
