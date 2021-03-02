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
#include <ns3/nist-lte-phy-error-model.h>
#include <fstream>
#include <iostream>

#include <ns3/simulator.h>
namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("NistLtePhyErrorModel");


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


/*static const double V2VPsschMovingSisoBlerCurveYaxis[50] = {
1,0.995,0.99,0.985,0.98,0.975,0.97,0.965,0.96,0.955,0.95,0.945,0.94,0.93,0.925,0.92,
};*/

  /** 
   * Table of SINR for the physical uplink shared channel
   * SINR range is provided for each MCS and HARQ Tx
   * Index is defined by 4 * MCS + HARQ Tx
   */
static const double PuschAwgnSisoBlerCurveXaxis[116][3] = {
{-9.6,-4.6,0.2},
{-12.6,-7.2,0.2},
{-15.2,-8.8,0.2},
{-15.6,-9.8,0.2},
{-7.4,-3.0,0.2},
{-10.4,-5.2,0.2},
{-11.8,-7.0,0.2},
{-13.4,-8.0,0.2},
{-6.0,-2.6,0.2},
{-9.0,-5.0,0.2},
{-10.8,-6.8,0.2},
{-12.2,-8.4,0.2},
{-5.4,-2.0,0.2},
{-8.4,-4.6,0.2},
{-10.0,-6.4,0.2},
{-11.4,-7.6,0.2},
{-4.4,-1.2,0.2},
{-7.4,-3.8,0.2},
{-9.2,-5.4,0.2},
{-10.8,-6.6,0.2},
{-3.4,-0.4,0.2},
{-6.2,-2.2,0.2},
{-7.8,-4.8,0.2},
{-9.6,-6.0,0.2},
{-2.4,0.4,0.2},
{-5.2,-2.4,0.2},
{-7.2,-4.0,0.2},
{-8.2,-5.4,0.2},
{-1.0,1.4,0.2},
{-4.2,-1.2,0.2},
{-6.0,-3.2,0.2},
{-7.4,-4.8,0.2},
{-0.4,2.0,0.2},
{-3.6,-1.0,0.2},
{-5.6,-2.2,0.2},
{-6.8,-3.8,0.2},
{0.6,2.8,0.2},
{-3.0,-0.6,0.2},
{-4.8,-1.8,0.2},
{-6.0,-3.4,0.2},
{1.8,3.4,0.2},
{-2.4,-0.6,0.2},
{-4.2,-2.2,0.2},
{-5.6,-3.6,0.2},
{2.6,4.8,0.2},
{-0.4,1.6,0.2},
{-1.8,0.2,0.2},
{-2.6,-0.8,0.2},
{3.6,5.4,0.2},
{0.2,2.0,0.2},
{-1.4,0.8,0.2},
{-2.2,-0.4,0.2},
{4.4,6.4,0.2},
{1.0,3.0,0.2},
{-0.8,1.2,0.2},
{-1.8,0.2,0.2},
{5.2,7.0,0.2},
{1.4,3.4,0.2},
{-0.2,1.4,0.2},
{-1.2,0.4,0.2},
{6.0,7.8,0.2},
{2.0,3.8,0.2},
{0.2,2.0,0.2},
{-0.8,1.0,0.2},
{6.6,8.4,0.2},
{2.6,4.2,0.2},
{0.6,2.4,0.2},
{-0.4,1.2,0.2},
{7.0,8.8,0.2},
{2.6,4.4,0.2},
{1.0,2.8,0.2},
{-0.2,1.4,0.2},
{7.8,9.6,0.2},
{3.2,4.8,0.2},
{1.4,3.0,0.2},
{0.2,1.8,0.2},
{8.6,10.4,0.2},
{3.6,5.4,0.2},
{1.8,3.6,0.2},
{0.6,2.2,0.2},
{9.8,11.6,0.2},
{4.4,6.0,0.2},
{2.4,4.0,0.2},
{1.2,2.6,0.2},
{10.4,12.2,0.2},
{6.0,7.6,0.2},
{3.2,5.0,0.2},
{1.8,3.4,0.2},
{11.2,13.0,0.2},
{6.4,8.0,0.2},
{3.6,5.4,0.2},
{2.2,3.6,0.2},
{12.0,13.6,0.2},
{6.6,8.2,0.2},
{4.0,7.2,0.2},
{2.4,4.0,0.2},
{12.6,14.4,0.2},
{7.0,8.6,0.2},
{4.4,5.8,0.2},
{2.8,4.2,0.2},
{13.4,15.2,0.2},
{7.2,8.6,0.2},
{4.6,6.2,0.2},
{3.0,4.4,0.2},
{14.0,15.8,0.2},
{7.4,9.4,0.2},
{5.0,6.6,0.2},
{3.4,4.8,0.2},
{14.8,16.6,0.2},
{7.8,9.6,0.2},
{5.4,7.0,0.2},
{3.6,5.2,0.2},
{17.2,18.8,0.2},
{9.2,11.0,0.2},
{6.6,8.0,0.2},
{4.8,6.2,0.2}
};

  /** 
   * BLER values for the physical uplink shared channel
   * One dimension version of original table of [116][33] 
   * Index is computed based on MCS, HARQ, and SINR
   */
static const double PuschAwgnSisoBlerCurveYaxis[3828] = {
1,0.9992,0.998,0.9984,0.996,0.9948,0.9868,0.9792,0.9544,0.9384,0.9052,0.8532,0.798,0.6936,0.6048,0.4948,0.3864,0.2972,0.198,0.1388,0.0832,0.0432,0.0228,0.0108,0.0044,0.002,0,0,0,0,0,0,0,
1,0.9992,0.9992,0.9984,0.9984,0.9936,0.9908,0.9776,0.9672,0.9408,0.9016,0.8688,0.7828,0.7156,0.626,0.5096,0.3944,0.2924,0.2044,0.1392,0.0836,0.0452,0.028,0.0128,0.0056,0.0044,0.0008,0,0,0,0,0,0,
1,0.9996,1,1,1,0.9996,0.9996,0.9988,0.9984,0.9968,0.9904,0.9772,0.9688,0.9536,0.9188,0.876,0.806,0.7336,0.6424,0.5384,0.424,0.306,0.222,0.1484,0.0828,0.0528,0.0268,0.0116,0.0056,0.0032,0.0008,0.0016,0,
1,0.9996,0.9996,0.9968,0.9944,0.994,0.9892,0.9748,0.9664,0.9408,0.914,0.8508,0.7884,0.7032,0.6136,0.5188,0.3944,0.3124,0.2088,0.1388,0.078,0.0408,0.0208,0.0104,0.0052,0.0028,0.0016,0,0.0004,0,0,0,0,
1,0.9988,0.9996,0.9992,0.9984,0.9956,0.988,0.97,0.95,0.8992,0.8388,0.7616,0.6416,0.512,0.3912,0.2492,0.1704,0.0996,0.05,0.0232,0.0104,0.0036,0.0012,0,0,0,0,0,0,0,0,0,0,
1,0.9996,1,0.9996,0.9976,0.9924,0.988,0.9764,0.95,0.902,0.8352,0.7752,0.638,0.5068,0.3912,0.2624,0.1768,0.0964,0.0468,0.02,0.01,0.0028,0.0008,0.0004,0,0.0004,0,0,0,0,0,0,0,
1,0.9996,0.998,0.994,0.9888,0.9732,0.9564,0.9156,0.8488,0.78,0.6856,0.5532,0.402,0.2776,0.1916,0.1164,0.0568,0.0292,0.016,0.0028,0.0004,0.0008,0,0.0004,0,0,0,0,0,0,0,0,0,
1,0.9996,1,0.9984,0.998,0.9916,0.9912,0.9736,0.9528,0.9008,0.8476,0.74,0.64,0.512,0.4016,0.2588,0.1528,0.0876,0.0556,0.0164,0.008,0.0024,0.0004,0.0004,0,0,0.0004,0,0,0,0,0,0,
1,0.998,0.994,0.9916,0.9872,0.9648,0.9188,0.876,0.7804,0.6476,0.512,0.3632,0.2308,0.1412,0.0808,0.0336,0.0144,0.0032,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9968,0.9924,0.9832,0.9528,0.908,0.852,0.7588,0.6272,0.5028,0.3452,0.2148,0.1268,0.068,0.0228,0.0124,0.0048,0.0016,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9964,0.9936,0.984,0.9684,0.9188,0.8524,0.7628,0.6576,0.512,0.3572,0.2256,0.1396,0.0604,0.0296,0.01,0.0032,0.0004,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9972,0.9968,0.992,0.98,0.9544,0.9188,0.8376,0.7476,0.618,0.4704,0.3548,0.2188,0.1308,0.0592,0.0292,0.01,0.004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9996,0.9952,0.99,0.9776,0.948,0.9096,0.8128,0.6976,0.58,0.4088,0.2836,0.166,0.0764,0.038,0.0132,0.0028,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9996,0.994,0.9892,0.9724,0.9568,0.8948,0.7872,0.6876,0.52,0.3716,0.2348,0.132,0.0592,0.03,0.0112,0.0024,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9976,0.9924,0.974,0.946,0.896,0.81,0.6932,0.5448,0.3808,0.2412,0.1456,0.0732,0.0384,0.0104,0.002,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9984,0.9992,0.9956,0.9864,0.97,0.942,0.8812,0.7776,0.6548,0.5208,0.3704,0.2316,0.1272,0.0516,0.0248,0.008,0.0048,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9988,0.9972,0.9864,0.9604,0.9172,0.8404,0.7188,0.576,0.412,0.2648,0.1448,0.0772,0.0324,0.0148,0.0032,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9988,0.9952,0.9872,0.962,0.9368,0.8408,0.7028,0.5548,0.388,0.2444,0.1276,0.0508,0.0224,0.0044,0.0008,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9996,0.9948,0.9896,0.9656,0.9256,0.8532,0.75,0.5968,0.4176,0.262,0.1436,0.068,0.0284,0.0088,0.0016,0.0012,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,1,0.9996,0.9992,0.9956,0.9884,0.9648,0.9168,0.8264,0.6904,0.5452,0.3756,0.2308,0.1216,0.0496,0.0148,0.0052,0.0012,0.0004,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9992,0.9992,0.9944,0.9832,0.946,0.8848,0.7708,0.592,0.4032,0.2288,0.1152,0.0488,0.0204,0.0052,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9984,0.994,0.9776,0.9448,0.882,0.7464,0.5756,0.3992,0.2316,0.1072,0.0428,0.0152,0.004,0.0004,0,0,0,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.998,0.9948,0.9816,0.954,0.8908,0.782,0.626,0.4224,0.2512,0.1236,0.0456,0.0136,0.0024,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,1,1,0.9988,0.9916,0.9748,0.9308,0.8608,0.7236,0.5492,0.3796,0.206,0.1092,0.034,0.0096,0.0024,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9992,0.9988,0.992,0.9672,0.9192,0.816,0.6552,0.4372,0.2428,0.1168,0.0436,0.014,0.0036,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9964,0.9872,0.9476,0.8688,0.738,0.5416,0.3056,0.1648,0.0608,0.0232,0.0036,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9988,0.9948,0.9836,0.9532,0.8792,0.7316,0.5516,0.3416,0.172,0.0736,0.0236,0.0076,0.0016,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9972,0.9944,0.98,0.9396,0.8336,0.7056,0.5032,0.3096,0.1484,0.0568,0.0168,0.0052,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9932,0.9788,0.9344,0.8396,0.6808,0.4552,0.2448,0.1252,0.044,0.014,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9888,0.9656,0.8952,0.7632,0.5624,0.346,0.164,0.0624,0.016,0.0072,0.0028,0,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9976,0.9812,0.9536,0.868,0.7188,0.504,0.2988,0.138,0.0528,0.0128,0.0016,0,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9976,0.9848,0.9452,0.8432,0.6936,0.4612,0.2484,0.1112,0.034,0.0088,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.998,0.992,0.9712,0.9128,0.7916,0.6032,0.3752,0.1744,0.0748,0.0164,0.0044,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.998,0.99,0.9616,0.896,0.7316,0.516,0.3044,0.1148,0.0392,0.0068,0.0012,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9976,0.9896,0.9552,0.854,0.6912,0.4688,0.2592,0.1164,0.0368,0.0068,0.002,0,0,0,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.996,0.986,0.9496,0.8456,0.6408,0.4164,0.2268,0.0808,0.0156,0.0036,0.0004,0,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9972,0.9832,0.9368,0.822,0.6452,0.416,0.2032,0.0748,0.0204,0.0048,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9916,0.9684,0.8888,0.7328,0.4828,0.2524,0.0988,0.02,0.0032,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9936,0.9656,0.8868,0.7216,0.4848,0.2608,0.1012,0.0272,0.0068,0.0016,0.0004,0,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.998,0.9892,0.9468,0.8464,0.6368,0.42,0.1724,0.0604,0.0124,0.0008,0,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.968,0.896,0.76,0.552,0.296,0.156,0.048,0.012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.996,0.96,0.864,0.708,0.464,0.228,0.068,0.024,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.996,0.98,0.932,0.792,0.5,0.284,0.112,0.044,0.012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9924,0.952,0.876,0.62,0.356,0.168,0.064,0.008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9984,0.9944,0.9696,0.884,0.7056,0.4076,0.1872,0.0572,0.0108,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9916,0.9432,0.8036,0.5388,0.236,0.0748,0.012,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9944,0.95,0.8272,0.5232,0.2292,0.0532,0.01,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9972,0.9736,0.8444,0.552,0.2356,0.0516,0.006,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.998,0.976,0.8968,0.7208,0.4448,0.2068,0.07,0.01,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.998,0.9816,0.9092,0.694,0.3604,0.1364,0.0312,0.004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.998,0.9784,0.8648,0.5912,0.2848,0.068,0.008,0.0008,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9892,0.9056,0.6764,0.298,0.07,0.01,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9948,0.9644,0.8816,0.6716,0.3856,0.1448,0.0408,0.0096,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9944,0.9576,0.84,0.5612,0.2776,0.08,0.0112,0.002,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.998,0.9752,0.8516,0.578,0.2304,0.0544,0.008,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9992,0.976,0.858,0.5472,0.2032,0.0384,0.0044,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9972,0.9824,0.9388,0.764,0.4952,0.2316,0.068,0.0152,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9888,0.9296,0.7248,0.4144,0.1584,0.0296,0.0044,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9984,0.9724,0.874,0.5732,0.2512,0.0576,0.0072,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9672,0.8324,0.5128,0.1524,0.024,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9984,0.9892,0.9384,0.7844,0.5344,0.2584,0.0672,0.0128,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9884,0.9376,0.7132,0.3944,0.124,0.024,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9968,0.9636,0.8092,0.4676,0.1572,0.0296,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9888,0.9184,0.6504,0.2688,0.0588,0.008,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9888,0.9432,0.818,0.516,0.2528,0.0848,0.012,0.0028,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.992,0.9356,0.7124,0.3968,0.1224,0.0164,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9956,0.952,0.7724,0.4128,0.1164,0.0152,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.978,0.8656,0.5116,0.1436,0.0176,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9944,0.958,0.8256,0.5796,0.2808,0.0892,0.0212,0.0024,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9944,0.952,0.7832,0.456,0.1688,0.0288,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9792,0.8308,0.4876,0.1592,0.0204,0.0024,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9836,0.8696,0.5252,0.162,0.022,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9932,0.9652,0.8544,0.6092,0.3064,0.1088,0.0268,0.0036,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9776,0.8456,0.5152,0.1848,0.034,0.0032,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9824,0.8816,0.5608,0.186,0.0312,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9984,0.9872,0.8512,0.476,0.1292,0.0156,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9944,0.9616,0.8724,0.6084,0.3148,0.1008,0.03,0.0032,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9984,0.9892,0.8896,0.6136,0.2332,0.0456,0.0032,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9912,0.904,0.6424,0.2424,0.0416,0.0036,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.98,0.8224,0.4436,0.0956,0.006,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9904,0.9352,0.7756,0.4664,0.2152,0.0564,0.0116,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9672,0.8204,0.446,0.1328,0.016,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9932,0.9048,0.6428,0.224,0.0304,0.0024,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.998,0.956,0.7084,0.266,0.038,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.996,0.9744,0.8728,0.6352,0.3344,0.106,0.028,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9984,0.972,0.8628,0.5456,0.2152,0.0452,0.0032,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9988,0.9612,0.7928,0.4352,0.1208,0.0204,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9984,0.9856,0.8704,0.5304,0.1668,0.0228,0.0024,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9984,0.9828,0.9216,0.7436,0.4376,0.1628,0.0356,0.0076,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9904,0.9452,0.7516,0.4024,0.1064,0.0188,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9928,0.9128,0.6628,0.288,0.0564,0.0064,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.998,0.9604,0.7348,0.334,0.07,0.0076,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9948,0.9636,0.8468,0.5896,0.3024,0.0876,0.0176,0.0036,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9704,0.832,0.5056,0.178,0.028,0.0024,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9736,0.8076,0.4508,0.1328,0.0216,0.0004,0,0,0,0,0,0,0,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9852,0.8582,0.4952,0.1424,0.0124,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9976,0.9808,0.9008,0.6876,0.382,0.1364,0.0308,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9928,0.9324,0.6848,0.2952,0.0672,0.008,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9984,0.9568,0.7564,0.3904,0.0924,0.0104,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9992,0.9616,0.7312,0.3208,0.056,0.004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9896,0.9376,0.794,0.5196,0.2176,0.058,0.0104,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.996,0.9416,0.7264,0.3396,0.0832,0.008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9796,0.85,0.4876,0.1308,0.0152,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.998,0.9744,0.7704,0.3552,0.0532,0.0036,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9988,0.9936,0.9648,0.8408,0.5556,0.2676,0.0752,0.012,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9988,0.9696,0.8016,0.4212,0.1108,0.0152,0.0004,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.984,0.8644,0.5256,0.1532,0.022,0.0012,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9948,0.9356,0.6424,0.2,0.0304,0.0008,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9964,0.9652,0.8532,0.6004,0.2884,0.092,0.0152,0.0036,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9968,0.9704,0.8224,0.4376,0.1012,0.0136,0.0016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.998,0.96,0.7648,0.3704,0.0832,0.0056,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.9868,0.842,0.4444,0.1012,0.0076,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9944,0.9536,0.8196,0.5356,0.2408,0.0756,0.01,0.002,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9996,0.996,0.9452,0.7292,0.3404,0.0712,0.004,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9976,0.9514,0.7112,0.2812,0.0376,0.004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0.9872,0.856,0.4268,0.078,0.0024,0.0004,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

  /** 
   * Table of SINR for the physical sidelink discovery channel
   * SINR range is provided for each HARQ Tx
   * Index is defined by HARQ Tx
   */
static const double PsdchAwgnSisoBlerCurveXaxis[4][3] = {
{-0.8,2.4,0.2},
{-4.4,-0.4,0.2},
{-6.0,-1.6,0.2},
{-7.2,-3.4,0.2}
};

  /** 
   * BLER values for the physical sidelink discovery channel
   * One dimension version of original table of [116][33] 
   * Index is computed based on MCS, HARQ, and SINR
   */
static const double PsdchAwgnSisoBlerCurveYaxis[92] = {
1,0.9984,0.9972,0.9936,0.98,0.9524,0.9148,0.8192,0.7016,0.5628,0.3812,0.2616,0.1224,0.0708,0.0252,0.0108,0.0028,0,0,0,0,0,0,
1,0.9996,0.9996,0.9964,0.9908,0.9752,0.944,0.8692,0.7884,0.6448,0.47,0.2956,0.188,0.0828,0.0296,0.0136,0.0036,0.002,0.0004,0.0008,0,0,0,
1,0.9996,0.994,0.9888,0.974,0.9344,0.8676,0.7572,0.6104,0.4436,0.2816,0.1676,0.0704,0.034,0.012,0.0028,0.0016,0.0008,0.0004,0,0.0004,0.0004,0,
1,0.9988,0.9956,0.9896,0.9648,0.9332,0.8548,0.742,0.58,0.4272,0.2528,0.1384,0.0756,0.0248,0.0108,0.0032,0,0,0.0004,0,0,0,0};

  /** 
   * Table of SINR for the physical sidelink control channel
   * SINR range is provided for each HARQ Tx
   * Index is defined by HARQ Tx
   */
static const double PscchAwgnSisoBlerCurveXaxis[1][3] = {
{-6.2,1.2,0.2}
};

  /** 
   * BLER values for the physical sidelink control channel
   * One dimension version of original table of [116][33] 
   * Index is computed based on MCS, HARQ, and SINR
   */
static const double PscchAwgnSisoBlerCurveYaxis[38] = {
1,0.9883,0.9784,0.9721,0.9631,0.9517,0.9325,0.9133,0.8783,0.849,0.8048,0.7565,0.6958,0.6325,0.5703,0.5049,0.4273,0.3733,0.2989,0.2437,0.1932,0.1467,0.1126,0.0785,0.0592,0.0423,0.0265,0.0186,0.011,0.0072,0.0031,0.0026,0.0011,0.0008,0.0003,0.0002,0.0001,0};

  /** 
   * Table of SINR for the physical sidelink broadcast channel
   * SINR range is provided for each MCS and HARQ Tx
   * Index is defined by 4 * MCS + HARQ Tx
   */
static const double PsbchAwgnSisoBlerCurveXaxis[1][3] = {
{-13.2,-4.8,0.2}
};

  /** 
   * BLER values for the physical sidelink broadcast channel
   * One dimension version of original table of [116][33] 
   * Index is computed based on MCS, HARQ, and SINR
   */
static const double PsbchAwgnSisoBlerCurveYaxis[43] = {
1,0.9979,0.9969,0.9958,0.995,0.9912,0.9912,0.9846,0.9766,0.9661,0.9587,0.9409,0.9217,0.8995,0.8787,0.8373,0.7927,0.744,0.6976,0.6348,0.5662,0.5032,0.4392,0.3677,0.3068,0.2522,0.2011,0.1504,0.1254,0.0878,0.0635,0.0436,0.032,0.0211,0.0146,0.008,0.0063,0.0046,0.0033,0.0017,0.001,0.0004,0};



int16_t
NistLtePhyErrorModel::GetRowIndex (uint16_t mcs, uint8_t harq)
{
  NS_LOG_FUNCTION (mcs << (uint16_t) harq);
  return 4*mcs + harq;
}

int16_t
NistLtePhyErrorModel::GetColIndex (double val, double min, double max, double step)
{
  NS_LOG_FUNCTION (val << min << max << step);
  //std::cout << "GetColIndex val=" << val << " min=" << min << " max=" << max << " step=" << step << std::endl;
  int16_t index = -1; //out of range
  if (val >= min)
    {
      val=std::min(val, max); //must avoid overflow
      index = (val-min)/step;
    }
  return index;
}

double
NistLtePhyErrorModel::GetBlerValue (const double (*xtable)[XTABLE_SIZE], const double (*ytable), const uint16_t ysize, uint16_t mcs, uint8_t harq, double sinr)
{
  NS_LOG_FUNCTION (mcs << (uint16_t) harq << sinr);
  double sinrDb = 10 * std::log10 (sinr);
  int16_t rIndex = GetRowIndex (mcs, harq);
  double bler = 1;
  

  //std::cout << "sinrDb=" << sinrDb << " min=" << xtable[rIndex][0] << " max=" << xtable[rIndex][1] << std::endl;
  if (sinrDb < xtable[rIndex][0])
    {
      bler = 1;
    } 
  else if (sinrDb  > xtable[rIndex][1]) 
    {
      bler = 0;
    } 
  else
    {    
      int16_t index1 = std::floor((sinrDb-xtable[rIndex][0])/xtable[rIndex][2]);
      int16_t index2 = std::ceil((sinrDb-xtable[rIndex][0])/xtable[rIndex][2]);
      if (index1 != index2) 
	{
	  //interpolate
	  double sinr1 = std::pow (10, (xtable[rIndex][0] + index1*xtable[rIndex][2]) / 10);
	  double sinr2 = std::pow (10, (xtable[rIndex][0] + index2*xtable[rIndex][2]) / 10);
	  double bler1 = ytable[rIndex*ysize+index1];
	  double bler2 = ytable[rIndex*ysize+index2];
	  bler = bler1 + (bler2-bler1)*(sinr-sinr1)/(sinr2-sinr1);
         // NS_LOG_UNCOND("rIndex: " << rIndex << ", ysize: " << ysize << ", index1: " << index1 << ", index2: " << index2 << ", rIndex*ysize+index1: " << rIndex*ysize+index1 << ", bler1: " << bler1);
	}
      else
	{
	  bler = ytable[rIndex*ysize+index1];
	}
    }


  return bler;
}

double
NistLtePhyErrorModel::GetSinrValue (const double (*xtable)[XTABLE_SIZE], const double (*ytable), const uint16_t ysize, uint16_t mcs, uint8_t harq, double bler)
{
  double sinr = 0;
  int16_t rIndex = GetRowIndex (mcs, harq);
  uint16_t index = 0;
  while (ytable[rIndex*ysize+index] > bler) 
    {
      index++;
    }

  if (ytable[rIndex*ysize+index] < bler) 
    {
      double sinr1 = std::pow (10, (xtable[rIndex][0] + (index-1)*xtable[rIndex][2]) / 10);
      double sinr2 = std::pow (10, (xtable[rIndex][0] + index*xtable[rIndex][2]) / 10);
      double bler1 = ytable[rIndex*ysize+index-1];
      double bler2 = ytable[rIndex*ysize+index];
      sinr = sinr1 + (bler - bler1) * (sinr2-sinr1) / (bler2-bler1);
    } 
  else
    {
      //last or equal element
      sinr = std::pow (10, (xtable[rIndex][0] + index*xtable[rIndex][2]) / 10);
    }
  return sinr;
}

double 
NistLtePhyErrorModel::GetValueForIndex (uint16_t index, double min, double max, double step)
{
  NS_LOG_FUNCTION (index << min << max << step);
  //std::cout << "GetValueForIndex " << index << " " << min << " " << max << " " << step << std::endl;
  double value = 1;
  if (index > -1) 
    {
      value = min + index * step;
    }
  return value;
}

uint16_t
NistLtePhyErrorModel::GetBlerIndex (double bler, uint16_t row, const double (*ytable), const uint16_t ysize)
{
  uint16_t index = 0;
  while (ytable[row*ysize+index] > bler) 
    {
      index++;
    }
  return index-1; //return the index of the last column where bler is above the target one
}

NistTbErrorStats_t
NistLtePhyErrorModel::GetBler (const double (*xtable)[XTABLE_SIZE], const double (*ytable), const uint16_t ysize, uint16_t mcs, uint8_t harq, double prevSinr, double newSinr)
  {
    NS_LOG_FUNCTION (mcs << (uint16_t) harq << prevSinr << newSinr);

    NistTbErrorStats_t tbStat;
    tbStat.tbler = 1;

    if (harq > 0 && prevSinr != newSinr)
      {
	//must combine previous and new transmission
	double prevBler = GetBlerValue (xtable, ytable, ysize, mcs, harq, prevSinr);
	double newBler = GetBlerValue (xtable, ytable, ysize, mcs, harq, newSinr);
	//compute effective BLER
	if (prevBler == 1 && newBler == 1) 
	  {
	    //if both have BLER of 1, the interpolation will not work
	    tbStat.tbler = 1;
	    tbStat.sinr = prevSinr > newSinr ? prevSinr : newSinr;
	  } 
	else
	  { 
	    if (newSinr > prevSinr) 
	      {
		tbStat.tbler = (prevBler + newBler * newSinr / prevSinr) / (1 + newSinr / prevSinr);
	      }
	    else 
	      {
		tbStat.tbler = (prevBler + newBler * prevSinr / newSinr) / (1 + prevSinr / newSinr);
	      }
	    //reverse lookup to find effective SINR
	    tbStat.sinr = GetSinrValue (xtable, ytable, ysize, mcs, harq, tbStat.tbler);
	  }
	NS_LOG_DEBUG ("prevBler=" << prevBler << " newBler=" << newBler << " bler=" << tbStat.tbler);	
	
      } else 
      {
	//first transmission or the SINR did not change
	tbStat.tbler = GetBlerValue (xtable, ytable, ysize, mcs, harq, newSinr);
	//double tmp = GetSinrValue (xtable, ytable, mcs, harq, tbStat.tbler);
	//std::cout << " newSinr=" << newSinr << " reverse lookup=" << tmp << std::endl;
	tbStat.sinr = newSinr;       
      }
    NS_LOG_INFO ("bler=" << tbStat.tbler << ", sinr=" << tbStat.sinr);
    //std::cout << "bler=" << tbStat.tbler << ", sinr=" << tbStat.sinr << std::endl;
    return tbStat;
  }


/*NistTbErrorStats_t
NistLtePhyErrorModel::GetV2VPsschBler (NistLteFadingModel fadingChannel, NistLteTxMode txmode, uint16_t mcs, double sinr, HarqProcessInfoList_t harqHistory, bool LOS, double distance)
{
   //randomNormal = CreateObject<NormalRandomVariable> ();
   // See 3GPP TR 37.885
   double sigmaNLOSv = 4; //dB, NLOSv Highway scenario
   double shadowingNLOSv = randomNormal->GetValue (5 + std::max(0.0,15*std::log10(distance)-41), (sigmaNLOSv*sigmaNLOSv));
  //Check mcs values
  if (mcs > 20) 
    {
      NS_FATAL_ERROR ("PSSCH modulation cannot exceed 20");
    }

  //Find the table to use
  const double (*xtable)[XTABLE_SIZE];
  const double *ytable;
  
  double ysize = 0;
  
  uint32_t speed = 30;
  uint32_t tbs = 190;
  const double (*v2vtable)[11]; 
  
  double sinrDb = 10 * std::log10 (sinr);
  double bler = 1;

  if (mcs == 10)  //QPSK-r07
    {
       if (tbs == 190)
        {
           if (speed == 30)
             {
                v2vtable = V2VPsschSisoBlerCurveQPSKr07_190B_30Kmh;
             }
           else 
             {
                v2vtable = V2VPsschSisoBlerCurveQPSKr07_190B_280Kmh;
             }
        }
        else   // 800 Bytes
          {
           if (speed == 30)
             {
                v2vtable = V2VPsschSisoBlerCurveQPSKr07_800B_30Kmh;
             }
           else 
             {
                v2vtable = V2VPsschSisoBlerCurveQPSKr07_800B_280Kmh;
             }
          }
    }

   int Nint = 10;
   double SINRMax = 20.0;
   double SINRMin = 0.0;

   double SINRInterval = (SINRMax-SINRMin) / Nint;
   int intervalIndex = 0; 

   if (sinrDb <= SINRMin)
     {
       bler = 1.0;
     }
   else if (sinrDb >= SINRMax)
     {
       bler = 0.0;
     }
   else
    {
      intervalIndex = std::floor (sinrDb * Nint / (SINRMax-SINRMin));

      bler = v2vtable [1][intervalIndex] + (sinrDb-v2vtable [0][intervalIndex]) * (v2vtable [1][intervalIndex+1] - v2vtable [1][intervalIndex] ) / (v2vtable [0][intervalIndex+1] - v2vtable [0][intervalIndex]);
    }

//   std::ofstream blerFileSinr;
//   blerFileSinr.open ("results/sidelink/blerFileV2V.csv", std::ios_base::app);
//   blerFileSinr << sinrDb << "," << bler << "\r\n";
//   blerFileSinr.close();

  switch (fadingChannel) {
  case AWGN:
    switch (txmode) {
    case SISO:
      xtable = PuschAwgnSisoBlerCurveXaxis;
      ytable = PuschAwgnSisoBlerCurveYaxis;
      ysize = PUSCH_AWGN_SIZE;
      break;
    default: 
      NS_FATAL_ERROR ("Transmit mode "<< txmode << " not supported in AWGN channel");
    }
    break;
  default:
    NS_FATAL_ERROR ("Fading channel " << fadingChannel << " not supported");
  }

  NistTbErrorStats_t tbStat = GetV2VBler();
 
  return tbStat;
}*/


// TODO FIXME New for V2X
NistTbErrorStats_t
NistLtePhyErrorModel::GetV2VPsschBler (NistLteFadingModel fadingChannel, NistLteTxMode txmode, uint16_t mcs, double sinr, HarqProcessInfoList_t harqHistory, bool LOS, double distance, uint32_t NPRB)
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
   NS_LOG_INFO("Number of occupied Physical Resource Blocks (PRBs) = " << NPRB);
   if (mcs > 20) 
     {
      NS_FATAL_ERROR ("PSSCH modulation cannot exceed 20");
    }
  // LOS = false;
   Xaxis = PSSCH_CDL_X;
   if (!LOS)
   {
	if (NPRB == 8)
	{
	        Yaxis = CDLBlerCurveQPSKr07_8RBs_NLOSv;
		NS_LOG_INFO("8 PRB-NLOS Curve Selected");
	}	
	else if (NPRB == 18)
	{
	        Yaxis = CDLBlerCurveQPSKr07_18RBs_NLOSv;
		NS_LOG_INFO("18 PRB-NLOS Curve Selected");
	}  
	else if (NPRB == 28)
	{
	        Yaxis = CDLBlerCurveQPSKr07_28RBs_NLOSv;
		NS_LOG_INFO("28 PRB-NLOS Curve Selected");
	}   
	else if (NPRB == 38)
	{
	        Yaxis = CDLBlerCurveQPSKr07_38RBs_NLOSv;
		NS_LOG_INFO("38 PRB-NLOS Curve Selected");
	}   
	else if (NPRB == 48)
	{
	        Yaxis = CDLBlerCurveQPSKr07_48RBs_NLOSv;
		NS_LOG_INFO("48 PRB-NLOS Curve Selected");
	}    
        else
        {
                NS_ASSERT_MSG(false, "Non-valid number of RBs");
        }
   }
   else
   {    
        if (NPRB == 8)
	{
		Yaxis =  CDLBlerCurveQPSKr07_8RBs_LOS;
		NS_LOG_INFO("8 PRB-LOS Curve Selected");
	}
	else if (NPRB == 18)
	{
		Yaxis =  CDLBlerCurveQPSKr07_18RBs_LOS;
		NS_LOG_INFO("18 PRB-LOS Curve Selected");
	} 
	else if (NPRB == 28)
	{
		Yaxis =  CDLBlerCurveQPSKr07_28RBs_LOS;
		NS_LOG_INFO("28 PRB-LOS Curve Selected");
	} 
	else if (NPRB == 38)
	{
		Yaxis =  CDLBlerCurveQPSKr07_38RBs_LOS;
		NS_LOG_INFO("38 PRB-LOS Curve Selected");
	} 
	else if (NPRB == 48)
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
NistLtePhyErrorModel::GetV2VPscchBler (NistLteFadingModel fadingChannel, NistLteTxMode txmode, uint16_t mcs, double sinr, HarqProcessInfoList_t harqHistory, bool LOS, double distance)
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
   if (mcs > 20) 
     {
      NS_FATAL_ERROR ("PSCCH modulation cannot exceed 20");
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
NistLtePhyErrorModel::GetPsschBler (NistLteFadingModel fadingChannel, NistLteTxMode txmode, uint16_t mcs, double sinr, HarqProcessInfoList_t harqHistory)
{
  //Check mcs values
  if (mcs > 20) 
    {
      NS_FATAL_ERROR ("PSSCH modulation cannot exceed 20");
    }

  //Find the table to use
  const double (*xtable)[XTABLE_SIZE];
  const double *ytable;
  double ysize = 0;

  switch (fadingChannel) {
  case AWGN:
    switch (txmode) {
    case SISO:
      xtable = PuschAwgnSisoBlerCurveXaxis;
      ytable = PuschAwgnSisoBlerCurveYaxis;
      ysize = PUSCH_AWGN_SIZE;
      break;
    default: 
      NS_FATAL_ERROR ("Transmit mode "<< txmode << " not supported in AWGN channel");
    }
    break;
  default:
    NS_FATAL_ERROR ("Fading channel " << fadingChannel << " not supported");
  }

  NistTbErrorStats_t tbStat; 
  


  
  if (harqHistory.size() == 0)
    { 
      tbStat = GetBler (xtable, ytable, ysize, mcs, 0, 0,  sinr);
    }
  else 
    {
      tbStat = GetBler (xtable, ytable, ysize, mcs, harqHistory.size(), harqHistory[harqHistory.size()-1].m_sinr,  sinr);
    }
 
if(harqHistory.size() == 0){

    /* std::ofstream outfile;
   outfile.open("results/sidelink/bler_sinr_pssch.csv", std::ios_base::app);
   outfile << sinr << ", " << 10 * std::log10 (sinr) << "," << tbStat.tbler << "," << harqHistory.size() << "\r\n";
   outfile.close(); */
}

//  NS_LOG_UNCOND("Fading model " << fadingChannel << " Tx Mode " << txmode << " SINR = " << sinr);
//  std::cin.get();
  /*std::ofstream blerFile;
  blerFile.open("blerFile.unimo", std::ios_base::app);
  blerFile << Simulator::Now() << ", " << "mcs= " << mcs << ", sinr= " << sinr << "\r\n";
  blerFile.close();

  blerFile.open("results/sidelink/SINR_tmp.csv", std::ios_base::app);
  blerFile << sinr << "\r\n"; // Linear SINR: log will be performed by Python
  blerFile.close();
                       */
  return tbStat;
}

NistTbErrorStats_t
NistLtePhyErrorModel::GetPsdchBler (NistLteFadingModel fadingChannel, NistLteTxMode txmode, double sinr, HarqProcessInfoList_t harqHistory)
{
  //Find the table to use
  const double (*xtable)[XTABLE_SIZE];
  const double *ytable;
  double ysize = 0;

  switch (fadingChannel) {
  case AWGN:
    switch (txmode) {
    case SISO:
      xtable = PsdchAwgnSisoBlerCurveXaxis;
      ytable = PsdchAwgnSisoBlerCurveYaxis;
      ysize = PSDCH_AWGN_SIZE;
      break;
    default: 
      NS_FATAL_ERROR ("Transmit mode "<< txmode << " not supported in AWGN channel");
    }
    break;
  default:
    NS_FATAL_ERROR ("Fading channel " << fadingChannel << " not supported");
  }

  NistTbErrorStats_t tbStat;
  if (harqHistory.size() == 0)
    {
      tbStat = GetBler (xtable, ytable, ysize, 0 /*since no mcs used*/, 0, 0,  sinr);
    }
  else 
    {
      tbStat = GetBler (xtable, ytable, ysize, 0 /*since no mcs used*/, harqHistory.size(), harqHistory[harqHistory.size()-1].m_sinr,  sinr);
    }

  return tbStat;
}

NistTbErrorStats_t
NistLtePhyErrorModel::GetPscchBler (NistLteFadingModel fadingChannel, NistLteTxMode txmode, double sinr)
{
  //Find the table to use
  const double (*xtable)[XTABLE_SIZE];
  const double *ytable;
  double ysize = 0;

  switch (fadingChannel) {
  case AWGN:
    switch (txmode) {
    case SISO:
      xtable = PscchAwgnSisoBlerCurveXaxis;
      ytable = PscchAwgnSisoBlerCurveYaxis;
      ysize = PSCCH_AWGN_SIZE;
      break;
    default: 
      NS_FATAL_ERROR ("Transmit mode "<< txmode << " not supported in AWGN channel");
    }
    break;
  default:
    NS_FATAL_ERROR ("Fading channel " << fadingChannel << " not supported");
  }

  NistTbErrorStats_t tbStat = GetBler (xtable, ytable, ysize, 0 /*since no mcs used*/, 0, 0,  sinr);
  //NS_LOG_UNCOND(ysize << ", " << tbStat.tbler );

  /* std::ofstream outfile;
   outfile.open("results/sidelink/bler_sinr_pscch.csv", std::ios_base::app);
   outfile << sinr << ", " << 10 * std::log10 (sinr) << "," << tbStat.tbler << "\r\n";
   outfile.close(); */

  return tbStat;
}

NistTbErrorStats_t
NistLtePhyErrorModel::GetPuschBler (NistLteFadingModel fadingChannel, NistLteTxMode txmode, uint16_t mcs, double sinr, HarqProcessInfoList_t harqHistory)
{
  //Check mcs values
  if (mcs > 28) 
    {
      NS_FATAL_ERROR ("PUSCH modulation cannot exceed 28");
    }

  //Find the table to use
  const double (*xtable)[XTABLE_SIZE];
  const double *ytable;
  double ysize = 0;

  switch (fadingChannel) {
  case AWGN:
    switch (txmode) {
    case SISO:
      xtable = PuschAwgnSisoBlerCurveXaxis;
      ytable = PuschAwgnSisoBlerCurveYaxis;
      ysize = PUSCH_AWGN_SIZE;
      break;
    default: 
      NS_FATAL_ERROR ("Transmit mode "<< txmode << " not supported in AWGN channel");
    }
    break;
  default:
    NS_FATAL_ERROR ("Fading channel " << fadingChannel << " not supported");
  }

  NistTbErrorStats_t tbStat;
  if (harqHistory.size() == 0)
    {
      tbStat = GetBler (xtable, ytable, ysize, mcs, 0, 0,  sinr);
    }
  else 
    {
      tbStat = GetBler (xtable, ytable, ysize, mcs, harqHistory.size(), harqHistory[harqHistory.size()-1].m_sinr,  sinr);
    }

  return tbStat;
}

NistTbErrorStats_t
NistLtePhyErrorModel::GetPsbchBler (NistLteFadingModel fadingChannel, NistLteTxMode txmode, double sinr)
{
  //Find the table to use
  const double (*xtable)[XTABLE_SIZE];
  const double *ytable;
  double ysize = 0;

  switch (fadingChannel) {
  case AWGN:
    switch (txmode) {
    case SISO:
      xtable = PsbchAwgnSisoBlerCurveXaxis;
      ytable = PsbchAwgnSisoBlerCurveYaxis;
      ysize = PSBCH_AWGN_SIZE;
      break;
    default:
      NS_FATAL_ERROR ("Transmit mode "<< txmode << " not supported in AWGN channel");
    }
    break;
  default:
    NS_FATAL_ERROR ("Fading channel " << fadingChannel << " not supported");
  }

  NistTbErrorStats_t tbStat = GetBler (xtable, ytable, ysize, 0 /*since no mcs used*/, 0, 0,  sinr);

  return tbStat;
}


} // namespace ns3
