#define makeMBDhist_cxx
#include "makeMBDhist.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>

void makeMBDhist::Loop(int ievt)
{
   Long64_t nentries = fChain->GetEntriesFast();
   bool isEvtPickup = (ievt>=0) ? true : false;
   if(isEvtPickup){evt_start = ievt; evt_end = ievt+1;}
   else{evt_start=0; evt_end = nentries;}
   
   TFile *wf = (isEvtPickup) ? new TFile(Form("OutputHist_event%d.root",ievt),"recreate") : new TFile("OutputHist_all.root","recreate");
   
   if (fChain == 0) return;

   for(int ic=0; ic<nChannels; ic++){
     h_ADC_s[ic] = new TH1D(Form("h_ADC_s_ch%d",ic),";ADC-pedestal;Counts",nADCbins,xADCmin,xADCmax); 
     h_ADC_n[ic] = new TH1D(Form("h_ADC_n_ch%d",ic),";ADC-pedestal;Counts",nADCbins,xADCmin,xADCmax); 
     h_TDC_s[ic] = new TH1D(Form("h_TDC_s_ch%d",ic),";TDC;Counts",nTDCbins,xTDCmin,xTDCmax); 
     h_TDC_n[ic] = new TH1D(Form("h_TDC_n_ch%d",ic),";TDC;Counts",nTDCbins,xTDCmin,xTDCmax); 
     h_TDC_diff[ic] = new TH1D(Form("h_TDC_diff_ch%d",ic),";TDC_{north}-TDC_{south};Counts",nTDCbins/4,xTDCdiffmin,xTDCdiffmax);
     h_TDC_diff_pedcut[ic] = new TH1D(Form("h_TDC_diff_pedcut_ch%d",ic),";TDC_{north}-TDC_{south};Counts",nTDCbins/4,xTDCdiffmin,xTDCdiffmax);
     if(isEvtPickup){
       h_waveform_ADC_s[ic] = new TH1D(Form("h_ADC_waveform_s_event%d_ch%d",ievt,ic),";Sample;ADC",nSamples,0,nSamples);
       h_waveform_TDC_s[ic] = new TH1D(Form("h_TDC_waveform_s_event%d_ch%d",ievt,ic),";Sample;ADC",nSamples,0,nSamples);
       h_waveform_ADC_n[ic] = new TH1D(Form("h_ADC_waveform_n_event%d_ch%d",ievt,ic),";Sample;ADC",nSamples,0,nSamples);
       h_waveform_TDC_n[ic] = new TH1D(Form("h_TDC_waveform_n_event%d_ch%d",ievt,ic),";Sample;ADC",nSamples,0,nSamples);
     }
   }
   
   h_TDC_avg_s = new TH1D("h_TDC_avg_s",";TDC^{avg}_{south};Counts",nTDCbins/2,xTDCmin,xTDCmax*1.5);
   h_TDC_avg_n = new TH1D("h_TDC_avg_n",";TDC^{avg}_{north};Counts",nTDCbins/2,xTDCmin,xTDCmax*1.5);
   h_TDC_diff_avg = new TH1D("h_TDC_diff_avg",";TDC^{avg}_{north}-TDC^{avg}_{south};Counts",nTDCbins/4,xTDCdiffmin,xTDCdiffmax);

   h_TDC_avg_s_pedcut = new TH1D("h_TDC_avg_s_pedcut",";TDC^{avg}_{south};Counts",nTDCbins/2,xTDCmin,xTDCmax*1.5);
   h_TDC_avg_n_pedcut = new TH1D("h_TDC_avg_n_pedcut",";TDC^{avg}_{north};Counts",nTDCbins/2,xTDCmin,xTDCmax*1.5);
   h_TDC_diff_avg_pedcut = new TH1D("h_TDC_diff_avg_pedcut",";TDC^{avg}_{north}-TDC^{avg}_{south};Counts",nTDCbins/4,xTDCdiffmin,xTDCdiffmax);

   h_ADC_sum_s = new TH1D("h_ADC_sum_s",";Charge sum (south);Counts",nADCbins,xADCmin,xADCmax*20);
   h_ADC_sum_n = new TH1D("h_ADC_sum_n",";Charge sum (north);Counts",nADCbins,xADCmin,xADCmax*20);
   h_ADC_corr = new TH2D("h_ADC_corr",";Charge sum (north); Charge sum (south)",nADCbins/2,0,xADCmax*20,nADCbins,0,xADCmax*20);
   h_ADC_corr_pedcut = new TH2D("h_ADC_corr_pedcut",";Charge sum (north); Charge sum (south)",nADCbins/2,0,xADCmax*20,nADCbins,0,xADCmax*20);

   Long64_t nbytes = 0, nb = 0;
   std::pair<int,int> adc_ch;
   double adc_pedsub = 0;
   double tdc_val = 0;
   int ichannel = -1;
   double sumadc_n =-1;double sumadc_s=-1;
   vector<float> tdc_ch(128);
   double tdcsum_s; 
   double tdcsum_n; 
   double tdcsum_n_sub, tdcsum_s_sub;
   int tdc_count_n_sub, tdc_count_s_sub;
   int goodevt=0;
     
   for (Long64_t jentry=evt_start; jentry<evt_end;jentry++){
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;

      sumadc_n=0; sumadc_s=0; tdcsum_s=0; tdcsum_n=0;tdcsum_n_sub=0;tdcsum_s_sub=0; tdc_count_n_sub=0; tdc_count_s_sub=0;
      for(int il = 0; il<adcloop; il++){
        adc_ch = MBD_ChannelMap(il);
        ichannel = adc_ch.second;
        if(adc_ch.first==0){
          tdc_val = TDC_PeakVal(jentry,il);
          tdc_ch[ichannel] = tdc_val;
          if(isEvtPickup){
             for(int isample=0; isample<nSamples;isample++){
               (ichannel<ch_ns_div) ? h_waveform_TDC_n[ichannel] -> SetBinContent(isample+1, adc[il][isample]) : h_waveform_TDC_s[ichannel-nChannels] -> SetBinContent(isample+1, adc[il][isample]);
             }
          }
        }
        else if(adc_ch.first==1){
          adc_pedsub = peak[il]-pedestal[il];
          if(ichannel < ch_ns_div){
            h_ADC_n[ichannel]->Fill(adc_pedsub);
            if(isEvtPickup){
              for(int isample=0; isample<nSamples;isample++){
                h_waveform_ADC_n[ichannel] -> SetBinContent(isample+1,adc[il][isample]);
              }
            }
            sumadc_n+=adc_pedsub;
          }
          else if(ichannel>=ch_ns_div){
            h_ADC_s[ichannel-nChannels]->Fill(adc_pedsub);
            if(isEvtPickup){
              for(int isample=0; isample<nSamples;isample++){
                h_waveform_ADC_s[ichannel-nChannels] -> SetBinContent(isample+1,adc[il][isample]);
              }
            }
            sumadc_s+= adc_pedsub;
          }
        }
      }

      for(int ich = 0; ich<nChannels; ich++){
        h_TDC_n[ich]->Fill(tdc_ch[ich]);
        h_TDC_s[ich]->Fill(tdc_ch[ich+nChannels]);
        h_TDC_diff[ich] -> Fill(tdc_ch[ich] - tdc_ch[ich+nChannels]);
        tdcsum_n += tdc_ch[ich];
        tdcsum_s += tdc_ch[ich+nChannels];
//        std::cout << tdcsum_s << ", " <<tdcsum_n<<std::endl
        if(tdc_ch[ich]>500){
          tdcsum_n_sub += tdc_ch[ich];
          tdc_count_n_sub ++;
        }
        if(tdc_ch[ich+nChannels]>500){
          tdcsum_s_sub += tdc_ch[ich+nChannels];
          tdc_count_s_sub ++;
        }
        if(sumadc_n>adccoincut && sumadc_s>adccoincut && tdc_ch[ich]>200 && tdc_ch[ich+nChannels]>200) h_TDC_diff_pedcut[ich] -> Fill(tdc_ch[ich] - tdc_ch[ich+nChannels]);
      }

      h_TDC_avg_s->Fill(tdcsum_s/nChannels);
      h_TDC_avg_n->Fill(tdcsum_n/nChannels);
      h_TDC_diff_avg->Fill(tdcsum_n/nChannels - tdcsum_s/nChannels);

      h_TDC_avg_s_pedcut->Fill(tdcsum_s_sub/tdc_count_s_sub);
      h_TDC_avg_n_pedcut->Fill(tdcsum_n_sub/tdc_count_n_sub);
      h_TDC_diff_avg_pedcut->Fill(tdcsum_n_sub/tdc_count_n_sub - tdcsum_s_sub/tdc_count_s_sub);

      h_ADC_sum_s->Fill(sumadc_s);
      h_ADC_sum_n->Fill(sumadc_n);
      h_ADC_corr->Fill(sumadc_n,sumadc_s);
      if(sumadc_n>adccoincut && sumadc_s>adccoincut){
        h_ADC_corr_pedcut->Fill(sumadc_n,sumadc_s);
        if(goodevt>20) continue;
        std::cout << "event number : " << jentry << std::endl;
        goodevt++;
      }
   }

   wf->cd();
   h_ADC_sum_s->Write();
   h_ADC_sum_n->Write();
   h_ADC_corr->Write();
   h_ADC_corr_pedcut->Write();

   h_TDC_avg_n->Write();
   h_TDC_avg_s->Write();
   h_TDC_diff_avg->Write();
   h_TDC_avg_n_pedcut->Write();
   h_TDC_avg_s_pedcut->Write();
   h_TDC_diff_avg_pedcut->Write();

   for(int ic=0;ic<nChannels;ic++){
     h_ADC_n[ic]->Write();
     h_ADC_s[ic]->Write();
     h_TDC_n[ic]->Write();
     h_TDC_s[ic]->Write();
     h_TDC_diff[ic]->Write();
     h_TDC_diff_pedcut[ic]->Write();
     if(isEvtPickup){
       h_waveform_ADC_s[ic]->Write();
       h_waveform_TDC_s[ic]->Write();
       h_waveform_ADC_n[ic]->Write();
       h_waveform_TDC_n[ic]->Write();
     }
   }
   wf->Close();
}

std::pair<int,int> makeMBDhist::MBD_ChannelMap(int ich)
{
  int rem = (ich / NADC_OFF);
  int isADC = -1;
  int chmap = -1;
  if(rem % 2 == 0){
    isADC = 0;
    chmap = ich - NADC_OFF * (rem/2);
  }
  else if(rem % 2 ==1){
    isADC = 1;
    chmap = ich - NADC_OFF * (rem+1)/2;
  }
  else{
    std::cout << "wrong channel mapping!!" << std::endl;
  }
  std::pair<int,int> ch;
  ch = std::make_pair(isADC,chmap);
  return ch;
};

double makeMBDhist::TDC_PeakVal(int i, int ich){
  int scan_start = 4;
  int scan_end= 31;
  fChain->GetEntry(i);
  double tdc_start = adc[ich][scan_start];
  double tdc_ret = tdc_start;
  int ichsel = scan_start;
  for(int is=scan_start; is<scan_end; is++){
    if(adc[ich][is]>tdc_ret){
      tdc_ret = adc[ich][is];
      ichsel = is;
    }
  }
  return tdc_ret - pedestal[ich];
};

void makeMBDhist::EventDisplay(int ievt)
{
  Long64_t nbytes = 0, nb = 0;
  for(int ic=0; ic<nChannels; ic++){
    h_waveform_ADC_s[ic] = new TH1D(Form("h_ADC_waveform_s_event%d_ch%d",ievt,ic),";Sample;ADC",nSamples,0,nSamples);
    h_waveform_TDC_s[ic] = new TH1D(Form("h_TDC_waveform_s_event%d_ch%d",ievt,ic),";Sample;ADC",nSamples,0,nSamples);
    h_waveform_ADC_n[ic] = new TH1D(Form("h_ADC_waveform_n_event%d_ch%d",ievt,ic),";Sample;ADC",nSamples,0,nSamples);
    h_waveform_TDC_n[ic] = new TH1D(Form("h_TDC_waveform_n_event%d_ch%d",ievt,ic),";Sample;ADC",nSamples,0,nSamples);
  }
  
  std::pair<int,int> adc_ch;
  int ichannel = -1;
  vector<float> tdc_ch(128);
  double adc_pedsub = 0;
  double tdc_val = 0;
  for(Long64_t jentry=ievt; jentry<ievt+1;jentry++){
    Long64_t ientry = LoadTree(jentry);
    if (ientry < 0) break;
    nb = fChain->GetEntry(jentry);

    for(int il = 0; il<adcloop; il++){
      adc_ch = MBD_ChannelMap(il);
      ichannel = adc_ch.second;
      if(adc_ch.first==0){
        tdc_val = TDC_PeakVal(jentry,il);
        tdc_ch[ichannel] = tdc_val;
        for(int isample=0; isample<nSamples;isample++){
          (ichannel<ch_ns_div) ? h_waveform_TDC_n[ichannel] -> SetBinContent(isample+1, adc[il][isample]) : h_waveform_TDC_s[ichannel-nChannels] -> SetBinContent(isample+1, adc[il][isample]);
        }
      }
      else if(adc_ch.first==1){
        adc_pedsub = peak[il]-pedestal[il];
        for(int isample=0; isample<nSamples;isample++){
          (ichannel < ch_ns_div) ? h_waveform_ADC_n[ichannel] -> SetBinContent(isample+1,adc[il][isample]) : h_waveform_ADC_s[ichannel-nChannels] -> SetBinContent(isample+1,adc[il][isample]);
        }
      }
    }
  }
};
