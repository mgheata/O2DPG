#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

TFile *fileOut = nullptr;
TFile *fileSummaryOutput = nullptr;
TFile *fileTestSummary = nullptr;

TString prefix = "";
int correlationCase = 0; // at the moment...Later I need to check that

struct results
{
  bool a;
  double b;
  double c;
  bool d;
};

// define "a monte" the possible avaiable tests
enum options {
  Chi2    = 0x01,
  BinCont     = 0x02,
  Nentries   = 0x04,
  // ...
};


void ProcessFile(TString fname, TString dirToAnalyse); // needs to be declared
void ProcessMonitorObjectCollection(
    o2::quality_control::core::MonitorObjectCollection *o2MonObjColl);
void ProcessMonitorObject(o2::quality_control::core::MonitorObject *monObj);
void WriteHisto(TObject *obj);
void WriteHisto2D(TObject *obj);
void WriteProfile(TObject *obj);
void WriteTEfficiency(TObject *obj);
bool AreIdenticalHistos(TH1 *hA, TH1 *hB);
void CompareHistos(TH1 *hA, TH1 *hB, TString monobj, int whichTest, double valChi2, double valMeanDiff,
		   bool firstComparison, bool finalComparison, TH2F *hSum, TH2F *hTests);
struct results CompareChiSquareBinContentNentr(TH1 *hA, TH1 *hB, options whichTest, double varChi2, double varMeanDiff);
void DrawRatio(TH1 *hR);
void DrawRelativeDifference(TH1 *hR);
void SelectCriticalHistos(TString whichdir);
void createTestsSummaryPlot(TFile *file, TString obj);

void ReleaseValidation(TString filename1 = "QCpass3",
                       TString filename2 = "QCpass2",
                       TString ObjectToAnalyse = "", int whichTest = 1, double valueChi2 = 1.5, double valueMeanDiff = 0.01, 
                       bool isOnGrid = false,
                       bool selectCritical = false) {

  if (whichTest < 1 || whichTest > 7) {
    printf("Error, please select which test you want to perform: \n");
    printf("1->Chi-square; 2--> ContBinDiff; 3 --> Chi-square+MeanDiff; 4->N entries; 5--> Nentries + Chi2; 6 --> N entries + MeanDiff; 7 --> Nentries + Chi2 + MeanDiff");
    return;
  }

 
  fileSummaryOutput =
      new TFile("Summary_" + ObjectToAnalyse + ".root", "recreate");
  fileSummaryOutput->Close();

  TFile *inFile1 = nullptr;
  TFile *inFile2 = nullptr;
  // open files to be read in
  if (isOnGrid) {
    TGrid::Connect("alien://");
    inFile1 = TFile::Open("alien:" + filename1 + ".root", "READ");
    inFile2 = TFile::Open("alien:" + filename2 + ".root", "READ");
    inFile1->ls();
    inFile2->ls();
  } else {
    inFile1 = TFile::Open(filename1 + ".root", "READ");
    inFile2 = TFile::Open(filename2 + ".root", "READ");
  }

  inFile1->ls();
  inFile2->ls();

  fileOut = new TFile("newfile1.root", "recreate");
  ProcessFile(filename1 + ".root", ObjectToAnalyse);
  fileOut->Close();
  delete fileOut; // do I need to remove it?? //
  fileOut = new TFile("newfile2.root", "recreate");
  ProcessFile(filename2 + ".root", ObjectToAnalyse);
  fileOut->Close();
  delete fileOut;

  TFile *fileA = new TFile("newfile1.root");
  TFile *fileB = new TFile("newfile2.root");

  bool isLastComparison =
      false; // It is true when the last histogram of the file is considered,
              // in order to properly close the pdf
  bool isFirstComparison = false; // to properly open the pdf 
  
  int nkeys = fileA->GetNkeys();

  int ntest = 1;
  int nhisto = nkeys;
  TH2F *hSummaryCheck = new TH2F("hSummaryCheck","",ntest,0,1,nhisto,0,2);
  hSummaryCheck->SetStats(000);
  hSummaryCheck->SetMinimum(-1E-6);
  
  TH2F *hSummaryTests = new TH2F("hSummaryTests","",3,0,1,nhisto,0,2);
  hSummaryTests->SetStats(000);
  hSummaryTests->SetMinimum(-1E-6);
  
  TList *lkeys = fileA->GetListOfKeys();
  for (int j = 0; j < nkeys; j++) {
    if (j == 0)
      isFirstComparison = true;
    if (j == nkeys - 1)
      isLastComparison = true;
    TKey *k = (TKey *)lkeys->At(j);
    TString cname = k->GetClassName();
    TString oname = k->GetName();
    if (cname.BeginsWith("TH")) {
      TH1 *hA = static_cast<TH1 *>(fileA->Get(oname.Data()));
      TH1 *hB = static_cast<TH1 *>(fileB->Get(oname.Data()));
      if (hA && hB) {
	printf("%s and %s compared \n", hA->GetName(), hB->GetName());
        bool ok = AreIdenticalHistos(hA, hB);
        if (ok)
          printf("%s       ---> IDENTICAL\n", oname.Data());
        else {
          CompareHistos(hA, hB, ObjectToAnalyse, whichTest, valueChi2, valueMeanDiff, isFirstComparison, isLastComparison, hSummaryCheck, hSummaryTests);
        }
      } else {
        if (!hA)
          printf("%s    ---> MISSING in first file\n", hA->GetName());
        if (!hB)
          printf("%s    ---> MISSING  in second file\n", hA->GetName());
      }
    }
  }
  
  TCanvas * summaryCheck = new TCanvas("summaryCheck","summaryCheck");
  Int_t MyPalette[100];
  Double_t R[] = {1.00, 1.00, 0.00};
  Double_t G[] = {0.00, 0.50, 1.00};
  Double_t B[] = {0.00, 0.00, 0.00};
  Double_t s[] = {0.00, 0.50, 1.00};
  Int_t FI = TColor::CreateGradientColorTable(3, s, R, G, B, 100);
  for (int i=0;i<100;i++) MyPalette[i] = FI+i;
  gStyle->SetGridStyle(3);
  gStyle->SetGridWidth(3);
  summaryCheck->SetGrid();
  hSummaryCheck->Draw("colz");
 
  TCanvas * summaryTests = new TCanvas("summaryTests","summaryTests");
  
  gStyle->SetGridStyle(3);
  summaryTests->SetGrid();
  hSummaryTests->Draw("colz");

  fileSummaryOutput = new TFile("Summary_" + ObjectToAnalyse + ".root", "update");
  hSummaryCheck->Write(Form("hSummaryCheck%d",whichTest) + ObjectToAnalyse);
  hSummaryTests->Write("hSummaryTests" + ObjectToAnalyse);
  if (selectCritical)
    SelectCriticalHistos(ObjectToAnalyse);
  return;
}

void ProcessFile(TString fname, TString dirToAnalyse) {

  TFile *fileBase = TFile::Open(fname.Data());
  int nkeys = fileBase->GetNkeys();
  TList *lkeys = fileBase->GetListOfKeys();
  for (int j = 0; j < nkeys; j++) {
    prefix = "";
    TKey *k = (TKey *)lkeys->At(j);
    TString cname = k->GetClassName();
    TString oname = k->GetName();
    printf("****** KEY %d: %s (class %s)   ******\n", j, oname.Data(),
           cname.Data());
    if (cname == "o2::quality_control::core::MonitorObjectCollection") {
      o2::quality_control::core::MonitorObjectCollection *o2MonObjColl =
          (o2::quality_control::core::MonitorObjectCollection *)fileBase->Get(
              oname.Data());
      TString objName = o2MonObjColl->GetName();
      if (dirToAnalyse.Length() > 0 && !objName.Contains(dirToAnalyse.Data())) {
        printf("Skip MonitorObjectCollection %s\n", objName.Data());
      } else {
        prefix.Append(Form("%s_", o2MonObjColl->GetName()));
        ProcessMonitorObjectCollection(o2MonObjColl);
      
      }
    } 
  }
  return;
}

void ProcessMonitorObjectCollection(
    o2::quality_control::core::MonitorObjectCollection *o2MonObjColl) {
  printf("--- Process o2 Monitor Object Collection %s ---\n",
         o2MonObjColl->GetName());
  int nent = o2MonObjColl->GetEntries();
  int Counts = 0;
  for (int j = 0; j < nent; j++) {
    TObject *o = (TObject *)o2MonObjColl->At(j);
    TString clname = o->ClassName();
    TString olname = o->GetName();
    printf("****** %s (class %s)   ******\n", olname.Data(), clname.Data());
    if (clname == "o2::quality_control::core::MonitorObject") {
      o2::quality_control::core::MonitorObject *monObj =
          (o2::quality_control::core::MonitorObject *)o2MonObjColl->FindObject(
              olname.Data());
      // prefix.Append(Form("%s_",monObj->GetName()));
      ProcessMonitorObject(monObj);
      Counts++;
      if (Counts == 40)
        break; // to avoid crushes
    } else if (clname.BeginsWith("TH")) {
      TObject *o = (TObject *)o2MonObjColl->FindObject(olname.Data());
      WriteHisto(o);
       }else if(clname.BeginsWith("TProfile")){
      TObject* o=(TObject*)o2MonObjColl->FindObject(olname.Data());
       WriteProfile(o);
    } else if(clname.BeginsWith("TEfficiency")){
      TObject* o=(TObject*)o2MonObjColl->FindObject(olname.Data());
       WriteTEfficiency(o);
    }
  }
  printf("%d objects processed \n", Counts);
  return;
}

void ProcessMonitorObject(o2::quality_control::core::MonitorObject *monObj) {
  printf("------ Process o2 Monitor Object %s ------\n", monObj->GetName());
  TObject *Obj = (TObject *)monObj->getObject();
  TString Clname = Obj->ClassName();
  TString Objname = Obj->GetName();
  printf("****** %s (class %s)   ******\n", Objname.Data(), Clname.Data());
  if (Clname.BeginsWith("TH")) {
    WriteHisto(Obj);
  } 
  else if (Clname.BeginsWith("TProfile")) {
    WriteProfile(Obj);
  } else if (Clname.BeginsWith("TEfficiency")) {
    WriteTEfficiency(Obj);
  } else
    printf("class %s needs to be analysed \n", Clname.Data());
  return;
}

void WriteHisto(TObject *obj) {
  TH1 *hA = static_cast<TH1 *>(obj);
  TString hAcln = hA->ClassName();
  TDirectory *current = gDirectory;
  TCanvas *cc = new TCanvas();
  if (hAcln.Contains("TH2"))
    hA->Draw("colz");
  else
    hA->DrawNormalized();
  cc->SaveAs(Form("%s.png", hA->GetName()));
  fileOut->cd();
  hA->Write(Form("%s%s", prefix.Data(), hA->GetName()));
  current->cd();
  return;
}

void WriteHisto2D(TObject *obj) {
  TH2 *hA2D = static_cast<TH2 *>(obj);
  TDirectory *current = gDirectory;
  new TCanvas();
  hA2D->Draw("colz");
  fileOut->cd();
  hA2D->Write(Form("%s%s", prefix.Data(), hA2D->GetName()));
  current->cd();
  return;
}

void WriteProfile(TObject *obj) { // should I further develop that?
  TProfile *hProf = static_cast<TProfile *>(obj);
  TDirectory *current = gDirectory;
  new TCanvas();
  hProf->Draw();
  fileOut->cd();
  hProf->Write(Form("%s%s", prefix.Data(), hProf->GetName()));
  current->cd();
  return;
}

void WriteTEfficiency(TObject *obj) { // should I further develop that?
  TEfficiency *hEff = static_cast<TEfficiency *>(obj);
  TDirectory *current = gDirectory;
  new TCanvas();
  hEff->Draw("AP");
  fileOut->cd();
  hEff->Write(Form("%s%s", prefix.Data(), hEff->GetName()));
  current->cd();
  return;
}

bool AreIdenticalHistos(TH1 *hA, TH1 *hB) {

  Long_t nEventsA = hA->GetEntries(); // temporary: we should use the number of
                                      // analyzed events
  Long_t nEventsB = hB->GetEntries(); // temporary: we should use the number of
                                      // analyzed events
  if (nEventsA != nEventsB) {
    // printf(" %s -> Different number of entries: A --> %ld, B --> %ld\n", hA->GetName(), nEventsA, nEventsB);
    return false;
  }
  for (int ix = 1; ix <= hA->GetNbinsX(); ix++) {
    for (int iy = 1; iy <= hA->GetNbinsY(); iy++) {
      for (int iz = 1; iz <= hA->GetNbinsZ(); iz++) {
        double cA = hA->GetBinContent(ix, iy, iz);
        double cB = hB->GetBinContent(ix, iy, iz);
        if (TMath::Abs(cA - cB) > 0.001 * TMath::Abs(cA))
          return false;
      }
    }
  }
  return true;
}

void CompareHistos(TH1 *hA, TH1 *hB, TString monobj, int whichTest, double valChi2, double valMeanDiff,
                   bool firstComparison, bool finalComparison, TH2F *hSum, TH2F * hTests) {
  // method to evaluate and draw the result of the comparison between plots
  hSum->SetStats(000);
  hSum->SetMinimum(-1E-6);
  hTests->SetStats(000);
  hTests->SetMinimum(-1E-6);
  
  int nEventsA = hA->GetEntries();
  int nEventsB = hB->GetEntries();
  
  TH1 *hACl = (TH1 *)hA->Clone("hACl"); // I will use these two clones of hA and
                                        // hB to perform other checks later..
  TH1 *hBCl = (TH1 *)hB->Clone("hBCl");
  
  TString outc = "";
  int colt = 1;
  
  // Bit Mask
  // my 3 possible tests are: 1) chi2;  2) meandiff; 3) N entries.  These tests can be combined in 7 different ways
  // std::vector<std::string> tests;
  
  vector<bool> test_results;
  vector<bool> criticaltest_results; // true for each critical test, false for each non-critical test
  
  
  struct results testResult;
  // test if each of the 3 bits is turned on in subset ‘i = whichTest’?
  // if yes, process the bit
  if ( (whichTest & Chi2) == Chi2) {
    testResult = CompareChiSquareBinContentNentr(hA, hB, Chi2 , valChi2, valMeanDiff);
    test_results.push_back(testResult.a);
    criticaltest_results.push_back(testResult.d);
    
    if(testResult.a == false) { 
      if(testResult.d == true) { // if the BAD test is critical (true), then we have BAD, otherwise just a WARNING
	hTests->Fill("Chi2 test", Form("%s",hA->GetName()),0); // BAD--> histo bin cont = 1
      } else { 
	hTests->Fill("Chi2 test", Form("%s",hA->GetName()),0.5); // WARNING--> histo bin cont = 2
      }
    } else {
      hTests->Fill("Chi2 test", Form("%s",hA->GetName()),1); // GOOD--> histo bin cont = 0 
    }      
  }
  if ( (whichTest & BinCont) == BinCont) {
    testResult = CompareChiSquareBinContentNentr(hA, hB, BinCont , valChi2, valMeanDiff);
    test_results.push_back(testResult.a);
    criticaltest_results.push_back(testResult.d);
    
    if(testResult.a == false) { 
      if(testResult.d == true) { // if the BAD test is critical (true), then we have BAD, otherwise just a WARNING
	hTests->Fill("Bin Cont test", Form("%s",hA->GetName()),0); // BAD--> histo bin cont = 1
      } else { 
	hTests->Fill("Bin Cont test", Form("%s",hA->GetName()),0.5); // WARNING--> histo bin cont = 2
      }
    } else {
      hTests->Fill("Bin Cont test", Form("%s",hA->GetName()),1); // GOOD--> histo bin cont = 0 
    }      
  }
  if ( (whichTest & Nentries) == Nentries) {
    testResult = CompareChiSquareBinContentNentr(hA, hB, Nentries , valChi2, valMeanDiff);
    test_results.push_back(testResult.a);
    criticaltest_results.push_back(testResult.d);
    
    if(testResult.a == false) { 
      if(testResult.d == true) { // if the BAD test is critical (true), then we have BAD, otherwise just a WARNING
	hTests->Fill("Num. entries test", Form("%s",hA->GetName()),0); // BAD--> histo bin cont = 1
      } else { 
	hTests->Fill("Num. entries test", Form("%s",hA->GetName()),0.5); // WARNING--> histo bin cont = 2
      }
    } else {
      hTests->Fill("Num. entries test", Form("%s",hA->GetName()),1); // GOOD--> histo bin cont = 0 
    }      
  }
  //}
  // if all tests subset of the check are GOOD, then the result is GOOD, otherwise it is BAD or WARNING
  // It is BAD if at least one of the BAD tests is a critical test
  //}
  if(std::find(test_results.begin(), test_results.end(), 0) != test_results.end()) { // 0->kFALSE
    if(std::find(criticaltest_results.begin(), criticaltest_results.end(), 1) != criticaltest_results.end()) { // if at al least one of the BAD test is critical (true), then total check is BAD
      outc = Form("Check %d: BAD", whichTest);
      hSum->Fill(Form("Check%d", whichTest), Form("%s",hA->GetName()),0);
      colt = kRed+1;	
    } else {  // otherwise it will be a WARNING
      outc = Form("Check %d: WARNING", whichTest);
      hSum->Fill(Form("Check%d", whichTest), Form("%s",hA->GetName()),0.5);
      colt = kOrange+1;	
    }
    
  } else {  // if all test are GOOD, then the total test result will be COMPATIBLE
    outc = Form("Check %d: COMPATIBLE", whichTest);
    colt = kGreen + 1;
    hSum->Fill(Form("Check%d", whichTest), Form("%s",hA->GetName()),1); 
  }
 
  TCanvas *c = new TCanvas(hA->GetName(), hA->GetName(), 1200, 600);
  if(firstComparison)
    c->Print("plots.pdf[");
  c->Divide(2, 1);
  c->cd(1);
  TString hcln = hA->ClassName();
  TString optD = "";
  if (hcln.Contains("TH2"))
    optD = "box";
  hA->SetLineColor(1);
  hA->SetMarkerColor(1);
  hA->Scale(1. / hA->GetEntries()); // normalize to the number of entries
  TH1 *hAc = (TH1 *)hA->DrawClone(optD.Data());
  hAc->SetStats(0);
  hB->SetLineColor(2);
  hB->SetMarkerColor(2);
  hB->Scale(1. / hB->GetEntries()); // normalize to the number of entries
  TH1 *hBc = (TH1 *)hB->DrawClone(Form("%ssames", optD.Data()));
  hBc->SetStats(0);
  // c->Update();
  TPaveStats *stA =
    (TPaveStats *)hAc->GetListOfFunctions()->FindObject("stats");
  if (stA) {
    stA->SetLineColor(1);
    stA->SetTextColor(1);
    stA->SetY1NDC(0.68);
    stA->SetY2NDC(0.88);
  }
  TPaveStats *stB =
    (TPaveStats *)hBc->GetListOfFunctions()->FindObject("stats");
  if (stB) {
    stB->SetLineColor(2);
    stB->SetTextColor(2);
    stB->SetY1NDC(0.45);
    stB->SetY2NDC(0.65);
  }
  
  c->cd(2);
  if (hcln.Contains("TH3")) {
    TH1D *hXa = ((TH3 *)hA)->ProjectionX(Form("%s_xA", hA->GetName()));
    TH1D *hXb = ((TH3 *)hB)->ProjectionX(Form("%s_xB", hB->GetName()));
    TH1D *hYa = ((TH3 *)hA)->ProjectionY(Form("%s_yA", hA->GetName()));
    TH1D *hYb = ((TH3 *)hB)->ProjectionY(Form("%s_yB", hB->GetName()));
    TH1D *hZa = ((TH3 *)hA)->ProjectionZ(Form("%s_zA", hA->GetName()));
    TH1D *hZb = ((TH3 *)hB)->ProjectionZ(Form("%s_zB", hB->GetName()));
    hXa->Divide(hXb);
    hYa->Divide(hYb);
    hZa->Divide(hZb);
    TPad *rpad = (TPad *)gPad;
    rpad->Divide(1, 3);
    rpad->cd(1);
    DrawRatio(hXa);
    rpad->cd(2);
    DrawRatio(hYa);
    rpad->cd(3);
    DrawRatio(hZa);

  } else {
    TH1 *hArat = (TH1 *)hA->Clone("hArat");
    hArat->Divide(hB);
    for (int k = 1; k <= hArat->GetNbinsX(); k++)
      hArat->SetBinError(k, 0.000000001);
    hArat->SetMinimum(
        TMath::Max(0.98, 0.95 * hArat->GetBinContent(hArat->GetMinimumBin()) -
                             hArat->GetBinError(hArat->GetMinimumBin())));
    hArat->SetMaximum(
		      TMath::Min(1.02, 1.05 * hArat->GetBinContent(hArat->GetMaximumBin()) +
				 hArat->GetBinError(hArat->GetMaximumBin())));
    hArat->SetStats(0);
    if (hcln.Contains("TH2"))
      hArat->Draw("colz");
    else if (hcln.Contains("TH1"))
      DrawRatio(hArat);
    else
      hArat->Draw();
  }
  c->cd(1);

  TLatex *toutc = new TLatex(0.2, 0.85, outc.Data());
  toutc->SetNDC();
  toutc->SetTextColor(colt);
  toutc->SetTextFont(62);
  toutc->Draw();
  // draw text
  TLegend *more = new TLegend(0.6,0.6,.9,.8);
  more->SetBorderSize(1);
  more->AddEntry((TObject*)0, Form("#chi^{2} / Nbins = %f", testResult.b),"");
  more->AddEntry((TObject*)0, Form("meandiff = %f", testResult.c),"");
  more->Draw("same");
 
  c->SaveAs(Form("%s_Ratio.png", hA->GetName()));
  fileSummaryOutput = new TFile("Summary_" + monobj + ".root", "update");
  c->Write(Form("%s%s_Ratio", prefix.Data(), hA->GetName()));
  fileSummaryOutput->ls();
  fileSummaryOutput->Close();
  c->Print("plots.pdf[");
  
  // Implement the plotting of the difference between the two histograms, and
  // the relative difference
  TCanvas *c1 = new TCanvas(hACl->GetName(), hACl->GetName(), 1200, 600);
  c1->Divide(2, 1);
  c1->cd(1);

  TString hAClcln = hACl->ClassName();
  TString noptD = "";
  if (hAClcln.Contains("TH2"))
    noptD = "colz"; // box
  hACl->SetLineColor(1);
  hACl->SetMarkerColor(1);
  hACl->Scale(1. / hACl->GetEntries());
  hBCl->Scale(1. / hBCl->GetEntries());

  // Subtraction
  TH1 *hDiff = (TH1 *)hACl->Clone("hDiff");
  hDiff->SetStats(0);
  hDiff->Add(hBCl, -1);
  hDiff->DrawClone(noptD.Data());
  
  TPaveStats *stACl =
      (TPaveStats *)hACl->GetListOfFunctions()->FindObject("stats");
  if (stACl) {
    stACl->SetLineColor(1);
    stACl->SetTextColor(1);
    stACl->SetY1NDC(0.68);
    stACl->SetY2NDC(0.88);
  }

  c1->cd(2);
  if (hcln.Contains("TH3")) {
    TH1D *hXaCl = ((TH3 *)hDiff)->ProjectionX(Form("%s_xA", hACl->GetName()));
    TH1D *hXbCl = ((TH3 *)hBCl)->ProjectionX(Form("%s_xB", hBCl->GetName()));
    TH1D *hYaCl = ((TH3 *)hDiff)->ProjectionY(Form("%s_yA", hACl->GetName()));
    TH1D *hYbCl = ((TH3 *)hBCl)->ProjectionY(Form("%s_yB", hBCl->GetName()));
    TH1D *hZaCl = ((TH3 *)hDiff)->ProjectionZ(Form("%s_zA", hACl->GetName()));
    TH1D *hZbCl = ((TH3 *)hBCl)->ProjectionZ(Form("%s_zB", hBCl->GetName()));
    hXaCl->Divide(hXbCl);
    hYaCl->Divide(hYbCl);
    hZaCl->Divide(hZbCl);
    TPad *rrpad = (TPad *)gPad;
    rrpad->Divide(1, 3);
    rrpad->cd(1);
    DrawRelativeDifference(hXaCl);
    rrpad->cd(2);
    DrawRelativeDifference(hYaCl);
    rrpad->cd(3);
    DrawRelativeDifference(hZaCl);

  } else {
    TH1 *hDiffRel = (TH1 *)hDiff->Clone("hDiffRel");
    hDiffRel->Divide(hBCl);
    for (int k = 1; k <= hDiffRel->GetNbinsX(); k++)
      hDiffRel->SetBinError(k, 0.000000001);
    hDiffRel->SetMinimum(TMath::Max(
        0.98, 0.95 * hDiffRel->GetBinContent(hDiffRel->GetMinimumBin()) -
                  hDiffRel->GetBinError(hDiffRel->GetMinimumBin())));
    hDiffRel->SetMaximum(TMath::Min(
        1.02, 1.05 * hDiffRel->GetBinContent(hDiffRel->GetMaximumBin()) +
                  hDiffRel->GetBinError(hDiffRel->GetMaximumBin())));
    hDiffRel->SetStats(0);
    TString hDiffRelcln = hDiffRel->ClassName();
    if (hDiffRelcln.Contains("TH2"))
      hDiffRel->Draw("colz");
    else if (hDiffRelcln.Contains("TH1"))
      DrawRelativeDifference(hDiffRel);
    else
      hDiffRel->Draw();
  }


  
  c1->cd(1);
  toutc->Draw();
  more->Draw("same");
  c1->SaveAs(Form("%s_Difference.png", hA->GetName()));
  fileSummaryOutput = new TFile("Summary_" + monobj + ".root", "update");
  c1->Write(Form("%s%s_Difference", prefix.Data(), hA->GetName()));
  fileSummaryOutput->ls();
  fileSummaryOutput->Close();
  if (finalComparison) {
    c1->Print("plots.pdf[");
    c1->Print("plots.pdf]");
  } else
    c1->Print("plots.pdf[");

  
}

void DrawRatio(TH1 *hR) {
  hR->SetMarkerStyle(20);
  hR->SetMarkerSize(0.5);
  hR->SetMinimum(
      TMath::Max(0.98, 0.95 * hR->GetBinContent(hR->GetMinimumBin()) -
                           hR->GetBinError(hR->GetMinimumBin())));
  hR->SetMaximum(
      TMath::Min(1.02, 1.05 * hR->GetBinContent(hR->GetMaximumBin()) +
                           hR->GetBinError(hR->GetMaximumBin())));
  hR->SetStats(0);
  hR->GetYaxis()->SetTitle("Ratio");
  hR->Draw("P");
  return;
}

void DrawRelativeDifference(TH1 *hR) {
  hR->SetMarkerStyle(20);
  hR->SetMarkerSize(0.5);
  hR->SetMinimum(
      TMath::Max(0.98, 0.95 * hR->GetBinContent(hR->GetMinimumBin()) -
                           hR->GetBinError(hR->GetMinimumBin())));
  hR->SetMaximum(
      TMath::Min(1.02, 1.05 * hR->GetBinContent(hR->GetMaximumBin()) +
                           hR->GetBinError(hR->GetMaximumBin())));
  hR->SetStats(0);
  hR->GetYaxis()->SetTitle("RelativeDifference");
  hR->Draw("P");
  return;
}

void SelectCriticalHistos(TString whichdir) {
  printf("Select all critical plots..... \n");

  vector<string> NamesFromTheList;
  fileSummaryOutput = new TFile("Summary_" + whichdir + ".root", "READ");
  fileSummaryOutput->ls();

  ifstream InputFile;
  InputFile.open("CriticalPlots.txt");
  string string;
  while (!InputFile.eof()) // To get all the lines
  {
    std::getline(InputFile, string); // Save the names in a string
    NamesFromTheList.push_back(
        string); // Save the histo names in the string vector
    cout << string << endl;
  }
  InputFile.close();

  // access the string vector elements
  std::cout << "Access the elements of the list of critical..." << std::endl;
  for (int i = 0; i < NamesFromTheList.size(); i++) {
    cout << NamesFromTheList[i] << endl;
  }
  TCanvas *critic_pdf = new TCanvas("critic_pdf", "critic_pdf");
  critic_pdf->Print("critical.pdf[");

  int Nkeys = fileSummaryOutput->GetNkeys();
  std::cout << "In the summary file there are " << Nkeys << " plots. \n "
            << std::endl;
  TList *Lkeys = fileSummaryOutput->GetListOfKeys();
  for (int j = 0; j < Nkeys; j++) {
    std::cout << "case " << j << std::endl;
    TKey *k = (TKey *)Lkeys->At(j);
    TString Cname = k->GetClassName();
    TString Oname = k->GetName();
    std::cout << Oname << " " << Cname << std::endl;
    for (int i = 0; i < NamesFromTheList.size(); i++) {
      std::cout << NamesFromTheList[i] << std::endl;
      if (Oname.Contains(NamesFromTheList[i]) && NamesFromTheList[i] != "") {
        std::cout << " name file and name from the list: " << Oname << " e "
                  << NamesFromTheList[i] << std::endl;
        TCanvas *ccc =
            static_cast<TCanvas *>(fileSummaryOutput->Get(Oname.Data()));
        // ccc->Draw();
        ccc->Print("critical.pdf[");
      }
    }
  }
  critic_pdf->Print("critical.pdf]");
  return;
}

struct results CompareChiSquareBinContentNentr(TH1 *hA, TH1 *hB, options whichTest, double valChi2, double valMeanDiff) {
  // implement here some simple checks that the two histograms are statistically
  // compatible
  int nEventsA = hA->GetEntries();
  int nEventsB = hB->GetEntries();
  
  TString oname = hA->GetEntries();

  double chi2 = 0;
  double meandiff = 0;
  struct results res; // "res" will collect values of chi2, meandiff and final test result
  if (nEventsA == 0 && nEventsB == 0) {
    printf("%s histos have both zero entries!", hA->GetName());
    res.a =  false;
    res.d = false; // no critical
    return res;
  }
  if (nEventsA == 0 && nEventsB != 0) {
    res.a = false;
    res.d = true; // critical
    printf("One of the %s histos has zero entries!", hA->GetName());
    return res;
  }
  if (nEventsA != 0 && nEventsB == 0) {
    printf("One of the %s histos has zero entries!", hA->GetName());
    res.a = false;
    res.d = true; // critical
    return res;
  }

 
  int nBins = 0;
  for (int ix = 1; ix <= hA->GetNbinsX(); ix++) {
    for (int iy = 1; iy <= hA->GetNbinsY(); iy++) {
      for (int iz = 1; iz <= hA->GetNbinsZ(); iz++) {
        double cA = hA->GetBinContent(ix, iy, iz);
        double eA = 0;
        if (cA < 0)
          printf("Negative counts!!! cA=%f in bin %d %d %d\n", cA, ix, iy, iz);
        else
          eA = TMath::Sqrt(cA);
        double cB = hB->GetBinContent(ix, iy, iz);
        double eB = 0;
        if (cB < 0)
          printf("Negative counts!!! cB=%f in bin %d %d %d\n", cB, ix, iy, iz);
        else
          eB = TMath::Sqrt(cB);
        double diff = cA - cB;
        if (cA > 0 && cB > 0) {
          double correl = 0.;
          if (correlationCase == 1) {
            // estimate degree of correlation from number of events in histogram
            // assume that the histogram with less events is a subsample of that
            // with more events
            correl = TMath::Sqrt(cA / cB);
            if (cA > cB)
              correl = 1. / correl;
          }
          double sigma2 =
              eA * eA + eB * eB - 2 * correl * eA * eB; // to be improved
          meandiff += (cA / nEventsA - cB / nEventsB);
          if (sigma2 > 0)
            chi2 += diff * diff / sigma2;
          nBins++;
        }
      }
    }
  }
  if (nBins > 1) {
    printf(" -> Different contents: %s  chi2/nBins=%f   meanreldiff=%f \n",
           hA->GetName(), chi2 / nBins, meandiff);
    bool retVal = true;
    switch (whichTest) {
    case Chi2:
      printf("chi-square test performed. \n");
      if (chi2 / nBins < valChi2) { //1.5
        printf("%s       ---> COMPATIBLE\n", oname.Data());
        retVal = true; 
      } else {
        printf("%s       ---> BAD\n", oname.Data());
        retVal = false;
      }
      res.a = retVal;
      res.b = chi2 / nBins;
      res.c = TMath::Abs(meandiff);
      res.d = true; // critical
      break;

    case BinCont:
      printf("bin-content test performed. \n");
      if (TMath::Abs(meandiff) < valMeanDiff) {
        printf("%s       ---> COMPATIBLE\n", oname.Data());
        retVal = true;
      } else {
        printf("%s       ---> BAD\n", oname.Data());
        retVal = false;
      }
      res.a = retVal;
      res.b = chi2 / nBins;
      res.c = TMath::Abs(meandiff);
      res.d = true; // critical
      break;

    case Nentries:
      printf("Test of number of entries performed. \n");
      retVal = true;
      if(nEventsA != nEventsB) {
	printf(" %s -> Different number of entries: A --> %d, B --> %d\n", hA->GetName(), nEventsA, nEventsB);
	retVal = false;
      }
      for (int ix = 1; ix <= hA->GetNbinsX(); ix++) {
	for (int iy = 1; iy <= hA->GetNbinsY(); iy++) {
	  for (int iz = 1; iz <= hA->GetNbinsZ(); iz++) {
	    double cA = hA->GetBinContent(ix, iy, iz);
	    double cB = hB->GetBinContent(ix, iy, iz);
	    if (TMath::Abs(cA - cB) > 0.001 * TMath::Abs(cA))
	      retVal = false;
	  }
	}
      } 
      
      res.a = retVal;
      res.b = chi2 / nBins;
      res.c = TMath::Abs(meandiff);
      res.d = false; // no critical
      break;
    }

    return res;
  }
  res.a = false;
  printf(" Histograms with empty bins");
  return res;
}
