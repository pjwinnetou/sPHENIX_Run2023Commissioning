#define makehist_cxx
#include "makehist.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>

void makehist::Loop(int ievt)
{
   if (fChain == 0) return;

   Long64_t nentries = fChain->GetEntriesFast();
   
   if(ievt>0){evt_start = ievt; evt_end = ievt+1;}
   else{evt_start=0; evt_end = nentries;}
   TFile *wf = (ievt>0) ? new TFile(Form("OutputHist_event%d.root",ievt),"recreate") : new TFile("OutputHist_all.root","recreate");
   double waveval[nsample];
   h_energy_map = new TH2D("h_energy_map",";#eta bin;#phi bin",netabins,0,netabins,nphibins,0,nphibins);

   h_energy_phi = new TH1D("h_energy_phi",";#phi bin;tower energy",nphibins,0,nphibins);
   h_energy_eta = new TH1D("h_energy_eta",";#eta bin;tower energy",netabins,0,netabins);
   h_energy = new TH1D("h_energy",";tower energy;",nenergy,0,energymax);

   h_event = new TH1D("h_event","",1,0,1);

   for(int ieta = 0; ieta < netabins; ieta++){
     for(int iphi=0; iphi<nphibins; iphi++){
       h_waveform[ieta][iphi] = new TH1D(Form("h_waveform_eta%d_phi%d",ieta,iphi),";sample;ADC",nsample,0,nsample); 
       if(ievt>0) h_waveform_event[ieta][iphi] = new TH1D(Form("h_waveform_event%d_eta%d_phi%d",ievt,ieta,iphi),";sample;ADC",nsample,0,nsample);
     }
   }

   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=evt_start; jentry<evt_end;jentry++){
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);  nbytes++;
      
      for(int is=0;is<size;is++){
        h_energy_map->Fill(etabin->at(is),phibin->at(is),energy->at(is));
        h_energy_eta->Fill(etabin->at(is),energy->at(is));
        h_energy_phi->Fill(phibin->at(is),energy->at(is));
        h_energy->Fill(energy->at(is));
        SetWaveFormVal(is,waveval);
        for(int iw=0;iw<nsample;iw++){
          h_waveform[etabin->at(is)][phibin->at(is)]->SetBinContent(iw+1,waveval[iw]);
          if(ievt>0) h_waveform_event[etabin->at(is)][phibin->at(is)]->SetBinContent(iw+1,waveval[iw]);
        }
      }
   }
   h_event->SetBinContent(1,nbytes);

   wf->cd();
   h_energy_map->Write();
   h_energy_eta->Write();
   h_energy_phi->Write();
   h_energy->Write();
   for(int ieta = 0; ieta < netabins; ieta++){
     for(int iphi=0; iphi<nphibins; iphi++){
        h_waveform[ieta][iphi]->Write();
        if(ievt>0) h_waveform_event[ieta][iphi]->Write();
     }
   }
   h_event->Write();
   wf->Close();
}
