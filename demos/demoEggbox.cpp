//
// Compile with: clang++ -o demoEggbox demoEggbox.cpp -L../build/ -I ../include/ -l multinest -stdlib=libc++ -std=c++11
// 

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include "Functions.h"
#include "File.h"
#include "MultiEllipsoidSampler.h"
#include "KmeansClusterer.h"
#include "EuclideanMetric.h"
#include "Prior.h"
#include "UniformPrior.h"
#include "NormalPrior.h"
#include "Results.h"
#include "Ellipsoid.h"
#include "ZeroModel.h"
#include "FerozReducer.h"
#include "ExponentialReducer.h"
#include "demoEggBox.h"


int main(int argc, char *argv[])
{
    unsigned long Nrows;
    int Ncols;
    ArrayXXd data;
  
    // Creating dummy arrays for the covariates and the observations.
    // They're not used because we compute our Likelihood directly. 

    ArrayXd covariates;
    ArrayXd observations;


    // -------------------------------------------------------------------
    // ----- First step. Set up the models for the inference problem ----- 
    // -------------------------------------------------------------------

    // Set up a dummy model. This won't be used because we're computing
    // the Likelihood directly, but the Likelihood nevertheless expects a model in 
    // its constructor.
    
    ZeroModel model(covariates);


    // -------------------------------------------------------
    // ----- Second step. Set up all prior distributions -----
    // -------------------------------------------------------

    int Ndimensions = 2;        // Number of free parameters (dimensions) of the problem
    vector<Prior*> ptrPriors(1);
    ArrayXd parametersMinima(Ndimensions);
    ArrayXd parametersMaxima(Ndimensions);
    parametersMinima <<  0.0, 0.0;         // Centroid x direction, Centroid y direction
    parametersMaxima << 10.0*Functions::PI, 10.0*Functions::PI;
    UniformPrior uniformPrior(parametersMinima, parametersMaxima);
    ptrPriors[0] = &uniformPrior;
   

    // -----------------------------------------------------------------
    // ----- Third step. Set up the likelihood function to be used -----
    // -----------------------------------------------------------------
    
    EggboxLikelihood likelihood(observations, model);
    

    // -------------------------------------------------------------------------------
    // ----- Fourth step. Set up the K-means clusterer using an Euclidean metric -----
    // -------------------------------------------------------------------------------

    EuclideanMetric myMetric;
    int minNclusters = 4;
    int maxNclusters = 20;
    int Ntrials = 10;
    double relTolerance = 0.01;

    KmeansClusterer kmeans(myMetric, minNclusters, maxNclusters, Ntrials, relTolerance); 


    // ---------------------------------------------------------------------
    // ----- Sixth step. Configure and start nested sampling inference -----
    // ---------------------------------------------------------------------
       
    bool printOnTheScreen = true;                    // Print results on the screen
    int initialNobjects = 2000;                      // Initial number of active points evolving within the nested sampling process.
    int minNobjects = 2000;                          // Minimum number of active points allowed in the nesting process.
    int maxNdrawAttempts = 10000;                    // Maximum number of attempts when trying to draw a new sampling point.
    int NinitialIterationsWithoutClustering = 1000;  // The first N iterations, we assume that there is only 1 cluster.
    int NiterationsWithSameClustering = 100;         // Clustering is only happening every X iterations.
    double initialEnlargementFraction = 1.5;         // Fraction by which each axis in an ellipsoid has to be enlarged.
                                                     // It can be a number >= 0, where 0 means no enlargement.
    double shrinkingRate = 0.2;                      // Exponent for remaining prior mass in ellipsoid enlargement fraction.
                                                     // It is a number between 0 and 1. The smaller the slower the shrinkage
                                                     // of the ellipsoids.
    double terminationFactor = 0.05;                 // Termination factor for nesting loop.


    // Start the computation

    MultiEllipsoidSampler nestedSampler(printOnTheScreen, ptrPriors, likelihood, myMetric, kmeans, 
                                        initialNobjects, minNobjects, initialEnlargementFraction, shrinkingRate);

    double toleranceOnEvidence = 0.01;
    FerozReducer livePointsReducer(nestedSampler, toleranceOnEvidence);

    nestedSampler.run(livePointsReducer, NinitialIterationsWithoutClustering, NiterationsWithSameClustering, maxNdrawAttempts, terminationFactor);


    // -------------------------------------------------------
    // ----- Last step. Save the results in output files -----
    // -------------------------------------------------------
   
    Results results(nestedSampler);
    results.writeParametersToFile("demoEggBox_Parameter");
    results.writeLogLikelihoodToFile("demoEggBox_LikelihoodDistribution.txt");
    results.writeEvidenceInformationToFile("demoEggBox_EvidenceInformation.txt");
    results.writePosteriorProbabilityToFile("demoEggBox_PosteriorDistribution.txt");

    double credibleLevel = 68.3;
    bool writeMarginalDistributionToFile = true;
    results.writeParametersSummaryToFile("demoEggBox_ParameterSummary.txt", credibleLevel, writeMarginalDistributionToFile);


    // That's it!

    return EXIT_SUCCESS;
}
