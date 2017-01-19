/*
 * Copyright (c) 2008-2010, Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#include "opt_ah.h"

#ifdef AH_SUPPORT_AR9300

#include    "ah.h"
#include    "ah_internal.h"
#include    "ar9300reg.h"

#ifdef ATH_SUPPORT_TxBF
#include    "ar9300_txbf.h"

#ifdef TXBF_TODO
const int Pn_value[8][8] = {
    {1,  1,  1,  1,  1,  1,  1,  1},
    {1,  1,  1,  1, -1, -1,  0,  0},
    {1,  1,  1, -1,  0, -1,  0, -1},
    {1,  1,  0,  0, -1,  0, -1, -1},
    {1,  1,  0, -1, -1, -1, -1,  0},
    {1,  1, -1, -1,  0,  0,  0,  0},
    {1,  0,  0,  0,  0,  0,  0,  0},
    {1,  0,  0,  0, -1, -1, -1,  0}};

COMPLEX const W3_normal[3][3] = {
    {{128, 0}, {128,    0}, {128,    0}},
    {{128, 0}, {-64,  111}, {-64, -111}},
    {{128, 0}, {-64, -111}, {-64,  111}}};

COMPLEX const W3_modify_2[3][3] = {
    {{91,  0}, { 91,    0}, { 91,    0}}, /* modified for NESS=2 osprey 1.0 */
    {{128, 0}, {-64,  111}, {-64, -111}},
    {{128, 0}, {-64, -111}, {-64,  111}}};

COMPLEX const W3_modify_1[3][3] = {
    {{128, 0}, {128,   0}, {128,    0}}, /* modified for NESS=1 osprey 1.0 */
    {{128, 0}, {-64, 111}, {-64, -111}},
    {{ 91, 0}, {-45, -78}, {-45,   78}}};

COMPLEX const W3_none[3][3] = {
    {{256, 0}, {  0, 0}, {  0, 0}},
    {{  0, 0}, {256, 0}, {  0, 0}},
    {{  0, 0}, {  0, 0}, {256, 0}}};

COMPLEX const W2_normal[2][2] = {
    {{ 128, 0}, {128, 0}},
    {{-128, 0}, {128, 0}}};

COMPLEX const W2_none[2][2] = {
    {{256, 0}, {  0, 0}},
    {{  0, 0}, {256, 0}}};

COMPLEX const ZERO = {0, 0};

COMPLEX const CSD2_TX_40M[2][128] = {{
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0}
  }, {
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49}
}};

COMPLEX const CSD2_TX_20M[2][64] = {{
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0},
    { 128, 0}, {128, 0}, {128, 0}, {128, 0}
  }, {
    { 128,    0}, { 118, -49}, { 91, -91}, { 49, -118},
    {   0, -128}, {-49, -118}, {-91, -91}, {-118, -49},
    {-128,    0}, {-118,  49}, {-91,  91}, {-49,  118},
    {   0,  128}, { 49,  118}, { 91,  91}, { 118,  49},
    { 128,    0}, { 118, -49}, { 91, -91}, { 49, -118},
    {   0, -128}, {-49, -118}, {-91, -91}, {-118, -49},
    {-128,    0}, {-118,  49}, {-91,  91}, {-49,  118},
    {   0,  128}, { 49,  118}, { 91,  91}, { 118,  49},
    { 128,    0}, { 118, -49}, { 91, -91}, { 49, -118},
    {   0, -128}, {-49, -118}, {-91, -91}, {-118, -49},
    {-128,    0}, {-118,  49}, {-91,  91}, {-49,  118},
    {   0,  128}, { 49,  118}, { 91,  91}, { 118,  49},
    { 128,    0}, { 118, -49}, { 91, -91}, { 49, -118},
    {   0, -128}, {-49, -118}, {-91, -91}, {-118, -49},
    {-128,    0}, {-118,  49}, {-91,  91}, {-49,  118},
    {   0,  128}, { 49,  118}, { 91,  91}, { 118,  49}
}};

COMPLEX const CSD3_TX_40M[3][128] = {{
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0}
  }, {
    { 128,    0}, { 126,  -25}, { 118,  -49}, { 106,  -71},
    {  91,  -91}, {  71, -106}, {  49, -118}, {  25, -126},
    {   0, -128}, { -25, -126}, { -49, -118}, { -71, -106},
    { -91,  -91}, {-106,  -71}, {-118,  -49}, {-126,  -25},
    {-128,    0}, {-126,   25}, {-118,   49}, {-106,   71},
    { -91,   91}, { -71,  106}, { -49,  118}, { -25,  126},
    {   0,  128}, {  25,  126}, {  49,  118}, {  71,  106},
    {  91,   91}, { 106,   71}, { 118,   49}, { 126,   25},
    { 128,    0}, { 126,  -25}, { 118,  -49}, { 106,  -71},
    {  91,  -91}, {  71, -106}, {  49, -118}, {  25, -126},
    {   0, -128}, { -25, -126}, { -49, -118}, { -71, -106},
    { -91,  -91}, {-106,  -71}, {-118,  -49}, {-126,  -25},
    {-128,    0}, {-126,   25}, {-118,   49}, {-106,   71},
    { -91,   91}, { -71,  106}, { -49,  118}, { -25,  126},
    {   0,  128}, {  25,  126}, {  49,  118}, {  71,  106},
    {  91,   91}, { 106,   71}, { 118,   49}, { 126,   25},
    { 128,    0}, { 126,  -25}, { 118,  -49}, { 106,  -71},
    {  91,  -91}, {  71, -106}, {  49, -118}, {  25, -126},
    {   0, -128}, { -25, -126}, { -49, -118}, { -71, -106},
    { -91,  -91}, {-106,  -71}, {-118,  -49}, {-126,  -25},
    {-128,    0}, {-126,   25}, {-118,   49}, {-106,   71},
    { -91,   91}, { -71,  106}, { -49,  118}, { -25,  126},
    {   0,  128}, {  25,  126}, {  49,  118}, {  71,  106},
    {  91,   91}, { 106,   71}, { 118,   49}, { 126,   25},
    { 128,    0}, { 126,  -25}, { 118,  -49}, { 106,  -71},
    {  91,  -91}, {  71, -106}, {  49, -118}, {  25, -126},
    {   0, -128}, { -25, -126}, { -49, -118}, { -71, -106},
    { -91,  -91}, {-106,  -71}, {-118,  -49}, {-126,  -25},
    {-128,    0}, {-126,   25}, {-118,   49}, {-106,   71},
    { -91,   91}, { -71,  106}, { -49,  118}, { -25,  126},
    {   0,  128}, {  25,  126}, {  49,  118}, {  71,  106},
    {  91,   91}, { 106,   71}, { 118,   49}, { 126,   25}
  }, {
    { 128,    0}, { 118,  -49}, {  91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, { -91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, { -91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, {  91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, {  91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, { -91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, { -91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, {  91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, {  91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, { -91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, { -91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, {  91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, {  91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, { -91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, { -91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, {  91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, {  91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, { -91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, { -91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, {  91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, {  91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, { -91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, { -91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, {  91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, {  91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, { -91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, { -91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, {  91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, {  91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, { -91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, { -91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, {  91,  91}, { 118,   49}
}};

COMPLEX const CSD3_TX_20M[3][64] = {{
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0},
    {128, 0}, {128, 0}, {128, 0}, {128, 0}
  }, {
    { 128,    0}, { 126,  -25}, { 118,  -49}, { 106,  -71},
    {  91,  -91}, {  71, -106}, {  49, -118}, {  25, -126},
    {   0, -128}, { -25, -126}, { -49, -118}, { -71, -106},
    { -91,  -91}, {-106,  -71}, {-118,  -49}, {-126,  -25},
    {-128,    0}, {-126,   25}, {-118,   49}, {-106,   71},
    { -91,   91}, { -71,  106}, { -49,  118}, { -25,  126},
    {   0,  128}, {  25,  126}, {  49,  118}, {  71,  106},
    {  91,   91}, { 106,   71}, { 118,   49}, { 126,   25},
    { 128,    0}, { 126,  -25}, { 118,  -49}, { 106,  -71},
    {  91,  -91}, {  71, -106}, {  49, -118}, {  25, -126},
    {   0, -128}, { -25, -126}, { -49, -118}, { -71, -106},
    { -91,  -91}, {-106,  -71}, {-118,  -49}, {-126,  -25},
    {-128,    0}, {-126,   25}, {-118,   49}, {-106,   71},
    { -91,   91}, { -71,  106}, { -49,  118}, { -25,  126},
    {   0,  128}, {  25,  126}, {  49,  118}, {  71,  106},
    {  91,   91}, { 106,   71}, { 118,   49}, { 126,   25}
  }, {
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49},
    { 128,    0}, { 118,  -49}, { 91, -91}, {  49, -118},
    {   0, -128}, { -49, -118}, {-91, -91}, {-118,  -49},
    {-128,    0}, {-118,   49}, {-91,  91}, { -49,  118},
    {   0,  128}, {  49,  118}, { 91,  91}, { 118,   49}
}};

const char Sin_table[64] = {
    127,  18, 90,  -91, 120, -44,  37, -122,
    127, -14, 65, -110, 106, -72,   6, -128,
    127,   2, 78, -101, 114, -58,  22, -126,
    125, -29, 51, -117,  96, -85, -10, -128,
    127,  10, 85,  -96, 117, -51,  29, -125,
    126, -22, 58, -114, 101, -78,  -2, -128,
    127,  -6, 72, -106, 110, -65,  14, -127,
    122, -37, 44, -120,  91, -90, -18, -127};

const char Cos_table[64] = {
    -18, 127,  91,  90,  44, 120, 122,  37,
     14, 127, 110,  65,  72, 106, 127,   6,
     -2, 127, 101,  78,  58, 114, 126,  22,
     29, 125, 117,  51,  85,  96, 127, -10,
    -10, 127,  96,  85,  51, 117, 125,  29,
     22, 126, 114,  58,  78, 101, 127,  -2,
      6, 127, 106,  72,  65, 110, 127,  14,
     37, 122, 120,  44,  90,  91, 127, -18};

const char PH_LUT[6] = {16, 9, 5, 3, 1, 1}; /* LUT for atan(2^-k) */


COMPLEX complex_saturate(COMPLEX input, int upper_bound, int low_bound)
{
    if (input.real > upper_bound) {
        input.real = upper_bound;
    }
    if (input.real < low_bound) {
        input.real = low_bound;
    }
    if (input.imag > upper_bound) {
        input.imag = upper_bound;
    }
    if (input.imag < low_bound) {
        input.imag = low_bound;
    }
    return input;
}

int int_saturate(int input, int upper_bound, int low_bound)
{
    if (input > upper_bound) {
        input = upper_bound;
    }
    if (input < low_bound) {
        input = low_bound;
    }
    return input;
}

int int_div_int_floor(int input, int number)
{
    int output;

    if (number == 0) {
        return (input * 10);  /* temp for test */
    }
    output = input / number;

    if (input < 0) {
        if ((output * number) != input) {
            output -= 1;
        }
    }

    return output;
}

COMPLEX complex_div_int_floor(COMPLEX input, int number)
{
    COMPLEX output;

    output.real = int_div_int_floor(input.real, number);
    output.imag = int_div_int_floor(input.imag, number);

    return output;
}

COMPLEX complex_add(COMPLEX summand, COMPLEX addend)
{
    COMPLEX result;

    result.real = summand.real + addend.real;
    result.imag = summand.imag + addend.imag;

    return result;
}

COMPLEX complex_multiply(COMPLEX multiplier, COMPLEX multiplicant)
{
    COMPLEX result;

    result.real =
        multiplier.real * multiplicant.real -
        multiplier.imag * multiplicant.imag;
    result.imag =
        multiplier.real * multiplicant.imag +
        multiplier.imag * multiplicant.real;

    return result;
}

int find_mag_approx(COMPLEX input)
{
    int result;
    int abs_i, abs_q, x, y;

    abs_i = abs(input.real);
    abs_q = abs(input.imag);

    if (abs_i > abs_q) {
        x = abs_i;
    } else {
        x = abs_q;
    }
    if (abs_i > abs_q) {
        y = abs_q;
    } else {
        y = abs_i;
    }

    result = x
      - int_div_int_floor(x, 32)
      + int_div_int_floor(y, 8)
      + int_div_int_floor(y, 4);

    return result;
}

COMPLEX complex_div_fixpt(COMPLEX numerator, COMPLEX denominator)
{
    COMPLEX result;
    int n1, d1, n2, d2;
    int div;
    int mag_num, mag_den, maxmag;

    mag_num = find_mag_approx(numerator);
    mag_den = find_mag_approx(denominator);

    numerator.real *= (1 << 9);
    numerator.imag *= (1 << 9);

    if (mag_num > mag_den) {
        maxmag = mag_num;
    } else {
        maxmag = mag_den;
    }
    numerator = complex_div_int_floor(numerator, maxmag);

    denominator.real *= (1 << 9);
    denominator.imag *= (1 << 9);
    denominator = complex_div_int_floor(denominator, maxmag);

    n1 = numerator.real;
    n2 = numerator.imag;
    d1 = denominator.real;
    d2 = denominator.imag;

    div = d1 * d1 + d2 * d2;
    if (div == 0) {
        div = 0;
        result.real = result.imag = (1 << 30) - 1; /* force to a max value */
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "==>%s:divide by 0\n", __func__);
    } else {
        /* result.real=((n1*d1+n2*d2)*(1<<10))/div; */
        result.real =
            int_div_int_floor(((n1 * d1 + n2 * d2) * (1 << 10)), div);
        /* result.imag=(((-n1)*d2+n2*d1)*(1<<10))/div; */
        result.imag =
            int_div_int_floor((((-n1) * d2 + n2 * d1) * (1 << 10)), div);
    }
    return result;
}

void
remove_3walsh(
    int nrx,
    int ntx,
    int k,
    COMPLEX (*h)[3][114],
    COMPLEX const (*w3)[3])
{
    COMPLEX h_tmp[3][3];
    int nr_idx, nc_idx, i;

    OS_MEMZERO(h_tmp, sizeof(h_tmp));

    for (nr_idx = 0; nr_idx < nrx; nr_idx++) {
        for (nc_idx = 0; nc_idx < ntx; nc_idx++) {
            for (i = 0; i < ntx; i++) {
                h_tmp[nr_idx][nc_idx] = complex_add(
                    h_tmp[nr_idx][nc_idx],
                    complex_multiply(h[nr_idx][i][k], w3[i][nc_idx]));
            }
        }
    }
    for (nr_idx = 0; nr_idx < nrx; nr_idx++) {
        for (nc_idx = 0; nc_idx < ntx; nc_idx++) {
            h[nr_idx][nc_idx][k].real = h_tmp[nr_idx][nc_idx].real;
            h[nr_idx][nc_idx][k].imag = h_tmp[nr_idx][nc_idx].imag;
        }
    }
}

void
remove_2walsh(
    int nrx,
    int ntx,
    int k,
    COMPLEX (*h)[3][114],
    COMPLEX const (*w2)[2])
{
    COMPLEX h_tmp[3][3];
    int nr_idx, nc_idx, i;

    OS_MEMZERO(h_tmp, sizeof(h_tmp));

    for (nr_idx = 0; nr_idx < nrx; nr_idx++) {
        for (nc_idx = 0; nc_idx < ntx; nc_idx++) {
            for (i = 0; i < ntx; i++) {
                h_tmp[nr_idx][nc_idx] = complex_add(
                    h_tmp[nr_idx][nc_idx],
                    complex_multiply(h[nr_idx][i][k], w2[i][nc_idx]));
            }
        }
    }
    for (nr_idx = 0; nr_idx < nrx; nr_idx++) {
        for (nc_idx = 0; nc_idx < ntx; nc_idx++) {
            h[nr_idx][nc_idx][k].real = h_tmp[nr_idx][nc_idx].real;
            h[nr_idx][nc_idx][k].imag = h_tmp[nr_idx][nc_idx].imag;
        }
    }
}

bool
rc_interp(COMPLEX (*ka)[TONE_40M], int8_t n_tone)
{
    int8_t i, j, null_cnt, valid_cnt;
    int8_t valid[114], null[114], valid_idx, null_idx, prev_idx;
    COMPLEX temp;

    for (i = 0; i < 3; i++) {
        null_cnt = valid_cnt = 0;
        for (j = 0; j < n_tone; j++) {
            if ((ka[i][j].real == 0) && (ka[i][j].imag == 0)) {
                null[null_cnt] = j;
                null_cnt++;
            } else {
                valid[valid_cnt] = j;
                valid_cnt++;
            }
        }
        if (null_cnt != 0) {
            if (valid_cnt == 0) {
                return false;
            } else {
                valid_idx = null_idx = 0;
                for (j = 0; j < n_tone; j++) {
                    if (null[null_idx] == j) {
                        if (valid_idx < valid_cnt) {
                            temp.real = valid[valid_idx] - j;
                            temp.imag = 0;
                            prev_idx = j - 1;
                            if (prev_idx < 0) {
                                prev_idx = 0;
                            }
                            ka[i][j] = complex_add(
                                complex_multiply(ka[i][prev_idx], temp),
                                ka[i][valid[valid_idx]]);
                            ka[i][j] = complex_div_int_floor(
                                ka[i][j], (valid[valid_idx] - j + 1));
                        } else {
                            ka[i][j] = ka[i][valid[valid_idx-1]];
                        }

                        null_idx++;
                        if (null_idx == null_cnt) {
                            break; /* no next null */
                        }
                    } else {
                        valid_idx++;
                    }
                }
            }
        }
    }
    return true;
}

/* convolution filter */
void
conv_fir(struct ath_hal *ah, COMPLEX *in, int8_t length)
{
    int8_t const filter_coef[7] = {-8, 16, 36, 40, 36, 16, -8};
    int i, j;
    COMPLEX *in_tmp;

    in_tmp = ath_hal_malloc(ah, (length + 12) * sizeof(COMPLEX));

    for (i = 0; i < length + 12; i++) {
        if (i < 6) {
            in_tmp[i] = ZERO;
        } else if ((i >= 6) && (i < 6 + length)) {
            in_tmp[i] = in[i - 6];
        } else if (i >= 6 + length) {
            in_tmp[i] = ZERO;
        }
    }
    for (i = 0; i < length + 6; i++) {
        in[i] = ZERO;
        for (j = 0; j < 7; j++) {
            in[i].real += filter_coef[6 - j] * in_tmp[i + j].real;
            in[i].imag += filter_coef[6 - j] * in_tmp[i + j].imag;
        }
    }
    ath_hal_free(ah, in_tmp);
}

/* apply channel filtering to radio coefficients */
void
rc_smoothing(struct ath_hal *ah, COMPLEX (*ka)[TONE_40M], int8_t bw)
{
    int8_t n_tone;
    int i, j;
    COMPLEX *rc_tmp, *rc_tmp1;

    if (bw == BW_40M) { /* 40M */
        n_tone = TONE_40M;
    } else {
        n_tone = TONE_20M;
    }

    rc_tmp = ath_hal_malloc(ah, n_tone * sizeof(COMPLEX));
    rc_tmp1 = ath_hal_malloc(ah, (n_tone + 9 + 6) * sizeof(COMPLEX));

    for (i = 1; i < 3; i++) {
        for (j = 0; j < n_tone; j++) {
            rc_tmp[(j + n_tone / 2) % n_tone] = ka[i][j];
        }
        if (bw == BW_40M) { /* 40M */
            /*
             * rc_tmp = [
             *     rc_tmp([4 3 2])
             *     rc_tmp(1:57)
             *     rc_tmp(57)
             *     rc_tmp(57)
             *     rc_tmp(57)
             *     rc_tmp(58:114)
             *     rc_tmp([113 112 111])];
             */
            for (j = 0; j < n_tone + 9; j++) {
                if (j < 3) {
                    rc_tmp1[j] = rc_tmp[3 - j];
                } else if ((j >= 3) && (j < 60)) {
                    rc_tmp1[j] = rc_tmp[j - 3];
                } else if ((j >= 60) && (j < 63)) {
                    rc_tmp1[j] = rc_tmp[56];
                } else if ((j >= 63) && (j < 120)) {
                    rc_tmp1[j] = rc_tmp[j - 6];
                } else if ((j >= 120) && (j < 123)) {
                    rc_tmp1[j] = rc_tmp[232 - j];
                }
            }
            conv_fir(ah, rc_tmp1, 123);
            for (j = 0; j < n_tone; j++) {
                if (j <= 56) {
                    rc_tmp[j] = rc_tmp1[j + 6];
                } else {
                    rc_tmp[j] = rc_tmp1[j + 9];
                }
            }
        } else { /* 20M */
            /*
             * [rc_tmp([4 3 2])
             *  rc_tmp(1:28)
             *  rc_tmp(28)
             *  rc_tmp(29:56)
             *  rc_tmp([55 54 53])];
             */
            for (j = 0; j < n_tone + 7; j++) {
                if (j < 3) {
                    rc_tmp1[j] = rc_tmp[3 - j];
                } else if ((j >= 3) && (j < 31)) {
                    rc_tmp1[j] = rc_tmp[j - 3];
                } else if (j == 31) {
                    rc_tmp1[j] = rc_tmp[27];
                } else if ((j > 31) && (j < 60)) {
                    rc_tmp1[j] = rc_tmp[j - 4];
                } else if (j >= 60) {
                    rc_tmp1[j] = rc_tmp[114 - j];
                }
            }
            conv_fir(ah, rc_tmp1, 63);
            for (j = 0; j < n_tone; j++) {
                if (j < 28) {
                    rc_tmp[j] = rc_tmp1[j + 6];
                } else {
                    rc_tmp[j] = rc_tmp1[j + 7];
                }
            }
        }
        for (j = 0; j < n_tone; j++) {
            ka[i][j] = complex_saturate(
                complex_div_int_floor(rc_tmp[(j + n_tone / 2) % n_tone], 128),
                ((1 << 7) - 1),
                -(1 << 7));
        }
    }
    ath_hal_free(ah, rc_tmp1);
    ath_hal_free(ah, rc_tmp);
}

/*
 *  A: BFer
 *  B: BFee
 *  ______________            ______________
 * | (A)          |          |          (B) |
 * | ka --> Atx --|->  Hab --|-> Brx        |
 * |              |          |              |
 * |        Arx <-|--  Hba <-|-- Btx <-- Kb |
 * |______________|          |______________|
 */
bool
ka_calculation(
    struct ath_hal *ah,
    int8_t n_tx_a,
    int8_t n_rx_a,
    int8_t n_tx_b,
    int8_t n_rx_b,
    COMPLEX (*h_ba_eff)[3][TONE_40M],
    COMPLEX (*h_eff_quan)[3][TONE_40M],
    u_int8_t *m_h,
    COMPLEX (*ka)[TONE_40M],
    u_int8_t n_ess_a,
    u_int8_t n_ess_b,
    int8_t bw)
{
    int8_t        i, j, k, nr_idx, nc_idx, min_b, n_tone;
    COMPLEX       h_tmp[3][3], csd_tmp[3][3];
    int8_t        used_bins[TONE_40M];
    COMPLEX       (*h_ab_eff)[3][TONE_40M];
    COMPLEX       a[6];
    bool          a_valid[6];
    u_int8_t      ka1_valid, ka2_valid;
    COMPLEX       ka_1, ka_2;
    int           mag;
    COMPLEX const (*w3a)[3], (*w3b)[3];
    COMPLEX const (*w2a)[2], (*w2b)[2];
    bool          status = true;

    h_ab_eff = ath_hal_malloc(ah, 3 * 3 * TONE_40M * sizeof(COMPLEX));
    if (bw == BW_40M) { /* 40M */
        j = -58;
        n_tone = TONE_40M;
        for (i = 0; i < n_tone; i++) {
            /* use_bin=mod(j,128)+1; */
            if (j < 0) {
                used_bins[i] = j + 128;
            } else {
                used_bins[i] = j;
            }
            j++;
            if (j == -1) {
                j = 2;
            }
        }
    } else { /* 20M */
        j = -28;
        n_tone = TONE_20M;
        for (i = 0; i < n_tone; i++) {
            /* use_bin=mod(j,64)+1; */
            if (j < 0) {
                used_bins[i] = j + 64;
            } else {
                used_bins[i] = j;
            }
            j++;
            if (j == 0) {
                j = 1;
            }
        }
    }
    OS_MEMZERO(h_ab_eff, 3 * 3 * TONE_40M * sizeof(COMPLEX));

    if (n_tx_b > n_rx_b) {
        min_b = n_rx_b;
    } else {
        min_b = n_tx_b;
    }

    if (n_ess_a == 0) {
        w3a = W3_none;   /* no walsh */
        w2a = W2_none;
    } else {
        w2a = W2_normal;
        w3a = W3_normal; /* should use corrected when osprey 1.0 and NESS=1 */
    }

    if (n_ess_b == 0) {
        w3b = W3_none;   /* no walsh */
        w2b = W2_none;
    } else {
        w2b = W2_normal;
        w3b = W3_normal; /* should use corrected when osprey 1.0 and NESS=1 */
    }

#if DEBUG_KA
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "==>%s:m_h:", __func__);

    for (k = 0; k < n_tone; k++) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, " %d,", m_h[k]);
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
#endif

    for (k = 0; k < n_tone; k++) {
        /* CSI2 to H */
        for (j = 0; j < NUM_ITER; j++) {
            for (nr_idx = 0; nr_idx < n_rx_b; nr_idx++) {
                for (nc_idx = 0; nc_idx < n_tx_a; nc_idx++) {
                    h_ab_eff[nr_idx][nc_idx][k].real +=
                        Pn_value[m_h[k]][j] *
                        (h_eff_quan[nr_idx][nc_idx][k].real >> (j + 1));
                    h_ab_eff[nr_idx][nc_idx][k].imag +=
                        Pn_value[m_h[k]][j] *
                        (h_eff_quan[nr_idx][nc_idx][k].imag >> (j + 1));
                }
            }
        }

#if DEBUG_KA
        if (k == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "==>%s: original h_ab_eff for tone 0:", __func__);
            for (nr_idx = 0; nr_idx < n_rx_b; nr_idx++) {
                for (nc_idx = 0; nc_idx < n_tx_a; nc_idx++) {
                    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                        " %d, %d;",
                        h_ab_eff[nr_idx][nc_idx][k].real,
                        h_ab_eff[nr_idx][nc_idx][k].imag);
                }
            }
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
        }
#endif

        /* remove walsh_A of h_ab_eff */
        if (n_tx_a == 3) {
            remove_3walsh(n_rx_b, n_tx_a, k, h_ab_eff, w3a);
        } else if (n_tx_a == 2) {
            remove_2walsh(n_rx_b, n_tx_a, k, h_ab_eff, w2a);
        }
#if DEBUG_KA
        if (k == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "==>%s:h_ab_eff for tone 0 after remove Walsh:", __func__);
            for (nr_idx = 0; nr_idx < n_rx_b; nr_idx++) {
                for (nc_idx = 0; nc_idx < n_tx_a; nc_idx++) {
                    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                        " %d, %d;",
                        h_ab_eff[nr_idx][nc_idx][k].real,
                        h_ab_eff[nr_idx][nc_idx][k].imag);
                }
            }
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
        }
#endif

        /* remove csd of h_ab_eff */
        OS_MEMZERO(h_tmp, sizeof(h_tmp));
        OS_MEMZERO(csd_tmp, sizeof(csd_tmp)); /* diag(csd2(:,used_bins(k))); */
        if (n_ess_a != 0) { /* not regular sounding */
            if (bw == BW_40M) { /* 40M */
                if (n_tx_a == 3) {
                    for (i = 0; i < n_tx_a; i++) {
                        csd_tmp[i][i].real = CSD3_TX_40M[i][used_bins[k]].real;
                        csd_tmp[i][i].imag = CSD3_TX_40M[i][used_bins[k]].imag;
                    }
                } else if (n_tx_a == 2) {
                    for (i = 0; i < n_tx_a; i++) {
                        csd_tmp[i][i].real = CSD2_TX_40M[i][used_bins[k]].real;
                        csd_tmp[i][i].imag = CSD2_TX_40M[i][used_bins[k]].imag;
                    }
                }
            } else { /* 20M */
                if (n_tx_a == 3) {
                    for (i = 0; i < n_tx_a; i++) {
                        csd_tmp[i][i].real = CSD3_TX_20M[i][used_bins[k]].real;
                        csd_tmp[i][i].imag = CSD3_TX_20M[i][used_bins[k]].imag;
                    }
                } else if (n_tx_a == 2) {
                    for (i = 0; i < n_tx_a; i++) {
                        csd_tmp[i][i].real = CSD2_TX_20M[i][used_bins[k]].real;
                        csd_tmp[i][i].imag = CSD2_TX_20M[i][used_bins[k]].imag;
                    }
                }
            }
        } else {
            for (i = 0; i < n_tx_a; i++) {
                csd_tmp[i][i].real = 128;
            }
        }

        for (nr_idx = 0; nr_idx < n_rx_b; nr_idx++) {
            for (nc_idx = 0; nc_idx < n_tx_a; nc_idx++) {
                for (i = 0; i < n_tx_a; i++) {
                    h_tmp[nr_idx][nc_idx] =
                        complex_add(h_tmp[nr_idx][nc_idx],
                            complex_multiply(
                                h_ab_eff[nr_idx][i][k], csd_tmp[i][nc_idx]));
                }
                h_ab_eff[nr_idx][nc_idx][k] =
                    complex_div_int_floor(h_tmp[nr_idx][nc_idx], (1 << 13));
            }
        }
#if DEBUG_KA
        if (k == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "==>%s:h_ab_eff for tone 0 after remove CSD:", __func__);
            for (nr_idx = 0; nr_idx < n_rx_b; nr_idx++) {
                for (nc_idx = 0; nc_idx < n_tx_a; nc_idx++) {
                    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                        " %d, %d;",
                        h_ab_eff[nr_idx][nc_idx][k].real,
                        h_ab_eff[nr_idx][nc_idx][k].imag);
                    }
            }
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
        }

        if (k == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "==>%s:original h_ba_eff for tone 0:", __func__);
            for (nr_idx = 0; nr_idx < n_rx_b; nr_idx++) {
                for (nc_idx = 0; nc_idx < n_tx_a; nc_idx++) {
                    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                        " %d, %d;",
                        h_ba_eff[nr_idx][nc_idx][k].real,
                        h_ba_eff[nr_idx][nc_idx][k].imag);
                    }
            }
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
        }
#endif

        if (min_b == 3) {
            remove_3walsh(n_rx_a, n_tx_b, k, h_ba_eff, w3b);
        } else if (min_b == 2) {
            remove_2walsh(n_rx_a, n_tx_b, k, h_ba_eff, w2b);
        }

        /* adjust format */
        if (min_b != 1) {
            for (nr_idx = 0; nr_idx < n_rx_a; nr_idx++) {
                for (nc_idx = 0; nc_idx < n_tx_b; nc_idx++) {
                    h_ba_eff[nr_idx][nc_idx][k] = complex_div_int_floor(
                        h_ba_eff[nr_idx][nc_idx][k], (1 << 6));
                }
            }
        }

#if DEBUG_KA
        if (k == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "==>%s:h_ba_eff for tone 0 after remove Walsh:", __func__);
            for (nr_idx = 0; nr_idx < n_rx_b; nr_idx++) {
                for (nc_idx = 0; nc_idx < n_tx_a; nc_idx++) {
                    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                        " %d, %d;",
                        h_ba_eff[nr_idx][nc_idx][k].real,
                        h_ba_eff[nr_idx][nc_idx][k].imag);
                }
            }
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
        }
#endif

        OS_MEMZERO(&ka_1, sizeof(COMPLEX));
        OS_MEMZERO(&ka_2, sizeof(COMPLEX));
        ka1_valid = 0;
        ka2_valid = 0;
        for (i = 0; i < min_b; i++) { /* 3 set of antenna */
            a[2 * i] = complex_div_fixpt(
                complex_multiply(h_ab_eff[i][1][k], h_ba_eff[0][i][k]),
                complex_multiply(h_ab_eff[i][0][k], h_ba_eff[1][i][k]));
            mag = find_mag_approx(a[2 * i]);
            a_valid[2 * i] = (mag > (RC_MIN)) & (mag < (RC_MAX));
            ka1_valid += a_valid[2 * i];

            if (n_tx_a == 3) { /* calculate ka2 */
                a[2 * i + 1] = complex_div_fixpt(
                    complex_multiply(h_ab_eff[i][2][k], h_ba_eff[0][i][k]),
                    complex_multiply(h_ab_eff[i][0][k], h_ba_eff[2][i][k]));
                mag = find_mag_approx(a[2 * i + 1]);
                a_valid[2 * i + 1] = (mag > (RC_MIN)) & (mag < (RC_MAX));
                ka2_valid += a_valid[2 * i + 1];
            }
        }

#if DEBUG_KA
        if (k == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "==>%s: original ka and valid for tone 0 :", __func__);
            for (i = 0; i < 6; i++) {
                HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                    "ka %d,%d,valid %d;", a[i].real, a[i].imag, a_valid[i]);
            }
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
        }
#endif

        /* average valid ka */
        for (i = 0; i < min_b; i++) {
            if (a_valid[2 * i]) {
                ka_1 = complex_add(ka_1, a[2 * i]);
            }
            if (n_tx_a == 3) {
                if (a_valid[2 * i + 1]) {
                    ka_2 = complex_add(ka_2, a[2 * i + 1]);
                }
            }
        }
        if (ka1_valid == 0) {
            ka_1 = ZERO;
            status = false;
        } else {
            ka_1 = complex_div_int_floor(ka_1, ka1_valid);
        }
        if (n_tx_a == 3) {
            if (ka2_valid == 0) {
                ka_2 = ZERO;
                status = false;
            } else {
                ka_2 = complex_div_int_floor(ka_2, ka2_valid);
            }
        }

        ka[0][k].real = 1024;
        ka[1][k] = ka_1;
        ka[2][k] = ka_2;

#if DEBUG_KA
        if (k == 0) {
            HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
                "==>%s:ka_1 %d %d;ka_2 %d %d\n",
                __func__, ka_1.real, ka_1.imag, ka_2.real, ka_2.imag);
        }
#endif
        if (CAL_GAIN == 0) {
            COMPLEX cal_tmp;

            cal_tmp.real = 1 << 7;
            cal_tmp.imag = 0;

            for (i = 0; i < 3; i++) {
                mag = find_mag_approx(ka[i][k]);

                if (mag == 0) {
                    ka[i][k] = ZERO;
                } else {
                    ka[i][k] = complex_div_int_floor(
                        complex_multiply(ka[i][k], cal_tmp), mag);
                }
            }
        } else {
            ka[0][k].real /= 16;
            ka[1][k] = complex_div_int_floor(ka_1, 16);
            if (n_tx_a == 3) {
                ka[2][k] = complex_div_int_floor(ka_2, 16);
            }
        }
        for (i = 0; i < min_b; i++) {
            /* saturation check ; check for ka_1 ka_2 */
            ka[i][k] = complex_saturate(ka[i][k], ((1 << 7) - 1), -(1 << 7));
        }
    }

#if DEBUG_KA
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "%s==> ka of tone 0 after calculation:", __func__);
    for (i = 0; i < 3; i++) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "%d,%d;", ka[i][0].real, ka[i][0].imag);
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
#endif

    ath_hal_free(ah, h_ab_eff);
    if (status == false) {
        status = rc_interp(ka, n_tone);
    }

#if DEBUG_KA
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "%s==>ka of tone 0 after interp:", __func__);
    for (i = 0; i < 3; i++) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "%d,%d;", ka[i][0].real, ka[i][0].imag);
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
#endif

    if (status == false) {
        return status;
    }

    if (SMOOTH) {
        rc_smoothing(ah, ka, bw);
    }

#if DEBUG_KA
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
        "%s==>ka of tone 0 after smoothing:", __func__);
    for (i = 0; i < 3; i++) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "%d,%d;", ka[i][0].real, ka[i][0].imag);
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
#endif

    for (k = 0; k < n_tone; k++) {
        for (i = 0; i < min_b; i++) {
            /* saturation check again */
            ka[i][k] = complex_saturate(ka[i][k], ((1 << 7) - 1), -(1 << 7));
        }
    }

#if DEBUG_KA
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "%s==>final ka of tone 0:", __func__);
    for (i = 0; i < 3; i++) {
        HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE,
            "%d,%d;", ka[i][0].real, ka[i][0].imag);
    }
    HDPRINTF(AH_NULL, HAL_DBG_UNMASKABLE, "\n");
#endif

    return  true;
}

void
cordic_rot(int x, int y, int *x_out, int *phase, int8_t *ph_idx, int8_t *signx)
{
    int8_t sign;
    int phase_acc;
    u_int8_t i;
    int xtemp, ytemp;

    if (x < 0) {
        *signx = -1;
    } else {
        *signx = 1;
    }

    x = (*signx) * x; /* abs */
    phase_acc = 0;
    *ph_idx = 0;
    for (i = 0; i < NUM_ITER_V; i++) {
        if (y < 0) {
            sign = 1;
        } else {
            sign = -1; /* sign of next iter. */
        }
        xtemp = x;
        ytemp = y;
        x = xtemp - sign * int_div_int_floor(ytemp, (1 << i));
        y = ytemp + sign * int_div_int_floor(xtemp, (1 << i));
        phase_acc -= phase_acc-sign * PH_LUT[i];
        x = int_saturate(x, (1 << NB_CORIDC) - 1, -(1 << NB_CORIDC));
        y = int_saturate(y, (1 << NB_CORIDC) - 1, -(1 << NB_CORIDC));
        if (sign == 1) {
            *ph_idx += (1 << i);
        }
    }
    *x_out =
        int_div_int_floor(x, 2) +
        int_div_int_floor(x, 8) -
        int_div_int_floor(x, 64);
    if (*signx == -1) {
        *phase = -phase_acc + (1 << (NB_PH + 1));
    } else if (phase_acc >= 0) {
        *phase = phase_acc;
    } else {
        *phase = phase_acc + (1 << (NB_PH + 2));
    }
}

void
compress_v(
    COMPLEX (*v)[3],
    u_int8_t nr,
    u_int8_t nc,
    u_int8_t *phi,
    u_int8_t *psi)
{
    int nr_idx, nc_idx;
    int x, y;
    int x_out, phase;
    int8_t ph_idx, signx;
    int phi_x, phi_y;
    COMPLEX rot;
    int i, j;
    int phi_idx = 0, psi_idx = 0;
    COMPLEX v_temp[3][3];
    COMPLEX temp, temp1, temp2;

    for (i = 0; i < 3; i++) {
        phi[i] = 0;
        psi[i] = 0;
    }
    /* rot least row to x axis */
    for (nc_idx = 0; nc_idx < nc; nc_idx++) {
        x = v[nr-1][nc_idx].real;
        y = v[nr-1][nc_idx].imag;
        cordic_rot(x, y, &x_out, &phase, &ph_idx, &signx);
        v[nr-1][nc_idx].real = x_out;
        v[nr-1][nc_idx].imag = 0;
        phi_x = signx*Cos_table[ph_idx];
        phi_y = Sin_table[ph_idx];
        phi_x = int_saturate(phi_x, (1 << NB_SIN) - 1, -(1 << NB_SIN));
        rot.real = phi_x;
        rot.imag = -phi_y;

        /* floor(v(1:end-1,col_idx)*rot*2^-NB_SIN+0.5+0.5i); */
        for (i = 0; i < nr - 1; i++) {
            v[i][nc_idx] = complex_multiply(v[i][nc_idx], rot);
            v[i][nc_idx].real += (1 << NB_SIN) / 2;
            v[i][nc_idx].imag += (1 << NB_SIN) / 2;
            v[i][nc_idx] = complex_div_int_floor(v[i][nc_idx], 1 << NB_SIN);
            v[i][nc_idx] = complex_saturate(
                v[i][nc_idx], (1 << (NB_PHIN - 2)) - 1, -(1 << (NB_PHIN - 2)));
        }
    }

    for (nc_idx = 0; nc_idx < nc; nc_idx++) {
        /* find phi */
        for (nr_idx = nc_idx; nr_idx < nr - 1; nr_idx++) {
            x = v[nr_idx][nc_idx].real;
            y = v[nr_idx][nc_idx].imag;
            cordic_rot(x, y, &x_out, &phase, &ph_idx, &signx);
            v[nr_idx][nc_idx].real = x_out;
            v[nr_idx][nc_idx].imag = 0;
            v[i][nc_idx] = complex_saturate(
                v[i][nc_idx], (1 << NB_PHIN) - 1, 1 << NB_PHIN);
            phi_x = signx * Cos_table[ph_idx];
            phi_y = Sin_table[ph_idx];
            phi_x = int_saturate(phi_x, (1 << NB_SIN) - 1, -(1 << NB_SIN));
            rot.real = phi_x;
            rot.imag = -phi_y;

            /*
            v(row_idx,col_idx+1:end) = floor(
                v(row_idx,col_idx+1:end)*rot*2^-NB_SIN+0.5+0.5i);
             */
            for (i = nc_idx + 1; i < nc; i++) {
                v[nr_idx][i] = complex_multiply(v[nr_idx][i], rot);
                v[nr_idx][i].real += (1 << NB_SIN) / 2;
                v[nr_idx][i].imag += (1 << NB_SIN) / 2;
                v[nr_idx][i] = complex_div_int_floor(v[nr_idx][i], 1 << NB_SIN);
                v[nr_idx][i] = complex_saturate(
                    v[nr_idx][i],
                    (1 << (NB_PHIN - 2)) - 1,
                    -(1 << (NB_PHIN - 2)));
            }
            phi[phi_idx] = phase;
            phi_idx++;
        }

        /* find psi */
        for (i = 0; i < nc; i++) {
            for (j = 0; j < nr; j++) {
                v_temp[j][i].real = v[j][i].real;
                v_temp[j][i].imag = v[j][i].imag;
            }
        }

        for (nr_idx = nc_idx + 1; nr_idx < nr; nr_idx++) {
            x = v[nc_idx][nc_idx].real;
            y = v[nr_idx][nc_idx].real;
            cordic_rot(x, y, &x_out, &phase, &ph_idx, &signx);
            v_temp[nc_idx][nc_idx].real = x_out;
            v_temp[nc_idx][nc_idx].imag = 0;
            v_temp[nr_idx][nc_idx].real = 0;
            v_temp[nr_idx][nc_idx].imag = 0;
            /*
            v_temp(col_idx,col_idx+1:end) =
                floor((v_tilde(col_idx,col_idx+1:end)
                       * signx*Cos_table(ph_idx)
                       + v_tilde(row_idx,col_idx+1:end)
                       * Sin_table(ph_idx))
                      * 2 ^ -NB_SIN + 0.5 + 0.5i);
             */
            temp1.real = signx * Cos_table[ph_idx];
            temp1.imag = 0;
            temp2.real = Sin_table[ph_idx];
            temp2.imag = 0;
            for (i = nc_idx + 1; i < nc; i++) {
                temp = complex_add(
                    complex_multiply(v[nc_idx][i], temp1),
                    complex_multiply(v[nr_idx][i], temp2));
                temp.real += (1 << NB_SIN) / 2;
                temp.imag += (1 << NB_SIN) / 2;
                v_temp[nc_idx][i] = complex_div_int_floor(temp, 1 << NB_SIN);
            }
            /*
            v_temp(row_idx,col_idx+1:end) =
                floor((v_tilde(col_idx,col_idx+1:end)
                       * -Sin_table(ph_idx)
                       + v_tilde(row_idx,col_idx+1:end)
                       * signx*Cos_table(ph_idx))
                      * 2 ^ -NB_SIN + 0.5 + 0.5i);
             */
            temp1.real = -Sin_table[ph_idx];
            temp1.imag = 0;
            temp2.real = signx * Cos_table[ph_idx];
            temp2.imag = 0;
            for (i = nc_idx + 1; i < nc; i++) {
                temp = complex_add(
                    complex_multiply(v[nc_idx][i], temp1),
                    complex_multiply(v[nr_idx][i], temp2));
                temp.real += (1 << NB_SIN) / 2;
                temp.imag += (1 << NB_SIN) / 2;
                v_temp[nr_idx][i] = complex_div_int_floor(temp, 1 << NB_SIN);
            }
            /*
            v_tmp(col_idx,col_idx+1:end) =
                int_saturate(v_tmp(col_idx,col_idx+1:end),
                    2^(NB_PHIN-2)-1,-2^(NB_PHIN-2));
            v_tmp(row_idx,col_idx+1:end) =
                int_saturate(v_tmp(row_idx,col_idx+1:end),
                    2^(NB_PHIN)-1,-2^(NB_PHIN));
             */
            for (i = nc_idx + 1; i < nc; i++) {
                v_temp[nc_idx][i] = complex_saturate(
                    v_temp[nc_idx][i],
                    (1 << (NB_PHIN - 2)) - 1,
                    -(1 << (NB_PHIN - 2)));
                v_temp[nr_idx][i] = complex_saturate(
                    v_temp[nr_idx][i],
                    (1 << NB_PHIN) - 1,
                    -(1 << NB_PHIN));
            }

            for (i = 0; i < nc; i++) {
                for (j = 0; j < nr; j++) {
                    v[j][i].real = v_temp[j][i].real;
                    v[j][i].imag = v_temp[j][i].imag;
                }
            }

            if (phase > (1 << (NB_PH + 1))) {
                phase = 0;
            } else if (phase >= (1 << NB_PH)) {
                phase = 1 << NB_PH;
            }
            psi[psi_idx] = phase;
            psi_idx++;
        }
    }
    if ((NB_PSI - NB_PH) < 0) {
        for (i = 0; i < 3; i++) {
            phi[i] = int_div_int_floor(phi[i], 1 << (NB_PH - NB_PSI));
            psi[i] = int_div_int_floor(psi[i], 1 << (NB_PH - NB_PSI));
        }
    }
}

void
h_to_csi(
    struct ath_hal *ah,
    u_int8_t n_tx,
    u_int8_t n_rx,
    COMPLEX (*h)[3][TONE_40M],
    u_int8_t *m_h,
    u_int8_t n_tone)
{
    u_int8_t i, j, k;
    u_int8_t const r_inv_quan[8] = {127, 114, 102, 91, 81, 72, 64, 57};
    int q_m_h[8];
    int max, *max_h, tmp, *m_h_linear;
    COMPLEX temp;

    max_h = ath_hal_malloc(ah, n_tone * sizeof(int));
    m_h_linear = ath_hal_malloc(ah, n_tone * sizeof(int));
    max = 0;
    for (k = 0; k < n_tone; k++) {
        max_h[k] = 0;
        for (j = 0; j < n_tx; j++) {
            /* search the max point of each tone */
            for (i = 0; i < n_rx; i++) {
                tmp = h[i][j][k].real;
                if (h[i][j][k].real < 0) {
                    tmp = -h[i][j][k].real;
                }
                if (tmp>max_h[k]) {
                    max_h[k] = tmp;
                }
                tmp = h[i][j][k].imag;
                if (h[i][j][k].imag < 0) {
                    tmp = -h[i][j][k].imag;
                }
                if (tmp > max_h[k]) {
                    max_h[k] = tmp;
                }
            }
        }
        if (max_h[k] > max) {
            max = max_h[k];
        }
    }

    for (i = 0; i < 8; i++) {
        q_m_h[i] = max * r_inv_quan[i] + (1 << 6);
        q_m_h[i] = int_div_int_floor(q_m_h[i], 1 << 7);
        q_m_h[i] = int_saturate(q_m_h[i], ((1 << 9) - 1), -(1 << 9));
    }
    for (k = 0; k < n_tone; k++) {
        for (i = 0; i < 8; i++) {
            if (q_m_h[7 - i] >= max_h[k]) {
                break;
            }
        }
        if (i == 8) {
            m_h[k] = 0;
        } else {
            m_h[k] = 7 - i;
        }
        m_h_linear[k] = q_m_h[m_h[k]];
        /* inverse the m_h_linear */
        tmp = m_h_linear[k] / 2 + (1 << NB_MHINV);
        m_h_linear[k] = int_div_int_floor(tmp, m_h_linear[k]);
        m_h_linear[k] = int_saturate(
            m_h_linear[k], ((1 << (NB_MHINV - 6)) - 1), -(1 << (NB_MHINV - 6)));
        /*
        H_quantized_FB(:,:,k) = floor(
            H_per_tone * m_h_linear_inv(k) *
            2 ^ (Nb - 1 - NB_MHINV) + 0.5 + 0.5 * sqrt(-1))
         */
        temp.real = 32;    /* 2^(Nb-1-NB_MHINV) * 0.5 */
        temp.imag = 32;
        for (j = 0; j < n_tx; j++) {
            for (i = 0; i < n_rx; i++) {
                /* search the max point of each tone */
                h[i][j][k].real *= m_h_linear[k];
                h[i][j][k].imag *= m_h_linear[k];
                h[i][j][k] = complex_add(h[i][j][k], temp);
                h[i][j][k] = complex_div_int_floor(h[i][j][k], 64);
                h[i][j][k] =
                    complex_saturate(h[i][j][k], ((1 << 7) - 1), -(1 << 7));
             }
        }
    }
    ath_hal_free(ah, max_h);
    ath_hal_free(ah, m_h_linear);
}
#endif

#endif /* ATH_SUPPORT_TxBF*/
#endif /* AH_SUPPORT_AR9300 */
