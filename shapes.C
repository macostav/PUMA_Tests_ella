
#include <iostream>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <random>

#include "Garfield/ComponentConstant.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/GeometrySimple.hh"
#include "Garfield/SolidSphere.hh"
#include "Garfield/MediumGas.hh"
#include "Garfield/ViewGeometry.hh"
#include "Garfield/SolidHole.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/MediumDiamond.hh"
#include "Garfield/SolidRidge.hh"
#include "Garfield/ViewMedium.hh"

#include <TPad.h>
#include <TApplication.h>

using namespace Garfield;
bool debug = true;

/*
* Visualizes a sphere filled with Xenon gas in 3 dimensions
*/
void makeSphere() {
    TApplication app("app", nullptr, nullptr);

    SolidSphere sphere(0,0,0,7);

    MediumGas gas;
    gas.SetComposition("xe", 100.);

    GeometrySimple geom;
    geom.AddSolid(&sphere, &gas);

    if (debug) {
        geom.PrintSolids();
    }  

    ViewGeometry view(&geom);

    TCanvas canvas("canvas", "ViewGeometry Canvas", 700, 600);
    view.SetCanvas(&canvas);
    view.Plot();

    app.Run();
}

/*
* Visualizes a ridge
*/
void makeRidge() {
    TApplication app("app", nullptr, nullptr);

    MediumDiamond diamond;
    
    SolidRidge ridge(0., 0., 0., 6., 6., 8., 0.);
    
    GeometrySimple geometry;
    geometry.AddSolid(&ridge, &diamond);

    ViewGeometry view(&geometry);
    ViewMedium mediumView(&diamond);

    TCanvas canvas("canvas", "ViewGeometry Canvas", 700, 600);

    view.SetCanvas(&canvas);
    mediumView.SetCanvas(&canvas);

    view.Plot3d();

    app.Run();
}


/*
* Only run 1 method call at once
*/
int main () {
    if (debug) {
        std::cout << "Program started." << std::endl;
    }

    //Method to visualize 3d sphere
    //makeSphere();

    //Method to visualize electric field
    makeRidge();
    
    return 0;
}