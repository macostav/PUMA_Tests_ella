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
    //Initialize application and canvas
    TApplication app("app", nullptr, nullptr);
    TCanvas canvas("Canvas", "Drift", 800, 800);

    //Import COMSOL model
    ComponentComsol pumaModel;
    pumaModel.Initialise("/data/emajkic/mesh_export_feb21.txt", "Garfield/Simulations/dielectric.dat", "/data/emajkic/data_export_feb21.txt", "mm");
    pumaModel.PrintRange;

    //Dimensions

    return 0;
}