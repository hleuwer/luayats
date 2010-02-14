/*************************************************************************
*
*		YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997	Chair for Telecommunications
*				Dresden University of Technology
*				D-01062 Dresden
*				Germany
*
**************************************************************************
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
**************************************************************************
*
*	Module author:		Matthias Baumann, TUD
*	Creation:		Nvo 14, 1997
*
*	History:	
*
*************************************************************************/

/*
*	A statistics object: calculation of confidence intervals
*
*	ConfidObj x {: LEVEL=0.99};
*		// LEVEL of confidence, default: 0.95
*
*	Commands:
*
*	void	x->Add(double)		// add a value
*	int	x->Len			// get current # of values
*	double	x->Val(int)		// get a value (1 ... Len)
*					// do not export: values-pointer may change!
*
*	void	x->Flush		// flush all values (Len := 0)
*
*	double	x->Mean			// mean
*	double	x->Var			// *empirical* variance
*	double	x->Lo			// lower bound of convid interv
*	double	x->Up			// upper bound of convid interv
*	double	x->Width		// Up - Mean
*	double	x->Lo(double)		// level given, may differ from LEVEL
*	double	x->Up(double)		// level given, may differ from LEVEL
*	double	x->Width(double)	// level given, may differ from LEVEL
*       double x->FairInd     	        // Fairnessindex (Jain)
*/

#include "defs.h"
#include "confid.h"

confidObj::confidObj()
{
  vectLen = vectGran;
  CHECK(values = new double [vectLen]);
  nVals = 0;
}

confidObj::~confidObj()
{
  delete[] values;
}

void confidObj::add(double v)
{
  values[nVals++] = v;
  if (nVals >= vectLen){
    double *tmp;
    vectLen += vectGran;
    CHECK(tmp = new double [vectLen]);
    for (int i = 0; i < nVals; ++i)
      tmp[i] = values[i];
    delete[] values;
    values = tmp;
  }
}

double confidObj::getMean(double *pv)
{
  double pm;
  meanVar(&pm, pv);
  return pm;
}

double confidObj::getVar(double *pm)
{
  double pv;
  meanVar(pm, &pv);
  return pv;
}

double confidObj::getLo(double lev)
{
  double mean, width;
  calcConf(lev, &mean, &width);
  return mean - width;
}

double confidObj::getUp(double lev)
{
  double mean, width;
  calcConf(lev, &mean, &width);
  return mean + width;
}

double confidObj::getWidth(double lev)
{
  double mean, width;
  calcConf(lev, &mean, &width);
  return width;
}

double confidObj::getMin(void)
{
  double min;
  int i;
		
  if(nVals <= 0)
    min = 0;
  else if(nVals == 1)
    min = values[0];
  else {
    
    min = values[0];
    for(i=1; i < nVals; i++)
      if( values[i] < min)
	min = values[i];
  }
  return min;
}

double confidObj::getMax(void)
{
  double max;
  int i;
		
  if(nVals <= 0)
    max = 0;
  else {
    max = values[0];
    for(i=1; i < nVals; i++)
      if( values[i] > max)
	max = values[i];
  }
  return max;
}

double confidObj::getFairInd(void)
{
  double sum;
  double sum_q;
  int i;
  double fairind;
  
  sum = sum_q = 0.0;
  
  for(i=0; i< nVals; i++){
    sum += values[i];
    sum_q += values[i]*values[i];
  }
  
  if(nVals > 0)
    fairind = sum / (nVals*sum_q) *sum;
  else
    fairind = 1.0;
  
  if(sum == 0.0)
    fairind = 1.0;
  
  return fairind;
}

double confidObj::getCorr(int lag, int batchsize)
{
  int i,k;
  int Nb;
  double n,nq, covariance, variance, variance_x, variance_y;
  double sum_x, sum_xq, sum_y, sum_yq, sum_xy;
  double *vals;

  // number of batches
  Nb = (int) nVals/batchsize;

  // now combine batchsize old variables into one new
  CHECK(vals = new double[Nb]);
  for(i=0; i<Nb; i++)
    {
      // put mean value of values[] on vals[]
      vals[i]=0.0;
      for(k=0;k<batchsize;k++)
	vals[i]+=values[i*batchsize+k];
      
      vals[i] /= batchsize;
    }		
  
  n = Nb - lag;
  nq = n*n;
  
  sum_x = sum_xq = 0.0;
  sum_y = sum_yq = 0.0;
  sum_xy = 0.0;
  
		
  // calculate sum of x and y and the squares
  for(i=0; i< Nb-lag; i++){
    sum_x += vals[i];
    sum_xq += vals[i]*vals[i];
    sum_y += vals[i+1];
    sum_yq += vals[i+1]*vals[i+1];
    sum_xy += vals[i]*vals[i+1];
  }
  
  // calculate the covariance;
  covariance = (sum_xy / n) / sum_y - sum_x / nq;
  covariance *= sum_y;
  
  // calculate the variance for x
  // x ranges from interval lagcount to the end
  if(sum_x != 0){
    variance_x = (sum_xq / n) / sum_x - sum_x / nq;
    variance_x *= sum_x;
  } else
    variance_x = 0.0;
  
  if(sum_y != 0){
    variance_y = (sum_yq / n )/ sum_y - sum_y / nq;
    variance_y *= sum_y;
  } else
    variance_y = 0.0;
      
  variance = sqrt(variance_x) * sqrt(variance_y); // this is NOT really the variance
  
  if(variance < 0)
    errm1s("%s: internal error: variance of counting process < 0", name);
  /*
    fprintf(stderr,"sum_x: %g\n",sum_x);
    fprintf(stderr,"sum_y: %g\n",sum_y);      
    fprintf(stderr,"sum_xq: %g\n",sum_xq);
    fprintf(stderr,"sum_yq: %g\n",sum_yq);
    fprintf(stderr,"sum_xy: %g\n",sum_xy);      
    fprintf(stderr,"n: %g\n",n);
    fprintf(stderr,"nq: %g\n",nq);      
    fprintf(stderr,"variance_x: %g\n",variance_x);
    fprintf(stderr,"variance_y: %g\n",variance_y);
    fprintf(stderr,"variance: %g\n",variance); 
    fprintf(stderr,"covariance: %g\n",covariance);   
  */

  if(variance == 0)
    return 1.0;
  else
    return (double) (covariance / variance);
}

void confidObj::meanVar(double *pm, double *pv)
{
  double mean, s2;
  int i;
  
  mean = 0.0;
  for (i = 0; i < nVals; ++i)
    mean += values[i];
  if (nVals)
    mean /= nVals;
  *pm = mean;
  s2 = 0.0;
  for (i = 0; i < nVals; ++i)
    s2 += (values[i] - mean) * (values[i] - mean);
  if (nVals > 1)
    s2 /= nVals - 1;
  *pv = s2;
}

void confidObj::calcConf(double lev, double *pm, double *pw)
{
  double s2;
  
  if (nVals == 0){
    *pm = *pw = 0.0;
  }
  else if (nVals == 1){
    *pm = values[0];
    *pw = 0.0;
  } else {
    meanVar(pm, &s2);
    *pw = studentDist(lev, nVals - 1) * sqrt(s2) / sqrt(nVals);
  }
}

//
// Quantiles of the student distribution.
// Confidence: 0.90, 0.95, 0.975, 0.99
// Table Bronstein, p. 22
//
double confidObj::studentDist(double perc, int degree)
{
  double	*p;
  
  static double	s90[32] = {
    0.0,
    6.314,		// 1
    2.920,
    2.353,		// 3
    2.132,
    2.015,		// 5
    1.943,
    1.895,		// 7
    1.860,
    1.833,		// 9
    1.812,
    1.812,		// 11
    1.782,
    1.782,		// 13
    1.761,
    1.761,		// 15
    1.746,
    1.746,		// 17
    1.734,
    1.734,		// 19
    1.725,
    1.725,		// 21
    1.717,
    1.717,		// 23
    1.711,
    1.711,		// 25
    1.706,
    1.706,		// 27
    1.701,
    1.701,		// 29
    1.697,		// 30
    1.645
  };		// > 30
  
  static double s95[32] = {
    0.0,
    12.706,		// 1
    4.303,
    3.128,		// 3
    2.776,
    2.571,		// 5
    2.447,
    2.365,		// 7
    2.306,
    2.262,		// 9
    2.228,
    2.228,		// 11
    2.179,
    2.179,		// 13
    2.145,
    2.145,		// 15
    2.120,
    2.120,		// 17
    2.101,
    2.101,		// 19
    2.086,
    2.086,		// 21
    2.074,
    2.074,		// 23
    2.064,
    2.064,		// 25
    2.056,
    2.056,		// 27
    2.048,
    2.048,		// 29
    2.042,		// 30
    1.960
  };		// > 30
  
  static double	s975[32] = {
    0.0,
    25.452,		// 1
    6.205,
    4.177,		// 3
    3.495,
    3.163,		// 5
    2.969,
    2.841,		// 7
    2.752,
    2.685,		// 9
    2.634,
    2.634,		// 11
    2.560,
    2.560,		// 13
    2.510,
    2.510,		// 15
    2.473,
    2.473,		// 17
    2.445,
    2.445,		// 19
    2.423,
    2.423,		// 21
    2.405,
    2.405,		// 23
    2.391,
    2.391,		// 25
    2.379,
    2.379,		// 27
    2.369,
    2.369,		// 29
    2.360,		// 30
    2.241
  };		// > 30
  
  static double	s99[32] = {
    0.0,
    63.657,		// 1
    9.925,
    5.841,		// 3
    4.604,
    4.032,		// 5
    3.707,
    3.499,		// 7
    3.355,
    3.250,		// 9
    3.169,
    3.169,		// 11
    3.055,
    3.055,		// 13
    2.977,
    2.977,		// 15
    2.921,
    2.921,		// 17
    2.878,
    2.878,		// 19
    2.845,
    2.845,		// 21
    2.819,
    2.819,		// 23
    2.797,
    2.797,		// 25
    2.779,
    2.779,		// 27
    2.763,
    2.763,		// 29
    2.750,		// 30
    2.576
  };		// >30
  
  if (degree < 1){
    errm0("internal error in confidObj::studentDist(): invalid degree");
    return 0.0;	// not reached
  }
  
  if (degree > 30)
    degree = 31;
  
  if (perc == 0.9)
    p = s90;
  else if (perc == 0.95)
    p = s95;
  else if (perc == 0.975)
    p = s975;
  else if (perc == 0.99)
    p = s99;
  else {
    errm0("internal error in confidObj::studentDist(): invalid level");
    return 0.0;	// not reached
  }
  
  return p[degree];
}



#if 0
void	confidObj::init()
{
	skip(CLASS);
	name = read_id(NULL);

	if (token == ':')
	{	skip(':');
		level = read_double("LEVEL");
		if (level != 0.9 && level != 0.95 && level != 0.975 &&level != 0.99)
			syntax1s("%s: only levels 0.9, 0.95, 0.975, and 0.99 available", name);
	}
	else	level = 0.95;

	vectLen = vectGran;
	CHECK(values = new double [vectLen]);
	nVals = 0;
}

int	confidObj::command(
	char	*s,
	tok_typ	*v)
{
	if (baseclass::command(s, v))
		return TRUE;

	v->tok = NILVAR;
	if (strcmp(s, "Flush") == 0)
	{	nVals = 0;
	}
	else if (strcmp(s, "Add") == 0)
	{	skip('(');
		values[nVals++] = read_double(NULL);
		skip(')');
		if (nVals >= vectLen)
		{	double	*tmp;
			vectLen += vectGran;
			CHECK(tmp = new double [vectLen]);
			for (int i = 0; i < nVals; ++i)
				tmp[i] = values[i];

			delete values;
			values = tmp;
		}
	}
	else if (strcmp(s, "Len") == 0)
	{	v->tok = IVAL;
		v->val.i = nVals;
	}
	else if (strcmp(s, "Val") == 0)
	{	// do not export: values-pointer may change!
		skip('(');
		int	idx;
		idx = read_int(NULL);
		if (idx < 1 || idx > nVals)
			syntax1d("invalid index %d", idx);
		skip(')');
		v->tok = DVAL;
		v->val.d = values[idx - 1];
	}
	else if (strcmp(s, "Mean") == 0)
	{	v->tok = DVAL;
		double	dmy;
		meanVar( &v->val.d, &dmy);
	}
	else if (strcmp(s, "Var") == 0)
	{	v->tok = DVAL;
		double	dmy;
		meanVar( &dmy, &v->val.d);
	}
	else if (strcmp(s, "Lo") == 0)
	{	double	lev;
		double	mean;
		double	width;
		if (token == '(')
		{	scan();
			lev = read_double(NULL);
			skip(')');
		}
		else	lev = level;

		calcConf(lev, &mean, &width);
		v->tok = DVAL;
		v->val.d = mean - width;
	}
	else if (strcmp(s, "Up") == 0)
	{	double	lev;
		double	mean;
		double	width;
		if (token == '(')
		{	scan();
			lev = read_double(NULL);
			skip(')');
		}
		else	lev = level;

		calcConf(lev, &mean, &width);
		v->tok = DVAL;
		v->val.d = mean + width;
	}
	else if (strcmp(s, "Width") == 0)
	{	double	lev;
		double	mean;
		double	width;
		if (token == '(')
		{	scan();
			lev = read_double(NULL);
			skip(')');
		}
		else	lev = level;

		calcConf(lev, &mean, &width);
		v->tok = DVAL;
		v->val.d = width;
	}
	else if (strcmp(s, "Min") == 0)
	{
		double	min;
		int i;
		
		if(nVals <= 0)
		   min = 0;
		else if(nVals == 1)
		   min = values[0];
		else
		{
		   min = values[0];
		   for(i=1; i < nVals; i++)
		      if( values[i] < min)
		         min = values[i];
		}

		v->tok = DVAL;
		v->val.d = min;
	}
	else if (strcmp(s, "Max") == 0)
	{
		double	max;
		int i;
		
		if(nVals <= 0)
		   max = 0;
		else
		{
		   max = values[0];
		   for(i=1; i < nVals; i++)
		      if( values[i] > max)
		         max = values[i];
		}

		v->tok = DVAL;
		v->val.d = max;
	}
	else if (strcmp(s, "FairInd") == 0)
	{
		double	sum;
		double	sum_q;
		int i;
		double fairind;
		
		sum = sum_q = 0.0;
		
		for(i=0; i< nVals; i++)
		{
		   sum += values[i];
		   sum_q += values[i]*values[i];
		}
		
		if(nVals > 0)
		   fairind = sum / (nVals*sum_q) *sum;
		else
		   fairind = 1.0;

		if(sum == 0.0)
		   fairind = 1.0;
		
		v->tok = DVAL;
		v->val.d = fairind;
	}
   else if (strcmp(s, "Corr") == 0)	// Correlation
   {
     int i,k;
     int lag,batchsize;
     int Nb;
     double n,nq, covariance, variance, variance_x, variance_y;
     double sum_x, sum_xq, sum_y, sum_yq, sum_xy;
     double *vals;

      skip('(');
      lag = read_int(NULL);
      skip(',');
      if (lag >= nVals)
         syntax1s1d("%s: lag too large, only %d values available", name,
	    nVals);

      batchsize = read_int(NULL);
      skip(')');

      // number of batches
      Nb = (int) nVals/batchsize;

      // now combine batchsize old variables into one new
      CHECK(vals = new double[Nb]);
      for(i=0; i<Nb; i++)
      {
	 // put mean value of values[] on vals[]
	 vals[i]=0.0;
	 for(k=0;k<batchsize;k++)
	    vals[i]+=values[i*batchsize+k];

	 vals[i] /= batchsize;
      }		

      n = Nb - lag;
      nq = n*n;
      
      sum_x = sum_xq = 0.0;
      sum_y = sum_yq = 0.0;
      sum_xy = 0.0;
		
		
      // calculate sum of x and y and the squares
      for(i=0; i< Nb-lag; i++)
      {
	 sum_x += vals[i];
	 sum_xq += vals[i]*vals[i];
	 sum_y += vals[i+1];
	 sum_yq += vals[i+1]*vals[i+1];
	 sum_xy += vals[i]*vals[i+1];
      }

      // calculate the covariance;
      covariance = (sum_xy / n) / sum_y - sum_x / nq;
      covariance *= sum_y;
      
      // calculate the variance for x
      // x ranges from interval lagcount to the end
      if(sum_x != 0)
      {
         variance_x = (sum_xq / n) / sum_x - sum_x / nq;
         variance_x *= sum_x;
      }
      else
         variance_x = 0.0;

      if(sum_y != 0)
      {
         variance_y = (sum_yq / n )/ sum_y - sum_y / nq;
         variance_y *= sum_y;
      }
      else
	 variance_y = 0.0;
      
      variance = sqrt(variance_x) * sqrt(variance_y); // this is NOT really the variance
      
      if(variance < 0)
         errm1s("%s: internal error: variance of counting process < 0", name);
/*
      fprintf(stderr,"sum_x: %g\n",sum_x);
      fprintf(stderr,"sum_y: %g\n",sum_y);      
      fprintf(stderr,"sum_xq: %g\n",sum_xq);
      fprintf(stderr,"sum_yq: %g\n",sum_yq);
		fprintf(stderr,"sum_xy: %g\n",sum_xy);      
      fprintf(stderr,"n: %g\n",n);
      fprintf(stderr,"nq: %g\n",nq);      
      fprintf(stderr,"variance_x: %g\n",variance_x);
      fprintf(stderr,"variance_y: %g\n",variance_y);
      fprintf(stderr,"variance: %g\n",variance); 
      fprintf(stderr,"covariance: %g\n",covariance);   
*/
      v->tok = DVAL;
      if(variance == 0)
	 v->val.d = 1.0;
      else
         v->val.d = (double) (covariance / variance);

      
   }
   else
      return FALSE;
			
   return TRUE;
}
#endif

