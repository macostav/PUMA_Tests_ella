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
  double radiusElectrons = radiusCathode / 3.0;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist_angle(0, 2 * M_PI);
  std::uniform_real_distribution<double> dist_radius(0, 1);

  double theta = dist_angle(gen);
  double r = radiusElectrons * std::sqrt(dist_radius(gen));

  double x = r * std::cos(theta);
  double y = r * std::sin(theta);

  return {x, y};
}

int main() {
  bool plotting = true;
  bool debug = true;

  TApplication app("app", nullptr, nullptr);

  // Load COMSOL model
  ComponentComsol pumaModel;
  pumaModel.Initialise(
      "/data/emajkic/mesh_export_feb21.mphtxt",
      "/home/emajkic/PUMA_Tests/Simulations/dielectric_py.txt",
      "/data/emajkic/data_export_feb21.txt", "mm");

  if (debug) {
    pumaModel.PrintRange();
    pumaModel.PrintMaterials();
  }

  std::cout << "Model Initialized \n";

  // ViewField and Canvas (conditionally created)
  TCanvas* cf = nullptr;
  ViewField* fieldView = nullptr;

  if (plotting) {
    fieldView = new ViewField(&pumaModel);
    fieldView->SetPlane(0, -1, 0, 0, 0, 0);  // XZ-plane
    fieldView->SetArea(-4, 0, 4, 6.5);
    fieldView->SetVoltageRange(-1700., 0.);

    cf = new TCanvas("cf", "Electric Field", 800, 800);
    fieldView->SetCanvas(cf);
    fieldView->Plot("emag", "colz");
  }

  // Setup gas
  MediumMagboltz gas;
  gas.SetTemperature(293.15);
  gas.SetPressure(760.0);
  gas.SetComposition("Xe", 100.);
  gas.LoadIonMobility("IonMobility_Xe+_P32_Xe.txt");

  if (!gas.LoadGasFile("xenon_table.gas")) {
    gas.GenerateGasTable(5, false);
    gas.WriteGasFile("xenon_table.gas");
  }
  gas.Initialise(false);
  std::cout << "Gas Initialized \n";

  // ViewMedium (must stay alive)
  ViewMedium* mediumView = nullptr;
  if (debug) {
    mediumView = new ViewMedium();
    mediumView->SetMedium(&gas);
    mediumView->PlotElectronVelocity('e');
    std::cout << "ViewMedium Initialized \n";
  }

  // Attach gas to field model
  pumaModel.SetGas(&gas);

  // Sensor setup
  Sensor sensor;
  sensor.AddComponent(&pumaModel);
  sensor.SetArea(-3, -3, -15, 3, 3, 5);  // [cm]
  std::cout << "Sensor Initialized \n";

  // TrackHeed setup
  TrackHeed trackHeed;
  trackHeed.SetParticle("e-");
  trackHeed.SetSensor(&sensor);
  std::cout << "TrackHeed Initialized \n";

  // Drift view setup (conditionally used)
  TCanvas* cD = nullptr;
  ViewDrift* driftView1 = nullptr;

  if (plotting) {
    driftView1 = new ViewDrift();
    driftView1->Clear();
    cD = new TCanvas("cD", "Drift View", 800, 800);
    driftView1->SetCanvas(cD);
    driftView1->SetArea(-0.5, -0.5, 0, 0.5, 0.5, 5);
    std::cout << "DriftView Initialized \n";
  }

  DriftLineRKF drift;
  drift.SetSensor(&sensor);
  if (plotting) drift.EnablePlotting(driftView1);

  //Histogram for e- speed
  TH1F* hSpeed = new TH1F("hSpeed", "Electron Drift Speeds;Speed [cm/#mus];Counts", 100, 0, 1); // adjust range as needed

  // Drift loop
  int nRuns = 150;
  for (int i = 0; i < nRuns; i++) {
    auto [x0, y0] = randInCircle(); //[cm]
    double z0 = 4.32; //[cm]
    std::cout << "x0, y0, z0: " << x0 << ", " << y0 << ", " << z0 << std::endl;

    // Simulate trackHeed
    double energy = 1e6;
    trackHeed.NewTrack(x0, y0, z0, 0, 0, 1, energy);

    double xc, yc, zc, tc;
    int nc, nsec;
    double ec, esec;

    while (trackHeed.GetCluster(xc, yc, zc, tc, nc, nsec, ec, esec)) {
      std::cout << "Cluster at (" << xc << ", " << yc << ", " << zc << ")\n";
      for (int i = 0; i < nc; ++i) {
        drift.DriftElectron(xc, yc, zc, tc);

        double x1, y1, z1, t1;
        int status2;
        drift.GetEndPoint(x1, y1, z1, t1, status2);

        double dx = x1 - xc;
        double dy = y1 - yc;
        double dz = z1 - zc;
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz); // cm
        double time = t1 - tc; // ns

        if (time > 0) {
          double speed = distance / time; // cm/ns = 10 cm/μs
          hSpeed->Fill(speed * 1e3); // convert to cm/μs
        }
      }
    }
  }

  // Visualize e- drift lines
  driftView1->Plot();

  // Visualize histogram of e- speed
  TCanvas* cHist = new TCanvas("cHist", "Electron Speeds", 800, 600);
  hSpeed->Draw();

  app.Run(true);

  // Cleanup
  delete cf;
  delete cD;
  delete fieldView;
  delete driftView1;
  delete mediumView;

  return 0;
}
