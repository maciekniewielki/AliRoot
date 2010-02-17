/**************************************************************************
 * This file is property of and copyright by the Experimental Nuclear     *
 * Physics Group, Dep. of Physics                                         *
 * University of Oslo, Norway, 2007                                       *
 *                                                                        *
 * Author: Per Thomas Hille <perthi@fys.uio.no> for the ALICE HLT Project.*
 * Contributors are mentioned in the code where appropriate.              *
 * Please report bugs to perthi@fys.uio.no                                *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/


// Extraction of Amplitude and peak
// position using specila algorithm
// from Alexei Pavlinov
// ----------------
// ----------------

#include "AliCaloRawAnalyzerFastFit.h"
#include "AliCaloFastAltroFitv0.h"
#include "AliCaloFitResults.h"
#include "AliCaloBunchInfo.h"

#include <iostream>

using namespace std;

ClassImp( AliCaloRawAnalyzerFastFit )

AliCaloRawAnalyzerFastFit::AliCaloRawAnalyzerFastFit() : AliCaloRawAnalyzer("Fast Fit (Alexei)")
{
  // Comment

  for(int i=0; i <  1008; i++)
    {
      fXAxis[i] = i;
    }

}

AliCaloRawAnalyzerFastFit::~AliCaloRawAnalyzerFastFit()
{

}


AliCaloFitResults 
AliCaloRawAnalyzerFastFit::Evaluate( const vector<AliCaloBunchInfo> &bunchvector, 
				    const UInt_t altrocfg1,  const UInt_t altrocfg2 )
{
  // Comment

  short maxampindex; //index of maximum amplitude
  short maxamp; //Maximum amplitude
  int index = SelectBunch( bunchvector,  &maxampindex,  &maxamp );
 
  if( index >= 0)
    {
      Float_t ped = ReverseAndSubtractPed( &(bunchvector.at(index))  ,  altrocfg1, altrocfg2, fReversed  );
      int first;
      int last;
      int maxrev =  maxampindex -  bunchvector.at(index).GetStartBin();
      short timebinOffset = maxampindex - (bunchvector.at(index).GetLength()-1);

      double maxf =  maxamp - ped;

      if ( maxf > fAmpCut )
	{
	  SelectSubarray( fReversed,  bunchvector.at(index).GetLength(), maxrev , &first, &last);
	  int nsamples =  last - first + 1;

	  //amp  = dAmp;
	  //	  time = dTime * GetRawFormatTimeBinWidth();

	  //	  int length =  bunchvector.at(index).GetLength(); 

	  if( ( nsamples  )  >= fNsampleCut )  
	    {
	      Double_t ordered[1008];

	      for(int i=0; i < nsamples ; i++ )
		{
		  ordered[i] = fReversed[first + i];
		}
	      /*
	      cout << __FILE__ << __LINE__ << "!!!!!!! USING these samples" << endl; 
	      for(int i=0; i < nsamples ; i++ )
		{
		  ordered[i] = fReversed[first + nsamples -i -1];
		  cout << ordered[i] << "\t" ;
		}
	      cout << __FILE__ << __LINE__ << "!!!!!!! Done printing" << endl; 
	      */

	      Double_t eSignal = 1; // nominal 1 ADC error
	      Double_t dAmp = maxf; 
	      Double_t eAmp = 0;
	      Double_t dTime = 0;
	      Double_t eTime = 0;
	      Double_t chi2 = 0;
	      Double_t dTau = 2.35; // time-bin units
	      
	      AliCaloFastAltroFitv0::FastFit(fXAxis, ordered , nsamples,
					     eSignal, dTau, dAmp, eAmp, dTime, eTime, chi2);
	   
	      return AliCaloFitResults(maxamp, ped, -1,  dAmp, dTime+timebinOffset,  chi2,  -3 );
	    }

	}
    }    
  
  return AliCaloFitResults(9999 , 9999 , 9999,  9999, 9999, 9999, 9999 );
}
