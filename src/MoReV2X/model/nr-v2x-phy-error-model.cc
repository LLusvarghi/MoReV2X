/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of
 * their official duties. Pursuant to titleElement 17 Section 105 of the United
 * States Code this software is not subject to copyright protection and
 * is in the public domain.
 * NIST assumes no responsibility whatsoever for its use by other parties,
 * and makes no guarantees, expressed or implied, about its quality,
 * reliability, or any other characteristic.

 * We would appreciate acknowledgement if the software is used.

 * NIST ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
 * DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING
 * FROM THE USE OF THIS SOFTWARE.
 *
 * Modified by: NIST
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */

#include <ns3/fatal-error.h>
#include <ns3/log.h>
#include <stdint.h>
#include <ns3/math.h>
#include <ns3/nr-v2x-phy-error-model.h>
#include <fstream>
#include <iostream>

#include <ns3/simulator.h>
namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("NrV2XPhyErrorModel");


// Huawei R1-160284 curvess
static const double V2VPsschMovingSisoBlerCurveXYaxis[2][11] = {
{0,2,4,6,8,10,12,14,16,18,20},
{0.9,0.6,0.2,0.04,0,0,0,0,0,0,0}
};

static const double V2VPsschSisoBlerCurveQPSKr07_190B_30Kmh[2][11] = {
{0,2,4,6,8,10,12,14,16,18,20},
{0.9,0.7,0.45,0.26,0.11,0.04,0.011,0,0,0,0}
};

static const double V2VPsschSisoBlerCurveQPSKr07_190B_280Kmh[2][11] = {
{0,2,4,6,8,10,12,14,16,18,20},
{1,0.9,0.7,0.38,0.13,0.046,0.006,0,0,0,0}
};

static const double V2VPsschSisoBlerCurveQPSKr07_800B_30Kmh[2][11] = {
{0,2,4,6,8,10,12,14,16,18,20},
{1,0.9,0.7,0.4,0.13,0.03,0,0,0,0,0}
};

static const double V2VPsschSisoBlerCurveQPSKr07_800B_280Kmh[2][11] = {
{0,2,4,6,8,10,12,14,16,18,20},
{1,1,0.9,0.73,0.4,0.17,0.07,0.04,0.03,0.028,0.023}
};

static const double PSSCH_CDL_X[46] = {-10,-9.5,-9,-8.5,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0,0.5,1,1.5,2,2.5,3,3.5,4,4.5,5,5.5,6,6.5,7,7.5,8,8.5,9,9.5,10,10.5,11,11.5,12,12.5};

static const double CDLBlerCurveQPSKr07_8RBs_LOS[46] = {1,1,1,1,1,1,1,1,1,1,1,1,1,0.9996,0.9959,0.9766,0.9129,0.7749,0.57,0.3535,0.1861,0.0831,0.031,0.0098,0.003,0.00088,0.00016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const double CDLBlerCurveQPSKr07_8RBs_NLOSv[46] = {1,1,1,1,1,1,0.9998,0.9994,0.9984,0.9957,0.9903,0.981,0.9663,0.9438,0.9131,0.8725,0.8234,0.7655,0.7019,0.6346,0.5645,0.4957,0.4282,0.3649,0.3063,0.2533,0.207,0.165,0.1303,0.1009,0.0771,0.0585,0.044,0.032,0.0226,0.016,0.0112,0.0079,0.0057,0.004,0.0026,0.0017,0.0012,0.00081,0.0005,0.00037};

static const double CDLBlerCurveQPSKr07_18RBs_LOS[46] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0.9991,0.9914,0.9577,0.8588,0.6777,0.4528,0.252,0.1184,0.0464,0.0163,0.0052,0.0015,0.00035,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const double CDLBlerCurveQPSKr07_18RBs_NLOSv[46] = {1,1,1,1,1,1,1,0.9999,0.9996,0.9988,0.9968,0.9928,0.9849,0.9715,0.9509,0.9213,0.8831,0.8351,0.7777,0.7149,0.6472,0.5781,0.509,0.4399,0.3745,0.3136,0.2595,0.2123,0.1701,0.1348,0.1041,0.0797,0.0596,0.0442,0.0315,0.0229,0.0165,0.0114,0.008,0.0055,0.0038,0.0026,0.0018,0.0011,0.00074,0.00052};

static const double CDLBlerCurveQPSKr07_28RBs_LOS[46] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,0.9998,0.9973,0.9788,0.9098,0.7542,0.5308,0.3077,0.1461,0.0575,0.0196,0.006,0.0015,0.0004,0.00016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const double CDLBlerCurveQPSKr07_28RBs_NLOSv[46] = {1,1,1,1,1,1,1,0.9998,0.9995,0.9982,0.9951,0.9895,0.9786,0.9616,0.9369,0.9023,0.8586,0.8058,0.7451,0.6792,0.6092,0.5384,0.47,0.4019,0.3393,0.2805,0.229,0.1852,0.1457,0.1128,0.0858,0.0639,0.0469,0.0334,0.0233,0.0164,0.0111,0.0074,0.0047,0.003,0.0018,0.001,0.00061,0.00034,0.00022,0.00014};

static const double CDLBlerCurveQPSKr07_38RBs_LOS[46] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,0.99978,0.99682,0.97615,0.90005,0.73455,0.50281,0.28011,0.12942,0.04895,0.01588,0.00441,0.00112,0.00023,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const double CDLBlerCurveQPSKr07_38RBs_NLOSv[46] = {1,1,1,1,1,1,1,0.9998,0.9993,0.9981,0.9948,0.9891,0.9785,0.9614,0.9352,0.9001,0.8556,0.802,0.7414,0.6742,0.6064,0.5349,0.4656,0.3987,0.3352,0.2767,0.225,0.1806,0.1419,0.1087,0.0816,0.0601,0.0431,0.0304,0.0206,0.0138,0.0089,0.0057,0.0036,0.0021,0.0012,0.00062,0.00035,0.00022,0.00011,0};

static const double CDLBlerCurveQPSKr07_48RBs_LOS[46] = {1,1,1,1,1,1,1,1,1,1,1,1,1,0.99998,0.99977,0.99617,0.97233,0.88934,0.71064,0.47351,0.25505,0.11283,0.04099,0.01272,0.0032,0.00063,0.00018,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const double CDLBlerCurveQPSKr07_48RBs_NLOSv[46] = {1,1,1,1,1,1,1,0.9999,0.9993,0.9979,0.9946,0.9888,0.9777,0.9591,0.9324,0.8968,0.8508,0.798,0.7355,0.6677,0.5986,0.5269,0.4577,0.3919,0.3271,0.2702,0.2195,0.1748,0.1363,0.1046,0.0773,0.0563,0.0403,0.0279,0.0184,0.0119,0.0076,0.0047,0.0028,0.0017,0.00094,0.00057,0.00025,0.0001,0,0};


static const double CDLBlerCurveQPSKr07_190B_LOS[46] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,0.9999,0.9988,0.9913,0.9555,0.8685,0.7006,0.4927,0.2943,0.1532,0.0659,0.0258,0.0093,0.0026,0.0011,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const double CDLBlerCurveQPSKr07_190B_NLOSv[46] = {1,1,1,1,1,1,1,0.9997,0.9997,0.9988,0.9965,0.9931,0.9865,0.9753,0.9537,0.925,0.8896,0.8427,0.7905,0.7259,0.6618,0.593,0.5251,0.4571,0.39,0.328,0.2698,0.2262,0.1862,0.1496,0.1153,0.0914,0.0696,0.0531,0.0392,0.0275,0.02,0.0141,0.0092,0.0065,0.0043,0.0035,0.0028,0.0022,0,0};

static const double CDLBlerCurveQPSKr07_720B_LOS[46] = {1,1,1,1,1,1,1,1,1,1,1,1,1,0.9999,0.9991,0.9933,0.9579,0.8484,0.6582,0.4181,0.2171,0.0943,0.033,0.0107,0.003,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const double CDLBlerCurveQPSKr07_720B_NLOSv[46] = {1,1,1,1,1,1,1,0.9996,0.9991,0.9966,0.9947,0.9884,0.9742,0.9527,0.9243,0.8872,0.8413,0.7834,0.7177,0.6509,0.5809,0.5111,0.4426,0.3771,0.3132,0.2544,0.2093,0.168,0.1317,0.1027,0.078,0.0569,0.0408,0.0283,0.0192,0.0126,0.0085,0.0046,0.0031,0.0020,0.0013,0,0,0,0,0};

static const double CDLBlerCurveQPSKr07_1000B_LOS[46] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,0.9995,0.995,0.9652,0.8652,0.6646,0.4296,0.2199,0.0921,0.0325,0.0103,0.0023,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const double CDLBlerCurveQPSKr07_1000B_NLOSv[46] = {1,1,1,1,1,1,1,0.9997,0.9991,0.9972,0.9927,0.9873,0.9757,0.9576,0.929,0.8917,0.8458,0.7858,0.7238,0.6573,0.5894,0.5157,0.4462,0.3814,0.3172,0.2614,0.212,0.1703,0.135,0.1028,0.0763,0.0537,0.0401,0.0272,0.0187,0.0115,0.0066,0.0031,0.0018,0,0,0,0,0,0,0};

static const double PSCCH_CDL_X[41] = {-20,-19.5,-19,-18.5,-18,-17.5,-17,-16.5,-16,-15.5,-15,-14.5,-14,-13.5,-13,-12.5,-12,-11.5,-11,-10.5,-10,-9.5,-9,-8.5,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

static const double CDLBlerCurveQPSKr013_PSCCH_NLOSv[41] = {0.9961,0.9959,0.9953,0.9946,0.991,0.9865,0.9784,0.9685,0.9546,0.9338,0.9054,0.8724,0.8349,0.7824,0.7258,0.66,0.5936,0.5246,0.4585,0.3954,0.3314,0.2769,0.2265,0.1807,0.1398,0.1065,0.0798,0.0597,0.0422,0.0285,0.0208,0.0149,0.0095,0.0053,0.003,0.0022,0.0016,0,0,0,0};

static const double CDLBlerCurveQPSKr013_PSCCH_LOS[41] = {0.9959,0.9956,0.9957,0.9952,0.9936,0.9929,0.9905,0.9856,0.9760,0.9602,0.9280,0.8838,0.8118,0.7182,0.6081,0.4802,0.3496,0.2282,0.136,0.0724,0.0326,0.0147,0.0047,0.0017,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


static const double Ericsson_PSSCH_CDL_X[16] = {-10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20};

static const double Ericsson_PSSCH_QPSKr05_NLOSv_15kHz_120kmh[16] = {1, 1, 1, 0.99, 0.7, 0.5, 0.3, 0.17, 0.07, 0.02, 0.006, 0.001, 0.0005, 0, 0, 0};

static const double Ericsson_PSSCH_QPSKr05_NLOSv_15kHz_280kmh[16] = {1, 1, 1, 1, 0.999, 0.7, 0.4, 0.2, 0.09, 0.03, 0.01, 0.002, 0.0006, 0.0001, 0, 0};

static const double Ericsson_PSSCH_QPSKr05_NLOSv_30kHz_120kmh[16] = {1, 1, 1, 0.95, 0.65, 0.45, 0.3, 0.15, 0.06, 0.02, 0.005, 0.001, 0, 0, 0, 0};

static const double Ericsson_PSSCH_QPSKr05_NLOSv_30kHz_280kmh[16] = {1, 1, 1, 0.95, 0.65, 0.5, 0.3, 0.15, 0.05, 0.015, 0.003, 0.0004, 0.0001, 0, 0, 0};

static const double Ericsson_PSSCH_QPSKr05_LOS_15kHz_120kmh[16] = {1, 1, 1, 0.97, 0.7, 0.4, 0.2, 0.1, 0.04, 0.01, 0.0014, 0.0001, 0, 0, 0, 0};

static const double Ericsson_PSSCH_QPSKr05_LOS_15kHz_280kmh[16] = {1, 1, 1, 1, 1, 0.8, 0.5, 0.19, 0.04, 0.007, 0.0002, 0, 0, 0, 0, 0};

static const double Ericsson_PSSCH_QPSKr05_LOS_30kHz_120kmh[16] = {1, 1, 1, 0.9, 0.6, 0.4, 0.2, 0.1, 0.04, 0.016, 0.005, 0.0009, 0.0002, 0, 0, 0};

static const double Ericsson_PSSCH_QPSKr05_LOS_30kHz_280kmh[16] = {1, 1, 1, 0.9, 0.7, 0.5, 0.22, 0.1, 0.03, 0.005, 0.0005, 0, 0, 0, 0, 0};

static const double Ericsson_PSCCH_CDL_X[31] = {-15, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static const double Ericsson_PSCCH_NLOSv_15kHz[31] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9, 0.8, 0.6, 0.5, 0.4, 0.3, 0.2, 0.12, 0.07, 0.05, 0.03, 0.02, 0.01, 0.006, 0.003, 0.002, 0, 0, 0, 0, 0, 0}; 

static const double Ericsson_PSCCH_NLOSv_30kHz[31] = {1, 1, 1, 1, 1, 1, 0.9, 0.75, 0.65, 0.5, 0.4, 0.3, 0.2, 0.1, 0.06, 0.04, 0.02, 0.012, 0.005, 0.003, 0.0015, 0.0008, 0.0001, 0, 0, 0, 0, 0, 0, 0, 0};

static const double Ericsson_PSCCH_LOS_15kHz[31] = {1, 1, 1, 1, 1, 1, 1, 1, 0.9, 0.7, 0.6, 0.5, 0.3, 0.2, 0.15, 0.07, 0.04, 0.02, 0.015, 0.006, 0.003, 0.002, 0.0009, 0.0006, 0.0001, 0, 0, 0, 0, 0, 0};

static const double Ericsson_PSCCH_LOS_30kHz[31] = {1, 1, 1, 1, 1, 1, 0.8, 0.6, 0.5, 0.3, 0.2, 0.1, 0.06, 0.03, 0.02, 0.01, 0.004, 0.003, 0.0015, 0.0004, 0.0002, 0.00012, 0, 0, 0, 0, 0, 0, 0, 0, 0};


static const double Huawei_PSSCH_CDL_X[8] = {-4, -2, 0, 2, 4, 6, 8, 10};

static const double Huawei_PSSCH_16QAMr05_30kHz_250kmh[8] = {1, 0.9, 0.6, 0.35, 0.12, 0.02, 0.0018, 0};


static const double Alejandro_TB_X[26] = {-10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const double Alejandro_TB_Y[26] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 0.97501, 0.6132, 0.4755, 0.3781, 0.2182, 0.1321, 0.0540, 0.0230, 0.0059, 0.0015, 0.0015, 0.0015, 0.0015, 0.0015, 0.0015, 0.0015, 0.0015};

static const double Alejandro_SCI_X[28] = {-12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const double Alejandro_SCI_Y[28] = {1, 1, 1, 0.9093, 0.7923, 0.6734, 0.5376, 0.3981, 0.2735, 0.1810, 0.1139, 0.0664, 0.0403, 0.0234, 0.0125, 0.0055, 0.0028, 0.0014, 0.00086, 0.00013, 0.00013, 0.00013, 0.00013, 0.00013, 0.00013, 0.00013, 0.00013, 0.00013};



// TODO FIXME New for V2X
NistTbErrorStats_t
NrV2XPhyErrorModel::GetV2VPsschBler (uint16_t mcs, double sinr, HarqProcessInfoList_t harqHistory, bool LOS, uint16_t SCS, uint32_t NPRB)
{
   //randomNormal = CreateObject<NormalRandomVariable> ();
   // See 3GPP TR 37.885
   double sinrDb = 10 * std::log10 (sinr);
   const double *Yaxis;
   const double *Xaxis;
   bool HighSINR = false, LowSINR = false;
   NistTbErrorStats_t tbStat;
   tbStat.tbler = 1; // Initialize to 1
   tbStat.sinr = sinr;
   //Check mcs values
   NS_LOG_DEBUG("Number of PSSCH Physical Resource Blocks (PRBs) = " << NPRB << " with MCS index: " << mcs << ", SCS = " << SCS << " kHz, SINR level = " << sinrDb);
//   std::cin.get();
   if (mcs > 27) 
     {
      NS_FATAL_ERROR ("MCS index cannot exceed 27");
    }
  // LOS = false;
   Xaxis = PSSCH_CDL_X;
   if (!LOS)
   {
	if ((NPRB == 8) || (NPRB == 10) || (NPRB == 12))
	{
	        Yaxis = CDLBlerCurveQPSKr07_8RBs_NLOSv;
		NS_LOG_INFO("8 PRB-NLOS Curve Selected");
	}	
	else if ((NPRB == 18) || (NPRB == 20) || (NPRB == 24))
	{
	        Yaxis = CDLBlerCurveQPSKr07_18RBs_NLOSv;
		NS_LOG_INFO("18 PRB-NLOS Curve Selected");
	}  
	else if ((NPRB == 28) || (NPRB == 30) || (NPRB == 36))
	{
	        Yaxis = CDLBlerCurveQPSKr07_28RBs_NLOSv;
		NS_LOG_INFO("28 PRB-NLOS Curve Selected");
	}   
	else if ((NPRB == 38) || (NPRB == 40) || (NPRB == 48))
	{
	        Yaxis = CDLBlerCurveQPSKr07_38RBs_NLOSv;
		NS_LOG_INFO("38 PRB-NLOS Curve Selected");
	}   
	else if ((NPRB == 48) || (NPRB == 50))
	{
	        Yaxis = CDLBlerCurveQPSKr07_48RBs_NLOSv;
		NS_LOG_INFO("48 PRB-NLOS Curve Selected");
	}    
        else
        {
                NS_LOG_UNCOND(NPRB);
                NS_ASSERT_MSG(false, "Non-valid number of RBs");
        }
   }
   else
   {    
        if ((NPRB == 8) || (NPRB == 10) || (NPRB == 12))
	{
		Yaxis =  CDLBlerCurveQPSKr07_8RBs_LOS;
		NS_LOG_INFO("8 PRB-LOS Curve Selected");
	}
	else if ((NPRB == 18) || (NPRB == 20) || (NPRB == 24))
	{
		Yaxis =  CDLBlerCurveQPSKr07_18RBs_LOS;
		NS_LOG_INFO("18 PRB-LOS Curve Selected");
	} 
	else if ((NPRB == 28) || (NPRB == 30) || (NPRB == 36))
	{
		Yaxis =  CDLBlerCurveQPSKr07_28RBs_LOS;
		NS_LOG_INFO("28 PRB-LOS Curve Selected");
	} 
	else if ((NPRB == 38) || (NPRB == 40) || (NPRB == 48))
	{
		Yaxis =  CDLBlerCurveQPSKr07_38RBs_LOS;
		NS_LOG_INFO("38 PRB-LOS Curve Selected");
	} 
	else if ((NPRB == 48) || (NPRB == 50))
	{
		Yaxis =  CDLBlerCurveQPSKr07_48RBs_LOS;
		NS_LOG_INFO("48 PRB-LOS Curve Selected");
	}   
	else
	{
                NS_ASSERT_MSG(false, "Non-valid number of RBs");
	}
   }

   if (sinrDb >= Xaxis[PSSCH_CDL_SIZE-1])
   {
        NS_LOG_DEBUG("Very high SINR: out of scale for this channel model");
	HighSINR = true;
        tbStat.tbler = 0;
	if (sinrDb == Xaxis[PSSCH_CDL_SIZE-1])
	        tbStat.tbler = Yaxis[PSSCH_CDL_SIZE-1];
   }
   else if (sinrDb < Xaxis[0])
   {
        NS_LOG_DEBUG("Very low SINR: out of scale for this channel model");
        LowSINR = true;
        tbStat.tbler = 1;
   }
  // For the moment I don't need to check any parameter, I am always assuming to have QPSK-0.7
   uint16_t index = 0;
   while(index < PSSCH_CDL_SIZE && !HighSINR && !LowSINR) // If it's a valid SINR value
   {
      // std::cout << "X " << Xaxis[index] << " Y " << Yaxis[index] << std::endl;
       if (sinrDb >= Xaxis[index] && sinrDb < Xaxis[index+1])
  		break;
       ++index;
   }  

   if (!HighSINR && !LowSINR)
   {
        tbStat.tbler = ((sinrDb - Xaxis[index+1])/(Xaxis[index] - Xaxis[index+1]))*Yaxis[index] - ((sinrDb - Xaxis[index])/(Xaxis[index] - Xaxis[index+1]))*Yaxis[index+1];
   } 

  NS_LOG_INFO("TBLER = " << tbStat.tbler << " with a SINR[dB] = " << sinrDb);
//  std::cin.get();

  return tbStat;
}

NistTbErrorStats_t
NrV2XPhyErrorModel::GetV2VPscchBler (uint16_t mcs, double sinr, HarqProcessInfoList_t harqHistory, bool LOS, uint16_t SCS, uint32_t NPRB)
{
   //randomNormal = CreateObject<NormalRandomVariable> ();
   // See 3GPP TR 37.885
   double sinrDb = 10 * std::log10 (sinr);
   const double *Yaxis;
   const double *Xaxis;
   bool HighSINR = false, LowSINR = false;
   NistTbErrorStats_t tbStat;
   tbStat.tbler = 1; // Initialize to 1
   tbStat.sinr = sinr;
   //Check mcs values
   NS_LOG_DEBUG("Number of PSSCH Physical Resource Blocks (PRBs) = " << NPRB << " with MCS index: " << mcs << ", SCS = " << SCS << " kHz, SINR level = " << sinrDb);
//   std::cin.get();
   if (mcs > 27) 
     {
      NS_FATAL_ERROR ("MCS index cannot exceed 27");
    }
  // LOS = false;
   Xaxis = PSCCH_CDL_X;
   if (!LOS)
   {
        Yaxis = CDLBlerCurveQPSKr013_PSCCH_NLOSv;
   }
   else
   {
	Yaxis =  CDLBlerCurveQPSKr013_PSCCH_LOS;
   }

   if (sinrDb >= Xaxis[PSCCH_CDL_SIZE-1])
   {
        NS_LOG_DEBUG("Very high SINR: out of scale for this channel model");
	HighSINR = true;
        tbStat.tbler = 0;
	if (sinrDb == Xaxis[PSCCH_CDL_SIZE-1])
	        tbStat.tbler = Yaxis[PSCCH_CDL_SIZE-1];
   }
   else if (sinrDb < Xaxis[0])
   {
        NS_LOG_DEBUG("Very low SINR: out of scale for this channel model");
        LowSINR = true;
        tbStat.tbler = 1;
   }
  // For the moment I don't need to check any parameter, I am always assuming to have QPSK-0.7
   uint16_t index = 0;
   while(index < PSCCH_CDL_SIZE && !HighSINR && !LowSINR) // If it's a valid SINR value
   {
      // std::cout << "X " << Xaxis[index] << " Y " << Yaxis[index] << std::endl;
       if (sinrDb >= Xaxis[index] && sinrDb < Xaxis[index+1])
  		break;
       ++index;
   }  

   if (!HighSINR && !LowSINR)
   {
        tbStat.tbler = ((sinrDb - Xaxis[index+1])/(Xaxis[index] - Xaxis[index+1]))*Yaxis[index] - ((sinrDb - Xaxis[index])/(Xaxis[index] - Xaxis[index+1]))*Yaxis[index+1];
   } 

  NS_LOG_INFO("TBLER = " << tbStat.tbler << " with a SINR[dB] = " << sinrDb);

  return tbStat;
}


NistTbErrorStats_t
NrV2XPhyErrorModel::GetNrV2XPsschBler (uint16_t mcs, double sinr, bool LOS, uint16_t SCS, double RelativeSpeed)
{
   double sinrDb = 10 * std::log10 (sinr);
   const double *Yaxis;
   const double *Xaxis;
   bool HighSINR = false, LowSINR = false;
   NistTbErrorStats_t tbStat;
   tbStat.tbler = 1; // Initialize to 1
   tbStat.sinr = sinr;
   //Check mcs values
   NS_LOG_DEBUG("Evaluating PSSCH with MCS index: " << mcs << ", SCS = " << SCS << " kHz, SINR level = " << sinrDb << ", LOS? " << LOS << ", relative speed = " << RelativeSpeed);
//   std::cin.get();
   if (mcs > 27) 
     {
      NS_FATAL_ERROR ("MCS index cannot exceed 27");
    }
  // LOS = false;
 /*  Xaxis = Ericsson_PSSCH_CDL_X;

   if (!LOS)
   {
     if (RelativeSpeed <= 200)
     {
       NS_LOG_DEBUG("Selecting 120 km/h NLOSv curves");
       if (SCS == 15)
         Yaxis = Ericsson_PSSCH_QPSKr05_NLOSv_15kHz_120kmh;
       else if (SCS == 30)
         Yaxis = Ericsson_PSSCH_QPSKr05_NLOSv_30kHz_120kmh;
       else
         NS_FATAL_ERROR("Unhandled SCS value");
     }
     else
     {
       NS_LOG_DEBUG("Selecting 280 km/h NLOSv curves");
       if (SCS == 15)
         Yaxis = Ericsson_PSSCH_QPSKr05_NLOSv_15kHz_280kmh;
       else if (SCS == 30)
         Yaxis = Ericsson_PSSCH_QPSKr05_NLOSv_30kHz_280kmh;
       else
         NS_FATAL_ERROR("Unhandled SCS value");
     }
   }
   else
   {    
     if (RelativeSpeed <= 200)
     {
       NS_LOG_DEBUG("Selecting 120 km/h LOS curves");
       if (SCS == 15)
         Yaxis = Ericsson_PSSCH_QPSKr05_LOS_15kHz_120kmh;
       else if (SCS == 30)
         Yaxis = Ericsson_PSSCH_QPSKr05_LOS_30kHz_120kmh;
       else
         NS_FATAL_ERROR("Unhandled SCS value");
     }
     else
     {
       NS_LOG_DEBUG("Selecting 280 km/h LOS curves");
       if (SCS == 15)
         Yaxis = Ericsson_PSSCH_QPSKr05_LOS_15kHz_280kmh;
       else if (SCS == 30)
         Yaxis = Ericsson_PSSCH_QPSKr05_LOS_30kHz_280kmh;
       else
         NS_FATAL_ERROR("Unhandled SCS value");
     }
   }

   if (sinrDb >= Xaxis[Ericsson_PSSCH_CDL_SIZE-1])
   {
        NS_LOG_DEBUG("Very high SINR: out of scale for this channel model");
	HighSINR = true;
        tbStat.tbler = 0;
	if (sinrDb == Xaxis[Ericsson_PSSCH_CDL_SIZE-1])
	        tbStat.tbler = Yaxis[Ericsson_PSSCH_CDL_SIZE-1];
   }
   else if (sinrDb < Xaxis[0])
   {
        NS_LOG_DEBUG("Very low SINR: out of scale for this channel model");
        LowSINR = true;
        tbStat.tbler = 1;
   }
  // For the moment I don't need to check any parameter, I am always assuming to have QPSK-0.7
   uint16_t index = 0;
   while(index < Ericsson_PSSCH_CDL_SIZE && !HighSINR && !LowSINR) // If it's a valid SINR value
   {
      // std::cout << "X " << Xaxis[index] << " Y " << Yaxis[index] << std::endl;
       if (sinrDb >= Xaxis[index] && sinrDb < Xaxis[index+1])
  		break;
       ++index;
   }  */

/*   Xaxis = Huawei_PSSCH_CDL_X;
   Yaxis = Huawei_PSSCH_16QAMr05_30kHz_250kmh;

   if (sinrDb >= Xaxis[Huawei_PSSCH_CDL_SIZE-1])
   {
        NS_LOG_DEBUG("Very high SINR: out of scale for this channel model");
	HighSINR = true;
        tbStat.tbler = 0;
	if (sinrDb == Xaxis[Huawei_PSSCH_CDL_SIZE-1])
	        tbStat.tbler = Yaxis[Huawei_PSSCH_CDL_SIZE-1];
   }
   else if (sinrDb < Xaxis[0])
   {
        NS_LOG_DEBUG("Very low SINR: out of scale for this channel model");
        LowSINR = true;
        tbStat.tbler = 1;
   }
  // For the moment I don't need to check any parameter, I am always assuming to have QPSK-0.7
   uint16_t index = 0;
   while(index < Huawei_PSSCH_CDL_SIZE && !HighSINR && !LowSINR) // If it's a valid SINR value
   {
      // std::cout << "X " << Xaxis[index] << " Y " << Yaxis[index] << std::endl;
       if (sinrDb >= Xaxis[index] && sinrDb < Xaxis[index+1])
  		break;
       ++index;
   }  */

   Xaxis = Alejandro_TB_X;
   Yaxis = Alejandro_TB_Y;

   if (sinrDb >= Xaxis[Alejandro_TB_SIZE-1])
   {
        NS_LOG_DEBUG("Very high SINR: out of scale for this channel model");
	HighSINR = true;
        tbStat.tbler = 0;
	if (sinrDb == Xaxis[Alejandro_TB_SIZE-1])
	        tbStat.tbler = Yaxis[Alejandro_TB_SIZE-1];
   }
   else if (sinrDb < Xaxis[0])
   {
        NS_LOG_DEBUG("Very low SINR: out of scale for this channel model");
        LowSINR = true;
        tbStat.tbler = 1;
   }

   uint16_t index = 0;
   NS_LOG_DEBUG("Printing the values");
   while(index < Alejandro_TB_SIZE)
   { 
     NS_LOG_DEBUG("(" <<Xaxis[index] << "," << Yaxis[index] << ")");
     ++index;
   }

   index = 0;
   while(index < Alejandro_TB_SIZE && !HighSINR && !LowSINR) // If it's a valid SINR value
   {
      // std::cout << "X " << Xaxis[index] << " Y " << Yaxis[index] << std::endl;
       if (sinrDb >= Xaxis[index] && sinrDb < Xaxis[index+1])
  		break;
       ++index;
   }  


   if (!HighSINR && !LowSINR)
   {
        tbStat.tbler = ((sinrDb - Xaxis[index+1])/(Xaxis[index] - Xaxis[index+1]))*Yaxis[index] - ((sinrDb - Xaxis[index])/(Xaxis[index] - Xaxis[index+1]))*Yaxis[index+1];
   } 

  NS_LOG_INFO("TBLER = " << tbStat.tbler << " with a SINR = " << sinrDb << " dB");
//  std::cin.get();

  return tbStat;
}


NistTbErrorStats_t
NrV2XPhyErrorModel::GetNrV2XPscchBler (uint16_t mcs, double sinr, bool LOS, uint16_t SCS) // Assuming a 12 RBs bandwidth for the PSCCH
{
   double sinrDb = 10 * std::log10 (sinr);
   const double *Yaxis;
   const double *Xaxis;
   bool HighSINR = false, LowSINR = false;
   NistTbErrorStats_t tbStat;
   tbStat.tbler = 1; // Initialize to 1
   tbStat.sinr = sinr;
   //Check mcs values
   NS_LOG_DEBUG("Evaluating PSCCH with MCS index: " << mcs << ", SCS = " << SCS << " kHz, SINR level = " << sinrDb << ", LOS? " << LOS);
//   std::cin.get();
   if (mcs > 27) 
     {
      NS_FATAL_ERROR ("MCS index cannot exceed 27");
    }
  // LOS = false;
 /*  Xaxis = Ericsson_PSCCH_CDL_X;
   if ((!LOS) || true) // Temporary solution to align with Alejandro
   {
     NS_LOG_DEBUG("Selecting NLOSv curves");
     if (SCS == 15)
       Yaxis = Ericsson_PSCCH_NLOSv_15kHz;
     else if (SCS == 30)
       Yaxis = Ericsson_PSCCH_NLOSv_30kHz;
     else
       NS_FATAL_ERROR("Unhandled SCS value");
   }
   else
   {
     NS_LOG_DEBUG("Selecting LOS curves");
     if (SCS == 15)
       Yaxis = Ericsson_PSCCH_LOS_15kHz;
     else if (SCS == 30)
       Yaxis = Ericsson_PSCCH_LOS_30kHz;
     else
       NS_FATAL_ERROR("Unhandled SCS value");
   }

   if (sinrDb >= Xaxis[Ericsson_PSCCH_CDL_SIZE-1])
   {
        NS_LOG_DEBUG("Very high SINR: out of scale for this channel model");
	HighSINR = true;
        tbStat.tbler = 0;
	if (sinrDb == Xaxis[Ericsson_PSCCH_CDL_SIZE-1])
	        tbStat.tbler = Yaxis[Ericsson_PSCCH_CDL_SIZE-1];
   }
   else if (sinrDb < Xaxis[0])
   {
        NS_LOG_DEBUG("Very low SINR: out of scale for this channel model");
        LowSINR = true;
        tbStat.tbler = 1;
   }
  // For the moment I don't need to check any parameter, I am always assuming to have QPSK-0.7
   uint16_t index = 0;
   while(index < Ericsson_PSCCH_CDL_SIZE && !HighSINR && !LowSINR) // If it's a valid SINR value
   {
      // std::cout << "X " << Xaxis[index] << " Y " << Yaxis[index] << std::endl;
       if (sinrDb >= Xaxis[index] && sinrDb < Xaxis[index+1])
  		break;
       ++index;
   }  */
   Xaxis = Alejandro_SCI_X;
   Yaxis = Alejandro_SCI_Y;

   if (sinrDb >= Xaxis[Alejandro_SCI_SIZE-1])
   {
        NS_LOG_DEBUG("Very high SINR: out of scale for this channel model");
	HighSINR = true;
        tbStat.tbler = 0;
	if (sinrDb == Xaxis[Alejandro_SCI_SIZE-1])
	        tbStat.tbler = Yaxis[Alejandro_SCI_SIZE-1];
   }
   else if (sinrDb < Xaxis[0])
   {
        NS_LOG_DEBUG("Very low SINR: out of scale for this channel model");
        LowSINR = true;
        tbStat.tbler = 1;
   }

   uint16_t index = 0;
   NS_LOG_DEBUG("Printing the values");
   while(index < Alejandro_SCI_SIZE)
   { 
     NS_LOG_DEBUG("(" <<Xaxis[index] << "," << Yaxis[index] << ")");
     ++index;
   }
   index = 0;
   while(index < Alejandro_SCI_SIZE && !HighSINR && !LowSINR) // If it's a valid SINR value
   {
      // std::cout << "X " << Xaxis[index] << " Y " << Yaxis[index] << std::endl;
       if (sinrDb >= Xaxis[index] && sinrDb < Xaxis[index+1])
  		break;
       ++index;
   }  

   if (!HighSINR && !LowSINR)
   {
        tbStat.tbler = ((sinrDb - Xaxis[index+1])/(Xaxis[index] - Xaxis[index+1]))*Yaxis[index] - ((sinrDb - Xaxis[index])/(Xaxis[index] - Xaxis[index+1]))*Yaxis[index+1];
   } 

  NS_LOG_INFO("TBLER = " << tbStat.tbler << " with a SINR[dB] = " << sinrDb);

  return tbStat;
}



} // namespace ns3
