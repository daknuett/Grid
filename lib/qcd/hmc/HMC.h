//--------------------------------------------------------------------
/*! @file HMC.h
 * @brief Declaration of classes for HybridMonteCarlo update
 *
 * @author Guido Cossu
 */
//--------------------------------------------------------------------
#ifndef HMC_INCLUDED
#define HMC_INCLUDED

#include <string>
#include <memory>

namespace Grid{
  namespace QCD{
    
    struct HMCparameters{
      Integer Nsweeps; /* @brief Number of sweeps in this run */
      Integer TotalSweeps; /* @brief If provided, the total number of sweeps */
      Integer ThermalizationSteps;
      Integer StartingConfig;
      Integer SaveInterval; //Setting to 0 does not save configurations
      std::string Filename_prefix; // To save configurations
      
      HMCparameters();
    };
    
    template <class IntegType> 
    class HybridMonteCarlo{
      const HMCparameters Params;
      GridSerialRNG sRNG;
      GridParallelRNG pRNG;
      std::unique_ptr< Integrator<IntegType> > MD;
      

      bool metropolis_test(const RealD DeltaH){
	RealD rn_test;
	RealD prob = std::exp(-DeltaH);
	random(sRNG,rn_test);
      
	std::cout<< "--------------------------------------------\n";
	std::cout<< "dH = "<<DeltaH << "  Random = "<< rn_test 
		 << "\nAcc. Probability = " << ((prob<1.0)? prob: 1.0)<< "   ";
      
	if((prob >1.0) || (rn_test <= prob)){       // accepted
	  std::cout <<"-- ACCEPTED\n";
	  return true;
	} else {                               // rejected
	  std::cout <<"-- REJECTED\n";
	  return false;
	}
      }

      RealD evolve_step(LatticeColourMatrix& Uin){
     
	MD->init(Uin,pRNG);     // set U and initialize P and phi's 
	RealD H0 = MD->S();     // current state            
	std::cout<<"Total H_before = "<< H0 << "\n";
      
	MD->integrate(0);
      
	RealD H1 = MD->S();     // updated state            
	std::cout<<"Total H_after = "<< H1 << "\n";
      
	return (H1-H0);
      }

      
      
    public:
      HybridMonteCarlo(Integrator<IntegType>& MolDyn, GridBase* grid):MD(&MolDyn),pRNG(grid){
	//FIXME

	// initialize RNGs
	sRNG.SeedRandomDevice();
	pRNG.SeedRandomDevice();
      }
      ~HybridMonteCarlo(){};



      void evolve(LatticeColourMatrix& Uin){
	Real DeltaH;
	Real timer;
      
	// Thermalizations
	for(int iter=1; iter <= Params.ThermalizationSteps; ++iter){
	  std::cout << "-- # Thermalization step = "<< iter <<  "\n";
	
	  DeltaH = evolve_step(Uin);
	
	  std::cout<< "[Timing] Trajectory time (s) : "<< timer/1000.0 << "\n";
	  std::cout<< "dH = "<< DeltaH << "\n";
	
	  // Update matrix
	  Uin = MD->get_U();  //accept every time
	}

	// Actual updates
	for(int iter=Params.StartingConfig; 
	    iter < Params.Nsweeps+Params.StartingConfig; ++iter){
	  std::cout << "-- # Sweep = "<< iter <<  "\n";
		
	  DeltaH = evolve_step(Uin);
		
	  if(metropolis_test(DeltaH))  Uin = MD->get_U();
	  // need sync?	
	}

      
      }


    };
    
  }// QCD
}// Grid


#endif 
