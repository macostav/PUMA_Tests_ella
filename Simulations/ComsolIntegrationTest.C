#include <cstdlib>
#include <TApplication.h>
#include <TCanvas.h>
#include "Garfield/ComponentComsol.hh"

using namespace Garfield;

int main() {
    TApplication app("app", nullptr, nullptr);
    
    ComponentComsol platesModel;
    platesModel.Initialise("/home/emajkic/COMSOL_Files/minimal_mesh2.mphtxt", "/home/emajkic/COMSOL_Files/minimal_dielectric_dat.txt", "/home/emajkic/COMSOL_Files/minimal_potential2.txt", "m");

    std::cout << "Initialized model" << std::endl;

    platesModel.PrintRange();
    platesModel.PrintMaterials();

    app.Run();
    return 0;
}
