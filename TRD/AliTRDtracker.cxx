/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/
                                                      
/*
$Log$
Revision 1.17  2002/06/13 12:09:58  hristov
Minor corrections

Revision 1.16  2002/06/12 09:54:36  cblume
Update of tracking code provided by Sergei

Revision 1.14  2001/11/14 10:50:46  cblume
Changes in digits IO. Add merging of summable digits

Revision 1.13  2001/05/30 12:17:47  hristov
Loop variables declared once

Revision 1.12  2001/05/28 17:07:58  hristov
Last minute changes; ExB correction in AliTRDclusterizerV1; taking into account of material in G10 TEC frames and material between TEC planes (C.Blume,S.Sedykh)

Revision 1.8  2000/12/20 13:00:44  cblume
Modifications for the HP-compiler

Revision 1.7  2000/12/08 16:07:02  cblume
Update of the tracking by Sergei

Revision 1.6  2000/11/30 17:38:08  cblume
Changes to get in line with new STEER and EVGEN

Revision 1.5  2000/11/14 14:40:27  cblume
Correction for the Sun compiler (kTRUE and kFALSE)

Revision 1.4  2000/11/10 14:57:52  cblume
Changes in the geometry constants for the DEC compiler

Revision 1.3  2000/10/15 23:40:01  cblume
Remove AliTRDconst

Revision 1.2  2000/10/06 16:49:46  cblume
Made Getters const

Revision 1.1.2.2  2000/10/04 16:34:58  cblume
Replace include files by forward declarations

Revision 1.1.2.1  2000/09/22 14:47:52  cblume
Add the tracking code

*/   

#include <iostream.h>

#include <TFile.h>
#include <TBranch.h>
#include <TTree.h>  
#include <TObjArray.h> 

#include "AliTRDgeometry.h"
#include "AliTRDparameter.h"
#include "AliTRDgeometryDetail.h"
#include "AliTRDcluster.h" 
#include "AliTRDtrack.h"
#include "../TPC/AliTPCtrack.h"

#include "AliTRDtracker.h"

ClassImp(AliTRDtracker) 

  const  Float_t     AliTRDtracker::fSeedDepth          = 0.5; 
  const  Float_t     AliTRDtracker::fSeedStep           = 0.10;   
  const  Float_t     AliTRDtracker::fSeedGap            = 0.25;  

  const  Float_t     AliTRDtracker::fMaxSeedDeltaZ12    = 40.;  
  const  Float_t     AliTRDtracker::fMaxSeedDeltaZ      = 25.;  
  const  Float_t     AliTRDtracker::fMaxSeedC           = 0.0052; 
  const  Float_t     AliTRDtracker::fMaxSeedTan         = 1.2;  
  const  Float_t     AliTRDtracker::fMaxSeedVertexZ     = 150.; 

  const  Double_t    AliTRDtracker::fSeedErrorSY        = 0.2;
  const  Double_t    AliTRDtracker::fSeedErrorSY3       = 2.5;
  const  Double_t    AliTRDtracker::fSeedErrorSZ        = 0.1;

  const  Float_t     AliTRDtracker::fMinClustersInSeed  = 0.7;  

  const  Float_t     AliTRDtracker::fMinClustersInTrack = 0.5;  
  const  Float_t     AliTRDtracker::fMinFractionOfFoundClusters = 0.8;  

  const  Float_t     AliTRDtracker::fSkipDepth          = 0.05;
  const  Float_t     AliTRDtracker::fLabelFraction      = 0.8;  
  const  Float_t     AliTRDtracker::fWideRoad           = 20.;

  const  Double_t    AliTRDtracker::fMaxChi2            = 24.; 


//____________________________________________________________________
AliTRDtracker::AliTRDtracker(const TFile *geomfile)
{
  // 
  //  Main constructor
  //  

  Float_t fTzero = 0;
  Int_t   version = 2;
  
  fAddTRDseeds = kFALSE;

  fGeom = NULL;
  
  TDirectory *savedir=gDirectory; 
  TFile *in=(TFile*)geomfile;  
  if (!in->IsOpen()) {
    printf("AliTRDtracker::AliTRDtracker(): geometry file is not open!\n");
    printf("    DETAIL TRD geometry and DEFAULT TRD parameter will be used\n");
  }
  else {
    in->cd();  
    in->ls();
    fGeom = (AliTRDgeometry*) in->Get("TRDgeometry");
    fPar  = (AliTRDparameter*) in->Get("TRDparameter");
    fGeom->Dump();
  }

  if(fGeom) {
    //    fTzero = geo->GetT0();
    fTzero = 0.;
    version = fGeom->IsVersion();
    printf("Found geometry version %d on file \n", version);
  }
  else { 
    printf("AliTRDtracker::AliTRDtracker(): cann't find TRD geometry!\n");
    printf("    DETAIL TRD geometry and DEFAULT TRD parameter will be used\n");
    fGeom = new AliTRDgeometryDetail(); 
    fPar = new AliTRDparameter();
  }

  savedir->cd();  


  //  fGeom->SetT0(fTzero);

  fEvent     = 0;

  fNclusters = 0;
  fClusters  = new TObjArray(2000); 
  fNseeds    = 0;
  fSeeds     = new TObjArray(2000);
  fNtracks   = 0;
  fTracks    = new TObjArray(1000);

  for(Int_t geom_s = 0; geom_s < kTRACKING_SECTORS; geom_s++) {
    Int_t tr_s = CookSectorIndex(geom_s);
    fTrSec[tr_s] = new AliTRDtrackingSector(fGeom, geom_s, fPar);
  }

  fSY2corr = 0.025;
  fSZ2corr = 12.;      

  // calculate max gap on track

  Double_t dxAmp = (Double_t) fGeom->CamHght();   // Amplification region
  Double_t dxDrift = (Double_t) fGeom->CdrHght(); // Drift region

  Double_t dx = (Double_t) fPar->GetTimeBinSize();   
  Int_t tbAmp = fPar->GetTimeBefore();
  Int_t maxAmp = (Int_t) ((dxAmp+0.000001)/dx);
  Int_t tbDrift = fPar->GetTimeMax();
  Int_t maxDrift = (Int_t) ((dxDrift+0.000001)/dx);

  tbDrift = TMath::Min(tbDrift,maxDrift);
  tbAmp = TMath::Min(tbAmp,maxAmp);

  fTimeBinsPerPlane = tbAmp + tbDrift;
  fMaxGap = (Int_t) (fTimeBinsPerPlane * fGeom->Nplan() * fSkipDepth);

  fVocal = kFALSE;

}   

//___________________________________________________________________
AliTRDtracker::~AliTRDtracker()
{
  delete fClusters;
  delete fTracks;
  delete fSeeds;
  delete fGeom;  
  delete fPar;  

  for(Int_t geom_s = 0; geom_s < kTRACKING_SECTORS; geom_s++) {
    delete fTrSec[geom_s];
  }
}   

//_____________________________________________________________________
inline Double_t f1trd(Double_t x1,Double_t y1,
		      Double_t x2,Double_t y2,
		      Double_t x3,Double_t y3)
{
  //
  // Initial approximation of the track curvature
  //
  Double_t d=(x2-x1)*(y3-y2)-(x3-x2)*(y2-y1);
  Double_t a=0.5*((y3-y2)*(y2*y2-y1*y1+x2*x2-x1*x1)-
                  (y2-y1)*(y3*y3-y2*y2+x3*x3-x2*x2));
  Double_t b=0.5*((x2-x1)*(y3*y3-y2*y2+x3*x3-x2*x2)-
                  (x3-x2)*(y2*y2-y1*y1+x2*x2-x1*x1));

  Double_t xr=TMath::Abs(d/(d*x1-a)), yr=d/(d*y1-b);

  return -xr*yr/sqrt(xr*xr+yr*yr);
}          

//_____________________________________________________________________
inline Double_t f2trd(Double_t x1,Double_t y1,
		      Double_t x2,Double_t y2,
		      Double_t x3,Double_t y3)
{
  //
  // Initial approximation of the track curvature times X coordinate
  // of the center of curvature
  //

  Double_t d=(x2-x1)*(y3-y2)-(x3-x2)*(y2-y1);
  Double_t a=0.5*((y3-y2)*(y2*y2-y1*y1+x2*x2-x1*x1)-
                  (y2-y1)*(y3*y3-y2*y2+x3*x3-x2*x2));
  Double_t b=0.5*((x2-x1)*(y3*y3-y2*y2+x3*x3-x2*x2)-
                  (x3-x2)*(y2*y2-y1*y1+x2*x2-x1*x1));

  Double_t xr=TMath::Abs(d/(d*x1-a)), yr=d/(d*y1-b);

  return -a/(d*y1-b)*xr/sqrt(xr*xr+yr*yr);
}          

//_____________________________________________________________________
inline Double_t f3trd(Double_t x1,Double_t y1,
		      Double_t x2,Double_t y2,
		      Double_t z1,Double_t z2)
{
  //
  // Initial approximation of the tangent of the track dip angle
  //

  return (z1 - z2)/sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}            

//___________________________________________________________________
Int_t AliTRDtracker::Clusters2Tracks(const TFile *inp, TFile *out)
{
  //
  // Finds tracks within the TRD. File <inp> is expected to contain seeds 
  // at the outer part of the TRD. If <inp> is NULL, the seeds
  // are found within the TRD if fAddTRDseeds is TRUE. 
  // The tracks are propagated to the innermost time bin 
  // of the TRD and stored in file <out>. 
  //

  LoadEvent();
 
  TDirectory *savedir=gDirectory;

  char   tname[100];

  if (!out->IsOpen()) {
    cerr<<"AliTRDtracker::Clusters2Tracks(): output file is not open !\n";
    return 1;
  }    

  sprintf(tname,"seedTRDtoTPC_%d",fEvent); 
  TTree tpc_tree(tname,"Tree with seeds from TRD at outer TPC pad row");
  AliTPCtrack *iotrack=0;
  tpc_tree.Branch("tracks","AliTPCtrack",&iotrack,32000,0); 

  sprintf(tname,"TreeT%d_TRD",fEvent);
  TTree trd_tree(tname,"TRD tracks at inner TRD time bin");
  AliTRDtrack *iotrack_trd=0;
  trd_tree.Branch("tracks","AliTRDtrack",&iotrack_trd,32000,0);  

  Int_t timeBins = fTrSec[0]->GetNumberOfTimeBins();
  Float_t foundMin = fMinClustersInTrack * timeBins; 

  if (inp) {
     TFile *in=(TFile*)inp;
     if (!in->IsOpen()) {
        cerr<<"AliTRDtracker::Clusters2Tracks(): file with seeds is not open !\n";
        cerr<<" ... going for seeds finding inside the TRD\n";
     }
     else {
       in->cd();
       sprintf(tname,"TRDb_%d",fEvent);  
       TTree *seedTree=(TTree*)in->Get(tname);  
       if (!seedTree) {
	 cerr<<"AliTRDtracker::Clusters2Tracks(): ";
	 cerr<<"can't get a tree with track seeds !\n";
	 return 3;
       }  
       AliTRDtrack *seed=new AliTRDtrack;
       seedTree->SetBranchAddress("tracks",&seed);

       Int_t n=(Int_t)seedTree->GetEntries();
       for (Int_t i=0; i<n; i++) {
	 seedTree->GetEvent(i);
	 seed->ResetCovariance(); 
	 fSeeds->AddLast(new AliTRDtrack(*seed));
	 fNseeds++;
       }          
       delete seed;
     }
  }

  out->cd();

  // find tracks from loaded seeds

  Int_t nseed=fSeeds->GetEntriesFast();
  Int_t i, found = 0;
  Int_t innerTB = fTrSec[0]->GetInnerTimeBin();

  for (i=0; i<nseed; i++) {   
    AliTRDtrack *pt=(AliTRDtrack*)fSeeds->UncheckedAt(i), &t=*pt; 
    FollowProlongation(t, innerTB); 
    if (t.GetNumberOfClusters() >= foundMin) {
      UseClusters(&t);
      CookLabel(pt, 1-fLabelFraction);
      //      t.CookdEdx();
    }
    iotrack_trd = pt;
    trd_tree.Fill();
    cerr<<found++<<'\r';     

    if(PropagateToTPC(t)) {
      AliTPCtrack *tpc = new AliTPCtrack(*pt,pt->GetAlpha());
      iotrack = tpc;
      tpc_tree.Fill();
      delete tpc;
    }  
    delete fSeeds->RemoveAt(i);
    fNseeds--;
  }     

  cout<<"Number of loaded seeds: "<<nseed<<endl;  
  cout<<"Number of found tracks from loaded seeds: "<<found<<endl;

  // after tracks from loaded seeds are found and the corresponding 
  // clusters are used, look for additional seeds from TRD

  if(fAddTRDseeds) { 
    // Find tracks for the seeds in the TRD
    Int_t timeBins = fTrSec[0]->GetNumberOfTimeBins();
  
    Int_t nSteps = (Int_t) (fSeedDepth / fSeedStep);
    Int_t gap = (Int_t) (timeBins * fSeedGap);
    Int_t step = (Int_t) (timeBins * fSeedStep);
  
    // make a first turn with tight cut on initial curvature
    for(Int_t turn = 1; turn <= 2; turn++) {
      if(turn == 2) {
	nSteps = (Int_t) (fSeedDepth / (3*fSeedStep));
	step = (Int_t) (timeBins * (3*fSeedStep));
      }
      for(Int_t i=0; i<nSteps; i++) {
	Int_t outer=timeBins-1-i*step; 
	Int_t inner=outer-gap;

	nseed=fSeeds->GetEntriesFast();
	printf("\n initial number of seeds %d\n", nseed); 
      
	MakeSeeds(inner, outer, turn);
      
	nseed=fSeeds->GetEntriesFast();
	printf("\n number of seeds after MakeSeeds %d\n", nseed); 
	
      
	for (Int_t i=0; i<nseed; i++) {   
	  AliTRDtrack *pt=(AliTRDtrack*)fSeeds->UncheckedAt(i), &t=*pt; 
	  FollowProlongation(t,innerTB); 
	  if (t.GetNumberOfClusters() >= foundMin) {
	    UseClusters(&t);
	    CookLabel(pt, 1-fLabelFraction);
	    t.CookdEdx();
	    cerr<<found++<<'\r';     
	    iotrack_trd = pt;
	    trd_tree.Fill();
	    if(PropagateToTPC(t)) {
	      AliTPCtrack *tpc = new AliTPCtrack(*pt,pt->GetAlpha());
	      iotrack = tpc;
	      tpc_tree.Fill();
	      delete tpc;
	    }	
	  }
	  delete fSeeds->RemoveAt(i);
	  fNseeds--;
	}
      }
    }
  }
  tpc_tree.Write(); 
  trd_tree.Write(); 
  
  cout<<"Total number of found tracks: "<<found<<endl;
    
  UnloadEvent();
    
  savedir->cd();  
  
  return 0;    
}     
     
  

//_____________________________________________________________________________
Int_t AliTRDtracker::PropagateBack(const TFile *inp, TFile *out) {
  //
  // Reads seeds from file <inp>. The seeds are AliTPCtrack's found and
  // backpropagated by the TPC tracker. Each seed is first propagated 
  // to the TRD, and then its prolongation is searched in the TRD.
  // If sufficiently long continuation of the track is found in the TRD
  // the track is updated, otherwise it's stored as originaly defined 
  // by the TPC tracker.   
  //  


  LoadEvent();

  TDirectory *savedir=gDirectory;

  TFile *in=(TFile*)inp;

  if (!in->IsOpen()) {
     cerr<<"AliTRDtracker::PropagateBack(): ";
     cerr<<"file with back propagated TPC tracks is not open !\n";
     return 1;
  }                   

  if (!out->IsOpen()) {
     cerr<<"AliTRDtracker::PropagateBack(): ";
     cerr<<"file for back propagated TRD tracks is not open !\n";
     return 2;
  }      

  in->cd();
  char   tname[100];
  sprintf(tname,"seedsTPCtoTRD_%d",fEvent);       
  TTree *seedTree=(TTree*)in->Get(tname);
  if (!seedTree) {
     cerr<<"AliTRDtracker::PropagateBack(): ";
     cerr<<"can't get a tree with seeds from TPC !\n";
     cerr<<"check if your version of TPC tracker creates tree "<<tname<<"\n";
     return 3;
  }

  AliTPCtrack *seed=new AliTPCtrack;
  seedTree->SetBranchAddress("tracks",&seed);

  Int_t n=(Int_t)seedTree->GetEntries();
  for (Int_t i=0; i<n; i++) {
     seedTree->GetEvent(i);
     Int_t lbl = seed->GetLabel();
     AliTRDtrack *tr = new AliTRDtrack(*seed,seed->GetAlpha());
     tr->SetSeedLabel(lbl);
     fSeeds->AddLast(tr);
     fNseeds++;
  }

  delete seed;
  delete seedTree;

  out->cd();

  AliTPCtrack *otrack=0;

  sprintf(tname,"seedsTRDtoTOF1_%d",fEvent);  
  TTree tofTree1(tname,"Tracks back propagated through TPC and TRD");
  tofTree1.Branch("tracks","AliTPCtrack",&otrack,32000,0);  

  sprintf(tname,"seedsTRDtoTOF2_%d",fEvent);  
  TTree tofTree2(tname,"Tracks back propagated through TPC and TRD");
  tofTree2.Branch("tracks","AliTPCtrack",&otrack,32000,0);  

  sprintf(tname,"seedsTRDtoPHOS_%d",fEvent);  
  TTree phosTree(tname,"Tracks back propagated through TPC and TRD");
  phosTree.Branch("tracks","AliTPCtrack",&otrack,32000,0);  

  sprintf(tname,"seedsTRDtoRICH_%d",fEvent);  
  TTree richTree(tname,"Tracks back propagated through TPC and TRD");
  richTree.Branch("tracks","AliTPCtrack",&otrack,32000,0);  

  sprintf(tname,"TRDb_%d",fEvent);  
  TTree trdTree(tname,"Back propagated TRD tracks at outer TRD time bin");
  AliTRDtrack *otrack_trd=0;
  trdTree.Branch("tracks","AliTRDtrack",&otrack_trd,32000,0);   
     
  Int_t found=0;  

  Int_t nseed=fSeeds->GetEntriesFast();

  Float_t foundMin = fMinClustersInTrack * fTimeBinsPerPlane * fGeom->Nplan(); 

  Int_t outermost_tb  = fTrSec[0]->GetOuterTimeBin();

  for (Int_t i=0; i<nseed; i++) {  

    AliTRDtrack *ps=(AliTRDtrack*)fSeeds->UncheckedAt(i), &s=*ps;
    Int_t expectedClr = FollowBackProlongation(s);
    Int_t foundClr = s.GetNumberOfClusters();
    Int_t last_tb = fTrSec[0]->GetLayerNumber(s.GetX());

    printf("seed %d: found %d out of %d expected clusters, Min is %f\n",
	   i, foundClr, expectedClr, foundMin);

    if (foundClr >= foundMin) {
      s.CookdEdx(); 
      CookLabel(ps, 1-fLabelFraction);
      UseClusters(ps);
      otrack_trd=ps;
      trdTree.Fill();
      cout<<found++<<'\r';
    }

    if(((expectedClr < 10) && (last_tb == outermost_tb)) ||
       ((expectedClr >= 10) && 
	(((Float_t) foundClr) / ((Float_t) expectedClr) >= 
         fMinFractionOfFoundClusters) && (last_tb == outermost_tb))) {

      Double_t x_tof = 375.5;
    
      if(PropagateToOuterPlane(s,x_tof)) {
	AliTPCtrack *pt = new AliTPCtrack(*ps,ps->GetAlpha());
	otrack = pt;
	tofTree1.Fill();
	delete pt;

	x_tof = 381.5;
    
	if(PropagateToOuterPlane(s,x_tof)) {
	  AliTPCtrack *pt = new AliTPCtrack(*ps,ps->GetAlpha());
	  otrack = pt;
	  tofTree2.Fill();
	  delete pt;

	  Double_t x_phos = 460.;
	  
	  if(PropagateToOuterPlane(s,x_phos)) {
	    AliTPCtrack *pt = new AliTPCtrack(*ps,ps->GetAlpha());
	    otrack = pt;
	    phosTree.Fill();
	    delete pt;
	    
	    Double_t x_rich = 490+1.267;
	    
	    if(PropagateToOuterPlane(s,x_rich)) {
	      AliTPCtrack *pt = new AliTPCtrack(*ps,ps->GetAlpha());
	      otrack = pt;
	      richTree.Fill();
	      delete pt;
	    }   
	  }
	}
      }      
    }
  }
  
  tofTree1.Write(); 
  tofTree2.Write(); 
  phosTree.Write(); 
  richTree.Write(); 
  trdTree.Write(); 

  savedir->cd();  
  cerr<<"Number of seeds: "<<nseed<<endl;  
  cerr<<"Number of back propagated TRD tracks: "<<found<<endl;

  UnloadEvent();

  return 0;

}


//---------------------------------------------------------------------------
Int_t AliTRDtracker::FollowProlongation(AliTRDtrack& t, Int_t rf)
{
  // Starting from current position on track=t this function tries
  // to extrapolate the track up to timeBin=0 and to confirm prolongation
  // if a close cluster is found. Returns the number of clusters
  // expected to be found in sensitive layers

  Float_t  wIndex, wTB, wChi2;
  Float_t  wYrt, wYclosest, wYcorrect, wYwindow;
  Float_t  wZrt, wZclosest, wZcorrect, wZwindow;
  Float_t  wPx, wPy, wPz, wC;
  Double_t Px, Py, Pz;
  Float_t  wSigmaC2, wSigmaTgl2, wSigmaY2, wSigmaZ2;

  Int_t trackIndex = t.GetLabel();  

  Int_t ns=Int_t(2*TMath::Pi()/AliTRDgeometry::GetAlpha()+0.5);     

  Int_t try_again=fMaxGap;

  Double_t alpha=t.GetAlpha();

  if (alpha > 2.*TMath::Pi()) alpha -= 2.*TMath::Pi();
  if (alpha < 0.            ) alpha += 2.*TMath::Pi();

  Int_t s=Int_t(alpha/AliTRDgeometry::GetAlpha())%AliTRDgeometry::kNsect;  

  Double_t x0, rho, x, dx, y, ymax, z;

  Int_t expectedNumberOfClusters = 0;
  Bool_t lookForCluster;

  alpha=AliTRDgeometry::GetAlpha();  // note: change in meaning

 
  for (Int_t nr=fTrSec[0]->GetLayerNumber(t.GetX()); nr>rf; nr--) { 

    y = t.GetY(); z = t.GetZ();

    // first propagate to the inner surface of the current time bin 
    fTrSec[s]->GetLayer(nr)->GetPropagationParameters(y,z,dx,rho,x0,lookForCluster);
    x = fTrSec[s]->GetLayer(nr)->GetX()-dx/2; y = t.GetY(); z = t.GetZ();
    if(!t.PropagateTo(x,x0,rho,0.139)) break;
    y = t.GetY();
    ymax = x*TMath::Tan(0.5*alpha);
    if (y > ymax) {
      s = (s+1) % ns;
      if (!t.Rotate(alpha)) break;
      if(!t.PropagateTo(x,x0,rho,0.139)) break;
    } else if (y <-ymax) {
      s = (s-1+ns) % ns;                           
      if (!t.Rotate(-alpha)) break;   
      if(!t.PropagateTo(x,x0,rho,0.139)) break;
    } 

    y = t.GetY(); z = t.GetZ();

    // now propagate to the middle plane of the next time bin 
    fTrSec[s]->GetLayer(nr-1)->GetPropagationParameters(y,z,dx,rho,x0,lookForCluster);
    x = fTrSec[s]->GetLayer(nr-1)->GetX(); y = t.GetY(); z = t.GetZ();
    if(!t.PropagateTo(x,x0,rho,0.139)) break;
    y = t.GetY();
    ymax = x*TMath::Tan(0.5*alpha);
    if (y > ymax) {
      s = (s+1) % ns;
      if (!t.Rotate(alpha)) break;
      if(!t.PropagateTo(x,x0,rho,0.139)) break;
    } else if (y <-ymax) {
      s = (s-1+ns) % ns;                           
      if (!t.Rotate(-alpha)) break;   
      if(!t.PropagateTo(x,x0,rho,0.139)) break;
    } 


    if(lookForCluster) {


      expectedNumberOfClusters++;       

      wIndex = (Float_t) t.GetLabel();
      wTB = nr;


      AliTRDpropagationLayer& time_bin=*(fTrSec[s]->GetLayer(nr-1));


      Double_t sy2=ExpectedSigmaY2(x,t.GetTgl(),t.GetPt());


      Double_t sz2=ExpectedSigmaZ2(x,t.GetTgl());

      Double_t road;
      if((t.GetSigmaY2() + sy2) > 0) road=10.*sqrt(t.GetSigmaY2() + sy2);
      else return expectedNumberOfClusters;
      
      wYrt = (Float_t) y;
      wZrt = (Float_t) z;
      wYwindow = (Float_t) road;
      t.GetPxPyPz(Px,Py,Pz);
      wPx = (Float_t) Px;
      wPy = (Float_t) Py;
      wPz = (Float_t) Pz;
      wC  = (Float_t) t.GetC();
      wSigmaC2 = (Float_t) t.GetSigmaC2();
      wSigmaTgl2    = (Float_t) t.GetSigmaTgl2();
      wSigmaY2 = (Float_t) t.GetSigmaY2();
      wSigmaZ2 = (Float_t) t.GetSigmaZ2();
      wChi2 = -1;            
      
      if (road>fWideRoad) {
	if (t.GetNumberOfClusters()>4)
	  cerr<<t.GetNumberOfClusters()
	      <<"FindProlongation warning: Too broad road !\n";
	return 0;
      }             

      AliTRDcluster *cl=0;
      UInt_t index=0;

      Double_t max_chi2=fMaxChi2;

      wYclosest = 12345678;
      wYcorrect = 12345678;
      wZclosest = 12345678;
      wZcorrect = 12345678;
      wZwindow  = TMath::Sqrt(2.25 * 12 * sz2);   

      // Find the closest correct cluster for debugging purposes
      if (time_bin) {
	Float_t minDY = 1000000;
	for (Int_t i=0; i<time_bin; i++) {
	  AliTRDcluster* c=(AliTRDcluster*)(time_bin[i]);
	  if((c->GetLabel(0) != trackIndex) &&
	     (c->GetLabel(1) != trackIndex) &&
	     (c->GetLabel(2) != trackIndex)) continue;
	  if(TMath::Abs(c->GetY() - y) > minDY) continue;
	  minDY = TMath::Abs(c->GetY() - y);
	  wYcorrect = c->GetY();
	  wZcorrect = c->GetZ();
	  wChi2 = t.GetPredictedChi2(c);
	}
      }                    

      // Now go for the real cluster search

      if (time_bin) {

	for (Int_t i=time_bin.Find(y-road); i<time_bin; i++) {
	  AliTRDcluster* c=(AliTRDcluster*)(time_bin[i]);
	  if (c->GetY() > y+road) break;
	  if (c->IsUsed() > 0) continue;
	  if((c->GetZ()-z)*(c->GetZ()-z) > 3 * sz2) continue;
	  Double_t chi2=t.GetPredictedChi2(c);
	  
	  if (chi2 > max_chi2) continue;
	  max_chi2=chi2;
	  cl=c;
	  index=time_bin.GetIndex(i);
	}               

	if(!cl) {

	  for (Int_t i=time_bin.Find(y-road); i<time_bin; i++) {
	    AliTRDcluster* c=(AliTRDcluster*)(time_bin[i]);
	    
	    if (c->GetY() > y+road) break;
	    if (c->IsUsed() > 0) continue;
	    if((c->GetZ()-z)*(c->GetZ()-z) > 2.25 * 12 * sz2) continue;
	    
	    Double_t chi2=t.GetPredictedChi2(c);
	    
	    if (chi2 > max_chi2) continue;
	    max_chi2=chi2;
	    cl=c;
	    index=time_bin.GetIndex(i);
	  }
	}	
	

	if (cl) {
	  wYclosest = cl->GetY();
	  wZclosest = cl->GetZ();

	  t.SetSampledEdx(cl->GetQ()/dx,t.GetNumberOfClusters()); 
	  if(!t.Update(cl,max_chi2,index)) {
	    if(!try_again--) return 0;
	  }  
	  else try_again=fMaxGap;
	}
	else {
	  if (try_again==0) break; 
	  try_again--;
	}

	/*
	if((((Int_t) wTB)%15 == 0) || (((Int_t) wTB)%15 == 14)) {
	  
	  printf(" %f", wIndex);       //1
	  printf(" %f", wTB);          //2
	  printf(" %f", wYrt);         //3
	  printf(" %f", wYclosest);    //4
	  printf(" %f", wYcorrect);    //5
	  printf(" %f", wYwindow);     //6
	  printf(" %f", wZrt);         //7
	  printf(" %f", wZclosest);    //8
	  printf(" %f", wZcorrect);    //9
	  printf(" %f", wZwindow);     //10
	  printf(" %f", wPx);          //11
	  printf(" %f", wPy);          //12
	  printf(" %f", wPz);          //13
	  printf(" %f", wSigmaC2*1000000);  //14
	  printf(" %f", wSigmaTgl2*1000);   //15
	  printf(" %f", wSigmaY2);     //16
	  //      printf(" %f", wSigmaZ2);     //17
	  printf(" %f", wChi2);     //17
	  printf(" %f", wC);           //18
	  printf("\n");
	} 
	*/                        
      }
    }  
  }
  return expectedNumberOfClusters;
  
  
}                

//___________________________________________________________________

Int_t AliTRDtracker::FollowBackProlongation(AliTRDtrack& t)
{
  // Starting from current radial position of track <t> this function
  // extrapolates the track up to outer timebin and in the sensitive
  // layers confirms prolongation if a close cluster is found. 
  // Returns the number of clusters expected to be found in sensitive layers

  Float_t  wIndex, wTB, wChi2;
  Float_t  wYrt, wYclosest, wYcorrect, wYwindow;
  Float_t  wZrt, wZclosest, wZcorrect, wZwindow;
  Float_t  wPx, wPy, wPz, wC;
  Double_t Px, Py, Pz;
  Float_t  wSigmaC2, wSigmaTgl2, wSigmaY2, wSigmaZ2;

  Int_t trackIndex = t.GetLabel();  

  Int_t ns=Int_t(2*TMath::Pi()/AliTRDgeometry::GetAlpha()+0.5);     

  Int_t try_again=fMaxGap;

  Double_t alpha=t.GetAlpha();

  if (alpha > 2.*TMath::Pi()) alpha -= 2.*TMath::Pi();
  if (alpha < 0.            ) alpha += 2.*TMath::Pi();

  Int_t s=Int_t(alpha/AliTRDgeometry::GetAlpha())%AliTRDgeometry::kNsect;  

  Int_t outerTB = fTrSec[0]->GetOuterTimeBin();

  Double_t x0, rho, x, dx, y, ymax, z;

  Bool_t lookForCluster;

  Int_t expectedNumberOfClusters = 0;
  x = t.GetX();

  alpha=AliTRDgeometry::GetAlpha();  // note: change in meaning


  for (Int_t nr=fTrSec[0]->GetLayerNumber(t.GetX()); nr<outerTB; nr++) { 

    y = t.GetY(); z = t.GetZ();

    // first propagate to the outer surface of the current time bin 

    fTrSec[s]->GetLayer(nr)->GetPropagationParameters(y,z,dx,rho,x0,lookForCluster);
    x = fTrSec[s]->GetLayer(nr)->GetX()+dx/2; y = t.GetY(); z = t.GetZ();

    if(!t.PropagateTo(x,x0,rho,0.139)) break;

    y = t.GetY();
    ymax = x*TMath::Tan(0.5*alpha);

    if (y > ymax) {
      s = (s+1) % ns;
      if (!t.Rotate(alpha)) break;
      if(!t.PropagateTo(x,x0,rho,0.139)) break;
    } else if (y <-ymax) {
      s = (s-1+ns) % ns;                           
      if (!t.Rotate(-alpha)) break;   
      if(!t.PropagateTo(x,x0,rho,0.139)) break;
    } 
    y = t.GetY(); z = t.GetZ();

    // now propagate to the middle plane of the next time bin 
    fTrSec[s]->GetLayer(nr+1)->GetPropagationParameters(y,z,dx,rho,x0,lookForCluster);

    x = fTrSec[s]->GetLayer(nr+1)->GetX(); y = t.GetY(); z = t.GetZ();

    if(!t.PropagateTo(x,x0,rho,0.139)) break;

    y = t.GetY();

    ymax = x*TMath::Tan(0.5*alpha);

    if(fVocal) printf("nr+1=%d, x %f, z %f, y %f, ymax %f\n",nr+1,x,z,y,ymax);

    if (y > ymax) {
      s = (s+1) % ns;
      if (!t.Rotate(alpha)) break;
      if(!t.PropagateTo(x,x0,rho,0.139)) break;
    } else if (y <-ymax) {
      s = (s-1+ns) % ns;              
      if (!t.Rotate(-alpha)) break;   
      if(!t.PropagateTo(x,x0,rho,0.139)) break;
    } 

    //    printf("label %d, pl %d, lookForCluster %d \n",
    //	   trackIndex, nr+1, lookForCluster);

    if(lookForCluster) {
      expectedNumberOfClusters++;       

      wIndex = (Float_t) t.GetLabel();
      wTB = fTrSec[s]->GetLayer(nr+1)->GetTimeBinIndex();

      AliTRDpropagationLayer& time_bin=*(fTrSec[s]->GetLayer(nr+1));
      Double_t sy2=ExpectedSigmaY2(t.GetX(),t.GetTgl(),t.GetPt());
      Double_t sz2=ExpectedSigmaZ2(t.GetX(),t.GetTgl());
      if((t.GetSigmaY2() + sy2) < 0) break;
      Double_t road = 10.*sqrt(t.GetSigmaY2() + sy2); 
      Double_t y=t.GetY(), z=t.GetZ();

      wYrt = (Float_t) y;
      wZrt = (Float_t) z;
      wYwindow = (Float_t) road;
      t.GetPxPyPz(Px,Py,Pz);
      wPx = (Float_t) Px;
      wPy = (Float_t) Py;
      wPz = (Float_t) Pz;
      wC  = (Float_t) t.GetC();
      wSigmaC2 = (Float_t) t.GetSigmaC2();
      wSigmaTgl2    = (Float_t) t.GetSigmaTgl2();
      wSigmaY2 = (Float_t) t.GetSigmaY2();
      wSigmaZ2 = (Float_t) t.GetSigmaZ2();
      wChi2 = -1;            
      
      if (road>fWideRoad) {
	if (t.GetNumberOfClusters()>4)
	  cerr<<t.GetNumberOfClusters()
	      <<"FindProlongation warning: Too broad road !\n";
	return 0;
      }      

      AliTRDcluster *cl=0;
      UInt_t index=0;

      Double_t max_chi2=fMaxChi2;

      wYclosest = 12345678;
      wYcorrect = 12345678;
      wZclosest = 12345678;
      wZcorrect = 12345678;
      wZwindow  = TMath::Sqrt(2.25 * 12 * sz2);   

      // Find the closest correct cluster for debugging purposes
      if (time_bin) {
	Float_t minDY = 1000000;
	for (Int_t i=0; i<time_bin; i++) {
	  AliTRDcluster* c=(AliTRDcluster*)(time_bin[i]);
	  if((c->GetLabel(0) != trackIndex) &&
	     (c->GetLabel(1) != trackIndex) &&
	     (c->GetLabel(2) != trackIndex)) continue;
	  if(TMath::Abs(c->GetY() - y) > minDY) continue;
	  minDY = TMath::Abs(c->GetY() - y);
	  wYcorrect = c->GetY();
	  wZcorrect = c->GetZ();
	  wChi2 = t.GetPredictedChi2(c);
	}
      }                    

      // Now go for the real cluster search

      if (time_bin) {

	for (Int_t i=time_bin.Find(y-road); i<time_bin; i++) {
	  AliTRDcluster* c=(AliTRDcluster*)(time_bin[i]);
	  if (c->GetY() > y+road) break;
	  if (c->IsUsed() > 0) continue;
	  if((c->GetZ()-z)*(c->GetZ()-z) > 3 * sz2) continue;
	  Double_t chi2=t.GetPredictedChi2(c);
	  
	  if (chi2 > max_chi2) continue;
	  max_chi2=chi2;
	  cl=c;
	  index=time_bin.GetIndex(i);
	}               

	if(!cl) {

	  for (Int_t i=time_bin.Find(y-road); i<time_bin; i++) {
	    AliTRDcluster* c=(AliTRDcluster*)(time_bin[i]);
	    
	    if (c->GetY() > y+road) break;
	    if (c->IsUsed() > 0) continue;
	    if((c->GetZ()-z)*(c->GetZ()-z) > 2.25 * 12 * sz2) continue;
	    
	    Double_t chi2=t.GetPredictedChi2(c);
	    
	    if (chi2 > max_chi2) continue;
	    max_chi2=chi2;
	    cl=c;
	    index=time_bin.GetIndex(i);
	  }
	}	
	
	if (cl) {
	  wYclosest = cl->GetY();
	  wZclosest = cl->GetZ();

	  t.SetSampledEdx(cl->GetQ()/dx,t.GetNumberOfClusters()); 
	  if(!t.Update(cl,max_chi2,index)) {
	    if(!try_again--) return 0;
	  }  
	  else try_again=fMaxGap;
	}
	else {
	  if (try_again==0) break; 
	  try_again--;
	}

	/*
	if((((Int_t) wTB)%15 == 0) || (((Int_t) wTB)%15 == 14)) {
	  
	  printf(" %f", wIndex);       //1
	  printf(" %f", wTB);          //2
	  printf(" %f", wYrt);         //3
	  printf(" %f", wYclosest);    //4
	  printf(" %f", wYcorrect);    //5
	  printf(" %f", wYwindow);     //6
	  printf(" %f", wZrt);         //7
	  printf(" %f", wZclosest);    //8
	  printf(" %f", wZcorrect);    //9
	  printf(" %f", wZwindow);     //10
	  printf(" %f", wPx);          //11
	  printf(" %f", wPy);          //12
	  printf(" %f", wPz);          //13
	  printf(" %f", wSigmaC2*1000000);  //14
	  printf(" %f", wSigmaTgl2*1000);   //15
	  printf(" %f", wSigmaY2);     //16
	  //      printf(" %f", wSigmaZ2);     //17
	  printf(" %f", wChi2);     //17
	  printf(" %f", wC);           //18
	  printf("\n");
	} 
	*/                        
      }
    }  
  }
  return expectedNumberOfClusters;
}         

//___________________________________________________________________

Int_t AliTRDtracker::PropagateToOuterPlane(AliTRDtrack& t, Double_t xToGo)
{
  // Starting from current radial position of track <t> this function
  // extrapolates the track up to radial position <xToGo>. 
  // Returns 1 if track reaches the plane, and 0 otherwise 

  Int_t ns=Int_t(2*TMath::Pi()/AliTRDgeometry::GetAlpha()+0.5);     

  Double_t alpha=t.GetAlpha();

  if (alpha > 2.*TMath::Pi()) alpha -= 2.*TMath::Pi();
  if (alpha < 0.            ) alpha += 2.*TMath::Pi();

  Int_t s=Int_t(alpha/AliTRDgeometry::GetAlpha())%AliTRDgeometry::kNsect;  

  Bool_t lookForCluster;
  Double_t x0, rho, x, dx, y, ymax, z;

  x = t.GetX();

  alpha=AliTRDgeometry::GetAlpha();  // note: change in meaning

  Int_t plToGo = fTrSec[0]->GetLayerNumber(xToGo);

  for (Int_t nr=fTrSec[0]->GetLayerNumber(x); nr<plToGo; nr++) { 

    y = t.GetY(); z = t.GetZ();

    // first propagate to the outer surface of the current time bin 
    fTrSec[s]->GetLayer(nr)->GetPropagationParameters(y,z,dx,rho,x0,lookForCluster);
    x = fTrSec[s]->GetLayer(nr)->GetX()+dx/2; y = t.GetY(); z = t.GetZ();
    if(!t.PropagateTo(x,x0,rho,0.139)) return 0;
    y = t.GetY();
    ymax = x*TMath::Tan(0.5*alpha);
    if (y > ymax) {
      s = (s+1) % ns;
      if (!t.Rotate(alpha)) return 0;
    } else if (y <-ymax) {
      s = (s-1+ns) % ns;                           
      if (!t.Rotate(-alpha)) return 0;   
    } 
    if(!t.PropagateTo(x,x0,rho,0.139)) return 0;

    y = t.GetY(); z = t.GetZ();

    // now propagate to the middle plane of the next time bin 
    fTrSec[s]->GetLayer(nr+1)->GetPropagationParameters(y,z,dx,rho,x0,lookForCluster);
    x = fTrSec[s]->GetLayer(nr+1)->GetX(); y = t.GetY(); z = t.GetZ();
    if(!t.PropagateTo(x,x0,rho,0.139)) return 0;
    y = t.GetY();
    ymax = x*TMath::Tan(0.5*alpha);
    if (y > ymax) {
      s = (s+1) % ns;
      if (!t.Rotate(alpha)) return 0;
    } else if (y <-ymax) {
      s = (s-1+ns) % ns;                           
      if (!t.Rotate(-alpha)) return 0;   
    } 
    if(!t.PropagateTo(x,x0,rho,0.139)) return 0;
  }
  return 1;
}         

//___________________________________________________________________

Int_t AliTRDtracker::PropagateToTPC(AliTRDtrack& t)
{
  // Starting from current radial position of track <t> this function
  // extrapolates the track up to radial position of the outermost
  // padrow of the TPC. 
  // Returns 1 if track reaches the TPC, and 0 otherwise 

  Int_t ns=Int_t(2*TMath::Pi()/AliTRDgeometry::GetAlpha()+0.5);     

  Double_t alpha=t.GetAlpha();

  if (alpha > 2.*TMath::Pi()) alpha -= 2.*TMath::Pi();
  if (alpha < 0.            ) alpha += 2.*TMath::Pi();

  Int_t s=Int_t(alpha/AliTRDgeometry::GetAlpha())%AliTRDgeometry::kNsect;  

  Bool_t lookForCluster;
  Double_t x0, rho, x, dx, y, ymax, z;

  x = t.GetX();

  alpha=AliTRDgeometry::GetAlpha();  // note: change in meaning

  Int_t plTPC = fTrSec[0]->GetLayerNumber(246.055);

  for (Int_t nr=fTrSec[0]->GetLayerNumber(x); nr>plTPC; nr--) { 

    y = t.GetY(); z = t.GetZ();

    // first propagate to the outer surface of the current time bin 
    fTrSec[s]->GetLayer(nr)->GetPropagationParameters(y,z,dx,rho,x0,lookForCluster);
    x = fTrSec[s]->GetLayer(nr)->GetX()-dx/2; y = t.GetY(); z = t.GetZ();
    if(!t.PropagateTo(x,x0,rho,0.139)) return 0;
    y = t.GetY();
    ymax = x*TMath::Tan(0.5*alpha);
    if (y > ymax) {
      s = (s+1) % ns;
      if (!t.Rotate(alpha)) return 0;
    } else if (y <-ymax) {
      s = (s-1+ns) % ns;                           
      if (!t.Rotate(-alpha)) return 0;   
    } 
    if(!t.PropagateTo(x,x0,rho,0.139)) return 0;

    y = t.GetY(); z = t.GetZ();

    // now propagate to the middle plane of the next time bin 
    fTrSec[s]->GetLayer(nr-1)->GetPropagationParameters(y,z,dx,rho,x0,lookForCluster);
    x = fTrSec[s]->GetLayer(nr-1)->GetX(); y = t.GetY(); z = t.GetZ();
    if(!t.PropagateTo(x,x0,rho,0.139)) return 0;
    y = t.GetY();
    ymax = x*TMath::Tan(0.5*alpha);
    if (y > ymax) {
      s = (s+1) % ns;
      if (!t.Rotate(alpha)) return 0;
    } else if (y <-ymax) {
      s = (s-1+ns) % ns;                           
      if (!t.Rotate(-alpha)) return 0;   
    } 
    if(!t.PropagateTo(x,x0,rho,0.139)) return 0;
  }
  return 1;
}         


//_____________________________________________________________________________
void AliTRDtracker::LoadEvent()
{
  // Fills clusters into TRD tracking_sectors 
  // Note that the numbering scheme for the TRD tracking_sectors 
  // differs from that of TRD sectors

  ReadClusters(fClusters);
  Int_t ncl=fClusters->GetEntriesFast();
  cout<<"\n LoadSectors: sorting "<<ncl<<" clusters"<<endl;
              
  UInt_t index;
  while (ncl--) {
    printf("\r %d left  ",ncl); 
    AliTRDcluster *c=(AliTRDcluster*)fClusters->UncheckedAt(ncl);
    Int_t detector=c->GetDetector(), local_time_bin=c->GetLocalTimeBin();
    Int_t sector=fGeom->GetSector(detector);
    Int_t plane=fGeom->GetPlane(detector);

    Int_t tracking_sector = CookSectorIndex(sector);

    Int_t gtb = fTrSec[tracking_sector]->CookTimeBinIndex(plane,local_time_bin);
    Int_t layer = fTrSec[tracking_sector]->GetLayerNumber(gtb);

    index=ncl;
    fTrSec[tracking_sector]->GetLayer(layer)->InsertCluster(c,index);
  }    
  printf("\r\n");

}

//_____________________________________________________________________________
void AliTRDtracker::UnloadEvent() 
{ 
  //
  // Clears the arrays of clusters and tracks. Resets sectors and timebins 
  //

  Int_t i, nentr;

  nentr = fClusters->GetEntriesFast();
  for (i = 0; i < nentr; i++) delete fClusters->RemoveAt(i);

  nentr = fSeeds->GetEntriesFast();
  for (i = 0; i < nentr; i++) delete fSeeds->RemoveAt(i);

  nentr = fTracks->GetEntriesFast();
  for (i = 0; i < nentr; i++) delete fTracks->RemoveAt(i);

  Int_t nsec = AliTRDgeometry::kNsect;

  for (i = 0; i < nsec; i++) {    
    for(Int_t pl = 0; pl < fTrSec[i]->GetNumberOfLayers(); pl++) {
      fTrSec[i]->GetLayer(pl)->Clear();
    }
  }

}

//__________________________________________________________________________
void AliTRDtracker::MakeSeeds(Int_t inner, Int_t outer, Int_t turn)
{
  // Creates track seeds using clusters in timeBins=i1,i2

  if(turn > 2) {
    cerr<<"MakeSeeds: turn "<<turn<<" exceeds the limit of 2"<<endl;
    return;
  }

  Double_t x[5], c[15];
  Int_t max_sec=AliTRDgeometry::kNsect;
  
  Double_t alpha=AliTRDgeometry::GetAlpha();
  Double_t shift=AliTRDgeometry::GetAlpha()/2.;
  Double_t cs=cos(alpha), sn=sin(alpha);
  Double_t cs2=cos(2.*alpha), sn2=sin(2.*alpha);
    
      
  Int_t i2 = fTrSec[0]->GetLayerNumber(inner);
  Int_t i1 = fTrSec[0]->GetLayerNumber(outer);
      
  Double_t x1 =fTrSec[0]->GetX(i1);
  Double_t xx2=fTrSec[0]->GetX(i2);
      
  for (Int_t ns=0; ns<max_sec; ns++) {
    
    Int_t nl2 = *(fTrSec[(ns-2+max_sec)%max_sec]->GetLayer(i2));
    Int_t nl=(*fTrSec[(ns-1+max_sec)%max_sec]->GetLayer(i2));
    Int_t nm=(*fTrSec[ns]->GetLayer(i2));
    Int_t nu=(*fTrSec[(ns+1)%max_sec]->GetLayer(i2));
    Int_t nu2=(*fTrSec[(ns+2)%max_sec]->GetLayer(i2));
    
    AliTRDpropagationLayer& r1=*(fTrSec[ns]->GetLayer(i1));
    
    for (Int_t is=0; is < r1; is++) {
      Double_t y1=r1[is]->GetY(), z1=r1[is]->GetZ();
      
      for (Int_t js=0; js < nl2+nl+nm+nu+nu2; js++) {
	
	const AliTRDcluster *cl;
	Double_t x2,   y2,   z2;
	Double_t x3=0., y3=0.;   
	
	if (js<nl2) {
	  if(turn != 2) continue;
	  AliTRDpropagationLayer& r2=*(fTrSec[(ns-2+max_sec)%max_sec]->GetLayer(i2));
	  cl=r2[js];
	  y2=cl->GetY(); z2=cl->GetZ();
	  
	  x2= xx2*cs2+y2*sn2;
	  y2=-xx2*sn2+y2*cs2;
	}
	else if (js<nl2+nl) {
	  if(turn != 1) continue;
	  AliTRDpropagationLayer& r2=*(fTrSec[(ns-1+max_sec)%max_sec]->GetLayer(i2));
	  cl=r2[js-nl2];
	  y2=cl->GetY(); z2=cl->GetZ();
	  
	  x2= xx2*cs+y2*sn;
	  y2=-xx2*sn+y2*cs;
	}                                
	else if (js<nl2+nl+nm) {
	  if(turn != 1) continue;
	  AliTRDpropagationLayer& r2=*(fTrSec[ns]->GetLayer(i2));
	  cl=r2[js-nl2-nl];
	  x2=xx2; y2=cl->GetY(); z2=cl->GetZ();
	}
	else if (js<nl2+nl+nm+nu) {
	  if(turn != 1) continue;
	  AliTRDpropagationLayer& r2=*(fTrSec[(ns+1)%max_sec]->GetLayer(i2));
	  cl=r2[js-nl2-nl-nm];
	  y2=cl->GetY(); z2=cl->GetZ();
	  
	  x2=xx2*cs-y2*sn;
	  y2=xx2*sn+y2*cs;
	}              
	else {
	  if(turn != 2) continue;
	  AliTRDpropagationLayer& r2=*(fTrSec[(ns+2)%max_sec]->GetLayer(i2));
	  cl=r2[js-nl2-nl-nm-nu];
	  y2=cl->GetY(); z2=cl->GetZ();
	  
	  x2=xx2*cs2-y2*sn2;
	  y2=xx2*sn2+y2*cs2;
	}
	
	if(TMath::Abs(z1-z2) > fMaxSeedDeltaZ12) continue;
	
	Double_t zz=z1 - z1/x1*(x1-x2);
	
	if (TMath::Abs(zz-z2)>fMaxSeedDeltaZ) continue;
	
	Double_t d=(x2-x1)*(0.-y2)-(0.-x2)*(y2-y1);
	if (d==0.) {cerr<<"TRD MakeSeeds: Straight seed !\n"; continue;}
	
	x[0]=y1;
	x[1]=z1;
	x[2]=f1trd(x1,y1,x2,y2,x3,y3);
	
	if (TMath::Abs(x[2]) > fMaxSeedC) continue;      
	
	x[3]=f2trd(x1,y1,x2,y2,x3,y3);
	
	if (TMath::Abs(x[2]*x1-x[3]) >= 0.99999) continue;
	
	x[4]=f3trd(x1,y1,x2,y2,z1,z2);
	
	if (TMath::Abs(x[4]) > fMaxSeedTan) continue;
	
	Double_t a=asin(x[3]);
	Double_t zv=z1 - x[4]/x[2]*(a+asin(x[2]*x1-x[3]));
	
	if (TMath::Abs(zv)>fMaxSeedVertexZ) continue;
	
	Double_t sy1=r1[is]->GetSigmaY2(), sz1=r1[is]->GetSigmaZ2();
	Double_t sy2=cl->GetSigmaY2(),     sz2=cl->GetSigmaZ2();
	Double_t sy3=fSeedErrorSY3, sy=fSeedErrorSY, sz=fSeedErrorSZ;  
	
	Double_t f20=(f1trd(x1,y1+sy,x2,y2,x3,y3)-x[2])/sy;
	Double_t f22=(f1trd(x1,y1,x2,y2+sy,x3,y3)-x[2])/sy;
	Double_t f24=(f1trd(x1,y1,x2,y2,x3,y3+sy)-x[2])/sy;
	Double_t f30=(f2trd(x1,y1+sy,x2,y2,x3,y3)-x[3])/sy;
	Double_t f32=(f2trd(x1,y1,x2,y2+sy,x3,y3)-x[3])/sy;
	Double_t f34=(f2trd(x1,y1,x2,y2,x3,y3+sy)-x[3])/sy;
	Double_t f40=(f3trd(x1,y1+sy,x2,y2,z1,z2)-x[4])/sy;
	Double_t f41=(f3trd(x1,y1,x2,y2,z1+sz,z2)-x[4])/sz;
	Double_t f42=(f3trd(x1,y1,x2,y2+sy,z1,z2)-x[4])/sy;
	Double_t f43=(f3trd(x1,y1,x2,y2,z1,z2+sz)-x[4])/sz;    
	
	c[0]=sy1;
	c[1]=0.;       c[2]=sz1;
	c[3]=f20*sy1;  c[4]=0.;   c[5]=f20*sy1*f20+f22*sy2*f22+f24*sy3*f24;
	c[6]=f30*sy1;  c[7]=0.;   c[8]=f30*sy1*f20+f32*sy2*f22+f34*sy3*f24;
	c[9]=f30*sy1*f30+f32*sy2*f32+f34*sy3*f34;
	c[10]=f40*sy1; c[11]=f41*sz1;   c[12]=f40*sy1*f20+f42*sy2*f22;
	c[13]=f40*sy1*f30+f42*sy2*f32;
	c[14]=f40*sy1*f40+f41*sz1*f41+f42*sy2*f42+f43*sz2*f43;
	
	UInt_t index=r1.GetIndex(is);
	
	AliTRDtrack *track=new AliTRDtrack(r1[is],index,x,c,x1,ns*alpha+shift);

	Int_t rc=FollowProlongation(*track, i2);     
	
	if ((rc < 1) ||
	    (track->GetNumberOfClusters() < 
	     (outer-inner)*fMinClustersInSeed)) delete track;
	else {
	  fSeeds->AddLast(track); fNseeds++;
	  cerr<<"\r found seed "<<fNseeds;
	}
      }
    }
  }
}            

//_____________________________________________________________________________
void AliTRDtracker::ReadClusters(TObjArray *array, const TFile *inp) 
{
  //
  // Reads AliTRDclusters (option >= 0) or AliTRDrecPoints (option < 0) 
  // from the file. The names of the cluster tree and branches 
  // should match the ones used in AliTRDclusterizer::WriteClusters()
  //

  TDirectory *savedir=gDirectory; 

  if (inp) {
     TFile *in=(TFile*)inp;
     if (!in->IsOpen()) {
        cerr<<"AliTRDtracker::ReadClusters(): input file is not open !\n";
        return;
     }
     else{
       in->cd();
     }
  }

  Char_t treeName[12];
  sprintf(treeName,"TreeR%d_TRD",fEvent);
  TTree *ClusterTree = (TTree*) gDirectory->Get(treeName);
  
  TObjArray *ClusterArray = new TObjArray(400); 
  
  ClusterTree->GetBranch("TRDcluster")->SetAddress(&ClusterArray); 
  
  Int_t nEntries = (Int_t) ClusterTree->GetEntries();
  printf("found %d entries in %s.\n",nEntries,ClusterTree->GetName());
  
  // Loop through all entries in the tree
  Int_t nbytes;
  AliTRDcluster *c = 0;
  printf("\n");

  for (Int_t iEntry = 0; iEntry < nEntries; iEntry++) {    
    
    // Import the tree
    nbytes += ClusterTree->GetEvent(iEntry);  
    
    // Get the number of points in the detector
    Int_t nCluster = ClusterArray->GetEntriesFast();  
    printf("\r Read %d clusters from entry %d", nCluster, iEntry);
    
    // Loop through all TRD digits
    for (Int_t iCluster = 0; iCluster < nCluster; iCluster++) { 
      c = (AliTRDcluster*)ClusterArray->UncheckedAt(iCluster);
      AliTRDcluster *co = new AliTRDcluster(*c);
      co->SetSigmaY2(c->GetSigmaY2() * fSY2corr);
      Int_t ltb = co->GetLocalTimeBin();
      if(ltb != 0) co->SetSigmaZ2(c->GetSigmaZ2() * fSZ2corr);
      
      array->AddLast(co);
      delete ClusterArray->RemoveAt(iCluster); 
    }
  }

  delete ClusterArray;
  savedir->cd();   

}

//______________________________________________________________________
void AliTRDtracker::ReadClusters(TObjArray *array, const Char_t *filename)
{
  //
  // Reads AliTRDclusters from file <filename>. The names of the cluster
  // tree and branches should match the ones used in
  // AliTRDclusterizer::WriteClusters()
  // if <array> == 0, clusters are added into AliTRDtracker fCluster array
  //

  TDirectory *savedir=gDirectory;

  TFile *file = TFile::Open(filename);
  if (!file->IsOpen()) {
    cerr<<"Can't open file with TRD clusters"<<endl;
    return;
  }

  Char_t treeName[12];
  sprintf(treeName,"TreeR%d_TRD",fEvent);
  TTree *ClusterTree = (TTree*) gDirectory->Get(treeName);

  if (!ClusterTree) {
     cerr<<"AliTRDtracker::ReadClusters(): ";
     cerr<<"can't get a tree with clusters !\n";
     return;
  }

  TObjArray *ClusterArray = new TObjArray(400);

  ClusterTree->GetBranch("TRDcluster")->SetAddress(&ClusterArray);

  Int_t nEntries = (Int_t) ClusterTree->GetEntries();
  cout<<"found "<<nEntries<<" in ClusterTree"<<endl;   

  // Loop through all entries in the tree
  Int_t nbytes;
  AliTRDcluster *c = 0;

  printf("\n");

  for (Int_t iEntry = 0; iEntry < nEntries; iEntry++) {

    // Import the tree
    nbytes += ClusterTree->GetEvent(iEntry);

    // Get the number of points in the detector
    Int_t nCluster = ClusterArray->GetEntriesFast();
    printf("\r Read %d clusters from entry %d", nCluster, iEntry);

    // Loop through all TRD digits
    for (Int_t iCluster = 0; iCluster < nCluster; iCluster++) {
      c = (AliTRDcluster*)ClusterArray->UncheckedAt(iCluster);
      AliTRDcluster *co = new AliTRDcluster(*c);
      co->SetSigmaY2(c->GetSigmaY2() * fSY2corr);
      Int_t ltb = co->GetLocalTimeBin();
      if(ltb != 0) co->SetSigmaZ2(c->GetSigmaZ2() * fSZ2corr);
      array->AddLast(co);
      delete ClusterArray->RemoveAt(iCluster);
    }
  }

  file->Close();
  delete ClusterArray;
  savedir->cd();

}                      


//__________________________________________________________________
void AliTRDtracker::CookLabel(AliKalmanTrack* pt, Float_t wrong) const {

  Int_t label=123456789, index, i, j;
  Int_t ncl=pt->GetNumberOfClusters();
  const Int_t range = fTrSec[0]->GetOuterTimeBin()+1;

  Bool_t label_added;

  //  Int_t s[range][2];
  Int_t **s = new Int_t* [range];
  for (i=0; i<range; i++) {
    s[i] = new Int_t[2];
  }
  for (i=0; i<range; i++) {
    s[i][0]=-1;
    s[i][1]=0;
  }

  Int_t t0,t1,t2;
  for (i=0; i<ncl; i++) {
    index=pt->GetClusterIndex(i);
    AliTRDcluster *c=(AliTRDcluster*)fClusters->UncheckedAt(index);
    t0=c->GetLabel(0);
    t1=c->GetLabel(1);
    t2=c->GetLabel(2);
  }

  for (i=0; i<ncl; i++) {
    index=pt->GetClusterIndex(i);
    AliTRDcluster *c=(AliTRDcluster*)fClusters->UncheckedAt(index);
    for (Int_t k=0; k<3; k++) { 
      label=c->GetLabel(k);
      label_added=kFALSE; j=0;
      if (label >= 0) {
	while ( (!label_added) && ( j < range ) ) {
	  if (s[j][0]==label || s[j][1]==0) {
	    s[j][0]=label; 
	    s[j][1]=s[j][1]+1; 
	    label_added=kTRUE;
	  }
	  j++;
	}
      }
    }
  }

  Int_t max=0;
  label = -123456789;

  for (i=0; i<range; i++) {
    if (s[i][1]>max) {
      max=s[i][1]; label=s[i][0];
    }
  }

  for (i=0; i<range; i++) {
    delete []s[i];
  }        

  delete []s;

  if ((1.- Float_t(max)/ncl) > wrong) label=-label;   

  pt->SetLabel(label); 

}


//__________________________________________________________________
void AliTRDtracker::UseClusters(const AliKalmanTrack* t, Int_t from) const {
  Int_t ncl=t->GetNumberOfClusters();
  for (Int_t i=from; i<ncl; i++) {
    Int_t index = t->GetClusterIndex(i);
    AliTRDcluster *c=(AliTRDcluster*)fClusters->UncheckedAt(index);
    c->Use();
  }
}


//_____________________________________________________________________
Double_t AliTRDtracker::ExpectedSigmaY2(Double_t r, Double_t tgl, Double_t pt)
{
  // Parametrised "expected" error of the cluster reconstruction in Y 

  Double_t s = 0.08 * 0.08;    
  return s;
}

//_____________________________________________________________________
Double_t AliTRDtracker::ExpectedSigmaZ2(Double_t r, Double_t tgl)
{
  // Parametrised "expected" error of the cluster reconstruction in Z 

  Double_t s = 6 * 6 /12.;  
  return s;
}                  


//_____________________________________________________________________
Double_t AliTRDtracker::GetX(Int_t sector, Int_t plane, Int_t local_tb) const 
{
  //
  // Returns radial position which corresponds to time bin <local_tb>
  // in tracking sector <sector> and plane <plane>
  //

  Int_t index = fTrSec[sector]->CookTimeBinIndex(plane, local_tb); 
  Int_t pl = fTrSec[sector]->GetLayerNumber(index);
  return fTrSec[sector]->GetLayer(pl)->GetX();

}


//_______________________________________________________
AliTRDtracker::AliTRDpropagationLayer::AliTRDpropagationLayer(Double_t x, 
	       Double_t dx, Double_t rho, Double_t x0, Int_t tb_index)
{ 
  //
  // AliTRDpropagationLayer constructor
  //

  fN = 0; fX = x; fdX = dx; fRho = rho; fX0 = x0;
  fClusters = NULL; fIndex = NULL; fTimeBinIndex = tb_index;


  for(Int_t i=0; i < (Int_t) kZONES; i++) {
    fZc[i]=0; fZmax[i] = 0;
  }

  fYmax = 0;

  if(fTimeBinIndex >= 0) { 
    fClusters = new AliTRDcluster*[kMAX_CLUSTER_PER_TIME_BIN];
    fIndex = new UInt_t[kMAX_CLUSTER_PER_TIME_BIN];
  }

  fHole = kFALSE;
  fHoleZc = 0;
  fHoleZmax = 0;
  fHoleYc = 0;
  fHoleYmax = 0;
  fHoleRho = 0;
  fHoleX0 = 0;

}

//_______________________________________________________
void AliTRDtracker::AliTRDpropagationLayer::SetHole(
	  Double_t Zmax, Double_t Ymax, Double_t rho, 
	  Double_t x0, Double_t Yc, Double_t Zc) 
{
  //
  // Sets hole in the layer 
  //

  fHole = kTRUE;
  fHoleZc = Zc;
  fHoleZmax = Zmax;
  fHoleYc = Yc;
  fHoleYmax = Ymax;
  fHoleRho = rho;
  fHoleX0 = x0;
}
  

//_______________________________________________________
AliTRDtracker::AliTRDtrackingSector::AliTRDtrackingSector(AliTRDgeometry* geo, Int_t gs, AliTRDparameter* par)
{
  //
  // AliTRDtrackingSector Constructor
  //

  fGeom = geo;
  fPar = par;
  fGeomSector = gs;
  fTzeroShift = 0.13;
  fN = 0;

  for(UInt_t i=0; i < kMAX_TIME_BIN_INDEX; i++) fTimeBinIndex[i] = -1;


  AliTRDpropagationLayer* ppl;

  Double_t x, xin, xout, dx, rho, x0;
  Int_t    steps;

  // set time bins in the gas of the TPC

  xin = 246.055; xout = 254.055; steps = 20; dx = (xout-xin)/steps;
  rho = 0.9e-3;  x0 = 28.94;

  for(Int_t i=0; i<steps; i++) {
    x = xin + i*dx + dx/2;
    ppl = new AliTRDpropagationLayer(x,dx,rho,x0,-1);
    InsertLayer(ppl);
  }

  // set time bins in the outer field cage vessel

  dx = 50e-4; xin = xout; xout = xin + dx; rho = 1.71; x0 = 44.77; // Tedlar
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);

  dx = 0.02; xin = xout; xout = xin + dx; rho = 1.45; x0 = 44.86; // prepreg
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);

  dx = 2.; xin = xout; xout = xin + dx; rho = 1.45*0.02; x0 = 41.28; // Nomex
  steps = 5; dx = (xout - xin)/steps;
  for(Int_t i=0; i<steps; i++) {
    x = xin + i*dx + dx/2;
    ppl = new AliTRDpropagationLayer(x,dx,rho,x0,-1);
    InsertLayer(ppl);
  }

  dx = 0.02; xin = xout; xout = xin + dx; rho = 1.45; x0 = 44.86; // prepreg
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);

  dx = 50e-4; xin = xout; xout = xin + dx; rho = 1.71; x0 = 44.77; // Tedlar
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);


  // set time bins in CO2

  xin = xout; xout = 275.0; 
  steps = 50; dx = (xout - xin)/steps;
  rho = 1.977e-3;  x0 = 36.2;
  
  for(Int_t i=0; i<steps; i++) {
    x = xin + i*dx + dx/2;
    ppl = new AliTRDpropagationLayer(x,dx,rho,x0,-1);
    InsertLayer(ppl);
  }

  // set time bins in the outer containment vessel

  dx = 50e-4; xin = xout; xout = xin + dx; rho = 2.7; x0 = 24.01; // Al
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);

  dx = 50e-4; xin = xout; xout = xin + dx; rho = 1.71; x0 = 44.77; // Tedlar
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);

  dx = 0.06; xin = xout; xout = xin + dx; rho = 1.45; x0 = 44.86; // prepreg
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);

  dx = 3.; xin = xout; xout = xin + dx; rho = 1.45*0.02; x0 = 41.28; // Nomex
  steps = 10; dx = (xout - xin)/steps;
  for(Int_t i=0; i<steps; i++) {
    x = xin + i*dx + dx/2;
    ppl = new AliTRDpropagationLayer(x,dx,rho,x0,-1);
    InsertLayer(ppl);
  }

  dx = 0.06; xin = xout; xout = xin + dx; rho = 1.45; x0 = 44.86; // prepreg
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);

  dx = 50e-4; xin = xout; xout = xin + dx; rho = 1.71; x0 = 44.77; // Tedlar
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);
  
  dx = 50e-4; xin = xout; xout = xin + dx; rho = 2.7; x0 = 24.01; // Al
  ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
  InsertLayer(ppl);

  Double_t xtrd = (Double_t) fGeom->Rmin();  

  // add layers between TPC and TRD (Air temporarily)
  xin = xout; xout = xtrd;
  steps = 50; dx = (xout - xin)/steps;
  rho = 1.2e-3;  x0 = 36.66;
  
  for(Int_t i=0; i<steps; i++) {
    x = xin + i*dx + dx/2;
    ppl = new AliTRDpropagationLayer(x,dx,rho,x0,-1);
    InsertLayer(ppl);
  }


  Double_t alpha=AliTRDgeometry::GetAlpha();

  // add layers for each of the planes

  Double_t dxRo = (Double_t) fGeom->CroHght();    // Rohacell 
  Double_t dxSpace = (Double_t) fGeom->Cspace();  // Spacing between planes
  Double_t dxAmp = (Double_t) fGeom->CamHght();   // Amplification region
  Double_t dxDrift = (Double_t) fGeom->CdrHght(); // Drift region  
  Double_t dxRad = (Double_t) fGeom->CraHght();   // Radiator
  Double_t dxTEC = dxRad + dxDrift + dxAmp + dxRo; 
  Double_t dxPlane = dxTEC + dxSpace; 

  Int_t tbBefore = (Int_t) (dxAmp/fPar->GetTimeBinSize());
  if(tbBefore > fPar->GetTimeBefore()) tbBefore = fPar->GetTimeBefore();

  Int_t tb, tb_index;
  const Int_t  nChambers = AliTRDgeometry::Ncham();
  Double_t  Ymax = 0, holeYmax = 0;
  Double_t *  Zc  = new Double_t[nChambers];
  Double_t * Zmax = new Double_t[nChambers];
  Double_t  holeZmax = 1000.;   // the whole sector is missing


  for(Int_t plane = 0; plane < AliTRDgeometry::Nplan(); plane++) {

    // Radiator 
    xin = xtrd + plane * dxPlane; xout = xin + dxRad;
    steps = 12; dx = (xout - xin)/steps; rho = 0.074; x0 = 40.6; 
    for(Int_t i=0; i<steps; i++) {
      x = xin + i*dx + dx/2;
      ppl = new AliTRDpropagationLayer(x,dx,rho,x0,-1);
      if((fGeom->GetPHOShole()) && (fGeomSector >= 2) && (fGeomSector <= 6)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      if((fGeom->GetRICHhole()) && (fGeomSector >= 12) && (fGeomSector <= 14)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      InsertLayer(ppl);
    }

    Ymax = fGeom->GetChamberWidth(plane)/2;
    for(Int_t ch = 0; ch < nChambers; ch++) {
      Zmax[ch] = fGeom->GetChamberLength(plane,ch)/2;
      Float_t pad = fPar->GetRowPadSize(plane,ch,0);
      Float_t row0 = fPar->GetRow0(plane,ch,0);
      Int_t nPads = fPar->GetRowMax(plane,ch,0);
      Zc[ch] = (pad * nPads)/2 + row0 - pad/2;
    }

    dx = fPar->GetTimeBinSize(); 
    rho = 0.00295 * 0.85; x0 = 11.0;  

    Double_t x0 = (Double_t) fPar->GetTime0(plane);
    Double_t xbottom = x0 - dxDrift;
    Double_t xtop = x0 + dxAmp;

    // Amplification region

    steps = (Int_t) (dxAmp/dx);

    for(tb = 0; tb < steps; tb++) {
      x = x0 + tb * dx + dx/2;
      tb_index = CookTimeBinIndex(plane, -tb-1);
      ppl = new AliTRDpropagationLayer(x,dx,rho,x0,tb_index);
      ppl->SetYmax(Ymax);
      for(Int_t ch = 0; ch < nChambers; ch++) {
	ppl->SetZmax(ch, Zc[ch], Zmax[ch]);
      }
      if((fGeom->GetPHOShole()) && (fGeomSector >= 2) && (fGeomSector <= 6)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      if((fGeom->GetRICHhole()) && (fGeomSector >= 12) && (fGeomSector <= 14)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      InsertLayer(ppl);
    }
    tb_index = CookTimeBinIndex(plane, -steps);
    x = (x + dx/2 + xtop)/2;
    dx = 2*(xtop-x);
    ppl = new AliTRDpropagationLayer(x,dx,rho,x0,tb_index);
    ppl->SetYmax(Ymax);
    for(Int_t ch = 0; ch < nChambers; ch++) {
      ppl->SetZmax(ch, Zc[ch], Zmax[ch]);
    }
    if((fGeom->GetPHOShole()) && (fGeomSector >= 2) && (fGeomSector <= 6)) {
      holeYmax = x*TMath::Tan(0.5*alpha);
      ppl->SetHole(holeYmax, holeZmax);
    }
    if((fGeom->GetRICHhole()) && (fGeomSector >= 12) && (fGeomSector <= 14)) {
      holeYmax = x*TMath::Tan(0.5*alpha);
      ppl->SetHole(holeYmax, holeZmax);
    }
    InsertLayer(ppl);

    // Drift region
    dx = fPar->GetTimeBinSize();
    steps = (Int_t) (dxDrift/dx);

    for(tb = 0; tb < steps; tb++) {
      x = x0 - tb * dx - dx/2;
      tb_index = CookTimeBinIndex(plane, tb);

      ppl = new AliTRDpropagationLayer(x,dx,rho,x0,tb_index);
      ppl->SetYmax(Ymax);
      for(Int_t ch = 0; ch < nChambers; ch++) {
	ppl->SetZmax(ch, Zc[ch], Zmax[ch]);
      }
      if((fGeom->GetPHOShole()) && (fGeomSector >= 2) && (fGeomSector <= 6)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      if((fGeom->GetRICHhole()) && (fGeomSector >= 12) && (fGeomSector <= 14)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      InsertLayer(ppl);
    }
    tb_index = CookTimeBinIndex(plane, steps);
    x = (x - dx/2 + xbottom)/2;
    dx = 2*(x-xbottom);
    ppl = new AliTRDpropagationLayer(x,dx,rho,x0,tb_index);
    ppl->SetYmax(Ymax);
    for(Int_t ch = 0; ch < nChambers; ch++) {
      ppl->SetZmax(ch, Zc[ch], Zmax[ch]);
    }
    if((fGeom->GetPHOShole()) && (fGeomSector >= 2) && (fGeomSector <= 6)) {
      holeYmax = x*TMath::Tan(0.5*alpha);
      ppl->SetHole(holeYmax, holeZmax);
    }
    if((fGeom->GetRICHhole()) && (fGeomSector >= 12) && (fGeomSector <= 14)) {
      holeYmax = x*TMath::Tan(0.5*alpha);
      ppl->SetHole(holeYmax, holeZmax);
    }
    InsertLayer(ppl);

    // Pad Plane
    xin = xtop; dx = 0.025; xout = xin + dx; rho = 1.7; x0 = 33.0;
    ppl = new AliTRDpropagationLayer(xin+dx/2,dx,rho,x0,-1);
    if((fGeom->GetPHOShole()) && (fGeomSector >= 2) && (fGeomSector <= 6)) {
      holeYmax = (xin+dx/2)*TMath::Tan(0.5*alpha);
      ppl->SetHole(holeYmax, holeZmax);
    }
    if((fGeom->GetRICHhole()) && (fGeomSector >= 12) && (fGeomSector <= 14)) {
      holeYmax = (xin+dx/2)*TMath::Tan(0.5*alpha);
      ppl->SetHole(holeYmax, holeZmax);
    }
    InsertLayer(ppl);

    // Rohacell
    xin = xout; xout = xtrd + (plane + 1) * dxPlane - dxSpace;
    steps = 5; dx = (xout - xin)/steps; rho = 0.074; x0 = 40.6; 
    for(Int_t i=0; i<steps; i++) {
      x = xin + i*dx + dx/2;
      ppl = new AliTRDpropagationLayer(x,dx,rho,x0,-1);
      if((fGeom->GetPHOShole()) && (fGeomSector >= 2) && (fGeomSector <= 6)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      if((fGeom->GetRICHhole()) && (fGeomSector >= 12) && (fGeomSector <= 14)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      InsertLayer(ppl);
    }

    // Space between the chambers, air
    xin = xout; xout = xtrd + (plane + 1) * dxPlane;
    steps = 5; dx = (xout - xin)/steps; rho = 1.29e-3; x0 = 36.66; 
    for(Int_t i=0; i<steps; i++) {
      x = xin + i*dx + dx/2;
      ppl = new AliTRDpropagationLayer(x,dx,rho,x0,-1);
      if((fGeom->GetPHOShole()) && (fGeomSector >= 2) && (fGeomSector <= 6)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      if((fGeom->GetRICHhole()) && (fGeomSector >= 12) && (fGeomSector <= 14)) {
	holeYmax = x*TMath::Tan(0.5*alpha);
	ppl->SetHole(holeYmax, holeZmax);
      }
      InsertLayer(ppl);
    }
  }    

  // Space between the TRD and RICH
  Double_t xRICH = 500.;
  xin = xout; xout = xRICH;
  steps = 200; dx = (xout - xin)/steps; rho = 1.29e-3; x0 = 36.66; 
  for(Int_t i=0; i<steps; i++) {
    x = xin + i*dx + dx/2;
    ppl = new AliTRDpropagationLayer(x,dx,rho,x0,-1);
    InsertLayer(ppl);
  }

  MapTimeBinLayers();
  delete [] Zc;
  delete [] Zmax;

}

//______________________________________________________

Int_t  AliTRDtracker::AliTRDtrackingSector::CookTimeBinIndex(Int_t plane, Int_t local_tb) const
{
  //
  // depending on the digitization parameters calculates "global"
  // time bin index for timebin <local_tb> in plane <plane>
  //

  Double_t dxAmp = (Double_t) fGeom->CamHght();   // Amplification region
  Double_t dxDrift = (Double_t) fGeom->CdrHght(); // Drift region  
  Double_t dx = (Double_t) fPar->GetTimeBinSize();  

  Int_t tbAmp = fPar->GetTimeBefore();
  Int_t maxAmp = (Int_t) ((dxAmp+0.000001)/dx);
  Int_t tbDrift = fPar->GetTimeMax();
  Int_t maxDrift = (Int_t) ((dxDrift+0.000001)/dx);

  Int_t tb_per_plane = TMath::Min(tbAmp,maxAmp) + TMath::Min(tbDrift,maxDrift);

  Int_t gtb = (plane+1) * tb_per_plane - local_tb - 1;

  if((local_tb < 0) && 
     (TMath::Abs(local_tb) > TMath::Min(tbAmp,maxAmp))) return -1;
  if(local_tb >= TMath::Min(tbDrift,maxDrift)) return -1;

  return gtb;


}

//______________________________________________________

void AliTRDtracker::AliTRDtrackingSector::MapTimeBinLayers() 
{
  //
  // For all sensitive time bins sets corresponding layer index
  // in the array fTimeBins 
  //

  Int_t index;

  for(Int_t i = 0; i < fN; i++) {
    index = fLayers[i]->GetTimeBinIndex();
    
    //    printf("gtb %d -> pl %d -> x %f \n", index, i, fLayers[i]->GetX());

    if(index < 0) continue;
    if(index >= (Int_t) kMAX_TIME_BIN_INDEX) {
      printf("*** AliTRDtracker::MapTimeBinLayers: \n");
      printf("    index %d exceeds allowed maximum of %d!\n",
	     index, kMAX_TIME_BIN_INDEX-1);
      continue;
    }
    fTimeBinIndex[index] = i;
  }

  Double_t x1, dx1, x2, dx2, gap;

  for(Int_t i = 0; i < fN-1; i++) {
    x1 = fLayers[i]->GetX();
    dx1 = fLayers[i]->GetdX();
    x2 = fLayers[i+1]->GetX();
    dx2 = fLayers[i+1]->GetdX();
    gap = (x2 - dx2/2) - (x1 + dx1/2);
    if(gap < -0.01) {
      printf("*** warning: layers %d and %d are overlayed:\n",i,i+1);
      printf("             %f + %f + %f > %f\n", x1, dx1/2, dx2/2, x2);
    }
    if(gap > 0.01) { 
      printf("*** warning: layers %d and %d have a large gap:\n",i,i+1);
      printf("             (%f - %f) - (%f + %f) = %f\n", 
	     x2, dx2/2, x1, dx1, gap);
    }
  }
}
  

//______________________________________________________


Int_t AliTRDtracker::AliTRDtrackingSector::GetLayerNumber(Double_t x) const
{
  // 
  // Returns the number of time bin which in radial position is closest to <x>
  //

  if(x >= fLayers[fN-1]->GetX()) return fN-1; 
  if(x <= fLayers[0]->GetX()) return 0; 

  Int_t b=0, e=fN-1, m=(b+e)/2;
  for (; b<e; m=(b+e)/2) {
    if (x > fLayers[m]->GetX()) b=m+1;
    else e=m;
  }
  if(TMath::Abs(x - fLayers[m]->GetX()) > 
     TMath::Abs(x - fLayers[m+1]->GetX())) return m+1;
  else return m;

}

//______________________________________________________

Int_t AliTRDtracker::AliTRDtrackingSector::GetInnerTimeBin() const 
{
  // 
  // Returns number of the innermost SENSITIVE propagation layer
  //

  return GetLayerNumber(0);
}

//______________________________________________________

Int_t AliTRDtracker::AliTRDtrackingSector::GetOuterTimeBin() const 
{
  // 
  // Returns number of the outermost SENSITIVE time bin
  //

  return GetLayerNumber(GetNumberOfTimeBins() - 1);
}

//______________________________________________________

Int_t AliTRDtracker::AliTRDtrackingSector::GetNumberOfTimeBins() const 
{
  // 
  // Returns number of SENSITIVE time bins
  //

  Int_t tb, layer;
  for(tb = kMAX_TIME_BIN_INDEX-1; tb >=0; tb--) {
    layer = GetLayerNumber(tb);
    if(layer>=0) break;
  }
  return tb+1;
}

//______________________________________________________

void AliTRDtracker::AliTRDtrackingSector::InsertLayer(AliTRDpropagationLayer* pl)
{ 
  //
  // Insert layer <pl> in fLayers array.
  // Layers are sorted according to X coordinate.

  if ( fN == ((Int_t) kMAX_LAYERS_PER_SECTOR)) {
    printf("AliTRDtrackingSector::InsertLayer(): Too many layers !\n");
    return;
  }
  if (fN==0) {fLayers[fN++] = pl; return;}
  Int_t i=Find(pl->GetX());

  memmove(fLayers+i+1 ,fLayers+i,(fN-i)*sizeof(AliTRDpropagationLayer*));
  fLayers[i]=pl; fN++;

}              

//______________________________________________________

Int_t AliTRDtracker::AliTRDtrackingSector::Find(Double_t x) const 
{
  //
  // Returns index of the propagation layer nearest to X 
  //

  if (x <= fLayers[0]->GetX()) return 0;
  if (x > fLayers[fN-1]->GetX()) return fN;
  Int_t b=0, e=fN-1, m=(b+e)/2;
  for (; b<e; m=(b+e)/2) {
    if (x > fLayers[m]->GetX()) b=m+1;
    else e=m;
  }
  return m;
}             

//______________________________________________________

void AliTRDtracker::AliTRDpropagationLayer::GetPropagationParameters(
        Double_t y, Double_t z, Double_t &dx, Double_t &rho, Double_t &x0, 
	Bool_t &lookForCluster) const
{
  //
  // Returns radial step <dx>, density <rho>, rad. length <x0>,
  // and sensitivity <lookForCluster> in point <y,z>  
  //

  dx  = fdX;
  rho = fRho;
  x0  = fX0;
  lookForCluster = kFALSE;

  // check dead regions
  if(fTimeBinIndex >= 0) {
    for(Int_t ch = 0; ch < (Int_t) kZONES; ch++) {
      if(TMath::Abs(z - fZc[ch]) < fZmax[ch]) 
	lookForCluster = kTRUE;
    }
    if(TMath::Abs(y) > fYmax) lookForCluster = kFALSE;
    if(!lookForCluster) { 
      //      rho = 1.7; x0 = 33.0; // G10 
    }
  }

  // check hole
  if(fHole && (TMath::Abs(y - fHoleYc) < fHoleYmax) && 
              (TMath::Abs(z - fHoleZc) < fHoleZmax)) {
    lookForCluster = kFALSE;
    rho = fHoleRho;
    x0  = fHoleX0;
  }         

  return;
}

//______________________________________________________

void AliTRDtracker::AliTRDpropagationLayer::InsertCluster(AliTRDcluster* c, 
							  UInt_t index) {

// Insert cluster in cluster array.
// Clusters are sorted according to Y coordinate.  

  if(fTimeBinIndex < 0) { 
    printf("*** attempt to insert cluster into non-sensitive time bin!\n");
    return;
  }

  if (fN== (Int_t) kMAX_CLUSTER_PER_TIME_BIN) {
    printf("AliTRDpropagationLayer::InsertCluster(): Too many clusters !\n"); 
    return;
  }
  if (fN==0) {fIndex[0]=index; fClusters[fN++]=c; return;}
  Int_t i=Find(c->GetY());
  memmove(fClusters+i+1 ,fClusters+i,(fN-i)*sizeof(AliTRDcluster*));
  memmove(fIndex   +i+1 ,fIndex   +i,(fN-i)*sizeof(UInt_t)); 
  fIndex[i]=index; fClusters[i]=c; fN++;
}  

//______________________________________________________

Int_t AliTRDtracker::AliTRDpropagationLayer::Find(Double_t y) const {

// Returns index of the cluster nearest in Y    

  if (y <= fClusters[0]->GetY()) return 0;
  if (y > fClusters[fN-1]->GetY()) return fN;
  Int_t b=0, e=fN-1, m=(b+e)/2;
  for (; b<e; m=(b+e)/2) {
    if (y > fClusters[m]->GetY()) b=m+1;
    else e=m;
  }
  return m;
}    









