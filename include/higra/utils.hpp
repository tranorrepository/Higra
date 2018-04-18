//
// Created by user on 3/30/18.
//

#pragma once


#include <exception>
#include <string>
#include <iostream>
#include "xtensor/xio.hpp"

#define HG_DEBUG

#ifndef __FUNCTION_NAME__
#ifdef WIN32   //WINDOWS
#define __FUNCTION_NAME__   __FUNCTION__
#else          //*NIX
#define __FUNCTION_NAME__   __func__
#endif
#endif

#ifdef HG_DEBUG
#define hg_assert(test, msg) do { \
    if(!(test)) {\
    throw std::runtime_error(std::string() + __FUNCTION_NAME__ + " in file " + __FILE__ + "(line:" + std::to_string(__LINE__) + "): "  + msg);} \
  } while (0)
#else
#define hg_assert(test, msg) ((void)0)
#endif




#include <boost/preprocessor/seq/for_each.hpp>

#define HG_XSTR(a) HG_STR(a)
#define HG_STR(a) #a


#define HG_SINTEGRAL_TYPES   (char)(short)(int)(long)

#define HG_UINTEGRAL_TYPES   (unsigned char)(unsigned short)(unsigned int)(unsigned long)

#define HG_INTEGRAL_TYPES    (char)(unsigned char)(short)(unsigned short)(int)(unsigned int)(long)(unsigned long)

#define HG_FLOAT_TYPES       (float)(double)

#define HG_NUMERIC_TYPES     (char)(unsigned char)(short)(unsigned short)(int)(unsigned int)(long)(unsigned long)(float)(double)

//BOOST_PP_SEQ_FOR_EACH(f,  x, t)	f(r, x,t0) f(r, x,t1)...f(r, x,tk)
#define HG_FOREACH(f, t) BOOST_PP_SEQ_FOR_EACH(f, ~, t)