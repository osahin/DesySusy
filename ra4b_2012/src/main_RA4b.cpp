//========================================================================//
//                                                                        //
//                         RA4 b                                          //
//                                                                        //
//========================================================================//
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include "TSystem.h"
#include "TH1.h"
#include "TH2D.h"
#include "TProfile.h"
#include "TFile.h"
#include "TNtuple.h"
#include "TTree.h"
#include "Math/VectorUtil.h"
#include "NtupleTools2.h"
#include "ConfigReader.h"
#include "CutFlow.h"
#include "eventselection.h"
#include "SampleInformation.h"
#include "Electron.h"
#include "Muon.h"
#include "Jet.h"
#include "makeElectrons.h"
#include "makeMuons.h"
#include "triggers_RA4b.h"
#include "triggers_RA4b_frequency.h"
#include "vertices_RA4b.h"
#include "metAndHT_RA4b.h"
#include "evtqual_RA4b.h"
#include "SetTriggers_RA4b.h"
#include "SetConditions_RA4b.h"
#include "cschalo_RA4b.h"
#include "trackingFailure_RA4b.h"
#include "THTools.h"
#include "TStopwatch.h"
#include "Electron.h"
#include "makeJets.h"
#include "HistoMaker.h"
#include "mt2w_interface.h"
#include "subTree.h"
#include "subTreeFactory.h"
#include "EventInfo.h"
#include "systematics_RA4b.h"
#include "makeSystematics.h"
#include "CMSSW_5_3_3_patch2/src/CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "CMSSW_5_3_3_patch2/src/CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"
#include "Utilities.cc"
#include "JetCorrectorParameters.cc"
#include "SimpleJetCorrectionUncertainty.cc"
#include "JetCorrectionUncertainty.cc"
#include "typelookup.cc"
#include "Exception.cc"
#include "TagEff.h"
#include "genJet.h"
#include <boost/shared_ptr.hpp>
#include "typedefs.h"
#include "trigStudyTree.h"
#include "francescoParticle.h"
#include "makeTracks.h"
#include "Tau.h"
#include "makeTaus.h"

using namespace std;
using namespace ROOT::Math::VectorUtil;
//===================================================================
//                   GLOBAL VARIABLES
//===================================================================

bool pcp = false; //Set to true for debugging.
TH1D* JetUncDiff= new TH1D("JetUncDiff","diff in jet uncertainty",50,-0.02,0.02);

vector<TH1D*> JetResPtBins;
vector<TH1D*> SMfactor_PtBins;
vector<TH1D*> JetResEtaBins;
vector<TH1D*> JetResPtBins_smear;
vector<TH1D*> JetResEtaBins_smear;
TH1D* distmatched;
TH1D* distmatched_cut;

//for (int k=0;k<5;++k){
//  JetResPtBins.push_back(new TH1D("JetRes_PtBin"+(TString)Form("%d",k),"jet Res Pt bin "+(TString)Form("%d",k),40,-0.2,0.2));
//  JetResEtaBins.push_back(new TH1D("JetRes_EtaBin"+(TString)Form("%d",k),"jet Res Eta bin "+(TString)Form("%d",k),40,-0.2,0.2));
//  JetResPtBins_smear.push_back(new TH1D("JetRes_PtBin"+(TString)Form("%d",k)+"_smear","jet Res Pt bin "+(TString)Form("%d",k),40,-0.2,0.2));
//  JetResEtaBins_smear.push_back(new TH1D("JetRes_EtaBin"+(TString)Form("%d",k)+"_smear","jet Res Eta bin "+(TString)Form("%d",k),40,-0.2,0.2));
// }

//Globals to remove
//bool checkthisevent=false;
//vector<string> triggernames;
//vector<string> triggernames_short;
//double EventWeight;



//================================================================================
int main(int argc, char** argv){

  //PCP=TRUE FOR DEBUGGING
  pcp=false;


  double EventWeight=1.0;

  //CONFIG. The program will always take the file config.txt in the output directory
  //specified in the execution of runOnAll
  string MainDir="./";
  ConfigReader config(MainDir+"config.txt",argc,argv);
  config.Add(MainDir+"para_config.txt");
  config.Add(MainDir+"pu_config.txt");

  //NAME OF THE FILE
  TString filename = config.getTString("filename");
  
  EasyChain* tree = new EasyChain("/susyTree/tree");
  // add file(s) or folder(s)
  int f = tree->AddSmart(filename);


  TString outname = config.getTString("outname",tree->GetUniqeName());
  string outname_string=(string)outname;
  // output directory and output file name(derived from input name)
  cout<<endl;
  cout<<"-----------------------------------------------"<<endl;
  cout<<"The output file name is "<<outname<<endl;
  cout<<"-----------------------------------------------"<<endl;
  cout<<endl;
  TFile *outfile = TFile::Open(outname,"RECREATE");
  //TFile *outprovafile = TFile::Open("outprovafile.root","RECREATE");
  // set output root file for cut flow (to be done!)
  //  CutSet::setTFile(outfile);
  

 
  string replacestring=".root";
  string treeFileName="";
  int pos = outname_string.find(replacestring);
  size_t thelength=replacestring.length();
  if (pos != string::npos){
    treeFileName= outname_string.replace(pos,thelength,"_tree.root");
  }
  //

  //==============SUBTREE
  //
  TFile *treeFile=0;
  subTree* SubTree=0;
  bool doSubTree=config.getBool("doSubTree",true);
  if(doSubTree){
    treeFile=TFile::Open((TString)treeFileName,"RECREATE");
    TString treeType = config.getTString("treeType","default"); 
    SubTree= subTreeFactory::NewTree(treeType,treeFile,(string)"");
  }
  //==========================================


  //==============SUBTREE FOR THE TRIGGER STUDY
  bool doTrigStudy=config.getBool("doTrigStudy",true);
  trigStudyTree* TrigStudyTree=0;
  if(TrigStudyTree){
    if(!treeFile) treeFile=TFile::Open((TString)treeFileName,"RECREATE");
    TrigStudyTree = (trigStudyTree*)subTreeFactory::NewTree("trigStudyTree",treeFile,(string)"");
  }
  //===========================================

  outfile->cd();  




  //data or monte carlo?
  bool isData=config.getBool("isData");
  //=================================================================



  //===========================================================================
  // SET THE INFORMATION ABOUT THE SAMPLE
  //===========================================================================
  SampleInformation mySampleInfo;

  //it HAS to be read from config.txt, otherwise the default is
  //undefined
  string WhatSample=config.getString("SampleName","undefined");
  string WhatSubSample=config.getString("SubSampleName","undefined");
  string WhatEstimation=config.getString("Estimation","undefined");
  string WhatTail=config.getString("Tail","undefined");
  mySampleInfo.SetInformation(WhatSample,WhatSubSample,WhatEstimation,WhatTail);
  //print it
  mySampleInfo.DumpInformation();
  //===========================================================================



  //===========================================================================
  // SET THE CONDITIONS FOR THE SAMPLE AND ESTIMATION
  //===========================================================================
  SetOfCuts myCuts;
  SetConditions_RA4b(mySampleInfo);
  //===========================================================================


  if(pcp)cout<<"going to set the triggers "<<endl;
  //===========================================================================
  // SET THE TRIGGERS
  //===========================================================================
  vector<const char*> triggernames;
  SetTriggers_RA4b(mySampleInfo,triggernames);
  //===========================================================================
  if(pcp)cout<<"out of set the triggers "<<endl;
  if(pcp)cout<<"check point 1"<<endl;







  //======================================================
  //WEIGHTS
  //======================================================



  //PU

  bool oldpuw = false; //the obselete method and values
  //if(WhatSample=="TTJets")oldpuw=true;

  vector<double> PUmc;
  vector<double> PUdata;
  vector<double> PUdata_up;
  vector<double> PUdata_down;
  int nobinsmc=0;
  int nobinsdata=0;
  int nobinsdata_up=0;
  int nobinsdata_down=0;
  if(!isData && !oldpuw){
    nobinsmc = config.getDouble("PU_"+WhatSample+"_"+WhatSubSample+"_mc",PUmc," ");
    nobinsdata = config.getDouble("PU_data",PUdata," ");
    //nobinsdata = config.getDouble("PU_data_up",  PUdata_up  ," ");
    //nobinsdata = config.getDouble("PU_data_down",PUdata_down," ");
  }
  else if(!isData && oldpuw){
    nobinsmc = config.getDouble("oldPU_"+WhatSample+"_"+WhatSubSample+"_mc",PUmc," ");
   } 

  if(nobinsmc!=nobinsdata && !oldpuw && !isData){
    cout << "problem in pu inf in para_config - number of bins in MC PU dist is different than data!" << endl;
    return 0;
  }

  double InitialEventWeight=1.0;             //Event weight do to Lumi and xsec.

  //Tag
  int bin=0;
  bin = config.getInt("lasttagbin",3); // set the lasttagbin i.e 3 to get tag weightings for 0,1,2,3+
  //TagEff tag(WhatSample,WhatSubSample);
  //tag.lastbin(bin);





  //keep track of the weights
  TH1::SetDefaultSumw2(true);
  //=========================




  //===============================================================
  //===============================================================
  //FILE READING (and all that)
  //===============================================================
  //===============================================================
  //files read;
  cout<<tree->GetNtrees()<<" Files read"<<endl;
  if(pcp)cout<<"check point 2"<<endl;
  //=================================================================
  int N = tree->GetEntries();
  //=================================================================
  cout<<"THERE ARE "<<N<<" EVENTS IN "<<filename<<endl;
  


  //=================================
  //SET THE CONTROL PLOTS HISTO MAKER
  //================================ 
  bool DoControlPlots=config.getBool("DoControlPlots",true); 
  //HistoMaker::setTFile(outfile); 
  HistoMaker ControlPlots("AnalyzeSUSY"," "); 
  ControlPlots.setTFile(outfile);
  outfile->cd();




  
  //================================================================
  TH1I* isdata = new TH1I("isdata","data =1 means Data",1,0,1);
  if(isData){isdata->SetBinContent(1,1);}
  else{isdata->SetBinContent(1,0);}
  //================================================================
 


  //================================================================   
  TH1I* num_entries = new TH1I("num_entries","number of entries",1,0,1);
  num_entries->SetBinContent(1,N);
  //================================================================

  //================================================================
  TH1I*num_entries_weighted = new TH1I("num_entries_weighted","number of entries with the event weight",1,0,1);
  num_entries_weighted->SetBinContent(1,N,EventWeight);
  //================================================================

  if(pcp)cout<<"check point before histos"<<endl;





  //===========================================
  //CUT FLOW MONITORING
  //===========================================
  //OK and OKold for the cutflow
  bool OK=false;
  bool OKold=true;
  CutSet globalFlow("global flow");
  CutSet::setTFile(outfile);
  CutSet* p2globalFlow= &globalFlow;
  CutSet systematicsFlow("systematics flow");
  CutSet selectedMuonFlow("selected_Muons");
  CutSet* pselectedMuonFlow = &selectedMuonFlow;

  //===========================================

    

  //===========================================
  bool turntriggersoff=config.getBool("TurnTriggersOff",false);
  if(turntriggersoff){
    cout<<"-----------TURNTRIGGERSOFF IS true!!-----------"<<endl;
    if(isData){
      cout<<endl;
      cout<<endl;
      cout<<"--------NO TRIGGERS, NO DATA------------------"<<endl;
      cout<<endl;
      cout<<endl;
      exit(1);
    }
  }
  //===========================================


  distmatched=new TH1D("distmatched","distance",40,0.0,0.6);
  distmatched_cut=new TH1D("distmatched_cut","distance",40,0.0,0.6);

  string smearing = config.getString("JetRes_smearing","mixed");
  cout<<"the smearing is "<<smearing<<" "<<endl;
  //  cout<<smearing==(string)"mixed"<<endl;

  TH1D* EW_AfterTrigger= new TH1D("EW_AfterTrigger","Event weight after the trigger",30,0.0,100.0);
  TH1D* EW_AfterPU=new TH1D("EW_AfterPU","Event Weight after PU RW",100,0.0,10.0);
  TH1D* checkMET= new TH1D("checkMET","check of the MET",30,0.0,300.0);
  

  TDirectory* metStudiesDir=outfile->mkdir("metStudies");                                                                                                                                                                                    
  metStudiesDir->cd();     


  vector<TH1D*> JetResPtBins_Matched;  
  vector<TH1D*> JetResEtaBins_Matched;  
  for (int k=0;k<5;++k){
    JetResPtBins.push_back(new TH1D("JetRes_PtBin"+(TString)Form("%d",k),"jet Res Pt bin "+(TString)Form("%d",k),40,-1.,1.5));
    SMfactor_PtBins.push_back(new TH1D("SMfactor_PtBin"+(TString)Form("%d",k),"jet Res Pt bin "+(TString)Form("%d",k),40,-0.2,0.2));
    JetResEtaBins.push_back(new TH1D("JetRes_EtaBin"+(TString)Form("%d",k),"jet Res Eta bin "+(TString)Form("%d",k),40,-1.5,1.));
    JetResPtBins_smear.push_back(new TH1D("JetRes_PtBin"+(TString)Form("%d",k)+"_smear","jet Res Pt bin "+(TString)Form("%d",k),40,-0.2,0.2));
    JetResEtaBins_smear.push_back(new TH1D("JetRes_EtaBin"+(TString)Form("%d",k)+"_smear","jet Res Eta bin "+(TString)Form("%d",k),40,-0.2,0.2));
    JetResPtBins_Matched.push_back(new TH1D("JetRes_PtBin"+(TString)Form("%d",k)+"_matched","jet Res Pt bin "+(TString)Form("%d",k)+" matched jets",40,-1.5,2.5));
    JetResEtaBins_Matched.push_back(new TH1D("JetRes_EtaBin"+(TString)Form("%d",k)+"_matched","jet Res Eta bin "+(TString)Form("%d",k)+" matched jets",40,-1.5,2.5));
  }
  TH1D* METRes= new TH1D("METRes","resolution of MET",30,-1.0,2.0);
  TH1D* METChange= new TH1D("METChange","change induced",30,-0.2,0.2);
  TH1D* newMETRes= new TH1D("newMETRes","new resolution of MET",30,-1.0,2.);
  TH1D* METRes_ACF= new TH1D("METRes_AfterCutF","resolution of MET after CutFlow",30,-1.0,2.0);
  TH1D* METChange_ACF= new TH1D("METChange_AfterCutF","change induced after CutFlow",30,-0.2,0.2);
  TH1D* newMETRes_ACF= new TH1D("newMETRes_AfterCutF","new resolution of MET after CutFlow",30,-1.0,2.);

  
  vector<TH1D*> METRes_METBins;
  vector<TH1D*> METRes_NMatchedJets_Bins;
  vector<TH1D*> METChange_NMatchedJets_Bins;
  vector<TH1D*> METRes_NJets_Bins;
  vector<TH1D*> METChange_METBins;
  vector<TH1D*> METRes_NMatchedJets_NJets;
  vector<TH1D*> METChange_NMatchedJets_NJets;

  for (int k=0;k<5;++k){
    METRes_METBins.push_back(new TH1D("METRes_METBin"+(TString)Form("%d",k),"MET Res MET bin "+(TString)Form("%d",k),30,-1.0,2.0));
    METChange_METBins.push_back(new TH1D("METChange_METBin"+(TString)Form("%d",k),"MET Change MET bin "+(TString)Form("%d",k),30,-0.2,0.2));
    METRes_NMatchedJets_Bins.push_back(new TH1D("METRes_NMatchedJets_Bin"+(TString)Form("%d",k),"MET Res NMatchedJets Bin "+(TString)Form("%d",k),30,-1.,1.5));
    METChange_NMatchedJets_Bins.push_back(new TH1D("METChange_NMatchedJets_Bin"+(TString)Form("%d",k),"MET Change NMatchedJets Bin "+(TString)Form("%d",k),30,-1.,1.5));
    METRes_NJets_Bins.push_back(new TH1D("METRes_NJets_Bin"+(TString)Form("%d",k),"MET Change NJets Bin "+(TString)Form("%d",k),30,-1.0,1.5));
  }

  for (int k=0;k<5;++k){
    for (int j=0;j<5;++j){
      METRes_NMatchedJets_NJets.push_back(new TH1D("METRes_NMatchedJets_NJet_Bin"+(TString)Form("%d",k)+(TString)Form("%d",j),"MET Res NMatchedJets Bin "+(TString)Form("%d",k),30,-1.,1.5));
      METChange_NMatchedJets_NJets.push_back(new TH1D("METChange_NMatchedJets_NJet_Bin"+(TString)Form("%d",k)+(TString)Form("%d",j),"MET Change NMatchedJets Bin "+(TString)Form("%d",k),30,-1.,1.5));
    }
  }
  


  TH2D* METRes_vs_MET=new TH2D("METRes_vs_MET","METRes_vs_MET",40,0.0,400.0,20,-1.0,3.0);
  
  TH1D* MTMu= new TH1D("MTMu","MTMu",50,0.,500);      


  outfile->cd(); 

  if(pcp)cout<<"check point before the event loop"<<endl;

  bool isquick=config.getBool("quick",true);
  bool quick=false;
  cout<<endl;
  cout<<endl;
  cout<<"isquick is"<<isquick<<endl;
  cout<<endl;
  cout<<endl;






  //===================================================================
  //SYSTEMATIC UNCERTAINTIES
  //===================================================================
  static bool doSystematics=config.getBool("doSystematics",true);
  doSystematics = doSystematics && !isData;
  bool OKsyst=false;
  Systematics systematics;
  if (!isquick && doSystematics){
    //it cant do systematics on the quick mode
    //systematics.AddUncertainty((string)"jetup",treeFile);
    //systematics.AddUncertainty((string)"jetdown",treeFile);
    //systematics.AddUncertainty((string)"clustersup",treeFile);
    //systematics.AddUncertainty((string)"clustersdown",treeFile);
    systematics.AddUncertainty((string)"jetRescentral",treeFile);
    systematics.AddUncertainty((string)"jetResup",treeFile);
    systematics.AddUncertainty((string)"jetResdown",treeFile);
  }

  
  //=============INITIALIZE THE JETUncertainty object
  typedef boost::shared_ptr<JetCorrectorParameters> JetCorrectorParam_Ptr;
  // Instantiate uncertainty sources
  const int nsrc = 16;
  const char* srcnames[nsrc] =
    {"Absolute", "HighPtExtra", "SinglePion", "Flavor", "Time",
     "RelativeJEREC1", "RelativeJEREC2", "RelativeJERHF",
     "RelativeStatEC2", "RelativeStatHF", "RelativeFSR",
     "PileUpDataMC", "PileUpOOT", "PileUpPt", "PileUpBias", "PileUpJetRate"};

  string jetuncfile;
  for (int isrc = 0; isrc < nsrc; isrc++) {
    const char *name = srcnames[isrc];
    
    bool isBatchJob=config.getBool("isBatchJob",false);
    std::string test1="src/Summer12_V2_DATA_AK5PF_UncertaintySources.txt";
    //std::string test2="../../Summer12_V2_DATA_AK5PF_UncertaintySources.txt";
    std::string test2="/afs/naf.desy.de/user/e/eron/scratch/UserCode/RA4bHead/UserCode/DesySusy/ra4b_2012/src/Summer12_V2_DATA_AK5PF_UncertaintySources.txt";
      if(!isBatchJob){
      jetuncfile=test1;
    }else{
      jetuncfile=test2;
    }

    JetCorrectorParameters *p;
    if (systematics.IsEnabled()){
      p= new JetCorrectorParameters(jetuncfile, name);
      systematics.vsrc.push_back(new JetCorrectionUncertainty(*p));
    }
  }
  //==========Total uncertainty for reference
  //  std::string test  ="/tmp/Summer12_V2_DATA_AK5PF_UncertaintySources.txt";
  //std::string test="src/Summer12_V2_DATA_AK5PF_UncertaintySources.txt";

  if(systematics.IsEnabled()){
    systematics.total = new JetCorrectionUncertainty(*(new JetCorrectorParameters(jetuncfile, "Total")));
  }













  //=============================================================================
  //=============================================================================
  //LOOP OVER EVENTS
  //=============================================================================
  //=============================================================================

  //N=1000;
  for(int i=0;i<N;++i){

    //This is necessary, do not touch!
    if(i==0){
      quick=false;
    }else{
      quick=isquick;
    }
     
    //continue;
    //    pcp=true;
    //if(i>10000)continue;

    timer();


    
    
    if(pcp)cout<<"check point about to get entry "<< i<<endl;

    //====================================================
    tree->GetEntry(i);
    //====================================================

    if(pcp)cout<<"check point after getting the entry "<< i<<endl;    
    if(pcp)cout<<"check point lets get the Event "<< i<<endl;    
    unsigned int Event   = tree->Get(Event,"event");    
    unsigned int Run   = tree->Get(Run,"run");    
    unsigned int lumiSection= tree->Get(lumiSection,"lumiSection");

    if(pcp)cout<<"check point got the Event "<< i<< " "<<Event<<endl;    

    
    //clear all the vectors inside systematics
    //otherwise they accumulate
    if (systematics.IsEnabled())systematics.Reset();


    //    pcp=true;
    //
    //    if (i>10){
    //      pcp=false;
    //      continue;
    //    }
    //    cout<<Event<<endl;


    //DEBUGGING:
    if(pcp){
      cout<<endl;
      cout<<endl;
      cout<<"**************************************"<<endl;
      cout<<"NEW EVENT : "<<Event<<endl;
      cout<<"**************************************"<<endl;
      cout<<endl;
      cout<<endl;
    }


    //================================================
    //EVENT WEIGHT
    //================================================
    EventWeight=InitialEventWeight;
    //================================================
    //  cout << "weight before PUrw -> " << EventWeight << endl;


    //==============================================
    // PILE UP RW
    //==============================================
    double PUWeight=1;
    double PUWeight_up=1;
    double PUWeight_down=1;
    int relevantNumPU=0;
    if(!isData) {
      float PUnumInter    = tree->Get( PUnumInter, "pileupTrueNumInteractionsBX0");
      relevantNumPU = (int) PUnumInter;
      if( relevantNumPU >= nobinsmc ) {
	cout << "something wrong with the pile up info!!! - exceed max number of vertex:     " << nobinsmc <<endl;
	return 0; 
      }

      else if( oldpuw) {
	PUWeight= PUmc.at( relevantNumPU );
      }
      else {
	PUWeight= PUdata.at( relevantNumPU )/PUmc.at( relevantNumPU );
	//#PUWeight_up= PUdata_up.at( relevantNumPU )/PUmc.at( relevantNumPU );
	//PUWeight_down= PUdata_down.at( relevantNumPU )/PUmc.at( relevantNumPU );
	PUWeight_up=0.0;
	PUWeight_down=0.0;
      }
      
      EventWeight *= PUWeight;
    }
    //cout << "weight before PUrw -> " << EventWeight << endl;
    EW_AfterPU->Fill(EventWeight);
    CutSet::global_event_weight  = EventWeight;
    HistoMaker::global_event_weight = EventWeight; //Set the weight to be used in the HistoMaker class
    //==============================================


    if(pcp){
      cout<<"=================================================== "<<endl;
      cout<<"going to check the event "<<Event<<endl;
      cout<<"=================================================== "<<endl;
    }





    //=====================================================================   
    //=====================================================================   
    //=====================================================================   
    //=====================================================================   
    

    //           C R E A T E    O B J E C T S                            //


    //=====================================================================   
    //=====================================================================   
    //=====================================================================   
    //=====================================================================   



    vector<fParticle> tracks; 

    //============================================
    // Make Muons
    vector<Muon> Muons;
    vector<Muon*> pMuons;
    vector<Muon*> LooseMuons;
    vector<Muon*> TightMuons;
    vector<Muon*> VetoMuons;


    vector<Muon*> goodMuons;
    vector<Muon*> selectedMuons; 

    Muons=makeAllMuons(tree);
    //makeGoodMuons( tree, Muons, goodMuons,pselectedMuonFlow);
    makeGoodMuons( tree, Muons, goodMuons);

    makeSelectedMuons( tree, Muons, selectedMuons,pselectedMuonFlow);
    makeSelectedMuons( tree, Muons, selectedMuons);
    makeTightMuons(tree,Muons,TightMuons);
    makeLooseMuons(tree,Muons,LooseMuons);
    makeVetoMuons(tree,Muons,VetoMuons);
    for (int imu=0; imu<Muons.size();++imu){
      pMuons.push_back(&Muons.at(imu));
    }
    //  makeSoftMuons(tree ,Muons,SoftMuons);
    //============================================


    //============================================
    // Make Electrons
    vector<Electron>  Electrons;
    vector<Electron*> pElectrons;
    vector<Electron*> LooseElectrons;
    vector<Electron*> TightElectrons;
    vector<Electron*> VetoElectrons;
    Electrons=makeAllElectrons(tree);
    //    makeLooseElectrons(tree,Electrons,LooseElectrons);
    makeTightElectrons(tree,Electrons,TightElectrons);
    makeVetoElectrons(tree, Electrons,VetoElectrons);
    for (int iel=0; iel<Electrons.size();++iel){
      pElectrons.push_back(&Electrons.at(iel));
    }
    //============================================


    vector<Tau>  taus;
    vector<Tau*> cleanTaus;
    taus = makeAllTaus( tree);
    //makeCleanTaus( taus, cleanTaus, goodMuons, goodElectrons);
    makeCleanTaus( taus, cleanTaus, goodMuons, TightElectrons);





    //LEPTON SELECTION//
    string SigMu = config.getString("SignalMuon_Selection","Selected");
    string WidMu = config.getString("WideMuon_Selection","Veto");
    string SigEl = config.getString("SignalElectron_Selection","Tight");
    string WidEl = config.getString("WideElectron_Selection","Veto");





    //SIGNAL AND WIDE LEPTONS//
    vector<Muon*> SignalMuons;
    vector<Muon*> WideMuons;
    for (int imu=0; (int)imu<Muons.size();++imu){
      if (Muons.at(imu).IsID(SigMu) ){  
	SignalMuons.push_back(&Muons.at(imu));
      }
      else if (!Muons.at(imu).IsID(SigMu) && Muons.at(imu).IsID(WidMu) ){
	WideMuons.push_back(&Muons.at(imu) ) ;
      }
    }
    vector<Electron*> SignalElectrons;
    vector<Electron*> WideElectrons;
    for (int imu=0; (int)imu<Electrons.size();++imu){
      if (Electrons.at(imu).IsID(SigEl) ){  
	SignalElectrons.push_back(&Electrons.at(imu));
      }
      else if (!Electrons.at(imu).IsID(SigEl) && Electrons.at(imu).IsID(WidEl) ){
	WideElectrons.push_back(&Electrons.at(imu) ) ;
      }
    }







    //============================================    
    // Make Jets
    //cout<<"vector of jets declared"<<endl;
    vector<Ptr_Jet> AllJets;
    vector<Ptr_Jet> GoodJets;
    vector<Ptr_Jet> CleanedJets;
    makeAllJets(tree,AllJets);
    makeGoodJets(tree,AllJets,GoodJets);

    makeCleanedJets( GoodJets, CleanedJets, pMuons, pElectrons);
    //makeCleanedJets( GoodJets, CleanedJets, goodMuons, pElectrons);

    //=======MATCHING OF JETS
    vector<Ptr_GenJet> allGenJets;
    if(!isData){
      makeAllGenJets(tree,allGenJets);
      matchGenJets(tree,allGenJets,AllJets);
    }
    //============================================

    //Define TAG algorithm & working points==============================
    static string btagAlgorithm= config.getString("bTagAlgorithm","CSV");
    static string btagWorkingPoint = config.getString("bTagWorkingPoint","Medium");
    int NumberOfbTags=0;
    
    for (int ijet=0;ijet<CleanedJets.size();++ijet){
      if(CleanedJets.at(ijet)->IsBJet("CSV","Medium")){
	NumberOfbTags++;
      }
    }
    //cout<<"putos btags "<<NumberOfbTags<<endl;

    //============================================
    //Make HT AND MHT
    //============================================
    double HT=0;
    double px=0;
    double py=0;
    double MHT=0;
    for(int ijet=0;ijet<(int)CleanedJets.size();++ijet){
      HT=HT+CleanedJets.at(ijet)->Pt();
      px+=  CleanedJets.at(ijet)->P4().Px();
      py+=  CleanedJets.at(ijet)->P4().Py();
    }
    MHT=sqrt(px*px + py*py);
    //============================================

    //============================================
    //Make MET
    LorentzM& PFmet = tree->Get(&PFmet, "metP4TypeIPF");
    double MET=(double)PFmet.Et() ;

    //============================================

    //RESOLUTION
    //LorentzM& TrueME=tree->Get(&PFmet, "metP4TypeIPF"); //just to initialize the ref
    double TrueMET;
    if(!isData){
      LorentzM& TrueME= tree->Get(&TrueME,"genmetP4True");  
      METRes->Fill((MET-TrueMET)/TrueMET,EventWeight);
    }
    if (pcp){
      cout<<endl;
      cout<<"MET = "<<MET<<endl;
      cout<<"HT = "<<HT<<endl;
      cout<<endl;
    }



    //===========================================
    //CALCULATE SYSTEMATICS VARIATIONS: JETS
    //===========================================
    if (systematics.IsEnabled()){


      ShiftJetEnergyScale(tree,systematics,AllJets,pMuons,pElectrons);
      ShiftClustersEnergyScale(tree,systematics,AllJets,pMuons,pElectrons);

      
      if( systematics.GetSysMap()["jetResup"]){
	//
	//
	rescaleJetsJER(AllJets,systematics);
	//=========RESCALE THE MET
	rescaleMET(tree,AllJets,systematics, "jetRescentral");
	rescaleMET(tree,AllJets,systematics, "jetResup");
	rescaleMET(tree,AllJets,systematics, "jetResdown");

	//rescaleMET(tree,systematics.GetsysJet("jetResup"),systematics,"jetResup");
	//rescaleMET(tree,systematics.GetsysJet("jetResdown"),systematics,"jetResdown");
	//cout<<"size of jetResup jets "<<systematics.GetsysJet("jetResup").size()<<endl;
	//======FEED THE RESCALED JETS TO makeGoodJets
	makeGoodJets(tree,systematics.GetsysJet("jetRescentral"),systematics.GetsysJet("jetRescentral_good"));
	makeGoodJets(tree,systematics.GetsysJet("jetResup"),systematics.GetsysJet("jetResup_good"));
	makeGoodJets(tree,systematics.GetsysJet("jetResdown"),systematics.GetsysJet("jetResdown_good"));
	//======CLEAN THEM
	makeCleanedJets(systematics.GetsysJet("jetRescentral_good"),systematics.GetsysJet("jetRescentral_good_clean"),pMuons,pElectrons);
	makeCleanedJets(systematics.GetsysJet("jetResup_good"),systematics.GetsysJet("jetResup_good_clean"),pMuons,pElectrons);
	makeCleanedJets(systematics.GetsysJet("jetResdown_good"),systematics.GetsysJet("jetResdown_good_clean"),pMuons,pElectrons);
	//	cout<<"size of jetResup good jets "<<systematics.GetsysJet("jetResup_good").size()<<endl;
	//=====GET THE NEW HT
	rescaleHT(systematics.GetsysJet("jetRescentral_good_clean"), systematics, "jetRescentral");
	rescaleHT(systematics.GetsysJet("jetResup_good_clean"), systematics, "jetResup");
	rescaleHT(systematics.GetsysJet("jetResdown_good_clean"), systematics, "jetResdown");
	//=======COLLECTION OF JETS TO BE STORED IN THE SYSTEMATICS TREE
	systematics.SetTreeJetCollection("jetRescentral","jetRescentral_good_clean");
	systematics.SetTreeJetCollection("jetResup","jetResup_good_clean");
	systematics.SetTreeJetCollection("jetResdown","jetResdown_good_clean");
	//======NUMBER OF BTAGS
	systematics.CalculateNumberOfbTags("jetRescentral","jetRescentral_good_clean");
	systematics.CalculateNumberOfbTags("jetResup","jetResup_good_clean");
	systematics.CalculateNumberOfbTags("jetResdown","jetResdown_good_clean");


	//CHANGE INDUCED IN MET
	double newMET=systematics.GetsysMETVector("jetResup")->Pt();
	METChange->Fill((newMET-MET)/MET,EventWeight);
	//RESOLUTION IN MET
	newMETRes->Fill((newMET-TrueMET)/TrueMET,EventWeight);

	//IN BINS OF HT
	//LorentzM& TrueME = tree->Get(&TrueME,"genmetP4True");
	int METbin=-1.0;
	int METbin2=-1.0;
	if (TrueMET < 50){
	  METbin=0;
	}else if(TrueMET<100){
	  METbin=1;
	}else{
	  METbin=2;
	}
	
	if(TrueMET<100){
	  METbin2=3;
	}else {
	  METbin2=4;
	}
	METRes_METBins.at(METbin)->Fill((newMET-TrueMET)/TrueMET,EventWeight);//
	METRes_METBins.at(METbin2)->Fill((newMET-TrueMET)/TrueMET,EventWeight);
	METChange_METBins.at(METbin)->Fill((newMET-MET)/MET,EventWeight);


	//Resolution of the matched jets:
	for(int ijet=0;ijet<(int)AllJets.size();++ijet){
	  int ptbin=-1;
	  int etabin=-1;
	  double ptGen=0;
	  double ptJet=0;
	  double etaJet=0;

	  if(AllJets.at(ijet)->IsMatch()){

	    ptGen=AllJets.at(ijet)->GetPartner()->Pt();
	    ptJet=AllJets.at(ijet)->Pt();
	    if(ptJet<30){ptbin =0;}
	    else if(ptJet<50){ptbin =1;}
	    else if(ptJet<80){ptbin =2;}
	    else if(ptJet<150){ptbin =3;}
	    else {ptbin =4;}
	    JetResPtBins_Matched.at(ptbin)->Fill((ptJet-ptGen)/ptGen,EventWeight);
	    
	    etaJet=AllJets.at(ijet)->Eta();
	    if(fabs(etaJet)<0.5){etabin =0;}
	    else if(fabs(etaJet)<1.1){etabin =1;}
	    else if(fabs(etaJet)<1.7){etabin =2;}
	    else if(fabs(etaJet)<2.3){etabin =3;}
	    else {etabin =4;}
	    JetResEtaBins_Matched.at(etabin)->Fill((ptJet-ptGen)/ptGen,EventWeight);

	  }

	}

	//RESOLUTION AS A FUNCTION OF NUMBER OF MATCHED JETS
	int N_matchedJets=0;
	for(int ijet=0;ijet<(int)AllJets.size();++ijet){
	  if(AllJets.at(ijet)->IsMatch()){
	    N_matchedJets++;
	  }
	}
	int njetsbin=(int)AllJets.size();
	int nmatchedjetsbin=N_matchedJets;

	if((int)AllJets.size()==0){
	  njetsbin=0;
	}else if((int)AllJets.size()==1 || (int)AllJets.size()==2){ 
	  njetsbin=1;
	}else if((int)AllJets.size()==3 || (int)AllJets.size()==4){
	  njetsbin=2;
	}else if((int)AllJets.size()==5 || (int)AllJets.size()==6){
	  njetsbin=3;	
	}else{
	  njetsbin=4;
	}

	if(N_matchedJets==0){
	  nmatchedjetsbin=0;
	}else if(N_matchedJets==1 || N_matchedJets==2){ 
	  nmatchedjetsbin=1;
	}else if(N_matchedJets==3 || N_matchedJets==4){
	  nmatchedjetsbin=2;
	}else if(N_matchedJets==5 || N_matchedJets==6){
	  nmatchedjetsbin=3;	
	}else{
	  nmatchedjetsbin=4;
	}


	if(newMET>100.){
	  METRes_NMatchedJets_Bins.at(nmatchedjetsbin)->Fill((newMET-TrueMET)/TrueMET,EventWeight);
	  METChange_NMatchedJets_Bins.at(nmatchedjetsbin)->Fill((newMET-MET)/MET,EventWeight);
	  METRes_NJets_Bins.at(njetsbin)->Fill((newMET-TrueMET)/TrueMET,EventWeight);
	  METRes_NMatchedJets_NJets.at(njetsbin*5+nmatchedjetsbin)->Fill((newMET-TrueMET)/TrueMET,EventWeight);
	  METChange_NMatchedJets_NJets.at(njetsbin*5+nmatchedjetsbin)->Fill((newMET-MET)/MET,EventWeight);
	}
	METRes_vs_MET->Fill(MET,(MET-TrueMET)/TrueMET,EventWeight);
      }
    }


    //=====================================================================   
    //=====================================================================   
    //=====================================================================   
    //=====================================================================   
    

    //                     C  U  T      F L O W                           //


    //=====================================================================   
    //=====================================================================   
    //=====================================================================   
    //=====================================================================   



    if(DoControlPlots)ControlPlots.MakePlots("Before_CutFlow", SignalMuons, TightElectrons, CleanedJets, PFmet); 


    //====================================================================
    // TRIGGERS
    //====================================================================
    if(!turntriggersoff ){
      OK = triggers_RA4b(tree, triggernames,EventWeight);
      //OK=true;
      //================================================
      if(pcp)cout<<"check point triggers called"<<endl;
      //
      if(i==0 && isquick){ OK=OK&&OKold; OKold=OK;}
      //cout<<"trigger is "<<OK<<endl;
      if( !globalFlow.keepIf("triggers", OK )  && quick ) continue;    
      EW_AfterTrigger->Fill(EventWeight);
      //
      if(DoControlPlots && OK)ControlPlots.MakePlots("Triggers", SignalMuons, TightElectrons, CleanedJets, PFmet); 
      //treeCuts["Triggers"]=OK;
    }
    //====================================================================



    //====================================================================
    //bool  hcalLaserEventFilterFlag   = tree->Get( hcalLaserEventFilterFlag,    "hcalLaserEventFilterFlag"  );
    bool hcalLaserEventFilterFlag = true;
    if (isData){ hcalLaserEventFilterFlag   = tree->Get( hcalLaserEventFilterFlag,    "hcalLaser2012EventFilterFlag"  );
      OK= hcalLaserEventFilterFlag;
      if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
      if( !globalFlow.keepIf( "hcalLaserFilter"        , OK     ) && quick ) continue;
    }
    //====================================================================


    //====================================================================
    bool eeBadSCPassed = tree->Get( eeBadSCPassed, "eeBadScFilterFlag"   );
    OK=eeBadSCPassed;
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if( !globalFlow.keepIf("eeBadSCFilter"     , OK) && quick ) continue;
    //====================================================================


    bool  ecalLaserCorrFilterFlag = tree->Get( ecalLaserCorrFilterFlag, "ecalLaserCorrFilterFlag");
    OK=ecalLaserCorrFilterFlag;
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if( !globalFlow.keepIf("ecalLaserCorrFilterFlag"   , OK) && quick ) continue;



    //====================================================================
    // SCRAPING VETO
    //====================================================================
    unsigned int nTracksAll=tree->Get(nTracksAll,"nTracksAll");
    unsigned int nTracksHighPurity=tree->Get(nTracksHighPurity,"nTracksHighPurity");

    OK=true;
    if(pcp)cout<<"check point trackpurity"<<endl;
    if(nTracksAll>10){
      OK=false;
      if(double(nTracksHighPurity)/nTracksAll > 0.25)OK=true;
    }

    if(i==0 && isquick){ OK=OK&&OKold; OKold=OK;}
    if(  !globalFlow.keepIf("Scraping_Veto", OK ) && quick ) continue;
    if(pcp)cout<<"pure tracks passed"<<endl;
    //if(DoControlPlots && OK)ControlPlots.MakePlots("Scraping_Veto", SignalMuons, TightElectrons, CleanedJets, PFmet); 
    //====================================================================    




    //====================================================================    
    // check vertices
    //====================================================================
    //At least one good vertex
    //good vertex=not fake,|d|<24.  
    vector<int> goodVert;
    if(pcp)cout<<"check point calling vertex"<<endl;     
    OK = vertices_RA4b(tree,goodVert);
    if(i==0 && isquick){ OK=OK&&OKold; OKold=OK;}
    if(  !globalFlow.keepIf("PV", OK)    && quick ) continue;
    if(pcp)cout<<"check point  vertex called"<<endl;
    //if(DoControlPlots && OK)ControlPlots.MakePlots("PV", SignalMuons, TightElectrons, CleanedJets, PFmet); 
    //====================================================================







    //====================================================================    
    // check event quality
    //====================================================================    
    //it only checks hbheNoiseFilterResult
    OK = evtqual_RA4b(tree);
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(pcp)cout<<"check point calling event quality"<<endl;
    if( !globalFlow.keepIf("HBHE", OK)          && quick ) continue;
    if(pcp)cout<<"noise passed"<<endl;
    //if(DoControlPlots && OK)ControlPlots.MakePlots("HBHE", SignalMuons, TightElectrons, CleanedJets, PFmet); 
    //====================================================================





    //====================================================================    
    // check CSCHALO
    //====================================================================    
    OK = cschalo_RA4b(tree);
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if( !globalFlow.keepIf("CSC_HALO", OK)          && quick ) continue;
    //if(DoControlPlots && OK)ControlPlots.MakePlots("CSC_HALO", SignalMuons, TightElectrons, CleanedJets, PFmet); 
    //====================================================================






    //====================================================================    
    // check trackingFailure
    //====================================================================    
    OK = trackingFailure_RA4b(tree);
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if( !globalFlow.keepIf("trackingFailure", OK)          && quick ) continue;
    //if(DoControlPlots && OK)ControlPlots.MakePlots("trackingFailure", SignalMuons, TightElectrons, CleanedJets, PFmet); 
    //====================================================================




    //====================================================================    
    // check ECAL_TP
    //====================================================================    
    //as of CMSSW427_patch7, RA2TPfilter_cff.py and EcalDeadCellEventFilter_cfi do the 
    //same.
    bool ECAL_TP = tree->Get(ECAL_TP,"ecalDeadCellTPFilterFlag");
    OK=ECAL_TP;
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if( !globalFlow.keepIf("ECAL_TP", OK)          && quick ) continue;
    //if(DoControlPlots && OK)ControlPlots.MakePlots("ECAL_TP", SignalMuons, TightElectrons, CleanedJets, PFmet); 
    //====================================================================
    //
    //    treeCuts["Cleaning"]=globalFlow.applyCuts("ECAL_TP trackingFailure CSC_HALO HBHE PV Scraping_Veto");

    //====================================================================    
    // check Mu BEfilter
    //====================================================================    
    //     bool ECAL_TP = tree->Get(ECAL_TP,"ecaldeadcellfilterflag");
    //     OK=ECAL_TP;
    //     if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    //     if( !globalFlow.keepIf("ECAL_TP", OK)          && quick ) continue;
    //====================================================================


    //===================================================================
    //MANY STRIPS
    //==================================================================
    bool manystripclus53XFilterFlag=false;
    bool toomanystripclus53XFilterFlag=false;
    bool logErrorTooManyClustersFilterFlag=false;

    if(isData){
      manystripclus53XFilterFlag=tree->Get(manystripclus53XFilterFlag,"manystripclus53XFilterFlag");
      toomanystripclus53XFilterFlag=tree->Get(toomanystripclus53XFilterFlag,"toomanystripclus53XFilterFlag");
      logErrorTooManyClustersFilterFlag=tree->Get(logErrorTooManyClustersFilterFlag,"logErrorTooManyClustersFilterFlag");
    }
    //cout<<"manystripclus53XFilterFlag "<<manystripclus53XFilterFlag<<endl;
    //cout<<"toomanystripclus53XFilterFlag "<<toomanystripclus53XFilterFlag<<endl;
    //cout<<"logErrorTooManyClustersFilterFlag "<<logErrorTooManyClustersFilterFlag<<endl;


    OK=!manystripclus53XFilterFlag && !toomanystripclus53XFilterFlag && !logErrorTooManyClustersFilterFlag;
    //OK=true;
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("manyStrips",OK) && quick)continue;


    //===================================================================
    //MANY RHO
    //==================================================================
    float rho=0;
    if(isData){ rho=tree->Get(rho, "rho");}
    float rhoMAX=config.getFloat( "Rho_Max", 40.);
    OK=rho<rhoMAX;
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("rho",OK) && quick)continue;    



    //===================================================================
    //DELTA PHI
    //==================================================================
    double deltaphi=0;
    if (isData){
      LorentzM& corMetGlobalMuonsP4Calo = tree->Get(&corMetGlobalMuonsP4Calo, "corMetGlobalMuonsP4Calo");
      deltaphi = DeltaPhi(corMetGlobalMuonsP4Calo,PFmet);
    }


    float deltaphi_MAX = config.getFloat("deltaphi_MAX",1.5);
    OK=deltaphi<deltaphi_MAX;
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("dphi_calo-MET",OK) && quick)continue;    
    //==================================================================


    /*
    if (CleanedJets.size()>12){
      vector<LorentzM>&  Jets_p4 = tree->Get(&Jets_p4, "ak5JetPFCorrectedP4Pat");
      cout<<"here the multiplicity is "<<CleanedJets.size()<<endl;
      cout<<"the original collection size is "<<Jets_p4.size()<<endl;
      cout<<"looking at all jets  now"<<endl;
      cout<<"Event is "<<Event<<endl;
      cout<<"lumiSection is "<<lumiSection<<endl;
      cout<<"Run is "<<Run<<endl;
      for (int kjet=0; kjet<AllJets.size();++kjet){
	cout<<" jet "<<kjet<<" has a pt of "<<AllJets.at(kjet)->Pt()<<endl;
      }
    }
    */

















   

    //===============================================
    //SINGLE LEPTON
    //===============================================
    if(pcp){
      cout<<"the sum of the sizes of leptons is "<<(int)SignalElectrons.size()+(int)SignalMuons.size()<<endl;
      cout<<SignalElectrons.size()<<" "<<SignalMuons.size()<<endl;
    }


    OK=SetOfCuts::Leptons.NUM.Examine((int)SignalElectrons.size()+(int)SignalMuons.size());
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    //cout<<"the size of ALLJets is -1 "<<AllJets.size()<<endl;       
    if(!globalFlow.keepIf("One_single_lepton",OK) && quick) continue;    




    Particle* theLepton;
    if(SetOfCuts::Leptons.NUM.GiveMeCut()==1 && SetOfCuts::Leptons.NUM.GiveMeRelation()=="equal"){
      if(globalFlow.DidItPass("One_single_lepton")){
	if (SignalElectrons.size()==1){
	  theLepton=SignalElectrons.at(0);
	}else if(SignalMuons.size()==1){
	  theLepton=SignalMuons.at(0);
	}
      }
    }



    




    //number of tags & tag w.
    //NumberOfbTags = tag.JetLoop(CleanedJets); //cout << "number of btags " << NumberOfbTags << endl; // loop over jets to get tag w., return number of btags

    //===================================
    //JETS
    //===================================
    //
    //globalFlow.keepIf(">=1 jet",CleanedJets.size()>=1);
    //globalFlow.keepIf(">=2 jet",CleanedJets.size()>=2);
    //globalFlow.keepIf("3>jets",CleanedJets.size()>=3);
    //globalFlow.keepIf("3=>jets and 1=>btags",NumberOfbTags>=1);
    //globalFlow.keepIf(">=4 jet",CleanedJets.size()>=4);
    //globalFlow.keepIf(">=4 and 1=> btagsjeCt",CleanedJets.size()>=4 && NumberOfbTags>=1);
    OK=SetOfCuts::Jets.NUM.Examine(CleanedJets.size());
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("Jet_Cuts",OK) && quick) continue;    
    //    events1<<Event<<endl;

    if(DoControlPlots && OK)ControlPlots.MakePlots("Jet_Cuts", SignalMuons, SignalElectrons, CleanedJets, PFmet); 
    //treeCuts["Jet_Cuts"]=OK;



    //==========JET SYSTEMATICS===============
    if (systematics.IsEnabled()){      
      if(systematics.GetSysMap()["jetup"]){
	OKsyst=SetOfCuts::Jets.NUM.Examine((int)systematics.GetsysJet("jetup_good_clean").size());
	globalFlow.keepIf("Jet_Cuts_jetup",OKsyst);
      }
      if(systematics.GetSysMap()["jetdown"]){
	OKsyst=SetOfCuts::Jets.NUM.Examine((int)systematics.GetsysJet("jetdown_good_clean").size());
	globalFlow.keepIf("Jet_Cuts_jetdown",OKsyst);
      }
      if(systematics.GetSysMap()["jetRescentral"]){
	OKsyst=SetOfCuts::Jets.NUM.Examine((int)systematics.GetsysJet("jetRescentral_good_clean").size());
	globalFlow.keepIf("Jet_Cuts_jetRescentral",OKsyst);
      }

      /*
      if(OK){
	cout<<"normal requirement OK"<<endl;
	cout<<"old jet collection "<<endl;
	for (int ijet=0;ijet<(int)CleanedJets.size();++ijet){
	  cout<<"old pt of jet "<<ijet<<" = "<<CleanedJets.at(ijet)->Pt()<<endl;
	}
	cout<<endl;
	cout<<" OKsyst was "<<OKsyst<<endl;
	cout<<"new jet collection "<<endl;
	for(int ijet=0;ijet<(int)systematics.GetsysJet("jetRescentral_good_clean").size();++ijet){
	  cout<<"new pt of jet "<<ijet<<" = "<<systematics.GetsysJet("jetRescentral_good_clean").at(ijet)->Pt()<<endl;
	}
	cout<<endl;
	cout<<endl;
	cout<<endl;

      }
      */

      if(systematics.GetSysMap()["jetResup"]){
	OKsyst=SetOfCuts::Jets.NUM.Examine((int)systematics.GetsysJet("jetResup_good_clean").size());
	globalFlow.keepIf("Jet_Cuts_jetResup",OKsyst);
      }
      if(systematics.GetSysMap()["jetResdown"]){
	OKsyst=SetOfCuts::Jets.NUM.Examine((int)systematics.GetsysJet("jetResdown_good_clean").size());
	globalFlow.keepIf("Jet_Cuts_jetResdown",OKsyst);
      }
    
    }
    //===================================

    



    //===================================
    //NUMBER OF BTAGS
    //===================================
    OK=SetOfCuts::Event.NumberOfBtags.Examine(NumberOfbTags);
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("NBtags",OK) && quick) continue;    
    if(DoControlPlots && OK)ControlPlots.MakePlots("NBtags", SignalMuons, SignalElectrons, CleanedJets, PFmet);     

    //    cout<<"found NumberOfbTags = "<<NumberOfbTags<<endl;
    //==========JET SYSTEMATICS===============
    if (systematics.IsEnabled()){      
      if(systematics.GetSysMap()["jetup"]){
	OKsyst=SetOfCuts::Event.NumberOfBtags.Examine(systematics.NumberOfbTags("jetup"));
	globalFlow.keepIf("NBtags_jetup",OKsyst);
      }
      if(systematics.GetSysMap()["jetdown"]){
	OKsyst=SetOfCuts::Event.NumberOfBtags.Examine(systematics.NumberOfbTags("jetdown"));
	globalFlow.keepIf("NBtags_jetdown",OKsyst);
      }
      if(systematics.GetSysMap()["jetRescentral"]){
	OKsyst=SetOfCuts::Event.NumberOfBtags.Examine(systematics.NumberOfbTags("jetupRescentral"));
	globalFlow.keepIf("NBtags_jetRescentral",OKsyst);
      }
      if(systematics.GetSysMap()["jetResup"]){
	OKsyst=SetOfCuts::Event.NumberOfBtags.Examine(systematics.NumberOfbTags("jetResup"));
	globalFlow.keepIf("NBtags_jetResup",OKsyst);
      }
      if(systematics.GetSysMap()["jetResdown"]){
	OKsyst=SetOfCuts::Event.NumberOfBtags.Examine(systematics.NumberOfbTags("jetResdown"));
	globalFlow.keepIf("NBtags_jetResdown",OKsyst);
      }
    
    }
    //===================================








    //=================================
    //MUONS
    //=================================
    //
    //
    //cout<<"sizes"<<endl;
    //cout<<SignalMuons.size()<<" "<<SignalElectrons.size()<<" "<<WideMuons.size()<<" "<<WideElectrons.size()<<endl;
    //cout<<endl;
    OK=SetOfCuts::SignalMuons.NUM.Examine(SignalMuons.size());
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("Signal_Muons",OK) && quick) continue;
    if(DoControlPlots && OK)ControlPlots.MakePlots("Signal_Muons", SignalMuons, TightElectrons, CleanedJets, PFmet); 
    //
    //
    //
    //
    OK=SetOfCuts::SignalElectrons.NUM.Examine(SignalElectrons.size());
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("Signal_Electrons",OK) && quick) continue;
    if(DoControlPlots && OK)ControlPlots.MakePlots("Signal_Electrons", SignalMuons, SignalElectrons, CleanedJets, PFmet); 
    //
    //
    //
    //
    //
    OK=SetOfCuts::WideMuons.NUM.Examine(WideMuons.size());
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("Wide_Muons",OK) && quick) continue;    
    if(DoControlPlots && OK)ControlPlots.MakePlots("Wide_Muons", WideMuons, TightElectrons, CleanedJets, PFmet); 
    //    treeCuts["Wide_Muons"]=OK;
    //
    //
    //
    //
    OK=SetOfCuts::WideElectrons.NUM.Examine(WideElectrons.size());
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("Wide_Electrons",OK) && quick) continue;    
    if(DoControlPlots && OK)ControlPlots.MakePlots("Wide_Electrons", SignalMuons, WideElectrons, CleanedJets, PFmet); 
    //treeCuts["Wide_Electrons"]=OK;
    //
    //===================================



    //========================================
    //=======TRACKS
    //========================================
    tracks = makeAllTracks( tree);
    if(globalFlow.DidItPass("One_single_lepton")){
      OK=IsoTrackVetoV4( theLepton, tracks);
    }else{
      OK=false;
    }
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("IsoTracks",OK) && quick) continue;    
    if(DoControlPlots && OK)ControlPlots.MakePlots("IsoTracks", SignalMuons, WideElectrons, CleanedJets, PFmet); 
    //========================================



    //========================================
    //=======TAUS
    //========================================
    vector<Tau*> vetoTaus; 
    if(globalFlow.DidItPass("One_single_lepton")){
      makeVetoTaus( tree, taus, vetoTaus, theLepton);
      OK = (vetoTaus.size() == 0);
    }else{
      OK=false;
    }
    if(i==0 && isquick){OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("TauVeto",OK) && quick) continue;    
    if(DoControlPlots && OK)ControlPlots.MakePlots("TauVeto", SignalMuons, WideElectrons, CleanedJets, PFmet); 









    //====================================================================
    //MZ AND RESONANT LEPTONS
    //====================================================================
    //this never returns false, it merely updates the values of
    //isZ and isResonant for the selectedLeptons

    bool doMzandResonance=false;
    if (doMzandResonance){
      if(pcp)cout<<"check xpoint calling mzVeto"<<endl;
      //=======================================================
      //      OK=mzveto_RA4b(tree,Leptons);
      //=======================================================
      if(pcp)cout<<"check point after mzVeto"<<endl;


      if(pcp)cout<<"check point calling resonances"<<endl;
      //=======================================================
      //OK=resonance_RA4b(tree,Leptons);
      //=======================================================
      if(pcp)cout<<"check point after resonances"<<endl;
    }
    //======================================================





    //===================================================================
    //HT
    //===================================================================
    HT=0.0;
    for(int ijet=0;ijet<(int)CleanedJets.size();++ijet){
      HT=HT+CleanedJets.at(ijet)->Pt();	
    }
    
    OK=SetOfCuts::Event.HT.Examine(HT);
    if(i==0 && isquick){ OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("HT", OK ) && quick ) continue;
    if(DoControlPlots && OK)ControlPlots.MakePlots("HT", SignalMuons, SignalElectrons, CleanedJets, PFmet); 


    //cout<<"the size of ALLJets is 2 "<<AllJets.size()<<endl;   

    if (systematics.IsEnabled()){
      if (systematics.GetSysMap()["jetup"]){
	//cout<<"testing HT = "<<systematics.GetsysHT("jetup")<<endl;
	OKsyst=SetOfCuts::Event.HT.Examine(systematics.GetsysHT("jetup"));
	//cout<<"pointer to the flow = "<<systematics.GetsysFlow("jetup")<<endl;
	globalFlow.keepIf("HT_jetup",OKsyst);
      }
      if (systematics.GetSysMap()["jetdown"]){
	OKsyst=SetOfCuts::Event.HT.Examine(systematics.GetsysHT("jetdown"));
	globalFlow.keepIf("HT_jetdown",OKsyst);
      }
      if (systematics.GetSysMap()["jetRescentral"]){
	OKsyst=SetOfCuts::Event.HT.Examine(systematics.GetsysHT("jetRescentral"));
	globalFlow.keepIf("HT_jetRescentral",OKsyst);
      }
      if (systematics.GetSysMap()["jetResup"]){
	OKsyst=SetOfCuts::Event.HT.Examine(systematics.GetsysHT("jetResup"));
	globalFlow.keepIf("HT_jetResup",OKsyst);
      }
      if (systematics.GetSysMap()["jetResdown"]){
	OKsyst=SetOfCuts::Event.HT.Examine(systematics.GetsysHT("jetResdown"));
	globalFlow.keepIf("HT_jetResdown",OKsyst);
      }
    }
    //===================================================================







    //cout<<"the size of ALLJets is 3 "<<AllJets.size()<<endl;   

    //====================================================================
    // CHECK MET 
    //====================================================================
    OK = metAndHT_RA4b(tree);
    if(pcp)cout<<"check point MET"<<endl;
    if(i==0 && isquick){ OK=OK&&OKold; OKold=OK;}
    if(  !globalFlow.keepIf("MET", OK ) && quick ) continue;
    if(pcp)cout<<"check point out of MET_HT"<<endl;
    if(DoControlPlots && OK)ControlPlots.MakePlots("MET", SignalMuons, SignalElectrons, CleanedJets, PFmet); 

    //SYSTEMATICS- CUT ON MET    
    if (systematics.IsEnabled()){
      typedef map<string, bool>::iterator iter;
      for(iter it=systematics.GetSysMap().begin(); it!=systematics.GetSysMap().end();++it){
	OKsyst=false;
	if(it->second){ //--->this "if" is redundant in principle//
	  OKsyst=SetOfCuts::Event.MET.Examine(systematics.GetsysMET(it->first));
	  globalFlow.keepIf("MET_"+it->first, OKsyst);
	}
      }
    }

    /*
    if (CleanedJets.size()>12){
      vector<LorentzM>&  Jets_p4 = tree->Get(&Jets_p4, "ak5JetPFCorrectedP4Pat");
      cout<<"here the multiplicity is "<<CleanedJets.size()<<endl;
      cout<<"the original collection size is "<<Jets_p4.size()<<endl;
      cout<<"looking at all jets  now"<<endl;
      cout<<"Event is "<<Event<<endl;
      cout<<"lumiSection is "<<lumiSection<<endl;
      cout<<"Run is "<<Run<<endl;
      for (int kjet=0; kjet<AllJets.size();++kjet){
	cout<<" jet "<<kjet<<" has a pt of "<<AllJets.at(kjet)->Pt()<<endl;
      }
    }
    */
    //====================================================================






    //===================================================================
    //CHECK MT2
    //===================================================================
    //Particle* theLepton;
    double MT2=0;
    if(SetOfCuts::Leptons.NUM.GiveMeCut()==1 && SetOfCuts::Leptons.NUM.GiveMeRelation()=="equal"){
      if(globalFlow.DidItPass("One_single_lepton")){

	double pt1=theLepton->Pt();
	double pt2=PFmet.pt();
	//MT2=sqrt( 2*pt1*pt2 * (1 - cos(DeltaPhi(theLepton->P4(), PFmet))));
	MT2=sqrt( 2*pt1*pt2 * (1 - cos(DeltaPhi(theLepton->P4(), PFmet))));

      }
    }
    //
    //
    //
    OK=SetOfCuts::Event.MT2.Examine(MT2);
    if(pcp)cout<<"check point MT2"<<endl;
    if(i==0 && isquick){ OK=OK&&OKold; OKold=OK;}
    if(!globalFlow.keepIf("MT2", OK ) && quick ) continue;
    if(DoControlPlots && OK)ControlPlots.MakePlots("MT2", SignalMuons, SignalElectrons, CleanedJets, PFmet); 

    /*
    cout<<endl;

    cout<<"the event is here "<<Event<<endl;
    cout<<"and MTMu is "<<MT2<<endl;
    cout<<"filling with a weight of "<<EventWeight<<endl;
    //
    cout<<"the ingredients are "<<endl;
    cout<<"muon pt "<<theLepton->Pt()<<endl;
    cout<<"the met "<<PFmet.pt()<<endl;
    cout<<"the angle "<<cos(DeltaPhi(theLepton->P4(), PFmet))<<endl;
    cout<<endl;
    */
    MTMu->Fill(MT2,EventWeight);

    //===================================================================





















    
    //-- Selection and tree-output for the triggerStudy: Mu
    if(mySampleInfo.GetEstimation()=="TrigStudy-mu" && OK){
            if(TightElectrons.size() > 0) cout<<"TightElectrons.size() = " << TightElectrons.size() << endl;
      bool foundDiLepton=false;
      //cut: check if there is an OS muon pair with 60 < Minv < 120
      if(SignalMuons.size()<2) foundDiLepton=false;
      else{
	for(Int_t i=0,N=SignalMuons.size();i<N-1; ++i){
	  for(Int_t j=i+1; j<N; ++j){
	    if(SignalMuons.at(i)->Charge() == SignalMuons.at(j)->Charge()) continue;
	    double diLepMass = (SignalMuons.at(i)->P4()+SignalMuons.at(j)->P4()).M();
	    if(diLepMass > 60. && diLepMass < 120.) foundDiLepton=true;
	  }
	  if(foundDiLepton) break;
	}
      }
      OK=foundDiLepton;

      vector<string> ElMatchedTriggerFilter; //no electron matching info is needed for the TrigStudy-mu selection
      map<string,int> HLTprescaled          = tree->Get(&HLTprescaled, "prescaled");
      map<string,bool> HLTtrigger           = tree->Get(&HLTtrigger, "triggered");
      vector<string> MuMatchedTriggerFilter = tree->Get(&MuMatchedTriggerFilter, "DESYtriggerMuMatchedTriggerFilter");
      map<string,string> TriggerMap         = tree->Get(&TriggerMap, "DESYtriggerNameMap");
      //--------------- keep only the trigger Info for the Tight Leptons 
      //(therefore, the trigger info of other leptons is not saved into the output tree)
      vector<string> keepMatchingInfoForTightMuon;
      for(Int_t i=0,N=SignalMuons.size(); i<N; ++i){
	Int_t indx = SignalMuons.at(i)->GetIndexInTree();
	if(indx>= MuMatchedTriggerFilter.size()){
	  cout<<"WARNING: Asked for Trigger matching info that does not exist!"<<endl;
	  cout<<"MuMatchedTriggerFilter.size() = " << MuMatchedTriggerFilter.size() << endl;
	  cout<<"Asking for element " << indx << endl;	  
	}
	else keepMatchingInfoForTightMuon.push_back(MuMatchedTriggerFilter.at(indx));
      }
      MuMatchedTriggerFilter = keepMatchingInfoForTightMuon;
      
      //small consistency check
      if(keepMatchingInfoForTightMuon.size() != SignalMuons.size()){
	cout<<"WARNING: Size of muon vector does not correspond to size of matching info vector!"<<endl;
	cout<<"SignalMuons.size()                   = " << SignalMuons.size() << endl;
	cout<<"keepMatchingInfoForTightMuon.size() = " << keepMatchingInfoForTightMuon.size() << endl;
      }
      if(TightElectrons.size()){
	cout<<"WARNING: Size of muon vector is not zero as it ought to be: "<< TightElectrons.size() << endl;
      }
      //cout<<"======================================"<<endl;
      //cout<<"Size: matchFilter | SignalMuons | keepFilters = " 
      //<< MuMatchedTriggerFilter.size() << " | "
      //<< SignalMuons.size() << " | "
      //<< keepMatchingInfoForTightMuon.size()
      //<<endl;

      //fill the leafs which will be saved in TrigStudyTree
      EventInfo myInfo;
      myInfo.HLTprescaled           = HLTprescaled;
      myInfo.HLTtrigger             = HLTtrigger;
      myInfo.ElMatchedTriggerFilter = ElMatchedTriggerFilter;
      myInfo.Event                  = Event;
      myInfo.EventWeight            = EventWeight;
      myInfo.MuMatchedTriggerFilter = MuMatchedTriggerFilter;
      myInfo.Run                    = Run;
      //if(!isData) myInfo.PUInter    = relevantNumPU;
      if(!isData) myInfo.PUInter    = goodVert.size();
      else        myInfo.PUInter    = goodVert.size();
      myInfo.PUWeight               = PUWeight;
      myInfo.TriggerMap             = TriggerMap;
      
      if(!globalFlow.keepIf("OS muon pair found", OK ) && quick ) continue;
//       cout<<"my info blabal"<<myInfo.ElMatchedTriggerFilter.size()<<endl;
      TrigStudyTree->Fill(&myInfo, tree, SignalMuons, TightElectrons, CleanedJets, PFmet);

      //cout<<"---------------------------------------------------------" <<endl;
      //       cout<<"ElMatchedTriggerFilter.size() = " << ElMatchedTriggerFilter.size() << endl;
      //       cout<<"MuMatchedTriggerFilter.size() = " << MuMatchedTriggerFilter.size() << endl;
      //       cout<<"HLTtrigger.size()             = " << HLTtrigger.size() << endl;
      //       cout<<"HLTprescaled.size()           = " << HLTprescaled.size() << endl;
      //       cout<<"TriggerMap.size()             = " << TriggerMap.size() << endl;
      //       for(map<string, bool>::iterator it=HLTtrigger.begin(); it != HLTtrigger.end(); ++it)    cout<<setw(80)<< it->first << " = " << it->second << endl;
      //       cout<<endl;
      //       for(map<string, int>::iterator it=HLTprescaled.begin(); it != HLTprescaled.end(); ++it) cout<<setw(80)<< it->first << " = " << it->second << endl;
      //       cout<<endl;
      //       for(map<string, string>::iterator it=TriggerMap.begin(); it != TriggerMap.end(); ++it)  cout<<setw(80)<< it->first << " = " << it->second << endl;
      //       cout<<endl;
    }
    
        //------------------------------------------------------------------- Selection and tree-output for the triggerStudy: El
    if(mySampleInfo.GetEstimation()=="TrigStudy-el" && OK){
      //cut: check if there is an OS electron pair with 60 < Minv < 120
      bool foundDiLepton=false;
      if(TightElectrons.size()<2) foundDiLepton=false;
      else{
	for(Int_t i=0,N=TightElectrons.size();i<N-1; ++i){
	  for(Int_t j=i+1; j<N; ++j){
	    if(TightElectrons.at(i)->Charge() == TightElectrons.at(j)->Charge()) continue;
	    double diLepMass = (TightElectrons.at(i)->P4()+TightElectrons.at(j)->P4()).M();
	    if(diLepMass > 60. && diLepMass < 120.) foundDiLepton=true;
	  }
	  if(foundDiLepton) break;
	}
      }
      OK=foundDiLepton;
      
      vector<string> ElMatchedTriggerFilter = tree->Get(&ElMatchedTriggerFilter, "DESYtriggerElMatchedTriggerFilter");
      map<string,int> HLTprescaled          = tree->Get(&HLTprescaled, "prescaled");
      map<string,bool> HLTtrigger           = tree->Get(&HLTtrigger, "triggered");
      vector<string> MuMatchedTriggerFilter; //no muon matching info is needed for the TrigStudy-el selection
      map<string,string> TriggerMap         = tree->Get(&TriggerMap, "DESYtriggerNameMap");
      
      //--------------------------- keep only the trigger Info for the Tight Leptons 
      //(therefore, the trigger info of other leptons is not saved into the output tree)
      vector<string> keepMatchingInfoForTightElectron;
      for(Int_t i=0,N=TightElectrons.size(); i<N; ++i){
	Int_t indx = TightElectrons.at(i)->GetIndexInTree();
	if(indx>= ElMatchedTriggerFilter.size()){
	  cout<<"WARNING: Asked for Trigger matching info that does not exist!"<<endl;
	  cout<<"ElMatchedTriggerFilter.size() = " << ElMatchedTriggerFilter.size() << endl;
	  cout<<"Asking for element " << indx << endl;	  
	}
	else keepMatchingInfoForTightElectron.push_back(ElMatchedTriggerFilter.at(indx));
      }
      ElMatchedTriggerFilter = keepMatchingInfoForTightElectron;

      //small consistency check
      if(keepMatchingInfoForTightElectron.size() != TightElectrons.size()){
	cout<<"WARNING: Size of electron vector does not correspond to size of matching info vector!"<<endl;
	cout<<"TightElectrons.size()                   = " << TightElectrons.size() << endl;
	cout<<"keepMatchingInfoForTightElectron.size() = " << keepMatchingInfoForTightElectron.size() << endl;
      }
      if(SignalMuons.size()){
	cout<<"WARNING: Size of muon vector is not zero as it ought to be: "<< SignalMuons.size() << endl;
      }
      //fill the leafs which will be saved in TrigStudyTree
      EventInfo myInfo;
      myInfo.HLTprescaled           = HLTprescaled;
      myInfo.HLTtrigger             = HLTtrigger;
      myInfo.ElMatchedTriggerFilter = ElMatchedTriggerFilter;
      myInfo.Event                  = Event;
      myInfo.EventWeight            = EventWeight;
      myInfo.MuMatchedTriggerFilter = MuMatchedTriggerFilter;
      //if(!isData) myInfo.PUInter    = relevantNumPU;
      if(!isData) myInfo.PUInter    = goodVert.size();
      else        myInfo.PUInter    = goodVert.size();
      myInfo.Run                    = Run;
      myInfo.PUWeight               = PUWeight;
      myInfo.TriggerMap             = TriggerMap;
      
      if(!globalFlow.keepIf("OS electron pair found", OK ) && quick ) continue;
      TrigStudyTree->Fill(&myInfo, tree, SignalMuons, TightElectrons, CleanedJets, PFmet);
      
      //cout<<"---------------------------------------------------------" <<endl;
      //       cout<<"ElMatchedTriggerFilter.size() = " << ElMatchedTriggerFilter.size() << endl;
      //       cout<<"MuMatchedTriggerFilter.size() = " << MuMatchedTriggerFilter.size() << endl;
      //       cout<<"HLTtrigger.size()             = " << HLTtrigger.size() << endl;
      //       cout<<"HLTprescaled.size()           = " << HLTprescaled.size() << endl;
      //       cout<<"TriggerMap.size()             = " << TriggerMap.size() << endl;
      //       for(map<string, bool>::iterator it=HLTtrigger.begin(); it != HLTtrigger.end(); ++it)    cout<<setw(80)<< it->first << " = " << it->second << endl;
      //       cout<<endl;
      //       for(map<string, int>::iterator it=HLTprescaled.begin(); it != HLTprescaled.end(); ++it) cout<<setw(80)<< it->first << " = " << it->second << endl;
      //       cout<<endl;
      //       for(map<string, string>::iterator it=TriggerMap.begin(); it != TriggerMap.end(); ++it)  cout<<setw(80)<< it->first << " = " << it->second << endl;
      //       cout<<endl;
    }












    
    //====================COPY AND APPLY THE CUTFLOWS
    vector<TString> globalFlowNames;
    //globalFlowNames.push_back("triggers");
    TString rawString;
    bool OKall=false;
    if(!isquick){
      globalFlowNames.push_back("hcalLaserFilter");
      globalFlowNames.push_back("eeBadSCFilter");
      globalFlowNames.push_back("ecalLaserCorrFilterFlag");
      globalFlowNames.push_back("Scraping_Veto");
      globalFlowNames.push_back("PV");
      globalFlowNames.push_back("HBHE");
      globalFlowNames.push_back("CSC_HALO");
      globalFlowNames.push_back("trackingFailure");
      globalFlowNames.push_back("ECAL_TP");
      globalFlowNames.push_back("manyStrips");
      globalFlowNames.push_back("rho");
      globalFlowNames.push_back("dphi_calo-MET");
      globalFlowNames.push_back("One_single_lepton");      
      globalFlowNames.push_back("Signal_Muons");
      globalFlowNames.push_back("Signal_Electrons");
      globalFlowNames.push_back("Wide_Muons");
      globalFlowNames.push_back("Wide_Electrons");
      globalFlowNames.push_back("IsoTracks");
      globalFlowNames.push_back("TauVeto");
      globalFlowNames.push_back("Jet_Cuts");
      globalFlowNames.push_back("NBtags");
      globalFlowNames.push_back("HT");
      globalFlowNames.push_back("MET");
      globalFlowNames.push_back("MT2");

      for(int iname=0;iname<(int)globalFlowNames.size();++iname){
	rawString+=globalFlowNames.at(iname)+" ";
      }
      
      OKall=globalFlow.applyCuts(rawString);
      //events1<<Event<<endl;
      if(pcp){
	cout<<"OKall is "<<OKall<<endl;
      }


      
      
      //METResolution AFTER THE CUTFLOW
      if(OKall){
	if (!isData){
	  LorentzM& TrueME = tree->Get(&TrueME,"genmetP4True");
	  double TrueMET=TrueME.Pt();
	  METRes_ACF->Fill((MET-TrueMET)/TrueMET,EventWeight);
	}
      }
      


      if(OKall){
	if (CleanedJets.size()>9){
	  vector<LorentzM>&  Jets_p4 = tree->Get(&Jets_p4, "ak5JetPFCorrectedP4Pat");
	  cout<<"here the multiplicity is "<<CleanedJets.size()<<endl;
	  cout<<"the original collection size is "<<Jets_p4.size()<<endl;
	  cout<<"looking at all jets  now"<<endl;
	  cout<<"Event is "<<Event<<endl;
	  cout<<"lumiSection is "<<lumiSection<<endl;
	  cout<<"Run is "<<Run<<endl;
	  for (int kjet=0; kjet<AllJets.size();++kjet){
	    cout<<" jet "<<kjet<<" has a pt of "<<AllJets.at(kjet)->Pt()<<endl;
	  }
	}
      }
      
      
    }
    
    
    
    if(systematics.IsEnabled()){
      
      typedef map<string,bool>::iterator map_it;
      for (map_it iter=systematics.GetSysMap().begin(); iter != systematics.GetSysMap().end(); iter++){
	if(iter->second){
	  if(iter->first=="jetup"){
	    
	    for (int iname=0;iname<(int)globalFlowNames.size();++iname){
	      //COPY THE CUTFLOW
	      if (globalFlowNames.at(iname) == "Jet_Cuts"){
		OK=globalFlow.DidItPass("Jet_Cuts_jetup");
	      }
	      else if (globalFlowNames.at(iname) == "NBtags"){
		OK=globalFlow.DidItPass("NBtags_jetup");
	      }
	      else if (globalFlowNames.at(iname) == "HT"){
		OK=globalFlow.DidItPass("HT_jetup");
	      }
	      else if (globalFlowNames.at(iname) == "MET"){
		OK=globalFlow.DidItPass("MET_jetup");
		//cout<<"did it pass "<<OK<<endl;
	      }
	      else{
		OK=globalFlow.DidItPass(globalFlowNames.at(iname));
	      }
	      //======copy the global flow
	      systematics.GetsysFlow("jetup")->keepIf(globalFlowNames.at(iname), OK);
	      
	    }//iname
	    //cout<<"the jet up cutflow "<<endl;
		
	  }
	  else if(iter->first=="jetdown"){
	    
	    for (int iname=0;iname<(int)globalFlowNames.size();++iname){
	      //COPY THE CUTFLOW
	      if (globalFlowNames.at(iname) == "Jet_Cuts"){
		OK=globalFlow.DidItPass("Jet_Cuts_jetdown");
	      }
	      else if (globalFlowNames.at(iname) == "NBtags"){
		OK=globalFlow.DidItPass("NBtags_jetdown");
	      }
	      else if (globalFlowNames.at(iname) == "HT"){
		OK=globalFlow.DidItPass("HT_jetdown");
	      }
	      else if (globalFlowNames.at(iname) == "MET"){
		OK=globalFlow.DidItPass("MET_jetdown");
	      }
	      else{
		OK=globalFlow.DidItPass(globalFlowNames.at(iname));
	      }
	      //======copy the global flow
	      systematics.GetsysFlow("jetdown")->keepIf(globalFlowNames.at(iname), OK);
	    }
	  }
	  else if(iter->first=="clustersup"){
	    for (int iname=0;iname<(int)globalFlowNames.size();++iname){
	      //COPY THE CUTFLOW
	      if (globalFlowNames.at(iname) == "MET"){
		OK=globalFlow.DidItPass("MET_clustersup");
	      }
	      else{
		OK=globalFlow.DidItPass(globalFlowNames.at(iname));		
	      }
	      systematics.GetsysFlow("clustersup")->keepIf(globalFlowNames.at(iname), OK);
	    }
	  }
	  else if(iter->first=="clustersdown"){
	    for (int iname=0;iname<(int)globalFlowNames.size();++iname){
	      //COPY THE CUTFLOW
	      if (globalFlowNames.at(iname) == "MET"){
		OK=globalFlow.DidItPass("MET_clustersdown");
	      }
	      else{
		OK=globalFlow.DidItPass(globalFlowNames.at(iname));		
	      }
	      //======copy the global flow
	      systematics.GetsysFlow("clustersdown")->keepIf(globalFlowNames.at(iname), OK);
	    }
	  }
	  else if(iter->first=="jetRescentral"){
	    for (int iname=0;iname<(int)globalFlowNames.size();++iname){
	      //COPY THE CUTFLOW
	      if (globalFlowNames.at(iname) == "Jet_Cuts"){
		OK=globalFlow.DidItPass("Jet_Cuts_jetRescentral");
	      }
	      else if (globalFlowNames.at(iname) == "NBtags"){
		OK=globalFlow.DidItPass("NBtags_jetRescentral");
	      }
	      else if (globalFlowNames.at(iname) == "HT"){
		OK=globalFlow.DidItPass("HT_jetRescentral");
	      }
	      else if (globalFlowNames.at(iname) == "MET"){
		OK=globalFlow.DidItPass("MET_jetRescentral");
	      }
	      else{
		OK=globalFlow.DidItPass(globalFlowNames.at(iname));
	      }
	      //======copy the global flow
	      systematics.GetsysFlow("jetRescentral")->keepIf(globalFlowNames.at(iname), OK);
	    }
	  }
	  else if(iter->first=="jetResup"){
	    for (int iname=0;iname<(int)globalFlowNames.size();++iname){
	      //COPY THE CUTFLOW
	      if (globalFlowNames.at(iname) == "Jet_Cuts"){
		OK=globalFlow.DidItPass("Jet_Cuts_jetResup");
	      }
	      else if (globalFlowNames.at(iname) == "NBtags"){
		OK=globalFlow.DidItPass("NBtags_jetResup");
	      }
	      else if (globalFlowNames.at(iname) == "HT"){
		OK=globalFlow.DidItPass("HT_jetResup");
	      }
	      else if (globalFlowNames.at(iname) == "MET"){
		OK=globalFlow.DidItPass("MET_jetResup");
	      }
	      else{
		OK=globalFlow.DidItPass(globalFlowNames.at(iname));
	      }
	      //======copy the global flow
	      systematics.GetsysFlow("jetResup")->keepIf(globalFlowNames.at(iname), OK);
	    }
	  }
	  else if(iter->first=="jetResdown"){
	    for (int iname=0;iname<(int)globalFlowNames.size();++iname){
	      //COPY THE CUTFLOW
	      if (globalFlowNames.at(iname) == "Jet_Cuts"){
		OK=globalFlow.DidItPass("Jet_Cuts_jetResdown");
	      }
	      else if (globalFlowNames.at(iname) == "NBtags"){
		OK=globalFlow.DidItPass("NBtags_jetResdown");
	      }
	      else if (globalFlowNames.at(iname) == "HT"){
		OK=globalFlow.DidItPass("HT_jetResdown");
	      }
	      else if (globalFlowNames.at(iname) == "MET"){
		OK=globalFlow.DidItPass("MET_jetResdown");
	      }
	      else{
		OK=globalFlow.DidItPass(globalFlowNames.at(iname));
	      }
	      //======copy the global flow
	      systematics.GetsysFlow("jetResdown")->keepIf(globalFlowNames.at(iname), OK);
	    }
	  }
	

	  //======APPLY THE CUTFLOW
	  //cout<<"applying the cutflow to"<<iter->first<<endl;
	  OK=systematics.GetsysFlow(iter->first)->applyCuts(EventWeight);
	  //record it
	  systematics.passCuts[iter->first]=OK;

	  if(iter->first=="jetResup"){
	    if(OK){
	      double newMET=systematics.GetsysMETVector("jetResup")->Pt();
	      METChange_ACF->Fill((newMET-MET)/MET,EventWeight);
	      //RESOLUTION IN MET
	      newMETRes_ACF->Fill((newMET-TrueMET)/TrueMET,EventWeight);
	    }
	  }


	}
      }

    }//systematics.IsEnabled()


    

    
    //avoid having the first iteration entering here when running in 
    //quick mode
    if(isquick && i==0 && !OK)continue;



    EventInfo info;
    info.Event=Event;
    info.Run=Run;
    info.EventWeight=EventWeight;
    info.PUWeight=PUWeight;
    info.PUWeight_up=PUWeight_up;
    info.PUWeight_down=PUWeight_down;
    info.NBtags=NumberOfbTags;
    //
    if(!isData) info.PUInter    = goodVert.size();
    else        info.PUInter    = goodVert.size();
    //
    //

    

    //=====================FILL THE subTree====================
    //
    vector<Jet*> pCleanedJets;
    for (int ij=0;ij<CleanedJets.size();++ij){
      pCleanedJets.push_back(CleanedJets.at(ij).get());
    }    
    if (isquick || (!isquick && OKall)){
      if(doSubTree){
	vector<Jet*> pCleanedJets;
	for (int ij=0;ij<CleanedJets.size();++ij){
	  pCleanedJets.push_back(CleanedJets.at(ij).get());
	}
	
	//if(pCleanedJets.size()<4){
	//cout<<"CATASTROPHIC ERROR"<<endl;
	//}
	//cout<<"filling subTree"<<endl;/
	//cout<<"putos btags "<<NumberOfbTags<<endl;
	SubTree->Fill( &info, tree, SignalMuons, SignalElectrons, pCleanedJets, PFmet);
      }
    }


	
    //====================SYSTEMATICS TREES=======================================
    if( systematics.IsEnabled()){
      typedef map<string,bool>::iterator map_it;
      for (map_it iter=systematics.GetSysMap().begin(); iter != systematics.GetSysMap().end(); iter++){
	if(iter->second){    
	  if(systematics.passCuts.at(iter->first)){
	    //maybe there is no jet collection for this systematic calculation:
	    //then the default good cleaned jets should be used
	    if( systematics.GetTreeJetCollection(iter->first).empty()){
	      for( int ijet=0; ijet<CleanedJets.size(); ++ijet){
		systematics.GetTreeJetCollection(iter->first).push_back(CleanedJets.at(ijet));
	      }
	    }
	    //
	    //FILL THE TREES
	    //
	    systematics.GetsysDefaultTree(iter->first)->Fill(&info, tree, SignalMuons,SignalElectrons, systematics.GetTreeJetCollection(iter->first),*systematics.GetsysMETVector(iter->first));
	  }
	}
      }
    }
    //=============================================================================




    if (i==N-1){
      cout<<"last event in the loop "<<i<<endl;
    }
    
  }//End of the event loop
  
  cout<<"end of the event loop"<<endl;


  if(pcp)cout<<"out of the event loop"<<endl;


  if(systematics.IsEnabled()){
    
    typedef map<string,bool>::iterator map_it;
    for (map_it iter=systematics.GetSysMap().begin(); iter != systematics.GetSysMap().end(); iter++){
      if(iter->second){
	//set a different directory for this
	systematics.GetsysFlow(iter->first)->setTDir((TString)iter->first);
	systematics.GetsysFlow(iter->first)->dumpToHist();
      }
    }
  }
  cout<<"dumped the flows"<<endl;
  //subTree->Write();
  //==========WRITE THE TREES
  //   if(systematics.IsEnabled()){
  //     typedef map<string,bool>::iterator map_it;
  //     for (map_it iter=systematics.GetSysMap().begin(); iter != systematics.GetSysMap().end(); iter++){
  //       if(iter->second){
  // 	systematics.GetsysDefaultTree((string)iter->first)->Write();
  //       }
  //     }
  //   }
  cout<<"writing the treeFile"<<endl;
  treeFile->Write(); //Write all hists and tree associated with treeFile
  treeFile->Close();
  cout<<"wrote and closed the treeFile"<<endl;


  //==================================================
  //REST OF THE HISTOGRAMS
  //==================================================
  CutSet::setTDir("CutFlow");
  outfile->cd();
  cout<<"writing the histos"<<endl;
  TListIter* list = new TListIter(gDirectory->GetList()); //-->iterates over list of objects in memory
  TObject *nobj=(TObject*) list->Next(); //nobj now points to the first object in memory
  while(nobj){

    if(nobj->IsA()->InheritsFrom("TH1")){

      nobj->Write();
      //nobj->Write("",TObject::kOverwrite) -->Overwrites the histogram if alredy present
    }
    //delete(nobj);
    //TObject *nobj=(TObject*) list->Next(); //nobj now points to the first object in memory
    nobj=list->Next();   //nobj point now to the next object
  }
  delete(list);
  cout<<"wrote the histos"<<endl;
  //==================================================

  

  metStudiesDir->Write();
  JetUncDiff->Write();
  
  //write the control plots

  //other dir
  //tag.finalize(); //write the files
  //config.printUsed();
  //  globalFlow.printAll();
  cout<<"dumping the main cutflow"<<endl;
  globalFlow.dumpToHist(); 
  selectedMuonFlow.dumpToHist(); 
  cout<<"cutflow dumped"<<endl;
  
  //systematics.~Systematics();
  
  
  //  ControlPlots.HistoMaker::~HistoMaker();
  //  globalFlow.CutSet::~CutSet();
  //cout<<"closing the file"<<endl;
  //outfile->Close();
  //cout<<"file closed"<<endl;
  


  //PROGRAM END
  //EndOfProgram:
  
  
  return 0;
  }

