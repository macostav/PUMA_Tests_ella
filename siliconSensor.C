#include <iostream>

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1F.h>

#include "Garfield/MediumSilicon.hh"
#include "Garfield/SolidBox.hh"
#include "Garfield/GeometrySimple.hh"
#include "Garfield/ComponentUser.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/AvalancheMC.hh"
#include "Garfield/ViewDrift.hh"



using namespace Garfield;

/*
* Simulates the signal in a planar silicon sensor due to the 
* passage of a charged particle. 
*
* Source: Garfield++ UserGuide 
*/

int main() {
    TApplication app("app", nullptr, nullptr);

    //Transport parameters for silicon
    MediumSilicon si;
    si.SetTemperature(293.15);

    //Active volume for detector
    constexpr double d = 100.e-4;
    SolidBox box(0, 0.5*d, 0, 2*d, 0.5*d, 2*d);

    //Set up geometry
    GeometrySimple geo;
    geo.AddSolid(&box, &si);

    //Depletion voltage for formula [V]
    constexpr double vdep = -20.;
    constexpr double vbias = -50.;

    //Lamba expression to calculate E-Field
    auto eLinear = [d,vbias,vdep](const double /*x*/, const double y,
                                  const double /*z*/,
                                  double &ex, double &ey, double &ez) {
        ex = ez = 0.;
        ey = (vbias - vdep) / d + 2*y*vdep / (d*d);
    };

    //Delegate calculation of E-Field to our lambda function
    ComponentUser linearField;
    linearField.SetGeometry(&geo);
    linearField.SetElectricField(eLinear);

    //Pass pointer to Component to a sensor
    Sensor sensor;
    sensor.AddComponent(&linearField);

    //Set voltages at planes to GND and Vbias
    ComponentAnalyticField wField;
    wField.SetGeometry(&geo);
    wField.AddPlaneY(0, vbias, "back");
    wField.AddPlaneY(d, 0, "front");

    //Define strip on front of sensor
    constexpr double pitch = 55.e-4;
    constexpr double halfpitch = 0.5*pitch;
    wField.AddStripOnPlaneY('z', d, -halfpitch, halfpitch, "strip");

    // Request signal calculation for the electrode named "strip",
    // using the weighting field provided by the Component object wField.
    sensor.AddElectrode(&wField, "strip");

    //Set granularity with which to record signal
    const unsigned int nTimeBins = 1000;
    const double tmin = 0.;
    const double tmax = 10.;
    const double tstep = (tmax - tmin) / nTimeBins;
    
    sensor.SetTimeWindow(tmin, tstep, nTimeBins);

    //Simulate electron/hole pairs produced by pion traversing sensor
    TrackHeed track;
    track.SetSensor(&sensor);
    track.SetParticle("pion");
    track.SetMomentum(180.e9);

    //Transport electrons and holes
    AvalancheMC drift;
    drift.SetSensor(&sensor);
    drift.SetDistanceSteps(1.e-4);

    //Run simulation
    double x0 = 0., y0 = 0., z0 = 0., t0 = 0.;
    double dx = 0., dy = 1., dz = 0.;
    track.NewTrack(x0, y0, z0, t0, dx, dy, dz);

    //Retrieve clusters along track
    for (const auto &cluster : track.GetClusters()) {
        //Loop over electrons in cluster
        for (const auto &electron : cluster.electrons) {
            //Simulate electron/hole drift lines
            drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);
            drift.DriftHole(electron.x, electron.y, electron.z, electron.t);

        }
    }

    ViewDrift driftView;
    driftView.SetArea(-0.5*d, 0, -0.5*d, 0.5*d, d, 0.5*d);
    track.EnablePlotting(&driftView);
    drift.EnablePlotting(&driftView);

    //Allegedly may take long
    constexpr bool twod = true;
    driftView.Plot(twod);

    TCanvas* cSignal = new TCanvas("cSignal", "", 600, 600);
    sensor.PlotSignal("strip", cSignal);

    app.Run();

    return 0;
}

